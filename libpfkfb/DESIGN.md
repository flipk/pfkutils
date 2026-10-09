# `libpfkfb`: Storage Engine & Persistence Layer Design Document

## 1. Architectural Overview

`libpfkfb` is a layered, block-structured, disk-backed storage engine written in C++. It provides dynamic variable-length file block allocations, multi-tiered page and chunk caching, transparent AES-256-CBC page-level encryption with HMAC-SHA256 authentication, on-line defragmentation/compaction, and disk-backed balanced B-trees indexing variable-length keys to 32-bit data handles.

The engine serves as the persistence foundation for higher-level applications within `pfkutils`, such as the content-addressed deduplicating backup tool (`pfkbak`).

```
+----------------------------------------------------------------------------+
|             Application Layer (pfkbak, fbdump, fbcompact, btdump)          |
+----------------------------------------------------------------------------+
|             Object-Relational / BST Layer (BtreeDbClasses.h)               |
+----------------------------------------------------------------------------+
|             Btree Layer (Btree.h, BtreeNode.cc, Btree.cc)                  |
|             Maps variable-length keys -> 32-bit FB_AUID_T                  |
+----------------------------------------------------------------------------+
|             FileBlock Layer (FileBlock_iface.h, FileBlockLocal*.cc)        |
|             Dynamic AU allocations, 3-level AUID->AUN map, Compaction      |
+----------------------------------------------------------------------------+
|             BlockCache Layer (BlockCache.h, BlockCache.cc)                 |
|             Sub-page / Multi-page chunk buffering & zero-copy windows      |
+----------------------------------------------------------------------------+
|             PageCache Layer (PageCache.h, PageCache.cc)                    |
|             4KB Page buffer pool, LRU eviction, dirty page sort & flush    |
+----------------------------------------------------------------------------+
|             PageIO Layer (PageIO.h, PageIO*.cc)                            |
|             Backends: File Descriptor, Sharded Directory Tree, Network TCP |
|             Crypto: AES-256-CBC + HMAC-SHA256 authenticated encryption     |
+----------------------------------------------------------------------------+
```

### Core Design Principles

1. **Strict Layer Decoupling**: Each layer abstracts specific concerns. `PageIO` knows only fixed physical pages. `PageCache` handles memory caps and page buffering. `BlockCache` handles arbitrary byte extents and page boundaries. `FileBlock` manages space allocation and logical handles. `Btree` manages indexing.
2. **Decoupled Identifiers (`AUID` vs. `AUN`)**: Applications reference storage using permanent 32-bit Allocation Unit Identifiers (`FB_AUID_T`). The physical location on disk (`FB_AUN_T`) can change during compaction or reallocation without invalidating application pointers.
3. **Zero-Copy Optimization**: When requested data extents reside fully within a single 4KB cache page, `BlockCache` returns a direct pointer into the `PageCache` buffer, avoiding intermediate memory copies.
4. **End-to-End Cryptographic Privacy**: When encryption is enabled, data is encrypted and authenticated per-page at the lowest I/O layer. Upper layers remain completely agnostic to encryption.
5. **Platform Independence & Endian Safety**: Structures written to disk use explicit endian-independent primitives from `libpfkutil/types.h` (`UINT16_t`, `UINT32_t`, `UINT64_t`) and Sun XDR-like serialization (`BST`).

---

## 2. PageIO Layer: Physical Storage & Transport Abstraction

The `PageIO` class (`PageIO.h`, `PageIO.cc`) forms the base pure-virtual interface for reading and writing fixed-size pages to physical storage or network endpoints.

### 2.1 Interface & Page Sizing

- **Page Size (`PCP_PAGE_SIZE`)**: 4096 bytes (4 KB).
- **Cipher Overhead (`HMAC_OVERHD`)**: 32 bytes (SHA-256 HMAC tag).
- **Ciphered Page Size (`CIPHERED_PAGE_SIZE`)**: 4128 bytes (4096 + 32).
- **Virtual Methods**:
  ```cpp
  virtual bool get_page(PageCachePage *pg) = 0;
  virtual bool put_page(PageCachePage *pg) = 0;
  virtual void truncate_pages(uint64_t num_pages) = 0;
  ```

### 2.2 Storage Backends

`PageIO` provides three backends, instantiated via `PageIO::open(path, create, mode)`:

