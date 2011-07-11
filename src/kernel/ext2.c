#include <tipos.h>
#include <device.h>
#include <ext2.h>
#include <utils.h>
#include <mm.h>
#include <debug.h>

#define MAX_EXT2_FILE_CHARDEVS 20

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

#define GET_INODE_SIZE(part_info) \
    ({ \
        uint16_t _inode_size = sizeof(ext2_inode); \
        if ((part_info)->superblock->revision_level > 0) \
            _inode_size = (part_info)->superblock->inode_size; \
        _inode_size; \
    })

#define GET_BLOCK_SIZE(part_info) \
    (1024 << (part_info)->superblock->log2_block_size)

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
    char type;
    char name[];
} __attribute__((__packed__)) ext2_direntry;

/*
typedef struct {
    uint32_t inode_no;
    uint16_t direntry_length;
    union {
        uint16_t name_length;
        struct {
            uint8_t name_length;
            char type;
        } typed;
    };
    uint8_t name[];
} __attribute__((__packed__)) ext2_direntry;
*/

/*
enum
{
    MinDirentSize = 4+2+1+1
};
*/

static uint8_t file_data_buf[PAGE_SIZE*4];
static uint8_t inode_buf[256];

static ext2_file_chardev ext2_file_chardevs[MAX_EXT2_FILE_CHARDEVS];
static ext2_file_chardev *free_ext2_file_chardevs;


static void initialize_part_info(ext2 *part_info);
static int get_inode(ext2 *part_info, uint32_t no, ext2_inode *inode_buf);
static uint32_t path2inode(ext2 *part_info, uint32_t dir_no,
    const char *relpath);
static int get_data(ext2 *part_info, ext2_inode *inode, void *buf);
static uint32_t get_indirect_data(ext2 *part_info, uint32_t bno, int level, uint32_t remaining, void *buf);
static int indirect_block_size(ext2 *part_info, int level);
static bd_addr_t baddr2bdaddr(ext2 *part_info, uint32_t bno, uint32_t offset);
static void initialize_ext2_file_chardev(ext2_file_chardev *fp);


void ext2_init() {
    CREATE_FREE_OBJS_LIST(free_ext2_file_chardevs, ext2_file_chardevs,
        MAX_EXT2_FILE_CHARDEVS);
}

void ext2_create(ext2 *part_info, blockdev *part) {
    memset(part_info, 0, sizeof(ext2));

    part_info->initialized = FALSE;
    part_info->part = part;
}

static void print_inode(ext2_inode *inode) {
    debug_printf("** Inode:\n");
    debug_printf("**  mode: %x\n" , inode->mode);
    debug_printf("**  uid: %x\n" , inode->uid);
    debug_printf("**  size: %x\n" , inode->size);
    debug_printf("**  atime: %x\n" , inode->atime);
    debug_printf("**  ctime: %x\n" , inode->ctime);
    debug_printf("**  mtime: %x\n" , inode->mtime);
    debug_printf("**  dtime: %x\n" , inode->dtime);
    debug_printf("**  gid: %x\n" , inode->gid);
    debug_printf("**  links_count: %x\n" , inode->links_count);
    debug_printf("**  sectors_count: %x\n" , inode->sectors_count);
    debug_printf("**  flags: %x\n" , inode->flags);
    debug_printf("**  os1: %x\n" , inode->os1);
    debug_printf("**  version: %x\n" , inode->version);
    debug_printf("**  file_acl: %x\n" , inode->file_acl);
    debug_printf("**  dir_acl: %x\n" , inode->dir_acl);
    debug_printf("**  fragment_addr: %x\n" , inode->fragment_addr);
    debug_printf("**  blocks: %x\n" , inode->blocks[0]);
    debug_printf("**  blocks: %x\n" , inode->blocks[1]);
    debug_printf("**  blocks: %x\n" , inode->blocks[2]);
    debug_printf("**  blocks: %x\n" , inode->blocks[3]);
    debug_printf("**  blocks: %x\n" , inode->blocks[4]);
    debug_printf("**  blocks: %x\n" , inode->blocks[5]);
}

/* Aqui se realiza la inicializacion de la estructura ext2. El resto de las
 * funciones no exportadas asumen que la estructura esta inicializada.
 */
