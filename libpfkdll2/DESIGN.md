# libpfkdll2: Intrusive Doubly-Linked Containers & Memory Integrity Library

## 1. Architectural Overview & Philosophy

`libpfkdll2` is a C++ (with C bindings) template library providing high-performance, **intrusive** container data structures designed for mission-critical systems programming. Authored by Phillip F. Knaack (initial specification March 2007, released into the public domain via the Unlicense), `libpfkdll2` forms the foundation for data management across several subsystems in `pfkutils`.

The core mission of `libpfkdll2` is to solve two fundamental problems that plague complex systems software:
1. **Dynamic Memory Allocation Overhead & Non-Determinism**: Standard non-intrusive containers (such as `std::list`, `std::map`, `std::unordered_map`) allocate node metadata wrappers on the heap each time an item is inserted. In real-time threads, high-frequency event loops, or memory-constrained daemons, dynamic heap allocation introduces latency jitter, memory fragmentation, and allocation failure edge cases.
2. **Latent Pointer Corruption in Custom Linked Lists**: When teams attempt to avoid container overhead by rolling custom linked lists or C-style macros, subtle bugs inevitably emerge—elements freed while still on lists, elements enqueued simultaneously onto conflicting lists, cyclic loops, or traversals over mutated pointers. These corruptions typically trigger crashes minutes or hours after the fault occurred, making post-mortem debugging extremely difficult.

`libpfkdll2` resolves both challenges by combining **zero-allocation intrusive linkage** with **fail-fast, self-validating integrity checking**:
- Data items directly embed an array of link structures (`LListLinks<T>`).
- Inserting, removing, or re-ordering elements requires **zero heap allocations** ($O(1)$ pointer operations).
- Every link tracks not only adjacent elements (`next`, `prev`), but also **points back to the owning container** (`onlist`).
- Every mutation and traversal validates pointer integrity and magic checksums, triggering immediate, deterministic crashes (`SIGABRT`) at the exact point of contract violation.

```
+-------------------------------------------------------------------------------+
|                                  Domain Object                                |
|  +-------------------------------------------------------------------------+  |
|  | Payload: int id; char buffer[512]; ...                                  |  |
|  +-------------------------------------------------------------------------+  |
|  | links[0]: next | prev | onlist (-> LListHash)    | checksum             |  |
|  | links[1]: next | prev | onlist (-> LListLRU)     | checksum             |  |
|  | links[2]: next | prev | onlist (-> Clean/Dirty)  | checksum             |  |
|  +-------------------------------------------------------------------------+  |
+-------------------------------------------------------------------------------+
```

---

## 2. Failure Modes Prevented: The "Why"

Linked list bugs in production environments are notoriously insidious due to the **time delay phenomenon**: a corrupted pointer or cross-linked node might sit dormant until another subsystem traverses the damaged chain long after the stack trace and register context of the offending code are gone.

`libpfkdll2` was explicitly designed to catch the following six classic failure modes on their **very first occurrence**:

### 2.1 Freeing an Element Still on a List (Use-After-Free & Cross-Linking)
- **Problem**: An element is released back to the free store (`delete` or `free()`), yet adjacent items in a list (or the list head/tail) still hold pointers to it. When the memory allocator subsequently reissues that memory block to another caller, modifying the memory corrupts the active linked list. If the block is enqueued onto another list, the two lists become cross-linked.
- **DLL2 Defense**: The destructor `~LListLinks()` explicitly validates that `onlist == 0`. If an item is deleted while still enqueued on any container, execution immediately aborts with:
  ```
  ERROR LLIST ITEM DELETED BUT STILL ON A LIST!
  ```

### 2.2 Enqueueing an Element Already on Another List (or the Same List)
- **Problem**: Code attempts to enqueue an item without verifying whether it is already enqueued. If it is already on another list, the previous list's pointers are severed, or two unrelated lists become intertwined. If re-added to the *same* list, the list can become circular, sending downstream loops into 100% CPU infinite hangs.
- **DLL2 Defense**: Before inserting any element, `LList::add()` and `add_before()`/`add_after()` invoke `onlist(x)` (checking if `links[instance].onlist != 0`). If non-zero, it immediately aborts:
  ```
  ERROR LLIST ENTRY ALREADY ON LIST
  ```

