# `backup` (`pfkbak`): Deduplicating Snapshot Backup Subsystem Design Document

## 1. Architectural Overview

The `backup` subsystem (compiled as `libpfkbak.a` and accessed via the `pfkbak` CLI applet) is an incremental, content-addressed, deduplicating snapshot archiver. It is designed to store point-in-time file system trees efficiently into disk-backed, transactional database repositories provided by `libpfkfb`.

```
+-----------------------------------------------------------------------------------------+
|                                    CLI Driver (pfkbak)                                  |
|                               (backup/main.cc, options.cc)                              |
+-----------------------------------------------------------------------------------------+
|                                  Controller (bakFile)                                   |
|               create.cc | update.cc | list.cc | extract.cc | export.cc | deletevers.cc  |
+-----------------------------------------------------------------------------------------+
|                               Schema & Object Model (bakDatum)                          |
|                       bakKey / bakData (BST_UNION) | bakFileContents                    |
|                                (backup/database_items.h)                                |
+-------------------------------------------------------------+---------------------------+
|               B-Tree Index Layer (libpfkfb Btree)           |     Data Repository       |
|                 Indexes: DBINFO, VERSIONINFO,               |   (FileBlockInterface)    |
|                 VERSIONINDEX, FILEINFO, BLOBHASH            |   Chained 64KB Data Chunks|
|                      (*.index repository)                   |     (*.data repository)   |
+-------------------------------------------------------------+---------------------------+
|                          Storage Foundation Layer (libpfkfb)                            |
|             FileBlockLocal (Dynamic AU Allocations, 3-level AUID->AUN Mapping)          |
|                       BlockCache (Chunk Buffering & Alignment)                          |
|                       PageCache (4KB LRU Page Pool & Flushing)                          |
|         PageIO (POSIX fd / Sharded Directory Trees / Transparent AES-256 Encryption)    |
+-----------------------------------------------------------------------------------------+
```

### Core Design Principles

1. **Content-Addressable Storage (CAS)**: Files are identified by their cryptographic SHA-256 digest and logical file size. Duplicate files across directory structures or successive snapshot generations share identical physical data blocks.
2. **Snapshot Immutability**: Each backup generation represents a distinct snapshot identified by an integer version number ($1, 2, 3, \dots$). Existing snapshots are immutable once committed.
3. **Dual-Repository Architecture**: Repository storage is physically bifurcated into a structured metadata index (`*.index`) and a bulk data block repository (`*.data`).
4. **Fast Incremental Ingestion**: File tree scans bypass content hashing for unchanged files by checking modification timestamps (`st_mtim` down to microsecond resolution) and file sizes against the immediately preceding snapshot.
5. **Chained Variable-Length Blob Storage**: Large files are segmented into linked data chunks stored in the data repository, terminating with an explicit null block handle.
6. **Built-in Compaction and Garbage Collection**: Deleting snapshot versions decrements blob reference counts and reclaims unreferenced data chains. Repositories automatically defragment and truncate via `FileBlockInterface::compact()` upon closing.
7. **Transparent Privacy and Sharding**: Through `libpfkfb`'s `PageIO` abstraction, repositories support transparent AES-256-CBC page encryption and three-level directory tree sharding without requiring application-level encryption logic.

---

## 2. Storage Subsystem & `libpfkfb` Integration

The backup system relies on `libpfkfb` for transaction safety, block allocation, caching, and disk indexing.

### 2.1 Dual-File Repository Layout

By default (`SINGLE_FILE_BACKUP == 0` in `backup/options.h`), the engine splits persistence across two files:

| File Suffix | Engine Component | Purpose | Configured Cache Size |
|---|---|---|---|
| `*.index` | `Btree` (`Btree.h`) | Stores serialized `bakKey` $\to$ `bakData` records via the index's underlying `FileBlockInterface`. | `CACHE_SIZE_INDEX = 100 MB` |
| `*.data` | `FileBlockInterface` | Stores chained 64KB raw file chunks (`bakFileContents`). | `CACHE_SIZE_DATA = 1000 MB` |

