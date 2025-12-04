```markdown
# Slab Coloring in Linux Kernel Memory Management

## Hardware Cache Operation

A hardware cache (L1 or L2) is significantly smaller than main RAM, requiring a **mapping function** to determine which memory addresses can be stored in which cache lines.

**Direct-Mapped Cache:**
```
RAM Address: [Tag | Index | Offset]

If cache has 8 lines (index 0-7):
- RAM address 0x0000 → cache line 0
- RAM address 0x0040 → cache line 0 (conflict!)
- RAM address 0x0080 → cache line 0 (conflict!)
- RAM address 0x0008 → cache line 1
```

Multiple RAM addresses map to the **same cache line**. When this occurs, the cache must evict the old data to load new data—this is **cache thrashing**.

## Problem Without Slab Coloring

Consider a slab allocator managing 128-byte objects:

**Without Coloring:**
```
Slab 1:                    Slab 2:                    Slab 3:
┌──────────────┐          ┌──────────────┐          ┌──────────────┐
│ Object A     │ offset 0  │ Object D     │ offset 0  │ Object G     │ offset 0
│ (128 bytes)  │          │ (128 bytes)  │          │ (128 bytes)  │
├──────────────┤          ├──────────────┤          ├──────────────┤
│ Object B     │ offset 128│ Object E     │ offset 128│ Object H     │ offset 128
│ (128 bytes)  │          │ (128 bytes)  │          │ (128 bytes)  │
├──────────────┤          ├──────────────┤          ├──────────────┤
│ Object C     │ offset 256│ Object F     │ offset 256│ Object I     │ offset 256
│ (128 bytes)  │          │ (128 bytes)  │          │ (128 bytes)  │
└──────────────┘          └──────────────┘          └──────────────┘
```

**Issue:**
- Objects A, D, and G all start at offset 0 in their respective slabs
- If these slabs are located at RAM addresses mapping to the **same cache set**, all three objects compete for **identical cache lines**
- When the kernel accesses Object A, then Object D, then returns to A, the cache continuously evicts and reloads data—**cache thrashing**

**Specific Scenario:**
```
Kernel alternates between:
- Object A at RAM 0x10000 → maps to cache line 0
- Object D at RAM 0x20000 → maps to cache line 0 (evicts A!)
- Back to Object A        → maps to cache line 0 (evicts D!)

Result: Dramatically reduced cache hit rate
```

## Slab Coloring Solution

Slab coloring introduces **different offsets** (colors) to objects in different slabs:

**With Coloring:**
```
Slab 1 (color=0):          Slab 2 (color=64):         Slab 3 (color=128):
┌──────────────┐          ┌──────────────┐          ┌──────────────┐
│ [free: 0]    │          │ [free: 64]   │          │ [free: 128]  │ ← colored offset
├──────────────┤          ├──────────────┤          ├──────────────┤
│ Object A     │ offset 0  │ Object D     │ offset 64 │ Object G     │ offset 128
├──────────────┤          ├──────────────┤          ├──────────────┤
│ Object B     │ offset 128│ Object E     │ offset 192│ Object H     │ offset 256
├──────────────┤          ├──────────────┤          ├──────────────┤
│ Object C     │ offset 256│ Object F     │ offset 320│ Object I     │ offset 384
└──────────────┘          └──────────────┘          └──────────────┘
```

**Advantage:**
- Object A at offset 0 → cache lines 0-1
- Object D at offset 64 → cache lines 1-2 (different set)
- Object G at offset 128 → cache lines 2-3 (different set)

Objects now **distribute across different cache lines** rather than competing for identical ones.

## Performance Trade-off

**Cost:** Memory overhead from coloring space (the free gaps at slab start)

**Benefit:** Substantially improved cache hit rates and system performance

This implements the **space-time tradeoff**: sacrificing minimal memory for significantly better access time through optimized cache behavior.
```