1. **`PageIOFileDescriptor` (`PageIOFileDescriptor.cc`)**:
   - Backed by a standard POSIX file descriptor using `open()`, `lseek()`, `read()`, `write()`, and `ftruncate()`.
   - Requires 64-bit file offsets (`sizeof(off_t) == 8`) with `O_LARGEFILE` support.
   - Calculates disk offset as:
     $$\text{offset} = \text{page\_number} \times \text{page\_size}$$
     where $\text{page\_size}$ is 4096 (plain) or 4128 (encrypted).

2. **`PageIODirectoryTree` (`PageIODirectoryTree.cc`)**:
   - Shards huge databases across a 3-level directory structure on the local filesystem to avoid operating system directory entry limits.
   - Each shard file holds 4096 pages (16 MB for plaintext, ~16.125 MB for encrypted).
   - Shard addressing:
     - Page number is decomposed: $\text{pagefile\_number} = \lfloor \text{page\_number} / \text{pgsize} \rfloor$, $\text{offset\_in\_file} = \text{page\_number} \pmod{\text{pgsize}}$.
     - Relative path generation: `level_3 / level_2 / level_1`, derived by partitioning `pagefile_number` into 12-bit chunks formatted as 3-digit lowercase hexadecimal directories.
   - Internal LRU file descriptor cache: capped at 128 open file descriptors (`max_file_pages = 128`), closing inactive shard descriptors via an intrusive `DLL3::List` and `DLL3::Hash`.

3. **`PageIONetworkTCPServer` (`PageIONetworkTCPServer.cc`)**:
   - Provides a client connection interface to access pages across a remote TCP service.

### 2.3 Transparent Page-Level Cryptography

Encryption is enabled transparently when a filename or connection string includes a password separated by a colon:

```
filename:password
path/to/directory/:password
fbsrv:192.168.1.10:2440:password
```

#### Cryptographic Specification
- **Cipher**: AES-256 in CBC mode (via `mbedtls/aes.h`).
- **Key Derivation**: 32-byte master key derived from the password string:
  $$\text{Key} = \text{SHA-256}(\text{password})$$
- **Initialization Vector (IV)**: Derived deterministically per page so that arbitrary random-access page reads and writes do not require tracking global IV states:
  $$\text{Hash} = \text{SHA-256}(\text{password} \mathbin{\Vert} \text{":"} \mathbin{\Vert} \text{page\_number})$$
  The 16-byte IV is formed by XORing the first 16 bytes of $\text{Hash}$ with the second 16 bytes:
  $$\text{IV}[i] = \text{Hash}[i] \oplus \text{Hash}[i + 16] \quad (0 \le i < 16)$$
- **Authentication**: HMAC-SHA256 (`mbedtls/md.h`) computed over the 4096-byte ciphertext using $\text{Key}$.
- **On-Disk Ciphered Page Layout**:
  ```
  +-------------------------------------------------------+-----------------------+
  |              4096 Bytes AES-256 Ciphertext            |  32 Bytes HMAC-SHA256 |
  +-------------------------------------------------------+-----------------------+
  ```
- **Integrity Verification**: On `decrypt_page()`, the HMAC is recalculated and verified against the stored tag before CBC decryption. A mismatch outputs an HMAC failure diagnostic.

### 2.4 URI Path Grammar & Parsing

`PageIO::open()` uses POSIX extended regular expressions (`path_pattern`) to parse targets:

| Format Pattern | Target Backend | Encryption |
|---|---|---|
| `fbsrv:<ip>:<port>` | `PageIONetworkTCPServer` | Plaintext |
| `fbsrv:<ip>:<port>:<key>` | `PageIONetworkTCPServer` | Encrypted |
| `fbsrv:<host>:<port>` | `PageIONetworkTCPServer` | Plaintext |
| `fbsrv:<host>:<port>:<key>`| `PageIONetworkTCPServer` | Encrypted |
| `<dirpath>/:<key>` | `PageIODirectoryTree` | Encrypted |
| `<dirpath>/` | `PageIODirectoryTree` | Plaintext |
| `<filepath>:<key>` | `PageIOFileDescriptor` | Encrypted |
| `<filepath>` | `PageIOFileDescriptor` | Plaintext |

---

## 3. PageCache Layer: In-Memory 4KB Buffer Pool

The `PageCache` class (`PageCache.h`, `PageCache_internal.h`, `PageCache.cc`) manages an in-memory buffer pool of 4KB pages on top of a `PageIO` instance.

### 3.1 Container Architecture & Intrusive Lists