chardev *ext2_open(ext2 *part_info, const char *filename, uint32_t flags) {
    if (strncmp(filename, "/disk/", 6))
        return NULL;

    if (!(part_info->initialized))
        initialize_part_info(part_info);

    uint32_t i = path2inode(part_info, 2, filename + 6);
    if (i == 0)
        return NULL;

    ext2_inode inode;
    get_inode(part_info, i, &inode);

    ext2_file_chardev *fp = POP(&free_ext2_file_chardevs);
    initialize_ext2_file_chardev(fp);

    get_data(part_info, &inode, fp->buf);

    return (chardev *)fp;
}

static sint_32 ext2_file_read(chardev *this, void *buf, uint32_t size) {
    if (this->clase != DEVICE_EXT2_FILE_CHARDEV)
        return -1;

    ext2_file_chardev *fp = (ext2_file_chardev *)this;

    uint32_t remainder = fp->buf_size - fp->buf_offset;
    void *fp_buf = (void *)fp->buf;
    uint32_t n = min(size, remainder);
    memcpy(buf, fp_buf + fp->buf_offset, n);

    debug_printf("Leyo %x bytes en el buffer %x\n", n, (uint32_t)buf);

    return n;
}

static uint_32 ext2_file_flush(chardev *this) {
    if (this->clase != DEVICE_EXT2_FILE_CHARDEV)
        return -1;

    ext2_file_chardev *fp = (ext2_file_chardev *)this;

    mm_mem_free(fp->buf);
    APPEND(&free_ext2_file_chardevs, fp);

    return 0;
}

static sint_32 ext2_file_seek(chardev *this, uint32_t pos) {
    if (this->clase != DEVICE_EXT2_FILE_CHARDEV)
        return -1;

    ext2_file_chardev *fp = (ext2_file_chardev *)this;

    if (pos >= fp->buf_size)
        return -1;

    fp->buf_offset = pos;
    return 0;
}

static void initialize_ext2_file_chardev(ext2_file_chardev *fp) {
    fp->clase = DEVICE_EXT2_FILE_CHARDEV;
    fp->refcount = 0;
    fp->read = ext2_file_read;
    fp->flush = ext2_file_flush;
    fp->seek = ext2_file_seek;

    fp->buf = mm_mem_kalloc();
    fp->buf_offset = 0;
    fp->buf_size = PAGE_SIZE;
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
    if ((sb->blocks_count % sb->blocks_per_group) > 0)
        bg_count += 1;

    part_info->bgd_table = mm_mem_kalloc();
    read_from_bdev(part_info->part,
        OFFSET_TO_BD_ADDR(part_info->part, EXT2_SUPERBLOCK_OFFSET + EXT2_SUPERBLOCK_SIZE),
        part_info->bgd_table,
        bg_count*sizeof(ext2_block_group_descriptor));
}

static int get_inode(ext2 *part_info, uint32_t no, ext2_inode *inode) {
    // Obtenemos el numero de Block Group
    uint32_t bg_no = (no - 1)/part_info->superblock->inodes_per_group;

    // Obtenemos el descriptor correspondiente al Block Group
    ext2_block_group_descriptor *bgd = part_info->bgd_table + bg_no;

    // Obtenemos la direccion en el blockdev de la tabla de inodos
    uint32_t block_size = GET_BLOCK_SIZE(part_info);

    // Calculamos el indice del inodo en la tabla de inodos
    uint32_t inode_index = (no - 1) % part_info->superblock->inodes_per_group;

    // Calculamos la direccion en el blockdev del inodo
    uint16_t inode_size = GET_INODE_SIZE(part_info);
    uint32_t bn = bgd->inode_table_bno +
        ((inode_index*inode_size) / block_size);
    uint32_t offset = (inode_index*inode_size) % block_size;

    read_from_bdev(part_info->part,
        baddr2bdaddr(part_info, bn, offset),
        inode, inode_size);

    return NULL;
}


