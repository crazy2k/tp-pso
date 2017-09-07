#include <tipos.h>
#include <device.h>
#include <ext2.h>
#include <utils.h>
#include <mm.h>
#include <errors.h>
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


#define GET_BLOCKS_PER_SECTION(fp) \
    ((fp)->buf_size/GET_BLOCK_SIZE((fp)->file_part_info))

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

//TODO: Add lock
static uint8_t file_data_buf[PAGE_SIZE*4];
static uint8_t inode_buf[256];

static ext2_file_chardev ext2_file_chardevs[MAX_EXT2_FILE_CHARDEVS];
static ext2_file_chardev *free_ext2_file_chardevs;


static void initialize_part_info(ext2 *part_info);
static int get_inode(ext2 *part_info, uint32_t no, ext2_inode *inode_buf);
static uint32_t path2inode(ext2 *part_info, uint32_t dir_no,
    const char *relpath);

static int get_data(ext2 *part_info, ext2_inode *inode, void *buf);
static int operate_data_with_file(ext2 *part_info, ext2_inode *inode, uint32_t first_bno, void *buf, uint32_t buf_size, bool write);
static void get_indirect_data_for_file(ext2 *part_info, uint32_t bno, int level, int *bno_offset, uint32_t *remaining, void **buf);

static int indirect_block_count(ext2 *part_info, int level);

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

    uint32_t no = path2inode(part_info, 2, filename + 6);
    if (no == 0)
        return NULL;

    ext2_inode inode;
    get_inode(part_info, no, &inode);

    ext2_file_chardev *fp = POP(&free_ext2_file_chardevs);
    initialize_ext2_file_chardev(fp);

    fp->file_size = inode.size;
    fp->file_no = no;
    fp->file_part_info = part_info;

    return (chardev *)fp;
}



int ext2_stat(ext2 *part_info, const char *filename, stat_t* st) {
    if (strncmp(filename, "/disk/", 6))
        return -ENOFILE;

    if (!(part_info->initialized))
        initialize_part_info(part_info);

    uint32_t no = path2inode(part_info, 2, filename + 6);
    if (no == 0)
        return -ENOFILE;

    ext2_inode inode;
    get_inode(part_info, no, &inode);

    st->inode = no;
    st->size = inode.size;

    return 0;
}

/*
 * Read/write from/to file. Write operation does not support file growing.
 */
static sint_32 ext2_file_operate(chardev *this, void *buf, uint32_t size, bool write) {
    if (this->clase != DEVICE_EXT2_FILE_CHARDEV)
        return -1;

    ext2_file_chardev *fp = (ext2_file_chardev *)this;

    debug_printf("ext2_file_operate: %s : file_size: %x, file_offset: %x\n",
        write == TRUE ? "write" : "read",
        fp->file_size, fp->file_offset);

    // `n` represents the bytes still to be read/written
    // for this operation to complete
    int n = min(size, fp->file_size - fp->file_offset);
    int result = n;

    // Get the inode
    ext2_inode inode;
    get_inode(fp->file_part_info, fp->file_no, &inode);

    // While there's more to be read/written
    while (n > 0) {
        // A section is a window the size of our buffer that contains blocks. It's
        // a window we move forward as we need, and starts at the first block that's
        // needed for this read/write

        // Ordinal number of the first data block that contains data we care
        // about for this offset
        uint32_t section_first_bno = fp->file_offset/(GET_BLOCK_SIZE(fp->file_part_info));
        uint32_t skipped_bytes = section_first_bno*GET_BLOCK_SIZE(fp->file_part_info);
        uint32_t buf_offset = (fp->file_offset - skipped_bytes) % fp->buf_size;
        uint32_t operated = min(n, fp->buf_size - buf_offset);

        if (write == TRUE) {
            memcpy(fp->buf, buf, operated);
        }

        // Do we have everything we need in the buffer/section?
        if (fp->buf_section != (section_first_bno/GET_BLOCKS_PER_SECTION(fp))) {
            operate_data_with_file(fp->file_part_info, &inode, section_first_bno,
                fp->buf, fp->buf_size, write);
            fp->buf_section = section_first_bno/GET_BLOCKS_PER_SECTION(fp);
        }

        debug_printf("ext2_file_read: file_offset: %x, buf_size: %x, buf_offset: %x, read: %x\n",
                    fp->file_offset, fp->buf_size, buf_offset, operated);

        if (write != TRUE) {
            memcpy(buf, fp->buf + buf_offset, operated);
        }


//        debug_printf("File's buffer: ");
//
//        uint32_t *buf32 = fp->buf;
//        int i;
//        for (i = 0; i < size/4; i++) {
//            debug_printf("%x ", *(buf32 + i));
//        }

        buf += operated;
        n -= operated;
        fp->file_offset += operated;
    }

    debug_printf("Leyo %d bytes en el buffer %x\n", result, (uint32_t)buf);

    return result;
}

static sint_32 ext2_file_read(chardev *this, void *buf, uint32_t size) {
    return ext2_file_operate(this, buf, size, FALSE);
}

static sint_32 ext2_file_write(chardev *this, const void *buf, uint32_t size) {
    return ext2_file_operate(this, buf, size, TRUE);
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

    //if (pos >= fp->buf_size)
    //    return -1;

    fp->file_offset = pos;
    return 0;
}

