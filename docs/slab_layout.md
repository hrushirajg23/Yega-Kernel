**An explanation of alignment effects + how object count adjusts**
for a page-sized slab containing **struct kmem_cache objects**.

---

# **Given**

* `PAGE_SIZE = 4096`
* `sizeof(slab_t) = 28`
* Alignment requirement = `32`
* `objsize = 100` (for struct kmem_cache)
* After alignment:

  ```
  slab_header_size = ALIGN(28, 32) = 32
  ```

So the slab memory layout starts like:

```
+--------------------+
| slab_t (aligned 32)|
+--------------------+
| objects …          |
```

---

# **Available memory for objects**

```
usable = 4096 - aligned_slab_header
usable = 4096 - 32 = 4064 bytes
```

Number of objects:

```
num_objects = usable / objsize
num_objects = 4064 / 100 = 40 (integer division)
```

So **40 kmem_cache objects fit**.

---

# **Final layout diagram **

```
PAGE (4096 bytes)
┌──────────────────────────────────────────────────────────────┐
│                        slab_t header (aligned)                │
│   struct slab_s {                                             │
│       list_head list;     // 8 bytes                          │
│       unsigned long colouroff;                                │
│       void *s_mem;                                            │
│       unsigned int inuse;                                     │
│       unsigned int free;                                      │
│       void *free_list;                                        │
│   };                                                          │
│   Original size = 28, aligned to 32 bytes                     │
└──────────────────────────────────────────────────────────────┘
 Offset 0x00 → 0x1F (32 bytes)

┌──────────────────────────────────────────────────────────────┐
│ object #0  (struct kmem_cache, 100 bytes)                     │
│   offset: 32 → 131                                            │
├──────────────────────────────────────────────────────────────┤
│ object #1                                                     │
│   offset: 132 → 231                                           │
├──────────────────────────────────────────────────────────────┤
│ object #2                                                     │
│   offset: 232 → 331                                           │
├──────────────────────────────────────────────────────────────┤
│ ... 37 more objects ...                                       │
├──────────────────────────────────────────────────────────────┤
│ object #39                                                    │
│   last object ends at: 32 + 40*100 = 4032 bytes               │
└──────────────────────────────────────────────────────────────┘

(64 bytes remain unused: 4096 - 4032 = 64)
```

---

# **What if alignment pushes header too far?**

Imagine:

* The slab header is 38 bytes
* Alignment = 32
  → `ALIGN(38, 32) = 64`

So now header uses 64 bytes rather than 38.

This reduces space for objects:

```
usable = 4096 - 64 = 4032
num_objects = 4032 / 100 = 40 still
```

But if `objsize` were larger (e.g., 200), then:

```
4032 / 200 = 20   instead of 20.29 → 20
```

If alignment makes header exceed a boundary such that:

```
aligned_header + (num_objects * objsize) > PAGE_SIZE
```

→ **Linux decrements `num_objects` by 1**

This is exactly what the kernel check does:

```c
if (slab_mgmt_size(align) + nr_objs * objsize > slab_size)
    nr_objs--;
```

This ensures all objects + aligned metadata fit.

---

# Why align the slab descriptor?

Because **objects inside the slab require alignment**—usually CPU cache alignment (like 32 bytes for x86 L1).
If `slab_t` is not aligned:

* object #0 might start at a bad boundary
* causing unaligned access
* causing cacheline split penalties
* SLAB allocator performance drops heavily

So it's crucial.

---

# Final summary

| Item                | Value    |
| ------------------- | -------- |
| Page size           | 4096     |
| slab_t size         | 28       |
| aligned slab_t size | 32       |
| object size         | 100      |
| number of objects   | **40**   |
| unused space        | 64 bytes |

Everything fits cleanly in one page frame.

---

# Slab colouring motive

The CPU cache uses bits of the physical address to pick a cache set (and the low bits for the byte/line offset).

If many objects all have the same offset inside their page, those objects share the same low physical address bits that the cache uses for indexing, so they tend to map to the same cache set.

If too many objects map to the same set, they exceed the cache associativity and evict each other → cache thrashing.

Slab coloring spreads object placements across different offsets (or pages chosen so the cache-index bits differ), avoiding the set conflicts.
