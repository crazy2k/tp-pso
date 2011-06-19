#include <tipos.h>
#include <device.h>
#include <ext2.h>
#include <utils.h>
#include <mm.h>

#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_SUPERBLOCK_SIZE 1024
#define EXT2_SUPERBLOCK_MAGIC 0xEF53
#define EXT2_ROOT_INODE_NO 2
#define EXT2_NDIRBLOCKS 12
#define EXT2_INDBLOCK EXT2_NDIRBLOCKS
#define EXT2_DINDBLOCK (EXT2_INDBLOCK + 1)
#define EXT2_TINDBLOCK (EXT2_DINDBLOCK + 1)
#define EXT2_NBLOCKS (EXT2_TINDBLOCK + 1)


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

// type in Inode.mode
IFMT = 0170000,
IFIFO = 0010000,
IFCHR = 0020000,
IFDIR = 0040000,
IFBLK = 0060000,
IFREG = 0100000,
IFLNK = 0120000,
IFSOCK = 0140000,
IFWHT = 0160000

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
    uint32_t blocks_count;
    uint32_t flags;
    uint32_t blocks[EXT2_NBLOCKS];
    uint32_t version;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t fragment_addr;
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

int read_from_blockdev(blockdev *bdev, uint64_t offset, void *buf,
    uint32_t size);


static void initialize_part_info(ext2 *part_info);
static ext2_inode *get_inode(ext2 *part_info, uint32_t no);


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

    //get_inode(EXT2_ROOT_INODE_NO);
}

static void initialize_part_info(ext2 *part_info) {
    // XXX: Aca asumo que el superblock entra en una pagina
    part_info->superblock = mm_mem_kalloc();
    read_from_blockdev(part_info->part, EXT2_SUPERBLOCK_OFFSET, part_info->superblock,
        EXT2_SUPERBLOCK_SIZE);

    ext2_superblock *sb = part_info->superblock;
    uint32_t bg_count = sb->blocks_count/sb->blocks_per_group;

    part_info->bgd_table = mm_mem_kalloc();
    read_from_blockdev(part_info->part, EXT2_SUPERBLOCK_OFFSET + EXT2_SUPERBLOCK_SIZE,
        part_info->bgd_table, bg_count*sizeof(ext2_block_group_descriptor));
}

static ext2_inode *get_inode(ext2 *part_info, uint32_t no) {
    // Obtenemos el numero de Block Group
    ext2_superblock *sb = part_info->superblock;
    uint32_t bg_no = (no - 1)/sb->inodes_per_group;

    // Obtenemos el descriptor correspondiente al Block Group
    ext2_block_group_descriptor *bgd = part_info->bgd_table + bg_no;

    // Obtenemos la direccion en el blockdev de la tabla de inodos
    uint64_t inode_table_bdaddr = (bgd->inode_table_baddr)*(sb->block_size);

    // Calculamos el indice del inodo en la tabla de inodos
    uint32_t inode_index = (no - 1) % part_info->superblock->inodes_per_group;

    // Calculamos la direccion en el blockdev del inodo
    uint64_t inode_bpaddr = inode_table_bdaddr +
        inode_index*sizeof(ext2_inode);

    ext2_inode *inode = mm_mem_kalloc();
    read_from_blockdev(part_info->part, inode_bpaddr, inode,
        sizeof(ext2_inode));

    return inode;
}

ext2_inode *path2inode(ext2_inode *dir, char *relpath) {
    return NULL;
}