`PageCache` relies on `PageCachePageList`, which embeds `PCPInt` nodes simultaneously across five intrusive containers from `libpfkutil/dll3.h`:

```
+--------------------------------------------------------------------------+
|                                  PCPInt                                  |
|  - uint64_t page_number                                                  |
|  - uint8_t *ptr (4096 bytes)                                             |
|  - int refcount (0 = unlocked, >0 = locked)                              |
|  - bool dirty                                                            |
+-------------------+------------------+-------------------+---------------+
          |                  |                   |                 |
          v                  v                   v                 v
     PageList_t         PageHash_t         PageLruLock_t     PageDirtyList_t
   (All cached pgs)  (Key: page_number)   (LRU or Locklist)  (Dirty pages)
```

1. **`pageList`**: Master doubly linked list of all cached pages.
2. **`hash`**: Hash table keyed by `page_number` for $O(1)$ page lookups.
3. **`lru`**: Doubly linked list tracking unreferenced pages (`refcount == 0`), ordered from newest (head) to oldest (tail).
4. **`locklist`**: Doubly linked list tracking pinned/in-use pages (`refcount > 0`).
5. **`dirty`**: Doubly linked list of all pages that have pending modifications.

### 3.2 Cache Lifecycle & Operations

- **`get(page_number, for_write)`**:
  - Checks `hash` for existing cached page. If present, increments `refcount` (moving from `lru` to `locklist` on first ref).
  - If not cached, allocates a new `PCPInt` and reads the page via `io->get_page()`.
  - **`for_write` Optimization**: If caller sets `for_write = true`, disk read is skipped; memory is zeroed out and marked dirty immediately.
- **`release(pg, dirty)`**:
  - Decrements `refcount`. When `refcount` reaches zero, page moves from `locklist` to the head of `lru`.
  - If unreferenced pages exceed `max_pages`, evicts the oldest page from the tail of `lru`. If dirty, writes it out via `io->put_page()`.
  - If any evicted page triggers a write, `PageCache` initiates a full `flush()` to maintain consistency.
- **`flush()`**:
  - Inspects `PageDirtyList_t`.
  - Collects pointers to all dirty pages into an array.
  - **I/O Sorting**: Sorts pages in ascending order of `page_number` using standard `qsort` and `page_compare()`. Sequential ordering maximizes contiguous disk writes and minimizes seek latency.
  - Sequentially writes all dirty pages via `io->put_page()`, resetting `dirty = false`.
- **`truncate_pages(num_pages)`**:
  - Traverses cached pages. Any unlocked page where $\text{page\_number} \ge \text{num\_pages}$ is removed from the cache.
  - Asserts that no pages $\ge \text{num\_pages}$ remain locked.
  - Calls `io->truncate_pages(num_pages)`.

---

## 4. BlockCache Layer: Arbitrary-Sized Block Buffering

The `BlockCache` class (`BlockCache.h`, `BlockCache_internal.h`, `BlockCache.cc`) abstracts arbitrary-sized byte regions (`off_t offset, int size`) on top of `PageCache`.

### 4.1 Boundary Splitting & Zero-Copy Optimization

When an application calls `BlockCache::get(offset, size, for_write)`:

1. **Page Range Calculation**:
   $$\text{starting\_page} = \lfloor \text{offset} / 4096 \rfloor, \quad \text{ending\_page} = \lfloor (\text{offset} + \text{size} - 1) / 4096 \rfloor$$
   $$\text{num\_pages} = \text{ending\_page} - \text{starting\_page} + 1$$
2. **Single-Page Optimization (Zero-Copy)**:
   - If $\text{num\_pages} == 1$, the block is fully contained within one 4KB page.
   - `BlockCacheBlock::ptr` points directly into the `PageCachePage::ptr` at offset $(\text{offset} \pmod{4096})$.
   - No auxiliary buffer is allocated. Reads and writes execute directly against page cache memory.
3. **Multi-Page Spanning**:
   - If $\text{num\_pages} > 1$, `BlockCacheBlock` allocates a contiguous intermediate buffer of size `size`.
   - References all spanned pages via an array `pages[num_pages]`.
   - Copies spanned bytes from each page into `ptr`.
   - In `for_write` mode, full internal pages are acquired with `for_write = true` to skip redundant disk reads.

### 4.2 Release & Flush Synchronization