### 2.3 Relative Insertion with Mismatched Container References
- **Problem**: Calling `add_after(new_item, existing_item)` or `add_before()` assumes `existing_item` resides in the target list. If `existing_item` belongs to a different list, the algorithm updates the head/tail pointers of the wrong list, severing or cross-linking both containers.
- **DLL2 Defense**: Both methods assert that `onthislist(existing)` is true (`existing->links[instance].onlist == (void*)this`) and that `!onlist(new_item)`. Violations immediately trigger:
  ```
  ERROR CANNOT DO add_after WHEN EXISTING ITEM NOT ON THIS LIST
  ```

### 2.4 Removing an Element from the Wrong Container
- **Problem**: A module passes an element to `container_A.remove(item)`, but the item is actually enqueued on `container_B` (or on no container at all). If unchecked, `container_A` decrements its count and splices adjacent pointers that might not belong to `container_A`, corrupting boundary pointers (`head`/`tail`).
- **DLL2 Defense**: `LList::remove()` validates `onthislist(x)`. If the link's `onlist` pointer does not match `(void*)this`, it aborts:
  ```
  ERROR LLIST ENTRY NOT ON THIS LIST
  ```

### 2.5 Traversal Invalidation During Element Relocation
- **Problem**: A common idiom walks a list with a `for` loop:
  ```cpp
  for (item = list.get_head(); item != NULL; item = list.get_next(item)) {
      if (should_move(item)) {
          list.remove(item);
          other_list.add(item);
      }
  }
  ```
  When the item is moved to `other_list`, its `next` pointer is updated to reflect its new position in `other_list`. In the next loop iteration, `list.get_next(item)` follows a pointer into `other_list`, inadvertently jumping lists and causing chaotic traversals.
- **DLL2 Defense**: When `strict_walking` is enabled (the default), `get_next(x)` and `get_prev(x)` assert `onthislist(x)`. If `item` is no longer on `list`, it halts instantly:
  ```
  ERROR get_next ITEM is not on this list!
  ```

### 2.6 Following Pointers of Deleted Elements
- **Problem**: If an element is dequeued and freed inside a loop, the subsequent `get_next()` reads freed memory, causing race conditions in multi-threaded code or segmentation/bus faults on demand-paged OS kernels where memory has been unmapped via `brk`/`mmap`.
- **DLL2 Defense**: `remove()` thoroughly scrubs pointers: `next = prev = onlist = NULL`, and recalculates checksums. Dereferencing an unlinked item immediately flags an invalid state.

---

## 3. Core Architectural Mechanisms & Invariants

### 3.1 Bidirectional Awareness: The `onlist` Pointer
Conventional doubly-linked lists store two pointers per node:
```
Node: [ next | prev ]
```
`libpfkdll2` introduces a third mandatory pointer:
```
Node: [ next | prev | onlist | checksum ]
```
- `onlist == NULL`: The item is guaranteed to be unattached to any container on this instance index.
- `onlist == (void*)container`: The item is currently enqueued on the specified container instance.

This single pointer allows constant-time $O(1)$ verification of list membership without needing to search the container.

### 3.2 Multi-List Membership via Link Arrays & Instance Numbers
A single domain object often needs to exist in multiple collections simultaneously (e.g., an LRU cache list, a hash table keyed by ID, and a work queue). Rather than using multiple heap wrappers or distinct named link members, `libpfkdll2` defines an array:
```cpp
struct MyItem {
    LListLinks<MyItem> links[NUM_LISTS];
    // ... payload ...
};
```
Each container template takes a compile-time `instance` integer:
```cpp
LList<MyItem, LIST_ACTIVE> active_list;
LListHash<MyItem, int, MyComparator, LIST_HASH> id_hash;
```
The container accesses only its designated index `links[instance]`.

#### Instance Multiplexing (Mutual Exclusion Optimization)
If two lists are mutually exclusive—meaning an item can never be on both lists simultaneously—they can **share the same instance index**.
For example, in a disk cache:
- An item is either *Clean* or *Dirty*, never both.
- Both `clean_list` and `dirty_list` can share instance index 2 (`CACHE_CLEAN_DIRTY_LIST`).
- This minimizes the footprint of the `links[]` array in every item.