/* Por ahora solo soporta inodes de 128 o 256 bytes */
static uint32_t path2inode(ext2 *part_info, uint32_t dir_no, const char *relpath) {
    uint32_t offset = 0, next_inode = 0;
    ext2_inode *dir_inode = (ext2_inode *) inode_buf;
    
    // Si el path es vacio devolver el propio dir
    if (!relpath || relpath[0] == '\0')
        return dir_no;

    // Cargar el inode
    get_inode(part_info, dir_no, dir_inode);

    // Un nodo intermedio debe ser un directorio
    if (TYPE_FROM_MODE(dir_inode->mode) != EXT2_INODE_TYPE_DIR)
        return 0;

    char* next_path = strchr(relpath, '/');
    int name_size = next_path ? next_path - relpath: strlen(relpath);

    // Cargar datos para el inode
    get_data(part_info, dir_inode, file_data_buf);

    // Buscar un entry dentro del directorio que corresponde con el path actual
    while (offset < dir_inode->size && next_inode == 0) {
        ext2_direntry *entry = (ext2_direntry *)(file_data_buf + offset);

        if ((name_size == entry->name_length) &&
            (strncmp(relpath, entry->name, name_size) == 0))

            next_inode = entry->inode_no;
        else
            offset += entry->direntry_length;
    }
    
    // La direccion del fs debe corresponder a un archivo existente
    if (next_inode == 0)
        return 0;

    return next_path ? path2inode(part_info, next_inode, next_path + 1) : next_inode;
}

/* Por ahora solo soporta archivos de 12 bloques (si los bloques son de 1024,
 * son 12KB).
 * XXX: El soporte para bloques indirectos no esta probado.
 */
static int get_data(ext2 *part_info, ext2_inode *inode, void *buf) {
    // Chequeamos si el archivo es mas grande que el buffer que tenemos
    if (inode->size > sizeof(file_data_buf))
        return -1;

    uint32_t block_size = GET_BLOCK_SIZE(part_info);

    uint32_t remaining = inode->size;
    if (remaining % block_size)
        remaining += block_size - (remaining % block_size);

    void *buf_pos;
    int i;
    for (i = 0, buf_pos = buf;
        (i < EXT2_INODE_DIRECT_COUNT) && (remaining > 0);
        i++, buf_pos += block_size, remaining -= block_size) {

        debug_printf("** ext2: get_data: remaining: %x\n", remaining);

        // Obtenemos el numero del bloque en el que se hallan los datos
        uint32_t bno = inode->blocks[i];

        read_from_bdev(part_info->part, 
            baddr2bdaddr(part_info, bno, 0), 
            buf_pos, block_size);
    }
    
    // Leemos los datos desde bloques indirectos, si los hay
    for (i = 1; (i < 4) && (remaining > 0); 
        i++, buf_pos += indirect_block_size(part_info, i)) {

        remaining = get_indirect_data(part_info, 
            inode->blocks[EXT2_INODE_DIRECT_COUNT - 1 + i], 
            i, remaining, buf_pos);
    }

    return 0;
}

static uint32_t get_indirect_data(ext2 *part_info, uint32_t bno, int level, uint32_t remaining, void *buf) {
    uint32_t block_size = GET_BLOCK_SIZE(part_info);

    if (level > 0) {
        int i; 
        int total_blocks = block_size/sizeof(uint32_t);
        uint32_t iblock_size = indirect_block_size(part_info, level - 1);
        uint32_t *blocks = (uint32_t *)mm_mem_kalloc();

        read_from_bdev(part_info->part,
            baddr2bdaddr(part_info, bno, 0), 
            (void *) blocks, block_size);

        for (i = 0; i < total_blocks; i++, buf += iblock_size)
            remaining = get_indirect_data(part_info, blocks[i], level - 1, remaining, buf);

        mm_mem_free((void *)blocks);
    } else if (remaining > 0) {
        read_from_bdev(part_info->part,
            baddr2bdaddr(part_info, bno, 0), 
            buf, block_size);
        remaining -= block_size;
    }

    return remaining;
}

static int indirect_block_size(ext2 *part_info, int level) {
    uint32_t block_size = GET_BLOCK_SIZE(part_info);

    return block_size * power(block_size/sizeof(uint32_t), level);
}

static bd_addr_t baddr2bdaddr(ext2 *part_info, uint32_t bno, uint32_t offset) {
    uint32_t sector_size = part_info->part->size,
             block_size = GET_BLOCK_SIZE(part_info);
    uint32_t c;

    if (block_size >= sector_size) {
        c = block_size/sector_size;

        return ((bd_addr_t) { 
            .sector = bno*c + offset/sector_size,
            .offset = offset % sector_size
        });
    } else {
        c = sector_size/block_size;
    
        return ((bd_addr_t) { 
            .sector = bno/c,
            .offset = offset + (bno % c) * block_size 
        });
    }
}