- **`release(bcb, dirty)`**:
  - If the block was marked dirty and $\text{num\_pages} > 1$, writes back the modified contiguous buffer across the individual `PageCachePage` buffers.
  - Calls `PageCache::release(p, dirty)` for all referenced pages.
  - Deletes auxiliary staging buffers and unlinks `bcb`.
- **`flush()`**:
  - Traverses outstanding active blocks in `BlockCacheList`.
  - Copies multi-page modified blocks back into their underlying `PageCachePage` buffers and marks them dirty.
  - Calls `PageCache::flush()`.

### 4.3 Concurrency & Access Invariants

- **Overlap Restriction**: Simultaneous concurrent requests for the same byte range or overlapping ranges through `BlockCache` are **strictly illegal**. Concurrent access leads to undefined cache incoherence.
- **Typed Access Templates**:
  - `BlockCacheT<T>`: RAII template wrapping a disk structure `T` at a given byte offset. Automatically handles `release()` on destruction or reallocation.
  - `BlockCacheAUT<T>`: Specialized derived template that addresses offsets by Allocation Unit Number (`AUN * AU_SIZE`).

---

## 5. FileBlock Layer: Dynamic Allocation & Space Management

The `FileBlockInterface` (`FileBlock_iface.h`) and `FileBlockLocal` (`FileBlockLocal.h`, `FileBlockLocal*.cc`) implement a disk allocation manager analogous to `malloc()` and `free()`, but operating on disk files.

### 5.1 Units and Identifiers

- **Allocation Unit (AU)**: Fundamental unit of disk space. Fixed at `AU_SIZE = 512` bytes.
- **Allocation Unit Number (`FB_AUN_T`)**: 64-bit integer representing the physical position in the file:
  $$\text{File Offset} = \text{AUN} \times 512$$
  $\text{AUN} = 0$ is reserved as an invalid/null pointer.
- **Allocation Unit Identifier (`FB_AUID_T`)**: 32-bit stable logical handle returned to callers. Identifies an allocated extent regardless of where it is physically moved during defragmentation. $\text{AUID} = 0$ is reserved for internal metadata (e.g., L2/L3 tables).

### 5.2 On-Disk Metadata Layout

#### The File Header (`_FileHeader` at AUN 0)
The header begins at byte offset 0 (`FileHeader::FILE_HEADER_AUN = 0`) and contains:

```
+--------------------------------------------------------------------------+
|                                InfoBlock                                 |
|  signature (0x0ecc0e35) | num_buckets (4096) | au_size (512)             |
|  used_aus | free_aus | used_extents | free_extents                       |
|  first_au | num_aus (file size in AUs) | auid_top | auid_stack_top       |
+--------------------------------------------------------------------------+
|                       DataInfoPtrs (64 x FB_AUID_t)                      |
|  Named bookmark entries mapping C-strings to FB_AUID_T                   |
+--------------------------------------------------------------------------+
|                       BucketList (4096 x FB_AUN_t)                       |
|  Heads of doubly-linked free lists for each AU size                      |
+--------------------------------------------------------------------------+
|                       BucketBitmap (128 x UINT32_t)                      |
|  4096 bits: bit N = 1 if BucketList[N] != 0                              |
+--------------------------------------------------------------------------+
|                       AuidL1Tab auid_l1 (4096 x FB_AUN_t)                |
|  Top 12 bits of AUID -> AUN of L2 translation table                      |
+--------------------------------------------------------------------------+
|                       AuidL1Tab auid_stack_l1 (4096 x FB_AUN_t)          |
|  Top 12 bits of stack_top -> AUN of L2 free-stack table                  |
+--------------------------------------------------------------------------+
```

1. **`InfoBlock`**:
   - `signature`: Magic value `0x0ecc0e35`.
   - `num_buckets`: Must equal 4096.
   - `au_size`: Must equal 512.
   - `first_au`: AUN of the first allocation unit after the header (`SIZE_TO_AUS(sizeof(_FileHeader))`).
   - `num_aus`: High watermark of allocated file extents (corresponds to physical file size).
   - `auid_top`: High-watermark counter for fresh AUID allocations.
   - `auid_stack_top`: Index pointing to the top of the recycled AUID stack.
2. **`DataInfoPtrs`**: Up to 64 named bookmarks mapping text strings (`MAX_INFO_NAME = 128`) to an AUID. Used to locate subsystem roots (e.g., B-tree metadata).
3. **`BucketList` & `BucketBitmap`**:
   - 4096 bucket lists. Buckets $0 \dots 4094$ correspond to free blocks of size $1 \dots 4095$ AUs.
   - Bucket 4095 is reserved for the unbounded end-of-file marker ($\text{size} = 0$, representing infinite space).
   - `BucketBitmap` tracks non-empty buckets. Word-skipping accelerates finding the first non-empty bucket with adequate capacity.