### 3.3 Memory Integrity & Checksum Verification
To guard against memory corruption caused by wild pointers, out-of-bounds array writes, or buffer overruns in neighboring fields, `libpfkdll2` computes arithmetic checksums over link structures.

#### Link Checksum Formula:
```cpp
unsigned long calc_checksum(void) const {
    return 0xb09d3086U + (unsigned long) next +
           (unsigned long) prev + (unsigned long) onlist;
}
```

#### Container Checksum Formula:
```cpp
unsigned long calc_checksum(void) const {
    return 0x2df692f8U + (unsigned long) head +
           (unsigned long) tail + (unsigned long) cnt;
}
```

Whenever any pointer is altered, `recalc()` updates the checksum. On reads, mutations, and destructions, `validate()` checks the stored checksum against `calc_checksum()`. A mismatch signals immediate memory corruption:
```
CHECKSUM ERROR IN DLL2 ELEMENT
CHECKSUM ERROR IN DLL2 LLIST
```

### 3.4 Full Pointer Scrubbing
Unlike naive list implementations that leave dangling `next`/`prev` pointers in detached nodes, `LList::remove()` unconditionally wipes all linkage pointers:
```cpp
ll->onlist = 0;
ll->next = ll->prev = NULL;
ll->recalc();
```
An unlinked node is strictly sanitized, preventing stale traversals.

### 3.5 Fail-Fast Diagnostic Architecture (`DLL2BAIL`)
`libpfkdll2` rejects graceful error degradation for internal data-structure contract violations. A corrupted linked list indicates compromised process memory; continuing execution risks persistent data corruption.

By default, any validation error invokes:
```cpp
#define DLL2BAIL(reason) { printf reason ; kill( 0, 6 ); }
```
`kill(0, 6)` raises `SIGABRT` to the current process group, generating an immediate core dump at the exact line of the failure. Developers can override `DLL2BAIL` prior to including `dll2.h` to redirect diagnostics to syslog or custom assertion handlers.

---

## 4. Container Catalog: Structures & Algorithms

`libpfkdll2` includes seven container types, each tailored to specific access patterns.

### 4.1 Summary of Containers & Complexities

| Container | Header | Primary Operations | Time Complexity | Memory Allocation |
|---|---|---|---|---|
| `LList<T, instance>` | `dll2.h` | `add`, `remove`, `add_before`, `add_after`, `dequeue_head`, `dequeue_tail` | $O(1)$ | Zero (Intrusive) |
| `LListHash<T, Key, Comp, instance>` | `dll2.h` | `add`, `remove`, `find` | $O(1)$ average | Dynamic bucket array rehash only; zero per item |
| `LListLRU<T, instance>` | `dll2.h` | `add`, `remove`, `promote`, `get_oldest` | $O(1)$ | Zero (Intrusive) |
| `LListHashLRU<T, Key, Comp, h_inst, lru_inst>` | `dll2.h` | `find`, `promote`, `dequeue_lru_head` | $O(1)$ average | Rehash only; zero per item |
| `LListOrderedQ<T, instance>` | `dll2.h` | `add` (in relative delta order), `remove`, head tick | $O(N)$ insert, $O(1)$ head dequeue/remove | Zero (Intrusive) |
| `LListBTREE<T, Key, Comp, instance, ORDER>` | `dll2_btree.h` | `add`, `remove`, `find`, `get_first` | $O(\log_B N)$ | Intrusive traversal stack; allocates tree nodes |
| C API (`DLL2_LIST`) | `dll2_c.h` | `DLL2_LIST_ADD`, `DLL2_LIST_REMOVE`, `DLL2_LIST_NEXT` | $O(1)$ | Zero (Intrusive) |

---

### 4.2 `LList<T, instance, strict_walking, use_checksum>`

The standard intrusive doubly-linked list.
- **Head & Tail Pointers**: Supports double-ended queue operations (`dequeue_head`, `dequeue_tail`).
- **Internal Counter**: Maintains a synchronized `cnt` field ($O(1)$ `get_cnt()`).
- **Safe Traversal Pattern**:
  When elements must be removed or moved during iteration, cache the next pointer before the mutation:
  ```cpp
  T *curr, *next;
  for (curr = list.get_head(); curr != NULL; curr = next) {
      next = list.get_next(curr); // Cache successor before mutating curr
      if (evaluate(curr)) {
          list.remove(curr);
      }
  }
  ```

