#include <tipos.h>
#include <device.h>
#include <ext2.h>
#include <utils.h>
#include <mm.h>
#include <debug.h>

#define EXT2_SUPERBLOCK_OFFSET      1024
#define EXT2_SUPERBLOCK_SIZE        1024
#define EXT2_SUPERBLOCK_MAGIC       0xEF53
#define EXT2_ROOT_INODE_NO          2

#define EXT2_INODE_DIRECT_COUNT     12
#define EXT2_INODE_INDIRECT_COUNT   1
#define EXT2_INODE_DINDIRECT_COUNT  1
#define EXT2_INODE_TINDIRECT_COUNT  1
#define EXT2_INODE_TOTALBLOCKS      \
    EXT2_INODE_DIRECT_COUNT + EXT2_INODE_INDIRECT_COUNT + \
    EXT2_INODE_DINDIRECT_COUNT + EXT2_INODE_TINDIRECT_COUNT

#define EXT2_INODE_TYPE_FIFO        0x1000
#define EXT2_INODE_TYPE_CHARDEV     0x2000
#define EXT2_INODE_TYPE_DIR         0x4000
#define EXT2_INODE_TYPE_BLOCKDEV    0x6000
#define EXT2_INODE_TYPE_REGULAR     0x8000
#define EXT2_INODE_TYPE_SLINK       0xA000
#define EXT2_INODE_TYPE_SOCKET      0xC000

#define TYPE_FROM_MODE(mode) ((mode) & 0xF000)

// Offset dentro del disco a bd_addr. (Sólo para 32 bits).
#define OFFSET_TO_BD_ADDR(bd, dev_offset) ((bd_addr_t) { \
    .sector = (dev_offset) / (bd)->size, \
    .offset = (dev_offset) % (bd)->size \
})

#define BLOCK_TO_BD_ADDR(fs, bd, block, offset) ({ \
    _shft = fs-> +  
    ((bd_addr_t) {}); \
})

/*
FIRSTINODE = 11,
VALIDFS = 0x0001,
ERRORFS = 0x0002,

NAMELEN = 255,

// permissions in Inode.mode
IEXEC = 00100,
IWRITE = 0200,
IREAD = 0400,
ISVTX = 01000,
ISGID = 02000,
ISUID = 04000,

#define DIRLEN(namlen)  (((namlen)+8+3)&~3)

*/


/*
enum
{
    GroupSize = 32
};
*/


/*
 * Inode
 */
typedef struct {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t sectors_count;
    uint32_t flags;
    uint32_t os1;
    uint32_t blocks[EXT2_INODE_TOTALBLOCKS];
    uint32_t version;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t fragment_addr;
    uint8_t os2[12];
} __attribute__((__packed__)) ext2_inode;

/*
 * Directory entry
 */
typedef struct {
    uint32_t inode_no;
    uint16_t direntry_length;
    uint8_t name_length;
    char *name;
} __attribute__((__packed__)) ext2_direntry;

/*
enum
{
    MinDirentSize = 4+2+1+1
};
*/

uint8_t file_data_buf[PAGE_SIZE*4];


static void initialize_part_info(ext2 *part_info);
static ext2_inode *get_inode(ext2 *part_info, uint32_t no);
static ext2_inode *path2inode(ext2 *part_info, ext2_inode *dir,
    const char *relpath);
static int get_data(ext2 *part_info, ext2_inode *inode, void *buf);


void ext2_init() {
}

void ext2_create(ext2 *part_info, blockdev *part) {
    memset(part_info, 0, sizeof(ext2));

    part_info->initialized = FALSE;
    part_info->part = part;
}

/* Aqui se realiza la inicializacion de la estructura ext2. El resto de las
 * funciones no exportadas asumen que la estructura esta inicializada.
 */
chardev *ext2_open(ext2 *part_info, const char *filename, uint32_t flags) {
    if (strncmp(filename, "/disk/", 6))
        return NULL;

    if (!(part_info->initialized))
        initialize_part_info(part_info);

    void *buf = mm_mem_kalloc();

    ext2_inode *inode = get_inode(part_info, 2);

    get_data(part_info, inode, buf);

    ext2_direntry *direntry0 = buf;
    vga_write(10, 0, direntry0->name, 0x0F);

    //ext2_inode *inode = path2inode(part_info,
    //    get_inode(part_info, EXT2_ROOT_INODE_NO), filename + 6);
}

static void initialize_part_info(ext2 *part_info) {
    // XXX: Aca asumo que el superblock entra en una pagina
    part_info->superblock = mm_mem_kalloc();
    read_from_bdev(part_info->part, 
        OFFSET_TO_BD_ADDR(part_info->part, EXT2_SUPERBLOCK_OFFSET),
        part_info->superblock, EXT2_SUPERBLOCK_SIZE);

    ext2_superblock *sb = part_info->superblock;

    if (sb->magic != EXT2_SUPERBLOCK_MAGIC)
        return;

    uint32_t bg_count = sb->blocks_count/sb->blocks_per_group;

    part_info->bgd_table = mm_mem_kalloc();
    read_from_bdev(part_info->part,
        OFFSET_TO_BD_ADDR(part_info->part, EXT2_SUPERBLOCK_OFFSET + EXT2_SUPERBLOCK_SIZE),
        part_info->bgd_table,
        bg_count*sizeof(ext2_block_group_descriptor));
}

static ext2_inode *get_inode(ext2 *part_info, uint32_t no) {
    // Obtenemos el numero de Block Group
    ext2_superblock *sb = part_info->superblock;
    uint32_t bg_no = (no - 1)/sb->inodes_per_group;

    // Obtenemos el descriptor correspondiente al Block Group
    ext2_block_group_descriptor *bgd = part_info->bgd_table + bg_no;

    // Obtenemos la direccion en el blockdev de la tabla de inodos
    uint32_t block_size = 1024 << sb->log2_block_size;

    // Calculamos el indice del inodo en la tabla de inodos
    uint32_t inode_index = (no - 1) % part_info->superblock->inodes_per_group;

    // Calculamos la direccion en el blockdev del inodo
    uint32_t bn = bgd->inode_table_bno + ((inode_index*sizeof(ext2_inode)) / block_size);
    uint32_t offset = (inode_index * sizeof(ext2_inode)) % block_size;


    ext2_inode *inode = mm_mem_kalloc();

    return read_from_bdev(part_info->part,
        BLOCK_TO_BD_ADDR(part_info->part, bn, offset), 
        inode, sizeof(ext2_inode));
}

static ext2_inode *path2inode(ext2 *part_info, ext2_inode *dir,
    const char *relpath) {

    if (TYPE_FROM_MODE(dir->mode) != EXT2_INODE_TYPE_DIR)
        return NULL;

    get_data(part_info, dir, file_data_buf);
}

static int get_data(ext2 *part_info, ext2_inode *inode, void *buf) {

    // Chequeamos si el archivo es mas grande que el buffer que tenemos
    if (inode->size > sizeof(file_data_buf))
        return -1;

    uint32_t block_size = 1024 << part_info->superblock->log2_block_size;

    void *buf_pos;
    int i;
    for (i = 0, buf_pos = buf;
        i < EXT2_INODE_DIRECT_COUNT;
        i++, buf_pos += block_size) {

        // Obtenemos el numero del bloque en el que se hallan los datos
        uint32_t bno = inode->blocks[i];

        read_from_bdev(part_info->part, 
            BLOCK_TO_BD_ADDR(part_info->part, bno, 0), 
            buf_pos, block_size);
    }

    return 0;
}