#### Extent Prefix: `_AUHead`
Every region in the file begins with an `_AUHead` header:

```
+------------------------------------+-------------------------------------+
|         prev (FB_AUN_t, 8B)        |     size_and_used (UINT32_t, 4B)    |
|   Physical AUN of previous extent  | Bit 31: USED flag, Bits 30-0: size  |
+------------------------------------+-------------------------------------+
|              one (FB_AUN_t, 8B)    |                 two (FB_AUN_t, 8B)  |
| Used: FB_AUID_T auid               | Free: bucket_prev AUN               |
| Free: bucket_next AUN              | Used: Unused / User Data Payload    |
+------------------------------------+-------------------------------------+
```

- **`used_size`**: 20 bytes consumed by the header in a used extent (`prev` + `size_and_used` + `one`).
- **`prev`**: AUN of the preceding physical extent on disk, enabling bidirectional neighbor lookups.
- **`size_and_used`**:
  - Bit 31 (`USED_MASK = 0x80000000`): 1 if extent is used, 0 if free.
  - Bits 30..0 (`SIZE_MASK = 0x7FFFFFFF`): Extent length in AUs. Size 0 denotes the EOF marker.

### 5.3 Three-Level AUID Translation & Free Stack

AUIDs are mapped to physical disk positions (AUNs) via a 3-level sparse page table:

```
32-Bit AUID / Stack Index:
+-----------------------+-----------------------+--------------------------+
|  Bits 31-20 (12 bits) |  Bits 19-10 (10 bits) |   Bits 9-0 (10 bits)     |
|       L1 Index        |       L2 Index        |        L3 Index          |
+-----------------------+-----------------------+--------------------------+
           |                       |                          |
           v                       v                          v
    AuidL1Tab (Header)         AuidL23Tab                 AuidL23Tab
     (4096 entries)          (1024 entries)             (1024 entries)
           |                       |                          |
           +--> AUN of L2 -------->+--> AUN of L3 ----------->+--> AUN / AUID
```

- **Translation Table (`auid_l1`)**:
  - $L_1$ is embedded in `FileHeader` (4096 AUNs).
  - $L_2$ and $L_3$ tables (`AuidL23Tab`, 1024 entries each) are dynamically allocated on disk as needed via `alloc_aun(sizeof(AuidL23Tab))`.
  - Leaf $L_3$ entries store the physical `AUN` of the user extent.
- **Free Stack Table (`auid_stack_l1`)**:
  - Identical 3-level structure indexed by `auid_stack_top`.
  - When an extent is freed, its AUID is pushed to the top of the stack and cleared from `auid_l1`.
  - When allocating, `auid_stack_top` is checked first. If $> 0$, an AUID is popped and reused; otherwise, `auid_top` is incremented.
- **Internal Marker**: Extents allocated for L2/L3 tables store $\text{AUID} = 0$ in their `_AUHead`.

### 5.4 Space Allocation & Coalescing

#### Allocation (`alloc(size)`)
1. Calculates required AUs:
   $$\text{num\_aus} = \text{SIZE\_TO\_AUS}(\text{size} + \text{\_AUHead::used\_size})$$
   Maximum single allocation is bounded by $\text{FILE\_BLOCK\_MAXIMUM\_ALLOCATION\_SIZE} = (4096 - 2) \times 512 = 2{,}096{,}128$ bytes (~2 MB).
2. `ffu_bucket(num_aus)` inspects `BucketBitmap` to find the smallest non-empty bucket list $\ge \text{num\_aus}$.
3. Dequeues the free extent from the bucket list.
4. If extent size $> \text{num\_aus}$, the block is split:
   - First part becomes the allocated extent of size $\text{num\_aus}$.
   - Remainder forms a new free extent with its own `_AUHead` and is enqueued into the appropriate bucket list.
5. Obtains an AUID via `alloc_auid(aun)`, writes it to `_AUHead`, and returns the handle.