---

### 4.3 `LListHash<T, Hash_Key_Type, Hash_Key_Comparator, instance>`

An intrusive hash table featuring **dynamic self-balancing** and **zero per-node allocation**.

#### Auto-Rehashing Engine
`LListHash` is backed by a dynamic array of `LList` buckets:
```cpp
theLListHash * hash; // array of LList<T, instance>
```
The bucket array size dynamically resizes through a hardcoded sequence of 16 prime numbers (`dll2_hash_primes` defined in `dll2_hash.cc`, computed by `calc_primes.c`):
`97`, `251`, `499`, `997`, `1999`, `4999`, `9973`, `24989`, `49999`, `99991`, `249989`, `499979`, `999983`, `2499997`, `4999999`, `9999991`.

After every insertion and deletion, `recalc_hash()` evaluates the average bucket load factor:
```cpp
int average = count / hashsize;
if ( average > 5 && hashorder < DLL2_NUM_HASH_PRIMES )
    rehash( dll2_hash_primes[ ++hashorder ] );
else if ( average < 1 && hashorder > 0 )
    rehash( dll2_hash_primes[ --hashorder ] );
```
Rehashing is entirely zero-copy: nodes are simply dequeued from old buckets and enqueued into new buckets without any heap allocations for nodes.

#### The $O(1)$ Reverse-Engineered Bucket Index (`rev_eng_hashind`)
A notable design innovation in `LListHash` is how `remove(t)` and `onthislist(t)` locate the bucket containing item `t`.
Normally, removing an item from a hash table requires either supplying the key or storing a bucket pointer in each node. `LListHash` avoids both by **reverse-engineering the bucket index directly from the `onlist` pointer**:
```cpp
theLListHash * rev_eng_hashind( T * t ) const {
    unsigned long min  = (unsigned long) & hash[ 0 ];
    unsigned long max  = (unsigned long) & hash[ hashsize-1 ];
    unsigned long mine = (unsigned long) t->links[instance].get_onlist();
    if ( mine == 0 || mine < min || mine > max ) return NULL;
    unsigned long off  = mine - min;
    if (( off % sizeof(theLListHash) ) != 0 )
        DLL2BAIL(( "LLISTHASH corrupt onlist pointer?\n" ));
    return (theLListHash *) mine;
}
```
Because `links[instance].onlist` points to the specific bucket `LList` in the contiguous `hash[]` array:
1. `LListHash` validates that `onlist` falls within the memory boundaries of `&hash[0]` and `&hash[hashsize-1]`.
2. It asserts proper alignment via modulo `sizeof(theLListHash)`.
3. It directly converts `mine` into `theLListHash*` and invokes `lst->remove(t)`.

This guarantees **true $O(1)$ removal without hashing the key, searching the bucket, or allocating node metadata**.

#### Comparator Contract
The user provides a stateless companion class:
```cpp
class MyComparator {
public:
    static int hash_key( MyItem * item );
    static int hash_key( const KeyType key );
    static bool hash_key_compare( MyItem * item, const KeyType key );
};
```

---

### 4.4 `LListLRU<T, instance>`

Implements a Least-Recently-Used queue on top of `LList`.
- **Invariants**:
  - `head`: Oldest item (eviction candidate).
  - `tail`: Newest / most recently promoted item.
- **Operations**:
  - `promote(T* t)`: If `t` is not already at the tail, removes `t` and re-adds it to the tail in $O(1)$ time.
  - `get_oldest()`: Returns `head` without modifying the list.
  - `dequeue_head()`: Evicts and returns the oldest element.

---

### 4.5 `LListHashLRU<T, Hash_Key_Type, Hash_Key_Comparator, hash_instance, lru_instance>`

A compound container coupling `LListHash` and `LListLRU` into an atomic abstraction.
- **Invariant Enforcement**: Guarantees transactional consistency between the hash index and eviction queue. An item cannot exist in the hash without being in the LRU, or vice-versa.
- **Encapsulation**: Component `hash` and `lru` instances are private; mutations (`add`, `remove`, `dequeue_lru_head`) update both structures simultaneously.

---

### 4.6 `LListOrderedQ<T, instance>`: The Relative Delta Timer Queue

