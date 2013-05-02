#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "mini_fs.h"


/**< ����4���겻��Ҫ�Ķ� */
#define FS_BLOCK     ((sizeof(FILE_LEN_TABLE) + SEGMENT_SIZE -1) / SEGMENT_SIZE) 
#define SUPER_BLOCK  ((sizeof(fs) + SEGMENT_SIZE -1) / SEGMENT_SIZE)       
#define SWAP_BLOCK   1  
#define _MAX_(a, b)  ((a) > (b) ? (a) : (b))
#define DISK_BLOCK   _MAX_((FS_BLOCK + SUPER_BLOCK + SWAP_BLOCK), FS_USE_SEGMENT_MAX)

#ifdef FS_DISK_RAM_FLASH
static BYTE __DISK[DISK_BLOCK*SEGMENT_SIZE];
WORD DISK = (WORD) &__DISK[0];
#endif

#ifndef FS_USE_MEM_SWAP
#define SWAP_ADDR		((WORD)(SEGMENT_SIZE * (FS_BLOCK + SUPER_BLOCK)))
#else
static BYTE __swap__[SEGMENT_SIZE];
static WORD const SWAP_ADDR = (WORD)&__swap__[0];
#endif
#define SUPER_ADDR	((WORD)(SEGMENT_SIZE * FS_BLOCK))

enum {BLOCK_UNUSED, BLOCK_USED, BLOCK_FAIL};

enum {
	FS_FLAG_CHANGED		= 0x01,
	FS_FLAG_SWAP_CLEAN	= 0x02,
	FS_FLAG_SWAP_DIRE		= 0x04,	//����segment_to_segment������1ʱ��ʾ�����ݴ�SWAP������DISK
	FS_FLAG_NO_FREE_SPACE = 1 << 3, //û�пհ��� 
};

enum {
	NORMAL_WRITE,
	DIRECT_WRITE,
};

static void __segment_erase(WORD addr);
static void __segment_read(WORD seg_addr, WORD buf, WORD len);
static void __segment_write(WORD seg_addr, WORD buf, WORD len);

static void data_to_swap(WORD swap_addr, WORD data_addr, WORD len);
static void segment_clean(WORD addr, WORD noused, WORD len);
typedef void (*op_fun_t)(WORD addr, WORD data, WORD len);
static void addr_split_opera(WORD addr, WORD data, WORD len, op_fun_t op);
static WORD get_phy_addr(WORD virt_addr);

/*******************************************************
***	�û��ӿڲ����
***	read����ֱ�ӷ���FLASH��ַ����
*** 	write������Ҫ����FLASH��д�������
***	erase��������ָ�����ļ�
*******************************************************/

#ifdef FS_DISK_ROM_FLASH
const BYTE* f_rom_read(file_id_t id, WORD offset) {
	if (id >= FILE_ID_END || offset >= fs.file[id].file_size)
		return NULL;
	return (const BYTE *)(fs.file[id].start_addr + offset);
}
#endif

WORD f_read(file_id_t id, WORD offset,	BYTE *buf, WORD len) {
	#define size fs.file[id].file_size
	if (id >= FILE_ID_END || buf == NULL || len > size || offset >= size \
		|| offset + len < len || offset + len < offset)
		return 0;
	if (offset + len > size)
		len = size - offset;
	#undef size
	addr_split_opera(fs.file[id].start_addr + offset, \
							(WORD)buf, len, (op_fun_t)__segment_read);
	return (fs.flag & FS_FLAG_NO_FREE_SPACE) ? 0 : len;
}

static WORD _f_write(file_id_t id,	WORD offset, const BYTE *data, WORD len, BYTE write_flag) {
	WORD n, file_addr, file_len;
	#define size fs.file[id].file_size
	if (id >= FILE_ID_END || len == 0 || data == NULL || len > size || offset >= size\
		|| offset + len < len || offset + len < offset)
		return 0;
	if (offset + len > size)
		len = size - offset;
	#undef size
	
	//ÿ��д�ļ�ʱ���������Ҫ�޸ĵĲ��ֺ���Ҫ׷�ӵĲ���
	//��Ҫ�޸ĵĲ�����Ƶ�����FLASH����׷�ӵĲ��������ļ�����ʱ�Ѿ���д����
	file_addr = fs.file[id].start_addr;
	file_len = fs.file[id].file_len;
	
	if (offset >= file_len) {
		//��Ҫ���������ֻ��Ҫ׷��
		fs.file[id].file_len = offset + len;
	} else if (offset < file_len && offset + len > file_len) {
		//��Ҫ���������һ����λ�����������ڲ�������һ������Ҫ׷��
		n = file_len - offset;
		if (write_flag != DIRECT_WRITE) 
			addr_split_opera(file_addr + offset, (WORD)data, n, segment_clean);
		fs.file[id].file_len = offset + len;
	} else {
		//��Ҫ�����������ȫλ�����������ڲ�
		if (write_flag != DIRECT_WRITE) 
			addr_split_opera(file_addr + offset, (WORD)data, len, segment_clean);
	}
	addr_split_opera(file_addr + offset, (WORD)data, len, __segment_write);
	fs.flag |= FS_FLAG_CHANGED;
	
	return (fs.flag & FS_FLAG_NO_FREE_SPACE) ? 0 : len;
}

