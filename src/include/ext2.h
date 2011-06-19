#ifndef __EXT2_H__

#define __EXT2_H__


/*
 * Superblock
 */
typedef struct {
    uint32_t inodes_count;
    uint32_t blocks_count;
    uint32_t reserved_blocks_count;
    uint32_t free_blocks_count;
    uint32_t free_inodes_count;
    uint32_t first_data_block;
    uint32_t log2_block_size;
    uint32_t log2_fragment_size;
    uint32_t blocks_per_group;
    uint32_t fragments_per_group;
    uint32_t inodes_per_group;
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t mount_count;
    uint16_t max_mount_count;
    uint16_t magic;
    uint16_t state;
    uint16_t error_procedure;
    uint16_t :16;
    uint32_t last_check;
    uint32_t check_interval;
    uint32_t os;
    uint32_t revision_level;
    uint16_t reserved_blocks_uid;
    uint16_t reserved_blocks_gid;
    
    // Solo disponibles con revision_level == 1
    uint32_t first_free_inode;
    uint16_t inode_size;
    uint16_t superblock_block_group_no;

    uint32_t reserved[233]; // Padding hasta el final del superblock
} __attribute__((__packed__)) ext2_superblock;


/*
 * Block Group Descriptor
 */
typedef struct {
    uint32_t block_bitmap_bno;
    uint32_t inode_bitmap_bno;
    uint32_t inode_table_bno;
    uint16_t free_blocks_count;
    uint16_t free_inodes_count;
    uint16_t dirs_count;
    uint16_t :14;
} __attribute__((__packed__)) ext2_block_group_descriptor;


/*
 * Informacion de la particion ext2
 */
typedef struct {
    ext2_superblock *superblock;
    ext2_block_group_descriptor *bgd_table;

    bool initialized;

    blockdev *part;
} __attribute__((__packed__)) ext2;


void ext2_create(ext2 *part_info, blockdev *part);
chardev *ext2_open(ext2 *part_info, const char *filename, uint32_t flags);


#endif