*(Note: If compiled with `#define SINGLE_FILE_BACKUP 1`, `fbi_data` aliases the B-tree's internal `FileBlockInterface`, storing both metadata and file payloads in a unified file block pool).*

### 2.2 Storage Engine Parameters

- **B-Tree Order (`BTREE_ORDER`)**: `25` (every non-leaf B-tree node contains up to 25 keys).
- **File Permissions**: Files are created with mode `0600` (`S_IRUSR | S_IWUSR`), restricting access strictly to the creating user.
- **Allocation Units (`FB_AUID_T`)**: 32-bit logical handles decouple application-level block pointers from physical on-disk offsets (`AUN`), allowing online defragmentation without invalidating stored chunk links.

### 2.3 Transparent Encryption and Directory Sharding

File paths supplied to `pfkbak` follow the syntax:
```
path/to/backup[:password]
path/to/backup/[:password]
```
The CLI parser (`backup/options.cc`) splits the input into the base path, optional directory sharding flag (`/`), and optional encryption password (`:password`):

```cpp
backupfile_index = filepart + ".index" + dirtree + password;
backupfile_data  = filepart + ".data"  + dirtree + password;
```

When `:password` is provided, `PageIO` encrypts every 4096-byte page using:
- **Cipher**: AES-256-CBC via `mbedtls`.
- **Master Key**: $\text{Key} = \text{SHA-256}(\text{password})$.
- **Per-Page IV**: $\text{IV}[i] = \text{Hash}[i] \oplus \text{Hash}[i + 16]$, where $\text{Hash} = \text{SHA-256}(\text{password} \mathbin{\Vert} \text{":"} \mathbin{\Vert} \text{page\_number})$.
- **Per-Page Authentication**: 32-byte HMAC-SHA256 appended to each encrypted page (expanding physical page size from 4096 to 4128 bytes).

When trailing `/` is provided, `PageIO` instantiates `PageIODirectoryTree`, sharding pages across a 3-level hierarchy of subdirectories formatted as hexadecimal partitions (`level_3/level_2/level_1`), bypassing local filesystem single-directory file limits.

---

## 3. Data Schema & Binary Serialization (`database_items.h`)

All metadata persisted in the B-tree index and data blocks utilizes the Sun XDR-like `BST` binary serialization framework (`libpfkutil/bst.h`).

### 3.1 B-Tree Key Schema (`bakKey`)

`bakKey` inherits `BST_UNION(NULL, 5)` and serializes a 1-byte discriminator tag `which()` followed by the active variant:

```
+---------------+-------------------------------------------------------------+
| Tag (1 byte)  | Key Variant Structure                                       |
+---------------+-------------------------------------------------------------+
| 0 (DBINFO)    | keystring: BST_STRING (Fixed constant: "BKDBINFOKEY")       |
+---------------+-------------------------------------------------------------+
| 1 (VERSIONINFO| version: BST_UINT32_t (Snapshot Version ID, 1..N)           |
+---------------+-------------------------------------------------------------+
| 2 (VERSIONINDX| version: BST_UINT32_t, group: BST_UINT32_t (0..M)           |
+---------------+-------------------------------------------------------------+
| 3 (FILEINFO)  | version: BST_UINT32_t, filename: BST_STRING                 |
+---------------+-------------------------------------------------------------+
| 4 (BLOBHASH)  | hash: BST_STRING (32 bytes binary SHA-256),                 |
|               | filesize: BST_UINT64_t                                      |
+---------------+-------------------------------------------------------------+
```

### 3.2 B-Tree Value Schema (`bakData`)

`bakData` mirrors `bakKey` with a corresponding 1-byte discriminator tag `which()`:

#### Variant 0: `DBINFO` (`dbinfo_data`)
Stores repository-wide metadata:
- `sourcedir`: `BST_STRING` — Absolute or relative path to the backup source root directory.
- `nextver`: `BST_UINT32_t` — Next snapshot version ID to be assigned (initialized to `1`).
- `versions`: `BST_ARRAY<BST_UINT32_t>` — Dynamic array of all currently active snapshot version IDs.
- `dbinfo_version`: `BST_UINT32_t` — Schema format version (enforced as `CURRENT_DBINFO_VERSION = 3`).

#### Variant 1: `VERSIONINFO` (`versioninfo_data`)
Summarizes a specific snapshot generation:
- `time`: `BST_TIMEVAL` — Snapshot creation timestamp (`btv_sec`, `btv_usec`).
- `filecount`: `BST_UINT32_t` — Total count of files, directories, and symlinks in the snapshot.
- `total_bytes`: `BST_UINT64_t` — Sum of uncompressed sizes of all regular files in the snapshot.

#### Variant 2: `VERSIONINDEX` (`versionindex_data`)
Indexes the list of files in a snapshot, partitioned into pages:
- `filenames`: `BST_ARRAY<BST_STRING>` — Array containing up to 100 paths (`MAX_FILENAMES = 100`).
- *Design rationale*: Grouping filenames in chunks of 100 avoids massive B-tree values that would exceed allocation unit limits or induce excessive deserialization overhead when querying specific files.

#### Variant 3: `FILEINFO` (`fileinfo_data`)
Stores metadata for a file in a specific snapshot:
- `hash`: `BST_STRING` — 32-byte binary SHA-256 digest (empty string `""` for symlinks).
- `time`: `BST_TIMEVAL` — Last modification timestamp (`mtime.tv_sec`, `mtime.tv_usec`; set to 0 for symlinks).
- `filesize`: `BST_UINT64_t` — File size in bytes (set to 0 for symlinks).
- `link_contents`: `BST_STRING` — Destination target path if the entry is a symbolic link; empty for regular files.

#### Variant 4: `BLOBHASH` (`blobhash_data`)
Content-addressable reference index for file payloads:
- `refcount`: `BST_UINT32_t` — Number of files across all snapshots currently referencing this payload.
- `first_auid`: `BST_FB_AUID_t` — 32-bit handle pointing to the initial `bakFileContents` chunk in `fbi_data`.

---

### 3.3 Raw File Chunk Storage (`bakFileContents`)

Payloads for regular files are segmented into singly linked chunks stored directly in the `fbi_data` (`*.data`) store:

```
+-----------------------------------------------------------------------------+
|                              bakFileContents                                |
+-----------------------------------------------------------------------------+
|  data: BST_STRING          | Chunk payload (length <= 65528 bytes)          |
|  next_auid: BST_FB_AUID_t  | Handle to next chunk in fbi_data (or 0 for EOF)|
+-----------------------------------------------------------------------------+
```

```
BLOBHASH
+-----------------------+
| refcount: 2           |
| first_auid: 0x0000104A| ----+
+-----------------------+     |
                              v
                  [bakFileContents: Chunk 0]
                  +-----------------------------------+
                  | data: 65,528 bytes                |
                  | next_auid: 0x0000108C             | ----+
                  +-----------------------------------+     |
                                                            v
                                                [bakFileContents: Chunk 1]
                                                +-----------------------------------+
                                                | data: 12,410 bytes                |
                                                | next_auid: 0x00000000 (EOF)       |
                                                +-----------------------------------+
```

#### Sizing Constraints & `MYBUFLEN` (65,528 Bytes)
`BST_STRING` serializes its string length as a 16-bit integer (`BST_UINT16_t`). Any attempt to encode lengths $\ge 65536$ causes integer truncation or overflow. To prevent this, chunk sizes are bounded by `MYBUFLEN = 65528` bytes (leaving headroom below the 64KB boundary).

#### Zero-Length File Handling
Zero-byte files are explicitly allocated a single `bakFileContents` block in `fbi_data` with `data` length 0 and `next_auid = 0`. The content hash recorded is the SHA-256 digest of an empty byte sequence (`e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`).

---

### 3.4 Encapsulation & RAII: `bakDatum`

The `bakDatum` class (`backup/database_items.h`) wraps B-tree queries and updates, enforcing atomic memory management and automatic persistence:

```cpp
class bakDatum {
    Btree * bt;
    FileBlockInterface * fbi; // From bt->get_fbi()
    FB_AUID_T data_id;
    bool dirty;
public:
    bakKey key;
    bakData data;
    ...
};
```

- **RAII Auto-Commit**: If `mark_dirty()` was invoked, the destructor `~bakDatum()` automatically calls `put()`.
- **`get()` Lifecycle**:
  1. Calls `bt->get(&key, &data_id)`.
  2. If found, retrieves the block from `fbi->get(data_id, false)`.
  3. Deserializes `data.bst_decode()`.
  4. Asserts discriminator integrity (`data.which() == key.which()`).
  5. Releases block read-lock (`fbi->release(fb, false)`).
- **`put()` Lifecycle**:
  1. Computes serialized byte size via `data.bst_calc_size()`.
  2. Allocates a new block or reallocates an existing one: `data_id != 0 ? fbi->realloc(data_id, len) : fbi->alloc(len)`.
  3. Obtains write-lock on block, executes `data.bst_encode()`, and releases with dirty flag.
  4. Stores `(&key, data_id)` into B-tree with `replace = true`.
  5. If an old, differing block handle was replaced, frees it via `fbi->free(old_data_id)`.
- **`del()` Lifecycle**:
  1. Deletes entry from B-tree: `bt->del(&key, &old_data_id)`.
  2. Frees corresponding metadata block: `fbi->free(old_data_id)`.

---

## 4. Operational Workflows & Algorithms

### 4.1 Repository Initialization (`create.cc`)

```
CLI: pfkbak c[vv] <file.bak[:pass]> <source_dir>
```

1. **Storage Provisioning**: Creates `*.index` (B-tree order 25, mode 0600, 100MB cache) and `*.data` (FileBlockLocal, mode 0600, 1000MB cache).
2. **Metadata Seeding**:
   - Initializes `DBINFO` record.
   - Sets `dbinfo_version = CURRENT_DBINFO_VERSION` (3).
   - Sets `sourcedir = opts.sourcedir`.
   - Sets `nextver = 1`.
   - Leaves `versions` array empty.
   - Marks dirty; written on scope exit.
3. **Execution Forwarding**: Directly invokes `_update()` to generate Version 1.

---

### 4.2 Incremental Backup Execution (`update.cc`)

```
CLI: pfkbak u[vv] <file.bak[:pass]>
```

```
+-----------------------------------------------------------------------------------+
|                              Load DBINFO & Check Version                          |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|               chdir(sourcedir) & Initialize BFS Queue: todo.push(".")             |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
                      +--------------------------------------+
                      |        Is todo queue empty?          |
                      +--------------------------------------+
                         /                                \
                 [No]   /                                  \   [Yes]
                       v                                    v
+--------------------------------------------+    +---------------------------------+
| Pop item = todo.front()                    |    | Finalize VERSIONINDEX (resize)  |
| lstat(item.name)                           |    | Write VERSIONINFO record        |
+--------------------------------------------+    | Increment nextver in DBINFO     |
       |                   |                 |    | Append version to DBINFO        |
   (Directory)         (Symlink)      (Regular File)
       |                   |                 |    +---------------------------------+
       v                   v                 v
+--------------+    +--------------+  +---------------------------------------------+
| pxfe_readdir |    | readlink()   |  | Query FILEINFO(prev_version, path)          |
| Push entries |    | FILEINFO:    |  +---------------------------------------------+
| to todo queue|    | hash="",     |         /                                   \
+--------------+    | link_contents|    [Match: mtime & size]               [Mismatch or New]
                    | mark_dirty() |        /                                         \
                    +--------------+       v                                           v
                                   +-------------------------+  +-------------------------------+
                                   | Reuse hash from prev ver|  | put_file(): Compute SHA-256   |
                                   | BLOBHASH: refcount++    |  | Lookup BLOBHASH(hash, size)   |
                                   +-------------------------+  | - Exists: refcount++          |
                                                \               | - New: Write 64KB chunk chain |
                                                 \              |        refcount=1             |
                                                  \             +-------------------------------+
                                                   \                         /
                                                    \                       /
                                                     v                     v
                                      +----------------------------------------------+
                                      | Write FILEINFO(current_version, path)        |
                                      | Append path to VERSIONINDEX (group)          |
                                      | If group reaches 100 entries: flush & group++|
                                      +----------------------------------------------+
```

#### Detailed Ingestion Steps

1. **Version Validation**: Validates `dbinfo_version == CURRENT_DBINFO_VERSION` (3). If mismatched, aborts unless the `OVERRIDE_VERSION=1` environment variable is set.
2. **Snapshot Determination**: Determines `version = dbi.nextver()`. Identifies `prev_version` from the last element of `dbi.versions`.
3. **Directory Traversal**:
   - `chdir(sourcedir)`.
   - Utilizes a FIFO queue (`std::list<fileInfo> todo`) starting at `"."`.
   - Examines file type using `lstat()`. Ignores non-regular/non-dir/non-symlink nodes (fifos, sockets, character/block devices).
   - If directory: scans entries with `pxfe_readdir`, filters `.` and `..`, and pushes child paths to `todo`.
4. **Symlink Handling**:
   - Reads destination using `readlink()`.
   - Populates `FILEINFO`: `hash = ""`, `filesize = 0`, `time = 0`, `link_contents = target`.
   - Symlinks bypass blob generation and do not consume data blocks in `fbi_data`.
5. **Regular File Ingestion & Deduplication**:
   - Increments snapshot `file_count` and adds `st_size` to `total_bytes`.
   - **Fast Change-Detection Bypass**:
     - Queries `FILEINFO(prev_version, path)`.
     - Compares `(mtime.sec, mtime.usec, filesize)` against `lstat` results.
     - **Match**: File is identical to previous version. Bypasses file reading and content hashing. Reuses previous `hash`, fetches `BLOBHASH(hash, filesize)`, and increments `refcount++`.
     - **Mismatch or First Appearance**: Invokes `put_file()`:
       1. Streams file in 16KB blocks to compute 32-byte binary SHA-256 digest (`mbedtls_sha256`).
       2. Looks up `BLOBHASH(hash, filesize)`.
       3. **Blob Exists**: Bumps `refcount++` (content-level deduplication across paths or identical revisions).
       4. **Blob Missing**: Opens file, reads into `MYBUFLEN` (65,528 bytes) chunks. Uses double-buffered file chunk structures (`conts[2]`) to link chunk $N$ to chunk $N+1$'s `auid`. Saves chunks to `fbi_data`. Registers new `BLOBHASH` with `refcount = 1` and `first_auid`.
   - Writes new `FILEINFO(version, path)` with current metadata.
6. **Manifest Pagination**: Appends filename to `VERSIONINDEX(version, group)`. Once 100 paths accumulate (`MAX_FILENAMES`), flushes to B-tree and increments `group++`.
7. **Snapshot Finalization**:
   - Writes `VERSIONINFO(version)` with timestamp `now`, `file_count`, and `total_bytes`.
   - Updates `DBINFO`: increments `nextver`, appends `version` to `versions` array, and commits.

---

### 4.3 Table of Contents Listing (`list.cc`)

```
CLI: pfkbak t[vv] <file.bak[:pass]>
```

The listing tool traverses the B-tree hierarchy based on the requested verbosity:

- **Standard (`-t`, `verbose == 0`)**:
  - Dumps `DBINFO` header: schema version, source directory, next version ID.
  - Iterates `versions` array: prints version ID, timestamp, file count, and aggregate logical byte size.
- **Verbose (`-tv`, `verbose == 1`)**:
  - Additionally iterates all `VERSIONINDEX(version, group)` groups.
  - Lists every relative file path within each snapshot.
- **Very Verbose (`-tvv`, `verbose == 2`)**:
  - Fetches `FILEINFO(version, path)` for every path.
  - For symlinks: displays `link target`.
  - For regular files: displays formatted 64-character hex hash, microsecond modification timestamp, byte size, and resolves `BLOBHASH` to display `blob refcount` and `first_auid`.

---

### 4.4 File Extraction (`extract.cc`)

```
CLI: pfkbak x[vv] <file.bak[:pass]> <version> [path_filter...]
```

1. **Version Validation**: Verifies that the requested version is registered in `DBINFO`.
2. **Selective Filtering (`match_file_name`)**: Compares file paths against optional CLI prefix filters:
   - If no paths specified: matches all.
   - If filters specified: evaluates `path.compare(0, pattern.len, pattern) == 0`.
3. **Restoration Pipeline (`extract_file`)**:
   - Reads `FILEINFO(version, path)`.
   - **Symlink**:
     - Ensures parent directory exists (`mkdir_minus_p`).
     - Calls POSIX `symlink(link_contents.c_str(), path.c_str())`.
   - **Regular File**:
     - Looks up `BLOBHASH(hash, filesize)` to obtain `first_auid`.
     - Ensures parent directories exist via `mkdir_minus_p`.
     - Opens destination file: `open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600)`.
     - Traverses chunk chain in `fbi_data`:
       ```cpp
       while (auid != 0) {
           FileBlock * fb = fbi_data->get(auid);
           bfc.bst_decode(fb->get_ptr(), fb->get_size());
           do_write(fd, bfc.data().c_str(), bfc.data().length());
           auid = bfc.next_auid();
           fbi_data->release(fb);
       }
       ```
     - Closes destination file.

---

### 4.5 GNU Tar Export (`export.cc`, `tarfile.cc`)

```
CLI: pfkbak X[vv] <file.bak[:pass]> <version> <outfile.tar> [path_filter...]
```

Exports a snapshot directly into a standard POSIX/GNU `ustar` tar archive without writing intermediate files to disk:

1. Opens destination tar file with mode `0600`.
2. Reuses `_extract(tarfd)` engine to iterate matched files.
3. For each regular file:
   - **Long Path Handling**: POSIX `ustar` headers accommodate paths up to 100 characters. If `path.size() > 99`:
     1. Emits a GNU LongLink header with filename `././@LongLink` and `typeflag = 'L'`.
     2. Writes the full path string as file payload.
     3. Emits null-byte padding up to a 512-byte boundary (`tarfile_emit_padding`).
     4. Emits a standard file header with path truncated to 100 bytes and `typeflag = '0'`.
   - **Header Emission (`populate_header`)**:
     - Mode: `0000644`.
     - UID / GID: `0000000` / `0000000` (`root` / `root`).
     - Size: octal representation (`%011o`).
     - Checksum: 8-byte octal sum of all 512 header bytes (calculated with checksum field set to spaces).
     - Magic: `ustar  \0`.
   - **Payload Streaming**: Streams data chunks directly from `fbi_data` to `tarfd`.
   - **Padding**: Emits null padding to align file data to the 512-byte tar block boundary (`tarfile_emit_padding`).
4. **Archive Footer**: Emits two consecutive 512-byte blocks of zeros (1024 bytes) via `tarfile_emit_footer()`.
5. *(Note on symlinks)*: If symlinks are encountered during `export`, `extract_file` reconstructs them on the local filesystem rather than emitting a tar symlink header (`typeflag = '2'`).

---

### 4.6 Snapshot Deletion & Space Reclamation (`deletevers.cc`)

```
CLI: pfkbak d[vv] <file.bak[:pass]> <ver1> [ver2...]
```

```
+-----------------------------------------------------------------------------------+
|               Verify all requested versions exist in DBINFO                       |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
|               Construct newdbinfo excluding the targeted versions                 |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
                      +--------------------------------------+
                      |      For each version to delete      |
                      +--------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
| 1. Delete VERSIONINFO(version) from B-tree                                        |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
| 2. Iterate and delete VERSIONINDEX(version, group) records                        |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
| 3. For each file in group:                                                        |
|    - Delete FILEINFO(version, filename)                                           |
|    - If symlink: continue (no data blocks)                                        |
|    - If regular file:                                                             |
|      * Fetch BLOBHASH(hash, filesize)                                             |
|      * If refcount > 1:                                                           |
|          refcount--; mark_dirty();                                                |
|      * If refcount == 1:                                                          |
|          Delete BLOBHASH from B-tree;                                             |
|          Traverse chunk chain in fbi_data:                                        |
|            fbi_data->free(auid);                                                  |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
| Commit newdbinfo to B-tree                                                        |
+-----------------------------------------------------------------------------------+
                                         |
                                         v
+-----------------------------------------------------------------------------------+
| Repository Closure: Trigger FileBlockInterface::compact() on *.index and *.data   |
+-----------------------------------------------------------------------------------+
```

#### Reclamation Mechanics

- **Reference Counting**: Only blobs whose reference count drops to 1 are deleted and deallocated. Shared blobs have their reference count decremented and their data chains remain intact.
- **Cascading Chain Freeing**: Chunk deletion traverses `next_auid` sequentially, calling `fbi_data->free(auid)` for each block.
- **Compaction**: Deleting blocks leaves holes in the underlying allocation tables. Defragmentation occurs when the repository closes.

---

## 5. Storage Compaction & Defragmentation

When a `bakFile` instance is destroyed (`backup/bakfile.cc`), it automatically evaluates both `.index` and `.data` storage engines for compaction:

```cpp
bakFile::~bakFile(void) {
    if (bt) {
        bt->get_fbi()->compact(&compactionStatus, NULL);
        delete bt;
    }
    if (fbi_data) {
        fbi_data->compact(&compactionStatus, NULL);
        delete fbi_data;
    }
}
```

### Compaction Heuristic (`compactionStatus`)

Compaction involves moving allocation units and defragmenting physical pages. To avoid unnecessary I/O overhead on small or minimally fragmented files, compaction is governed by:

```cpp
static bool compactionStatus(FileBlockStats *stats, void *arg) {
    if (stats->num_aus < 1000)
        return false; // Skip small repositories (< 1000 allocation units)
    uint32_t max_free = stats->num_aus / 100;
    if (stats->free_aus <= max_free)
        return false; // Skip if free allocation units are <= 1%
    return true;      // Defragment and truncate
}
```

If compaction executes:
1. `FileBlockLocal::compact()` shifts occupied allocation units into contiguous lower offsets.
2. Updates internal `AUID` $\to$ `AUN` translation tables (preserving persistent `FB_AUID_T` handles).
3. Truncates unused trailing pages from the physical file via `ftruncate()`.

---

## 6. Security, Integrity & Fault Tolerance

1. **Cryptographic Protection**:
   - AES-256-CBC page encryption prevents physical inspection of stored blocks.
   - Per-page HMAC-SHA256 ensures tampering, bit-rot, or disk corruption triggers immediate authentication failures on read.
2. **Signal Resilience**:
   - `pfkbak_main` executes `signal(SIGPIPE, SIG_IGN)` at startup, ensuring broken pipes during export or listing operations do not terminate the process without clean repository closure and cache flushing.
3. **Schema Compatibility Guard**:
   - `DBINFO` includes `CURRENT_DBINFO_VERSION = 3`.
   - All update, delete, list, and extract routines verify this version before operating on the database, preventing accidental corruption from binary version mismatches.
   - Forcing overrides requires explicitly setting `OVERRIDE_VERSION=1` in the process environment.
4. **Filesystem Boundary Protection**:
   - Created files and exported archives are locked to `0600` permissions.
   - Extracted directories are provisioned with `0700` permissions via `mkdir_minus_p`.

---

## 7. CLI Command Reference & Examples

### Command Grammar
```
pfkbak <opcode>[v|vv] <archive_path>[:password] [arguments...]
```

### Supported Operations

| Opcode | Arguments | Description |
|---|---|---|
| `c` | `<archive>[:pass] <source_dir>` | Initialize database and create Version 1 from `<source_dir>`. |
| `u` | `<archive>[:pass]` | Scan `<source_dir>` and record a new incremental snapshot version. |
| `t` | `<archive>[:pass]` | List repository metadata and snapshot versions (`v` for files, `vv` for hashes/AUIDs). |
| `d` | `<archive>[:pass] <ver> [ver...]` | Delete one or more snapshot versions and reclaim orphaned storage. |
| `x` | `<archive>[:pass] <ver> [path...]` | Extract version `<ver>` (or specific path prefixes) to the local filesystem. |
| `X` | `<archive>[:pass] <ver> <out.tar> [path...]` | Stream version `<ver>` into a POSIX/GNU tar archive `<out.tar>`. |

### Execution Examples

```bash
# 1. Create an encrypted backup repository from ~/projects
pfkbak c /var/backups/myrepo:SecretPassword123 /home/user/projects

# 2. Perform an incremental snapshot update with verbose logging
pfkbak uv /var/backups/myrepo:SecretPassword123

# 3. List all snapshots and contained files with SHA-256 hashes
pfkbak tvv /var/backups/myrepo:SecretPassword123

# 4. Extract snapshot version 2 into the current working directory
pfkbak x /var/backups/myrepo:SecretPassword123 2

# 5. Extract only the "docs/" subtree from snapshot 2
pfkbak x /var/backups/myrepo:SecretPassword123 2 ./docs

# 6. Export snapshot 3 directly to a tarball
pfkbak X /var/backups/myrepo:SecretPassword123 3 snapshot3.tar

# 7. Delete obsolete snapshot versions 1 and 2
pfkbak d /var/backups/myrepo:SecretPassword123 1 2

# 8. Use directory sharding for extremely large archives
pfkbak c /mnt/storage/hugebackup/:SecretPassword123 /data/bigset
```