WORD f_write(file_id_t id,	WORD offset, const BYTE *data, WORD len) {
	return _f_write(id, offset, data, len, NORMAL_WRITE);
}

WORD 	f_write_direct(file_id_t id, WORD offset,	const BYTE *data, WORD len) {
	return _f_write(id, offset, data, len, DIRECT_WRITE);
}		
	
WORD f_copy(file_id_t dst, WORD dst_offset, file_id_t src, WORD src_offset, WORD len) {
	BYTE buf[F_COPY_CACHE_SIZE];
	int i;
	WORD len1 = len;

	for (i = len / F_COPY_CACHE_SIZE; i > 0; i--, dst_offset += F_COPY_CACHE_SIZE, \
			src_offset += F_COPY_CACHE_SIZE, len -= F_COPY_CACHE_SIZE) 
	{
		f_read(src, src_offset, buf, F_COPY_CACHE_SIZE);
		f_write(dst, dst_offset, buf, F_COPY_CACHE_SIZE);
	}
	
	if (len != 0) {
		f_read(src, src_offset, buf, len);
		f_write(dst, dst_offset, buf, len);
	}
	
	fs.flag |= FS_FLAG_CHANGED;
	return len1;	
}

void f_erase(file_id_t id) {
	//���ļ����ȵ���0ʱ��Ҳ������segment_erase����
	if ( id >= FILE_ID_END )
		return;
	addr_split_opera(fs.file[id].start_addr, \
			(WORD)NULL, fs.file[id].file_len, segment_clean);
	fs.file[id].file_len = 0;
	
	fs.flag |= FS_FLAG_CHANGED;
}

WORD f_len(file_id_t id) {
	if ( id >= FILE_ID_END)
		return 0;
	return fs.file[id].file_len;
}

WORD f_size(file_id_t id) {
	if ( id >= FILE_ID_END)
		return 0;
	return fs.file[id].file_size;
}

WORD f_addr(file_id_t id) {
	if ( id >= FILE_ID_END)
		return 0;
	return fs.file[id].start_addr;
}

void f_sync(void) {
	if (fs.flag & FS_FLAG_CHANGED) {
		fs.flag &= ~FS_FLAG_CHANGED;
		addr_split_opera(SUPER_ADDR, (WORD)&fs, sizeof(fs), segment_clean);
		addr_split_opera(SUPER_ADDR, (WORD)&fs, sizeof(fs), __segment_write);
	}
}

void f_init(void) {
	file_id_t id;
	
	__segment_read(SUPER_ADDR + offsetof(fs_t, valid), (WORD)&fs.valid, sizeof(fs.valid));
	if (fs.valid != 0x76) {
#ifdef FS_ENABLE_BLOCK_MGMT
		int n;
		for (n = 0; n < DISK_BLOCK; n++) {
			fs.block_map[n] = n; //��ʼ��ʱ�߼�������������ϵ��Ӧ��
			fs.block_status[n] = BLOCK_UNUSED;
			fs.block_wc[n] = 0;
		}
#endif
		addr_split_opera(0, (WORD)NULL, sizeof(FILE_LEN_TABLE), segment_clean);
		for (id = FILE1; id < FILE_ID_END; id++) {
			fs.file[id].file_len = 0;
		}
		fs.valid = 0x76;
		fs.flag = FS_FLAG_CHANGED;
	} else {
		__segment_read(SUPER_ADDR + offsetof(fs_t, flag), (WORD)&fs.flag, sizeof(fs.flag));
		for (id = FILE1; id < FILE_ID_END; id++) {
			//ֻ�Ӵ��̵��ļ�ϵͳ������ļ��ĵ�ǰ���ȣ��ļ�����ʼ��ַ�ʹ�С����rom�еġ�
			__segment_read(SUPER_ADDR + offsetof(fs_t, file) \
				+ offsetof(struct file_info_t, file_len) + id * sizeof(struct file_info_t ),\
				(WORD)&fs.file[id].file_len, sizeof(fs.file[id].file_len));
			//��������ļ����ȴ����ļ������������ô��ļ���
			//��Ҫ��Ϊ��Ԥ�������ļ�ϵͳʱ���ղ���block����û�����ļ�д�ļ���Ϣ�������´ζ�ȡ��ʱ������
			//�����Ϊ��Ԥ����������ļ��ṹ��ROM�е��ļ��ṹ��һ�£������ļ�ϵͳ�ṹ����
			if (fs.file[id].file_len > fs.file[id].file_size) {
				f_erase(id);
				fs.flag |= FS_FLAG_CHANGED;
			}
		}		
	}
	
	f_sync();
}


