#ifndef __EXT2_H__

#define __EXT2_H__

#include <fs.h>

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
    uint16_t pad;
    uint8_t reserved[12];
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


typedef struct ext2_file_chardev ext2_file_chardev;
struct ext2_file_chardev {
    uint32_t clase;
    uint32_t refcount;
    chardev_flush_t *flush;
    chardev_read_t *read;
    chardev_write_t *write;
    chardev_seek_t *seek;

    void *buf;
    uint32_t buf_size;
    uint32_t buf_section;

    ext2 *file_part_info;
    uint32_t file_no;
    uint32_t file_size;
    uint32_t file_offset;

    ext2_file_chardev *next;
    ext2_file_chardev *prev;
}__attribute__((__packed__));


void ext2_init();
void ext2_create(ext2 *part_info, blockdev *part);
chardev *ext2_open(ext2 *part_info, const char *filename, uint32_t flags);
int ext2_stat(ext2 *part_info, const char *filename, stat_t* st);


#endif