#### Freeing (`free(auid)`)
1. Translates AUID to physical AUN using the 3-level table.
2. Reads `_AUHead` of the extent, its predecessor (`prev`), and its successor ($\text{aun} + \text{size}$).
3. **Four-Way Coalescing**:
   - **Case 1 (Both Prev and Next Free)**: Merges prev, current, and next into one contiguous extent. Updates the successor's successor `prev` link.
   - **Case 2 (Prev Free Only)**: Merges prev and current.
   - **Case 3 (Next Free Only)**: Merges current and next.
   - **Case 4 (Neither Free)**: Marks current extent as free without merging.
4. Enqueues the coalesced extent into the matching bucket list and sets the corresponding `BucketBitmap` bit.
5. Pushes the AUID onto the free stack via `free_auid(auid)`.

---

## 6. Defragmentation & Compaction

Over repeated allocation and deletion cycles, databases fragment and leave free space holes between active blocks. `FileBlockLocal::compact()` (`FileBlockLocalCompact.cc`) implements an online defragmenter and consolidator.

### 6.1 Compaction Algorithm

```
+--------------------------------------------------------------------------+
| 1. Scan auid_l1 and auid_stack_l1: build in-memory dependency graph      |
|    of all L2 and L3 table AUNs and their parent table positions.         |
+--------------------------------------------------------------------------+
| 2. Loop until user callback requests stop or free space target met:      |
|    a. Locate the last used extent preceding EOF.                         |
|    b. Check if an existing free bucket can hold it.                      |
|       - YES: Relocate extent directly into the free hole.                |
|       - NO:  Initiate "Downshifting" from the start of the file:         |
|              Shift used extents downward into lower holes, coalescing    |
|              free space upward until a hole large enough is formed.      |
|    c. Update mapping:                                                    |
|       - If user extent: update AUID translation table.                   |
|       - If L2/L3 table: update parent L1/L2 table pointer.               |
+--------------------------------------------------------------------------+
| 3. Truncate physical file via BlockCache::truncate() to reclaim disk.    |
+--------------------------------------------------------------------------+
```

### 6.2 L2/L3 Dependency Tracking (`L2L3s`)

Because L2 and L3 table extents do not possess AUIDs ($\text{AUID} = 0$), they cannot be relocated via normal AUID translation. Before shifting blocks:
- `compact()` walks the L1 tables in the header and records every active L2 and L3 table AUN in an in-memory hash container (`L2L3s`).
- When an extent with $\text{AUID} == 0$ is relocated:
  - If L2 table: its new AUN is updated in the parent L1 table (`auid_l1` or `auid_stack_l1`).
  - If L3 table: its new AUN is updated in the parent L2 table on disk.
  - The entry is rehashed in `L2L3s`.

### 6.3 Progressive Downshifting

When the last extent in the file is too large to fit in any existing free hole:
1. Compaction scans forward from `first_au` to find the first free region.
2. Moves the used extent immediately following the hole downward into that free region (`move_unit`).
3. Coalesces free space that accumulates behind the moved block.
4. Repeats until a contiguous free extent large enough to receive the target end-of-file block is generated.

### 6.4 File Truncation

Once the target threshold is reached (e.g. free space $\le 5\%$ or user callback returns `false`), the EOF marker sits immediately after the last used extent. `compact()` computes:
$$\text{new\_size} = (\text{num\_aus} + 1) \times \text{AU\_SIZE}$$
and calls `BlockCache::truncate(new_size)` which flushes caches and executes `ftruncate()` to physically shrink the disk file.

---

## 7. Btree Layer: Disk-Backed Balanced Search Tree

The `Btree` and `BtreeInternal` classes (`Btree.h`, `Btree_internal.h`, `Btree.cc`, `BtreeNode.cc`) implement an on-disk balanced search tree supporting arbitrary variable-length binary keys.

### 7.1 Tree Invariants & Constraints

- **Order ($M$)**: Must be an **odd integer** ($3 \le M \le 51$).
- **Node Capacities**:
  - Maximum items per node: $M - 1$ keys and data pointers.
  - Maximum child pointers per node: $M$.
  - Minimum items per internal/leaf node: $\lfloor M / 2 \rfloor$ items (except the root, which may contain $1 \dots M - 1$ items).
- **Height**: Balanced; all leaf nodes reside at the identical depth.

### 7.2 On-Disk Node Layout (`_BTNodeDisk`)

Nodes are stored as variable-sized `FileBlock` allocations identified by an `FB_AUID_T`:

```
+-------------------------------------------------------------------------------+
| magic (0x4463ab2d, 4B) | _numitems (UINT16_t, 2B: ROOT, LEAF, item count)     |
+-------------------------------------------------------------------------------+
| BTNodeItem items[M-1]:                                                        |
|   - FB_AUID_t ptr       (Left child node AUID)                                |
|   - UINT16_t  keysize   (Length of key bytes)                                 |
|   - FB_AUID_t data      (Application data handle)                             |
+-------------------------------------------------------------------------------+
| FB_AUID_t trailing_ptr  (Rightmost child node AUID)                           |
+-------------------------------------------------------------------------------+
| Packed Key Data Bytes (Contiguous blob of all item keys)                      |
+-------------------------------------------------------------------------------+
```

- Total node allocation size on disk:
  $$\text{node\_size} = \text{sizeof}(\text{\_BTNodeDisk}) + (M-1) \times \text{sizeof}(\text{BTNodeItem}) + \text{sizeof}(\text{FB\_AUID\_t}) + \sum_{i=0}^{N-1} \text{keysize}_i$$
- When keys change size or items are inserted/removed, `BTNode::store()` resizes the on-disk node extent using `fbi->realloc(fbn, new_size)`.

### 7.3 Node Caching (`BTNodeCache`)

- Caches in-memory decoded node representations (`BTNode`) with a maximum capacity of `MAX_NODES = 2000`.
- Manages an intrusive LRU list and hash table keyed on `FB_AUID_T`.
- Nodes hold reference counts (`refcount`). While pinned during tree operations, nodes cannot be evicted.
- Unpinned dirty nodes are serialized and flushed via `BTNode::store()` upon LRU eviction or cache destruction.

### 7.4 Core Algorithms

#### 1. Search (`get(key, keylen, &data_id)`)
- Traverses downward from `info.root_fbn`.
- At each node, `walknode()` performs binary/linear comparison (`compare_keys`) against node keys.
- If an exact match is found, retrieves `data_id` and returns `true`.
- If a leaf is reached without a match, returns `false`. Otherwise, follows child pointer `ptrs[idx]`.

#### 2. Insertion (`put(key, keylen, data_id, replace, ...)`)
- Traverses down to the appropriate leaf, pushing each visited node onto an in-memory ancestor path stack (`nodewalker`).
- If key matches an existing entry:
  - If `replace == true`, overwrites data handle (optionally returning old handle) and returns.
  - If `replace == false`, aborts with `false`.
- If key is new:
  - Inserts item into the leaf in sorted order.
  - If node item count exceeds $M - 1$, `splitnode()` splits the node into two nodes of $\lfloor M / 2 \rfloor$ items each and extracts the median pivot item.
  - Promotes the pivot item and new child pointer to the parent node.
  - Continues splitting and promoting up the ancestor stack as long as parents overflow.
  - If the root node overflows, splits the root and creates a new root containing only the promoted pivot item, incrementing tree depth.

#### 3. Deletion (`del(key, keylen, &old_data_id)`)
- Traverses down to locate the target key, recording the ancestor path.
- If the target key is in an internal node:
  - Swaps the item with its in-order successor from a leaf.
- Deletes the key from the leaf and decrements item count.
- Underflow Handling (if node items $< \lfloor M / 2 \rfloor$ and node is not root):
  - **Borrow (Steal) from Left/Right Sibling**: If adjacent sibling has $> \lfloor M / 2 \rfloor$ items, rotates the parent pivot into the underflowing node and promotes the sibling's boundary item into the parent pivot.
  - **Coalesce (Merge) with Sibling**: If adjacent siblings have only $\lfloor M / 2 \rfloor$ items, merges the node, sibling, and parent pivot into a single full node ($M - 1$ items). Deallocates the empty sibling node.
  - Repeats underflow checks up the ancestor stack. If the root node shrinks to 0 items, the merged child becomes the new root, decrementing tree depth.

#### 4. Iteration (`iterate(BtreeIterator*)`)
- Executes an in-order recursive traversal across all nodes (`iterate_node`).
- Invokes user-defined `bti->handle_item(key, len, data_id)`.
- If the callback returns `false`, terminates traversal early. Modification operations are locked during iteration (`iterate_inprogress`).

---

## 8. High-Level Serialization & Database Classes

`libpfkfb` provides seamless integration with the `libpfkutil/bst.h` binary serialization framework to serialize complex C++ data structures without compiler padding or platform-dependent endian issues.