/*******************************************************
***	�������������
***	��Ҫ�漰�������Ϳ����ʱ���������������Ĳ���
*******************************************************/

//���������
static void addr_split_opera(WORD addr, WORD data, WORD len, op_fun_t op) {
	int i, temp_off, temp_len = len;
	WORD split_unit = (op == segment_clean) ?\
							SEGMENT_SIZE : MAX_WRITE_UNIT;
	//��һ��������δ����Ĳ���
	if ((temp_off = addr % split_unit) != 0) {
		if (temp_off + len > split_unit) 
			temp_len = split_unit - temp_off;
		op(addr, data, temp_len);
		addr += temp_len; data += temp_len; len -= temp_len;
	}
	//�ڶ���, ����պö���Ĳ���
	for (i = len / split_unit; i > 0; i--, addr += split_unit, data += split_unit, len -= split_unit)
		op(addr, data, split_unit);
	//������,�������ʣ�µ�
	if (len != 0)
		op(addr, data, len);
}


#define is_contain(a, b, c,d ) ((d) >= (a) && (c) < (b))
#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

static void __segment_op(WORD a, WORD b) {
	WORD id, c, d, min, max;
	WORD seg_addr = ( a / SEGMENT_SIZE) * SEGMENT_SIZE;
	//fprintf(stderr, "%s(%d,%d,%d,%d)\n", __FUNCTION__, seg_addr, a, b, step);

	for (id = FILE1; id < FILE_ID_END; id++) {
		c = fs.file[id].start_addr;
		d = c + fs.file[id].file_len;
		if (!is_contain(seg_addr, seg_addr + SEGMENT_SIZE, c, d))	//�ļ����ڴ˿���	
			continue;
		d =  d > seg_addr + SEGMENT_SIZE ? seg_addr + SEGMENT_SIZE: d;
		c = c < seg_addr ? seg_addr : c;
		//fprintf(stderr, "file %d start %d, len = %d, in this segment %d, step %d\n", id + 1, c, d, seg_addr, step);
		min = MIN(a, d);
		max = MAX(c, b);
		//fprintf(stderr, "a=%d,b=%d,c=%d,d=%d\n",a,b,c,d);
		if (c < min) {
			//fprintf(stderr, "area 1: %d, %d, %d, %d\n", a, b, c, d);
			fs.flag &= ~FS_FLAG_SWAP_CLEAN;
			addr_split_opera(SWAP_ADDR + c - seg_addr, c, min - c, data_to_swap);
		}
		if (d > max) {
			//fprintf(stderr, "area 2: %d, %d, %d, %d\n", a, b, c, d);
			fs.flag &= ~FS_FLAG_SWAP_CLEAN;
			addr_split_opera(SWAP_ADDR + max - seg_addr, max, d - max, data_to_swap);
		}
	}
}

/************************************************
*********���segment��ָ�����������*************
************************************************/
static void segment_clean(WORD addr, WORD noused, WORD len) {
	//fprintf(stderr, "%s(%d,%d,,%d)\n", __FUNCTION__, seg_addr, offset, len);
	if (!(fs.flag & FS_FLAG_SWAP_CLEAN)) {
		__segment_erase(SWAP_ADDR);
		fs.flag |= FS_FLAG_SWAP_CLEAN;
	}
	fs.flag &= ~FS_FLAG_SWAP_DIRE;
	__segment_op(addr, addr + len);
	__segment_erase((addr / SEGMENT_SIZE) * SEGMENT_SIZE);
	if (!(fs.flag & FS_FLAG_SWAP_CLEAN)) {	//˵���ղ��и������ݵĶ���
		fs.flag |= FS_FLAG_SWAP_DIRE;	//�������ݸ��Ʒ���
		__segment_op(addr, addr + len);
	}
}