static void initialize_ext2_file_chardev(ext2_file_chardev *fp) {
    fp->clase = DEVICE_EXT2_FILE_CHARDEV;
    fp->refcount = 0;
    fp->read = ext2_file_read;
    fp->write = ext2_file_write;
    fp->flush = ext2_file_flush;
    fp->seek = ext2_file_seek;

    fp->buf = mm_mem_kalloc();
    fp->buf_size = PAGE_SIZE;
    fp->buf_section = 0xFFFFFFFF;

    fp->file_part_info = NULL;
    fp->file_no = 0;
    fp->file_size = 0;
    fp->file_offset = 0;
}

static void initialize_part_info(ext2 *part_info) {
    // El superblock ocupa 1024 bytes, por lo que entra en una pagina
    part_info->superblock = mm_mem_kalloc();
    operate_with_bdev(part_info->part,
        OFFSET_TO_BD_ADDR(part_info->part, EXT2_SUPERBLOCK_OFFSET),
        part_info->superblock, EXT2_SUPERBLOCK_SIZE, FALSE);

    ext2_superblock *sb = part_info->superblock;

    if (sb->magic != EXT2_SUPERBLOCK_MAGIC)
        return;

    uint32_t bg_count = sb->blocks_count/sb->blocks_per_group;
    if ((sb->blocks_count % sb->blocks_per_group) > 0)
        bg_count += 1;

    part_info->bgd_table = mm_mem_kalloc();
    operate_with_bdev(part_info->part,
        OFFSET_TO_BD_ADDR(part_info->part, EXT2_SUPERBLOCK_OFFSET + EXT2_SUPERBLOCK_SIZE),
        part_info->bgd_table,
        bg_count*sizeof(ext2_block_group_descriptor),
        FALSE);
    part_info->initialized = TRUE;
}

static int get_inode(ext2 *part_info, uint32_t no, ext2_inode *inode) {
    // Obtenemos el numero de Block Group
    debug_printf("get_inode: part_info: %x\n", part_info);
    debug_printf("get_inode: part_info->superblock: %x\n", part_info->superblock);
    debug_printf("get_inode: Inodes count: %x\n", part_info->superblock->inodes_count);
    debug_printf("get_inode: Inodes per group: %x\n", part_info->superblock->inodes_per_group);
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

    operate_with_bdev(part_info->part,
        baddr2bdaddr(part_info, bn, offset),
        inode, inode_size, FALSE);

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

static int get_data(ext2 *part_info, ext2_inode *inode, void *buf) {
    return operate_data_with_file(part_info, inode, 0, buf, inode->size, FALSE);
}

/*
 * Reads buf_size bytes of data into the buf buffer, from the file represented
 * by inode
 */
static int operate_data_with_file(ext2 *part_info, ext2_inode *inode,
    uint32_t first_bno, void *buf, uint32_t buf_size, bool write) {

    uint32_t /*blocks_read = 0,*/ block_size = GET_BLOCK_SIZE(part_info);

    /* Total bytes to read */
    uint32_t remaining = min(inode->size, buf_size);
    /* If bytes is not a multiple of the block size, align to the next block size multiple */
    if (remaining % block_size > 0)
        remaining += block_size - (remaining % block_size);

    void *buf_pos;
    int i;
    for (i = first_bno, buf_pos = buf;
        (i < EXT2_INODE_DIRECT_COUNT) && (remaining > 0);
        i++, buf_pos += block_size, remaining -= block_size) {

        debug_printf("** ext2: get_data: remaining: %x\n", remaining);

        // Obtenemos el numero del bloque en el que se hallan los datos
        uint32_t bno = inode->blocks[i];

        operate_with_bdev(part_info->part,
            baddr2bdaddr(part_info, bno, 0),
            buf_pos, block_size, write);
    }

    int bno_offset = first_bno - EXT2_INODE_DIRECT_COUNT;
    // Leemos los datos desde bloques indirectos, si los hay
    for (i = 1; (i < 4) && (remaining > 0); i++) {
        get_indirect_data_for_file(part_info,
            inode->blocks[EXT2_INODE_DIRECT_COUNT - 1 + i], i,
            &bno_offset, &remaining, &buf_pos);
    }

    return 0;
}

static void get_indirect_data_for_file(ext2 *part_info, uint32_t bno, int level, int *bno_offset, uint32_t *remaining, void **buf) {
//    debug_printf("** ext2: get_indirect_data_for_file: remaining : %x\n", remaining);
    uint32_t block_size = GET_BLOCK_SIZE(part_info);

    if (level > 0) {
        int i, total_blocks = block_size/sizeof(uint32_t);
        uint32_t *blocks = (uint32_t *)mm_mem_kalloc();

        int blocks_next_level = indirect_block_count(part_info, level-1);

        operate_with_bdev(part_info->part,
            baddr2bdaddr(part_info, bno, 0),
            (void *) blocks, block_size, FALSE);

        for (i = 0; i < total_blocks; i++) {
            if (blocks_next_level > *bno_offset)
                get_indirect_data_for_file(part_info, blocks[i], level - 1, bno_offset, remaining, buf);
            else
                *bno_offset -= blocks_next_level;
        }

        mm_mem_free((void *)blocks);
    } else if (*remaining > 0) {
        if (*bno_offset <= 0) {
            int read = min(*remaining, block_size);
            operate_with_bdev(part_info->part,
                baddr2bdaddr(part_info, bno, 0),
                *buf, read, FALSE);
           *remaining -= read;
           *buf += read;
        } else
            (*bno_offset)--;
    } else
        return;
}

static int indirect_block_count(ext2 *part_info, int level) {
    return power(GET_BLOCK_SIZE(part_info)/sizeof(uint32_t), level);
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