`LListOrderedQ` is a specialized priority queue designed after the classic UNIX kernel callout / timer wheel queue.

#### Node Requirement
The stored type `T` must define an integer member:
```cpp
int ordered_queue_key;
```

#### Relative Delta Encoding
Instead of storing absolute timestamps (which would require updating every queued item on every tick), keys are stored **relative to the preceding item**:

```
Queue: [Head: T1 (key=12)] -> [T2 (key=3)] -> [T3 (key=5)]
```
- Real time for T1 to expire: 12 ticks.
- Real time for T2 to expire: $12 + 3 = 15$ ticks.
- Real time for T3 to expire: $15 + 5 = 20$ ticks.

#### Operational Mechanics
1. **Clock Tick**: Only the head element's `ordered_queue_key` is decremented. When it hits zero, `dequeue_head()` removes it. The next element becomes the head, and its key already represents the exact number of ticks until its own expiration.
2. **Insertion (`add(t, timeout)`)**: Walks forward, subtracting each node's relative key from `timeout` until `timeout < curr->ordered_queue_key`. Slices the node into position and adjusts the successor's relative key:
   ```cpp
   curr->ordered_queue_key -= new_oq_key;
   oq.add_before( t, curr );
   ```
3. **Removal (`remove(t)`)**: If `t` has a successor `n`, `t`'s remaining relative delta is folded back into `n`:
   ```cpp
   if ( n ) n->ordered_queue_key += t->ordered_queue_key;
   oq.remove( t );
   ```

---

### 4.7 `LListBTREE<T, Key, Comparator, instance, ORDER>`

An in-memory, order-parameterized B-Tree (`dll2_btree.h`).

#### Intrusive Traversal Stack (`NODESTORE`)
B-Tree operations (splits, promotions, sibling borrows, coalescing) require backtracking along the traversal path from the root to the leaf. Traditional implementations either use recursion or dynamic stack buffers.

`LListBTREE` solves this by having `LListBTREENode` embed `LListLinks`:
```cpp
template <class T, int ORDER, ...>
struct LListBTREENode {
    LListLinks<LListBTREENode> links[1];
    char numkeys, cur;
    bool leaf, root;
    LListBTREENode * nodes[ORDER];
    T * keys[ORDER-1];
};
```
During traversal, the path is tracked using an intrusive `LList<NODE, 0>` named `NODESTORE`. **Zero heap memory is allocated to maintain traversal history during insertions or deletions.**