/************************************************
*****************��临�ƺ���********************
************************************************/
#define SWAP(a, b) do {\
									WORD temp; \
									if (fs.flag & FS_FLAG_SWAP_DIRE) {\
										temp = a; a = b; b = temp;\
									} \
									b = get_phy_addr(b);\
						} while (0);

#if defined(FS_DISK_ROM_FLASH) || defined(FS_DISK_RAM_FLASH)
static void data_to_swap(WORD swap_addr, WORD data_addr, WORD len) {
	SWAP(swap_addr, data_addr);
	__segment_write(swap_addr, data_addr, len);
}
#endif

#ifdef FS_DISK_SPI_FLASH
static void data_to_swap(WORD swap_addr, WORD data_addr, WORD len) {
	BYTE buf[F_COPY_CACHE_SIZE];	//SPI FLASH��Ҫһ����ʱ�ս����������
	BYTE i;
	SWAP(swap_addr, data_addr);
	
	for (i = len / F_COPY_CACHE_SIZE; i > 0; i--, swap_addr += F_COPY_CACHE_SIZE, \
			data_addr += F_COPY_CACHE_SIZE, len -= F_COPY_CACHE_SIZE) 
	{
		__segment_read(data_addr, (WORD)buf, F_COPY_CACHE_SIZE);
		__segment_write(swap_addr, (WORD)buf, F_COPY_CACHE_SIZE);
	}
	
	if (len != 0) {
		__segment_read(data_addr, (WORD)buf, len);
		__segment_write(swap_addr, (WORD)buf, len);
	}
}
#endif

/*******************************************************
***	�������Ԫ
***	�ļ�ϵͳʹ�������ĵ�ַ������������Щ��ַ���ܲ���������
*******************************************************/
#ifdef FS_ENABLE_BLOCK_MGMT
//�߼���ַ�������ַ��ת��
//block_map�洢�����ļ�ϵͳʹ�õ�������˳��
//�ļ�ϵͳʹ�õĿ��ַ�������ģ�����Щ���������Ͽ��ܾͲ��������ģ�
//�ļ�ϵͳʹ�õĿ�0�����������n��
//(virt_addr) / SEGMENT_SIZE �õ��ļ�ϵͳ����n���������ŵ�ʵ���������block_map[n]
static WORD get_phy_block(WORD virt_addr) {
	WORD phy_block = fs.block_map[virt_addr / SEGMENT_SIZE];
	//fprintf(stderr, "phy_block = %lu, virt_addr = %lu\n", phy_block, virt_addr);
	return phy_block;
}
static WORD get_phy_addr(WORD virt_addr) { 
	#ifdef FS_USE_MEM_SWAP
	if (virt_addr - SWAP_ADDR >= 0 && virt_addr - SWAP_ADDR < SEGMENT_SIZE)
		return virt_addr;
	#endif
	WORD phy_addr = DISK + get_phy_block(virt_addr) * SEGMENT_SIZE + (virt_addr) % SEGMENT_SIZE;
	//fprintf(stderr, "virt_addr %lu -> phy_addr %lu \n", virt_addr, phy_addr);
	return phy_addr;
}
//���ҿ��п飬����-1��˵��û�п��п�
static int get_free_block(void) {
	int i;
	for (i = 0; i < DISK_BLOCK; i++)
		if (fs.block_status[i] == BLOCK_UNUSED) {
			fs.block_status[i] = BLOCK_USED;
			return i;
		}
	return -1;
}