### 8.1 `FileBlockBST`
- Derives from `BST` and wraps an underlying `FileBlockInterface`.
- Encapsulates encoding, decoding, sizing, and disk allocation:
  - `get(id)`: Fetches extent `id`, initializes a `BST_STREAM_BUFFER`, and decodes fields.
  - `put()`: Calculates serialized size via `BST_OP_CALC_SIZE`, allocates or reallocates the extent via `fbi->alloc()` / `fbi->realloc()`, and encodes fields.
  - `bst_free()`: Recursively frees allocated dynamic members.

### 8.2 Object-Relational Database Abstraction (`BtreeDbClasses.h`)

`BtreeDbClasses.h` provides macro templates for strongly typed key-value records:

```cpp
#define DB_ITEM_CLASS(datum)                                            \
    class datum : public DB_ITEM {                                      \
    public:                                                             \
        datum(Btree * _bt) : DB_ITEM(_bt), data(_bt->get_fbi()) { }     \
        datum##Key key;                                                 \
        datum##Data data;                                               \
        bool put(bool repl=false) { return _put(&key, &data, repl); }   \
        bool get(void)            { return _get(&key, &data); }         \
        bool del(void)            { return _del(&key, &data); }         \
    }
```

- When using `DB_ITEM_CLASS(myRecord)`:
  - Defines `myRecordKey` (derived from `BST`).
  - Defines `myRecordData` (derived from `FileBlockBST`).
  - Calling `myRecord.put()` automatically serializes the data to a new AUID extent and indexes the serialized key into the B-tree.
  - When replacing an existing record, the old data AUID is automatically freed from the `FileBlockInterface`.

---

## 9. Diagnostic & Maintenance Utilities

Three standalone maintenance CLI utilities located in `main/` inspect and manage `libpfkfb` databases:

1. **`fbdump <dbfile>` (`main/fbdump.cc`)**:
   - Opens the target file via `FileBlockInterface::openFile`.
   - Displays extent statistics: AU size, used/free AUs, used/free regions, and total AUs.
   - Executes `FileBlockLocal::validate(true)`, printing the entire allocation map, free lists, bucket headers, and data info blocks while asserting internal consistency.
2. **`fbcompact <dbfile>` (`main/fbcompact.cc`)**:
   - Opens the database and prints storage statistics before compaction.
   - Runs `fbi->compact()` with `compaction_status_function`, streaming ongoing progress (free AU count) and halting when free space falls below 1% of total file size.
   - Prints storage statistics after compaction, reporting reclaimed space.
3. **`btdump <dbfile>` (`main/btdump.cc`)**:
   - Traverses a B-tree database using `BtreeIterator`.
   - Dumps tree parameters (root AUID, node count, record count, depth, order).
   - Recursively outputs every key and associated data payload in formatted hexadecimal.

---

## 10. Threading, Concurrency & Invariant Reference

### Concurrency Model
- **Single-Threaded Context**: `PageCache`, `BlockCache`, `FileBlockLocal`, and `Btree` are **not thread-safe**. Concurrent access to the same database from multiple threads requires external synchronization (e.g. `pthread_mutex_t`).
- **No Overlapping Block Holds**: Applications must not hold overlapping or duplicate `BlockCacheBlock` instances simultaneously.
- **Iteration Lockout**: Writing to a `Btree` while an `iterate()` operation is in progress triggers an immediate fatal error (`iterate_inprogress`).

### Integrity & Safety Checklist

| Invariant Area | Rule | Enforcement Mechanism |
|---|---|---|
| **Magic Signatures** | Header must match `0x0ecc0e35`; Btree info must match `0x0d83f387`; Node must match `0x4463ab2d`. | `valid_file()` validation checks during open. |
| **Odd B-tree Order** | Tree order $M$ must be odd and $\le 51$. | Checked in `Btree::init_file()` and `BtreeInternal`. |
| **Extent Sizes** | Max single allocation cannot exceed $(4096 - 2) \times 512$ bytes (~2 MB). | Enforced in `FileBlockLocal::alloc()`. |
| **AUID Lifetime** | `AUID == 0` is strictly reserved for L2/L3 table metadata. | Checked during compaction and translation. |
| **Intrusive Links** | Node cannot be deleted or double-enqueued without list removal. | Asserted via `DLL3` container methods. |
| **Memory Cleanup** | `FileBlockBST` objects must call `bst_free()` before destruction. | Asserted in `~FileBlockBST()` via `SIGABRT`. |
| **File Compaction** | Compaction cannot run if active `FileBlock` handles are checked out. | Validated via `active_blocks.get_cnt() > 1`. |