#### Rebalancing Invariants
- **Insertion**: Full nodes split at `HALF_ORDER = ORDER / 2`. The pivot key ascends to the parent. Overflows at the root create a new root, increasing tree depth.
- **Deletion**:
  - Non-leaf items are replaced by the smallest key in their right subtree.
  - If a leaf drops below `HALF_ORDER` keys, the tree attempts **redistribution** ("stealing" a key and subtree from an adjacent left or right sibling with $> \text{HALF\_ORDER}$ keys).
  - If siblings are at capacity, it performs **coalescing** (merging two half-full siblings plus the parent's pivot key into a single full node).
  - Collapsing the root decreases tree depth.

---

## 5. The C API: `dll2_c.h` and `dll2_c.c`

For pure-C modules or C/C++ interface boundaries, `libpfkdll2` provides a C binding supporting doubly-linked lists.

### 5.1 Data Structures
```c
typedef struct {
    void             * links_next;
    void             * links_prev;
    struct DLL2_LIST * links_onlist;
#if DLL2_MAGIC_VALIDATIONS
    int                links_magic;  /* 0x4bf4e59c */
#endif
} DLL2_LINKS;

typedef struct DLL2_LIST {
    void * list_head;
    void * list_tail;
    int    list_count;
    int    list_index;
#if DLL2_MAGIC_VALIDATIONS
    int    list_magic;   /* 0x5c2df716 */
#endif
} DLL2_LIST;
```

### 5.2 Dynamic Offset Calculation
Because C lacks templates and inheritance, the runtime resolves the `links` member location using byte offsets:
```c
#define DLL2_LINKS_OFFSET(item) \
    (((unsigned int)((item)->links)) - ((unsigned int)(item)))
```
Functions like `dll2_list_add` take `links_offset` and use pointer arithmetic:
```c
#define LINKS(list,item) \
    (&((DLL2_LINKS*)(((unsigned int)item) + links_offset))[list->list_index])
```

### 5.3 Manual Lifecycle Contracts
Because C has no RAII constructors or destructors, lifecycle macros must be manually invoked:
- Item Creation: `DLL2_ITEM_INIT(item_ptr);`
- Item Freeing: `DLL2_ITEM_DEINIT(item_ptr);` (asserts item is not on any list)
- List Creation: `DLL2_LIST_INIT(&list, instance_index);`
- List Freeing: `DLL2_LIST_DEINIT(&list);` (asserts list is empty)

---

## 6. Design Decisions & Trade-Offs

### 6.1 Intrusive vs. Non-Intrusive Containers
- **Decision**: Intrusive nodes embedded in domain structures.
- **Rationale**:
  - Eliminates heap allocator locks and fragmentation in real-time execution paths.
  - Guarantees $O(1)$ removal given only a pointer to the item itself (no search required).
  - Natural representation of objects existing on multiple simultaneous collections.
- **Trade-off**: Data structures must be defined with explicit container awareness (`links[N]`).

### 6.2 Array-Based Links (`links[N]`) vs. Distinct Named Members
- **Decision**: Array indexed by integer instance numbers.
- **Rationale**: Allows generic container templates to bind to an index (`instance`) rather than requiring member pointers or macros. Enables instance multiplexing for mutually exclusive lists.
- **Trade-off**: The `links` member must be strictly named `links`. In C++, friend declarations in `LListLinks` were historically pinned to specific instance bounds (`0..3`).

### 6.3 Fail-Fast Termination vs. Error Return Codes
- **Decision**: Immediate `SIGABRT` via `kill(0, 6)` upon validation failure.
- **Rationale**: Memory corruptions in intrusive lists compromise process state. Returning an error code allows corrupted state to persist, guaranteeing later nondeterministic failures. Failing fast produces a core dump pinpointing the exact offending instruction.
- **Trade-off**: Fatal aborts are unsuitable for unhandled user input errors, but ideal for internal invariant validation.

### 6.4 Concurrency Model
- **Decision**: No internal mutex locking inside `libpfkdll2` containers.
- **Rationale**: High-performance intrusive lists are often used within thread-local queues, single-threaded event loops, or structures already guarded by higher-level locks. Locking at the list level would impose significant overhead.
- **Evolution**: This design led to `libpfkutil/dll3`, which introduced compile-time lock checking (`lockWarn`).

---

## 7. Concrete Usage Guide

### 7.1 Basic Doubly-Linked List (`LList`)

```cpp
#include <stdio.h>
#include "dll2.h"

enum {
    WORK_QUEUE,
    NUM_LISTS
};

struct Job {
    LListLinks<Job> links[NUM_LISTS];
    int job_id;
    const char * command;

    Job(int id, const char * cmd) : job_id(id), command(cmd) {}
};

int main() {
    LList<Job, WORK_QUEUE> queue;

    Job j1(101, "compile");
    Job j2(102, "link");
    Job j3(103, "test");

    // Add elements (adds to tail)
    queue.add(&j1);
    queue.add(&j2);
    queue.add(&j3);

    printf("Queue size: %d\n", queue.get_cnt());

    // Iteration
    for (Job * j = queue.get_head(); j != NULL; j = queue.get_next(j)) {
        printf("Job %d: %s\n", j->job_id, j->command);
    }

    // Safe mutation/deletion during walk
    Job * curr, * next;
    for (curr = queue.get_head(); curr != NULL; curr = next) {
        next = queue.get_next(curr);
        if (curr->job_id == 102) {
            queue.remove(curr);
        }
    }

    // Dequeue all
    while (Job * j = queue.dequeue_head()) {
        printf("Processed Job %d\n", j->job_id);
    }

    return 0;
}
```

---

### 7.2 Self-Balancing Hash Table (`LListHash`)

```cpp
#include <stdio.h>
#include <string.h>
#include "dll2.h"

enum {
    SYM_HASH,
    NUM_LISTS
};

struct Symbol {
    LListLinks<Symbol> links[NUM_LISTS];
    char name[32];
    int address;

    Symbol(const char * n, int addr) : address(addr) {
        strncpy(name, n, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }
};

class SymbolComparator {
public:
    static int hash_key(Symbol * s) {
        return hash_key(s->name);
    }
    static int hash_key(const char * str) {
        unsigned int h = 0;
        while (*str) h = (h * 31) ^ (unsigned char)(*str++);
        return (int)(h & 0x7fffffff);
    }
    static bool hash_key_compare(Symbol * s, const char * str) {
        return strcmp(s->name, str) == 0;
    }
};

typedef LListHash<Symbol, const char *, SymbolComparator, SYM_HASH> SymbolTable;

int main() {
    SymbolTable table;

    Symbol * s1 = new Symbol("main", 0x1000);
    Symbol * s2 = new Symbol("printf", 0x2000);

    table.add(s1);
    table.add(s2);

    Symbol * found = table.find("main");
    if (found) {
        printf("Found main at %#x\n", found->address);
    }

    table.remove(s1);
    delete s1;

    table.remove(s2);
    delete s2;

    return 0;
}
```

---

### 7.3 Compound Cache Pattern (Hash + LRU + State Partitions)

This pattern (exemplified in `example_block_cache.cc`) demonstrates how an object participates in a Hash, an LRU eviction list, and state-partitioned lists using instance multiplexing.

```cpp
#include <stdio.h>
#include <string.h>
#include "dll2.h"

enum {
    CACHE_HASH,
    CACHE_LRU,
    CACHE_CLEAN_DIRTY,  // Clean and Dirty lists share this instance!
    NUM_LISTS
};

struct CacheBlock {
    LListLinks<CacheBlock> links[NUM_LISTS];
    int block_id;
    char data[4096];

    CacheBlock(int id) : block_id(id) {}
};

class BlockComparator {
public:
    static int hash_key(CacheBlock * b) { return b->block_id; }
    static int hash_key(int id) { return id; }
    static bool hash_key_compare(CacheBlock * b, int id) { return b->block_id == id; }
};

class DiskCache {
    typedef LListHash<CacheBlock, int, BlockComparator, CACHE_HASH> Hash_t;
    typedef LListLRU<CacheBlock, CACHE_LRU> LRU_t;
    typedef LList<CacheBlock, CACHE_CLEAN_DIRTY> PartitionList_t;

    Hash_t          hash;
    LRU_t           lru;
    PartitionList_t clean_list;
    PartitionList_t dirty_list;
    int             max_capacity;

public:
    DiskCache(int cap) : max_capacity(cap) {}

    CacheBlock * access(int id, bool will_modify) {
        CacheBlock * b = hash.find(id);
        if (!b) {
            // Evict if at capacity
            while (lru.get_cnt() >= max_capacity) {
                CacheBlock * oldest = lru.dequeue_head();
                if (dirty_list.onthislist(oldest)) {
                    // Flush to disk before evicting
                    dirty_list.remove(oldest);
                } else {
                    clean_list.remove(oldest);
                }
                hash.remove(oldest);
                delete oldest;
            }

            b = new CacheBlock(id);
            hash.add(b);
            lru.add(b);
            if (will_modify) dirty_list.add(b);
            else             clean_list.add(b);
        } else {
            lru.promote(b);
            if (will_modify && clean_list.onthislist(b)) {
                clean_list.remove(b);
                dirty_list.add(b);
            }
        }
        return b;
    }
};
```

---

### 7.4 Relative Delta Timer Queue (`LListOrderedQ`)

```cpp
#include <stdio.h>
#include "dll2.h"

enum {
    TIMER_INSTANCE,
    NUM_LISTS
};

struct TimerEvent {
    LListLinks<TimerEvent> links[NUM_LISTS];
    int ordered_queue_key; // Required member for LListOrderedQ
    int event_id;

    TimerEvent(int id) : event_id(id), ordered_queue_key(0) {}
};

int main() {
    LListOrderedQ<TimerEvent, TIMER_INSTANCE> timers;

    TimerEvent t1(1), t2(2), t3(3);

    // Enqueue timers with absolute deadlines: 12, 15, and 20 seconds
    timers.add(&t1, 12);
    timers.add(&t2, 15);
    timers.add(&t3, 20);

    // Internal representation:
    // t1->ordered_queue_key == 12
    // t2->ordered_queue_key == 3   (15 - 12)
    // t3->ordered_queue_key == 5   (20 - 15)

    // Simulate clock ticks:
    int current_tick = 0;
    while (timers.get_cnt() > 0) {
        current_tick++;
        TimerEvent * head = timers.get_head();
        if (head) {
            head->ordered_queue_key--;
            if (head->ordered_queue_key <= 0) {
                timers.remove(head);
                printf("[Tick %d] Event %d expired!\n", current_tick, head->event_id);
            }
        }
    }

    return 0;
}
```

---

### 7.5 In-Memory B-Tree (`LListBTREE`)

```cpp
#include <stdio.h>
#include "dll2.h"
#include "dll2_btree.h"

struct Record {
    LListLinks<Record> links[1];
    int key;
    const char * value;

    Record(int k, const char * v) : key(k), value(v) {}
};

class RecordComparator {
public:
    static int key_compare(Record * r, int key) {
        if (r->key < key) return 1;
        if (r->key > key) return -1;
        return 0;
    }
    static int key_compare(Record * r1, Record * r2) {
        if (r1->key < r2->key) return 1;
        if (r1->key > r2->key) return -1;
        return 0;
    }
    static char * key_format(Record * r) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "%d", r->key);
        return buf;
    }
};

int main() {
    // B-Tree with branching order 5
    LListBTREE<Record, int, RecordComparator, 0, 5> tree;

    Record r1(50, "fifty");
    Record r2(20, "twenty");
    Record r3(80, "eighty");

    tree.add(&r1);
    tree.add(&r2);
    tree.add(&r3);

    Record * res = tree.find(20);
    if (res) {
        printf("Found: %s\n", res->value);
    }

    tree.remove(&r2);
    return 0;
}
```

---

### 7.6 Pure C API Usage (`dll2_c.h`)

```c
#include <stdio.h>
#include <stdlib.h>
#include "dll2_c.h"

enum {
    C_LIST_INDEX,
    C_NUM_LISTS
};

typedef struct {
    int id;
    DLL2_LINKS links[C_NUM_LISTS];
} CItem;

int main() {
    DLL2_LIST my_list;
    DLL2_LIST_INIT(&my_list, C_LIST_INDEX);

    CItem * item = (CItem*) malloc(sizeof(CItem));
    DLL2_ITEM_INIT(item);
    item->id = 42;

    DLL2_LIST_ADD(&my_list, item);

    printf("List size: %d\n", DLL2_LIST_SIZE(&my_list));

    // Traversal
    CItem * curr;
    for (curr = (CItem*) DLL2_LIST_HEAD(&my_list); curr != NULL;
         curr = (CItem*) DLL2_LIST_NEXT(&my_list, curr)) {
        printf("Item ID: %d\n", curr->id);
    }

    DLL2_LIST_REMOVE(&my_list, item);
    DLL2_ITEM_DEINIT(item);
    free(item);

    DLL2_LIST_DEINIT(&my_list);
    return 0;
}
```

---

## 8. Evolution: From `libpfkdll2` to `libpfkutil/dll3`

While `libpfkdll2` established the core principles of intrusive memory protection, production experience across `pfkutils` drove several architectural refinements that culminated in `libpfkutil/dll3`:

1. **Inheritance-Based Linkage vs. Member Arrays**:
   - `dll2`: Required a struct member `links[N]` and friend declarations for template instances.
   - `dll3`: Switched to template inheritance:
     ```cpp
     class MyItem : public DLL3::List<MyItem, LIST_ID>::Links { ... };
     ```
     This eliminated fixed link array dimensions and improved type safety.
2. **Integrated Concurrency Assertions (`lockWarn`)**:
   - `dll2`: Containers had no thread awareness; missing locks caused race conditions.
   - `dll3`: Containers derive from `WaitUtil::Lockable`. Operations evaluate `lockwarn()`, throwing assertions if accessed by a thread without holding the container's associated mutex.
3. **Template Metaprogramming & Modern C++ Support**:
   - `dll3` restructured templates into `.tcc` files, refined namespace encapsulation (`namespace DLL3`), and expanded validation options (`validate` flags).

Despite the existence of `dll3`, `libpfkdll2` remains an active and critical component within `pfkutils`, providing rock-solid intrusive container semantics, in-memory B-Trees, and self-balancing hash tables with proven reliability.