static void __segment_erase(WORD addr) {
	int phy_block;
	#ifdef FS_USE_MEM_SWAP
	if (addr - SWAP_ADDR >= 0 && addr - SWAP_ADDR < SEGMENT_SIZE)
		return;
	#endif
	phy_block = get_phy_block(addr);
	while (fs.block_wc[phy_block] >= SEGMENT_ERASE_MAX || segment_erase(get_phy_addr(addr)) != true) {
		//fprintf(stdout, "block %u failed, ", phy_block);
		fs.block_status[phy_block] = BLOCK_FAIL;	//����������Ϊ����
		if ((phy_block = get_free_block()) == -1) {
			fs.flag |= FS_FLAG_NO_FREE_SPACE;
			//fprintf(stdout, "----no free block----\n");
			return;
		}
		//fprintf(stdout, "request new block %d for it.\n", phy_block);
		fs.block_map[(addr) / SEGMENT_SIZE] = phy_block;	//���¿�ӳ��
	}
	fs.block_wc[phy_block]++;
	fs.block_status[phy_block] = BLOCK_USED;		//�ɹ�ʱҲǿ�Ƹ����£���Ҫ��Ϊ��initʱ����ÿ�����״̬
}
static void __segment_read(WORD seg_addr, WORD buf, WORD len) {
	#ifdef FS_USE_MEM_SWAP
	if (seg_addr - SWAP_ADDR >= 0 && seg_addr - SWAP_ADDR < SEGMENT_SIZE) {
		memcpy((void *)buf, (void *)seg_addr, len);
		return;
	}
	#endif
	while (segment_read(get_phy_addr(seg_addr), buf, len) != true) {
		//fprintf(stdout, "disk failed\n");
	}
}
static void __segment_write(WORD seg_addr, WORD buf, WORD len) {
	int phy_block;
	#ifdef FS_USE_MEM_SWAP
	if (seg_addr - SWAP_ADDR >= 0 && seg_addr - SWAP_ADDR < SEGMENT_SIZE) {
		memcpy((void *)seg_addr, (void *)buf, len);
		return;
	}
	#endif
	phy_block =  get_phy_block(seg_addr);
	while (segment_write(get_phy_addr(seg_addr), buf, len) != true) {
		//fprintf(stdout, "disk failed\n");
		//�Ƚ���������copy����������
		if (!(fs.flag & FS_FLAG_SWAP_CLEAN)) {
			__segment_erase(SWAP_ADDR);
			fs.flag |= FS_FLAG_SWAP_CLEAN;
		}
		//��ֹ����Ĳ���ʧ��
		if (!(fs.flag & FS_FLAG_NO_FREE_SPACE))
			__segment_op(seg_addr, seg_addr + len);

		//����������Ϊ����
		fs.block_status[phy_block] = BLOCK_FAIL;	
		fs.flag &= ~FS_FLAG_SWAP_DIRE;
		//����µĴ洢��
		if ((phy_block = get_free_block()) == -1) {
			fs.flag |= FS_FLAG_NO_FREE_SPACE;
			return;	//todo �ò����¿���δ���
		}
		fs.block_map[(seg_addr) / SEGMENT_SIZE] = phy_block;	//���¿�ӳ��

		//�����¿�, ����ԭ���е���������copy���¿���
		__segment_erase((seg_addr / SEGMENT_SIZE) * SEGMENT_SIZE);
		if (!(fs.flag & FS_FLAG_SWAP_CLEAN)) {	//˵���ղ��и������ݵĶ���
			fs.flag |= FS_FLAG_SWAP_DIRE;	//�������ݸ��Ʒ���
			if (!(fs.flag & FS_FLAG_NO_FREE_SPACE))	//ҲҪ��ֹ����ʧ��
				__segment_op(seg_addr, seg_addr + len);
		}
	}
	fs.block_status[phy_block] = BLOCK_USED;		//�ɹ�ʱҲǿ�Ƹ����£���Ҫ��Ϊ��initʱ����ÿ�����״̬ 
}
#else //��ʹ�û������Ԫ
static WORD get_phy_addr(WORD virt_addr) { 
	#ifdef FS_USE_MEM_SWAP
	if (virt_addr - SWAP_ADDR >= 0 && virt_addr - SWAP_ADDR < SEGMENT_SIZE)
		return virt_addr;
	#endif
	return DISK + virt_addr;
}
static void __segment_erase(WORD seg_addr) {
	#ifdef FS_USE_MEM_SWAP
	if (seg_addr - SWAP_ADDR >= 0 && seg_addr - SWAP_ADDR < SEGMENT_SIZE)
		return;
	#endif
	segment_erase(get_phy_addr(seg_addr));
}
static void __segment_read(WORD seg_addr, WORD buf, WORD len) {
	#ifdef FS_USE_MEM_SWAP
	if (seg_addr - SWAP_ADDR >= 0 && seg_addr - SWAP_ADDR < SEGMENT_SIZE) {
		memcpy((void *)buf, (void *)seg_addr, len);
		return;
	}
	#endif
	segment_read(get_phy_addr(seg_addr), buf, len);
}
static void __segment_write(WORD seg_addr, WORD buf, WORD len) {
	#ifdef FS_USE_MEM_SWAP
	if (seg_addr - SWAP_ADDR >= 0 && seg_addr - SWAP_ADDR < SEGMENT_SIZE) {
		memcpy((void *)seg_addr, (void *)buf, len);
		return;
	}
	#endif
	segment_write(get_phy_addr(seg_addr), buf, len);
}
#endif

