# Ext2 Filesystem Layout

This document describes the physical layout of the Ext2 filesystem implementation for the first 3 Block Groups.

## Global Parameters
- **Block Size:** 1024 bytes
- **Blocks Per Group:** 8192
- **Inodes Per Group:** 1712
- **Inode Table Size:** 214 blocks

---

## Block Group 0
**Range:** Blocks 0 - 8191
**Role:** Primary Group (Contains Boot Block, Master Superblock, Master BGDT)

| Block Number | Content | Notes |
| :--- | :--- | :--- |
| **0** | Reserved / Boot Code | Always reserved. |
| **1** | **Superblock** | Primary copy. |
| **2** | **BGDT** | Block Group Descriptor Table. |
| **3** | **Block Bitmap** | Tracks usage of blocks in Group 0. |
| **4** | **Inode Bitmap** | Tracks usage of inodes in Group 0. |
| **5 - 218** | **Inode Table** | 214 Blocks long. (Indices 1 to 1712) |
| **219+** | Data Blocks | Free for file content / directories. |

**Offset Logic:**
- Start: `0`
- Extra (SB+BGDT): `2`
- Constant: `1`
- **Bitmap Location:** `0 + 2 + 1 = 3`

---

## Block Group 1
**Range:** Blocks 8192 - 16383
**Role:** Backup Group (Contains Backup Superblock, Backup BGDT)

| Block Number | Content | Notes |
| :--- | :--- | :--- |
| **8192** | Reserved / Padding | Maintains alignment with Group 0 structure. |
| **8193** | **Superblock** | Backup copy. |
| **8194** | **BGDT** | Backup copy. |
| **8195** | **Block Bitmap** | Tracks usage of blocks in Group 1. |
| **8196** | **Inode Bitmap** | Tracks usage of inodes in Group 1. |
| **8197 - 8410** | **Inode Table** | 214 Blocks long. |
| **8411+** | Data Blocks | Free for file content. |

**Offset Logic:**
- Start: `8192`
- Extra (SB+BGDT): `2`
- Constant: `1`
- **Bitmap Location:** `8192 + 2 + 1 = 8195`

---

## Block Group 2
**Range:** Blocks 16384 - 24575
**Role:** Standard Group (No Backups)

| Block Number | Content | Notes |
| :--- | :--- | :--- |
| **16384** | Reserved / Padding | Maintains alignment. Uses relative index 0. |
| **16385** | **Block Bitmap** | Note: No SB/BGDT here. |
| **16386** | **Inode Bitmap** | |
| **16387 - 16600** | **Inode Table** | 214 Blocks long. |
| **16601+** | Data Blocks | |

**Offset Logic:**
- Start: `16384`
- Extra: `0`
- Constant: `1`
- **Bitmap Location:** `16384 + 0 + 1 = 16385`

---

## Important Implementation Detail
The calculation `group_base + extra_blocks + 1` is used to determine the Block Bitmap location. This implicitly reserves the **first block of every group** (relative index 0) as padding, regardless of whether it serves as a Boot Block or not.
