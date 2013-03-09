#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "mini_fs.h"


#define SWAP_ADDR		((WORD)(SEGMENT_SIZE * (FS_BLOCK + SUPER_BLOCK)))
#define SUPER_ADDR	((WORD)(SEGMENT_SIZE * FS_BLOCK))

#define CHECK_ARGC

enum {
	FS_FLAG_CHANGED		= 0x01,
	FS_FLAG_SWAP_CLEAN	= 0x02,
	FS_FLAG_SWAP_DIRE		= 0x04,	//����segment_to_segment������1ʱ��ʾ�����ݴ�SWAP������DISK
};

enum {
	NORMAL_WRITE,
	DIRECT_WRITE,
};

static void data_to_swap(WORD swap_addr, WORD data_addr, WORD len);
static void segment_clean(WORD addr, WORD noused, WORD len);
typedef void (*op_fun_t)(WORD addr, WORD data, WORD len);
static void addr_split_opera(WORD addr, WORD data, WORD len, op_fun_t op);

/**<  �����ַ�������ַת�� */
extern const BYTE *DISK;
#define VIRT2PHY(virt) ((WORD)(&DISK[virt]))

/*******************************************************
***	�û��ӿڲ����
***	read����ֱ�ӷ���FLASH��ַ����
*** 	write������Ҫ����FLASH��д�������
***	erase��������ָ�����ļ�
*******************************************************/

#ifdef FS_DISK_ROM_FLASH

const BYTE* f_rom_read(file_id_t id, WORD offset) {
#ifdef CHECK_ARGC
	if (id >= FILE_ID_END || offset >= fs.file[id].file_size)
		return NULL;
#endif
	return (const BYTE *)(VIRT2PHY(fs.file[id].start_addr + offset));
}
#endif

BYTE*	f_read(file_id_t id, WORD offset,	BYTE *buf, WORD len) {
#ifdef CHECK_ARGC
	#define size fs.file[id].file_size
	//offset+len���������offset>=size
	if (id >= FILE_ID_END || buf == NULL || \
		len > size || offset >= size
	)
		return NULL;
	if (offset + len > size)
		len = size - offset;
	#undef size
#endif
	addr_split_opera(VIRT2PHY(fs.file[id].start_addr + offset), \
							(WORD)buf, len, (op_fun_t)segment_read);
	return buf;
}

static WORD _f_write(file_id_t id,	WORD offset, const BYTE *data, WORD len, BYTE write_flag) {
	WORD n, file_addr, file_len;
#ifdef CHECK_ARGC
	#define size fs.file[id].file_size
	if (id >= FILE_ID_END || len == 0 || data == NULL \
		|| len > size || offset >= size
	)
		return 0;
	if (offset + len > size)
		len = size - offset;
	#undef size
#endif
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
			addr_split_opera(VIRT2PHY(file_addr + offset), (WORD)data, n, segment_clean);
		fs.file[id].file_len = offset + len;
	} else {
		//��Ҫ�����������ȫλ�����������ڲ�
		if (write_flag != DIRECT_WRITE) 
			addr_split_opera(VIRT2PHY(file_addr + offset), (WORD)data, len, segment_clean);
	}
	addr_split_opera(VIRT2PHY(file_addr + offset), (WORD)data, len, segment_write);
	fs.flag |= FS_FLAG_CHANGED;
	
	return len;
}

WORD f_write(file_id_t id,	WORD offset, const BYTE *data, WORD len) {
	return _f_write(id, offset, data, len, NORMAL_WRITE);
}

WORD 	f_write_direct(file_id_t id, WORD offset,	const BYTE *data, WORD len) {
	return _f_write(id, offset, data, len, DIRECT_WRITE);
}		
	
WORD f_copy(file_id_t dst, WORD dst_offset, file_id_t src, WORD src_offset, WORD len) {
	BYTE buf[SEGMENT_TO_SEGMENT_BUF];
	int i;
#ifdef CHECK_ARGC
	if (dst >= FILE_ID_END || src >= FILE_ID_END || len == 0 \
			|| dst_offset >= fs.file[dst].file_size || src_offset >= fs.file[src].file_size
		)
		return 0;
	if (dst_offset + len > fs.file[dst].file_size)
		len = fs.file[dst].file_size - dst_offset;	
	if (src_offset + len > fs.file[src].file_size)
		len = fs.file[src].file_size - src_offset;
#endif
	for (i = len >> 5; i > 0; i--, dst_offset += SEGMENT_TO_SEGMENT_BUF, \
			src_offset += SEGMENT_TO_SEGMENT_BUF, len -= SEGMENT_TO_SEGMENT_BUF) 
	{
		f_read(src, src_offset, buf, SEGMENT_TO_SEGMENT_BUF);
		f_write(dst, dst_offset, buf, SEGMENT_TO_SEGMENT_BUF);
	}
	
	if (len != 0) {
		f_read(src, src_offset, buf, len);
		f_write(dst, dst_offset, buf, len);
	}
	
	fs.flag |= FS_FLAG_CHANGED;
	return len;	
}

void f_erase(file_id_t id) {
#ifdef CHECK_ARGC
	if ( id >= FILE_ID_END)
		return;
#endif
	//���ļ����ȵ���0ʱ����������segment_erase����
	addr_split_opera(VIRT2PHY(fs.file[id].start_addr), \
			(WORD)NULL, fs.file[id].file_len, segment_clean);
	fs.file[id].file_len = 0;
	
	fs.flag |= FS_FLAG_CHANGED;
}

WORD f_len(file_id_t id) {
#ifdef CHECK_ARGC
	if ( id >= FILE_ID_END)
		return 0;
#endif
	return fs.file[id].file_len;
}

WORD f_size(file_id_t id) {
#ifdef CHECK_ARGC
	if ( id >= FILE_ID_END)
		return 0;
#endif
	return fs.file[id].file_size;
}

WORD f_addr(file_id_t id) {
#ifdef CHECK_ARGC
	if ( id >= FILE_ID_END)
		return 0;
#endif
	return fs.file[id].start_addr;
}

void f_sync(void) {
	if (fs.flag & FS_FLAG_CHANGED) {
		addr_split_opera(VIRT2PHY(SUPER_ADDR), (WORD)&fs, sizeof(fs), segment_clean);
		addr_split_opera(VIRT2PHY(SUPER_ADDR), (WORD)&fs, sizeof(fs), segment_write);
		fs.flag &= ~FS_FLAG_CHANGED;
	}
}

void f_init(void) {
	file_id_t id;
	BYTE p;
	segment_read(VIRT2PHY(SUPER_ADDR) + offsetof(fs_t, valid), (WORD)&p, sizeof(p));
	
	if (p != 0x76) {
		addr_split_opera(VIRT2PHY(0), (WORD)NULL, sizeof(FILE_LEN_TABLE), segment_clean);
		for (id = FILE1; id < FILE_ID_END; id++) {
			fs.file[id].file_len = 0;
		}
		fs.valid = 0x76;
		fs.flag |= FS_FLAG_CHANGED;
		f_sync();
	} else {
		segment_read(VIRT2PHY(SUPER_ADDR), (WORD)&fs, sizeof(fs));
	}
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
		c = VIRT2PHY(fs.file[id].start_addr);
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
			addr_split_opera(VIRT2PHY(SWAP_ADDR) + c - seg_addr, c, min - c, data_to_swap);
		}
		if (d > max) {
			//fprintf(stderr, "area 2: %d, %d, %d, %d\n", a, b, c, d);
			fs.flag &= ~FS_FLAG_SWAP_CLEAN;
			addr_split_opera(VIRT2PHY(SWAP_ADDR) + max - seg_addr, max, d - max, data_to_swap);
		}
	}
}

/************************************************
*********���segment��ָ�����������*************
************************************************/
static void segment_clean(WORD addr, WORD noused, WORD len) {
	//fprintf(stderr, "%s(%d,%d,,%d)\n", __FUNCTION__, seg_addr, offset, len);
	if (!(fs.flag & FS_FLAG_SWAP_CLEAN)) {
		segment_erase(VIRT2PHY(SWAP_ADDR));
		fs.flag |= FS_FLAG_SWAP_CLEAN;
	}
	fs.flag &= ~FS_FLAG_SWAP_DIRE;
	__segment_op(addr, addr + len);
	segment_erase((addr / SEGMENT_SIZE) * SEGMENT_SIZE);
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
						} while (0);

#if defined(FS_DISK_ROM_FLASH) || defined(FS_DISK_RAM_FLASH)
static void data_to_swap(WORD swap_addr, WORD data_addr, WORD len) {
	SWAP(swap_addr, data_addr);
	segment_write(swap_addr, data_addr, len);
}
#endif

#ifdef FS_DISK_SPI_FLASH
static void data_to_swap(WORD swap_addr, WORD data_addr, WORD len) {
	BYTE buf[SEGMENT_TO_SEGMENT_BUF];	//SPI FLASH��Ҫһ����ʱ�ս����������
	BYTE i;

	SWAP(swap_addr, data_addr);
	
	for (i = len / SEGMENT_TO_SEGMENT_BUF; i > 0; i--, swap_addr += SEGMENT_TO_SEGMENT_BUF, \
			data_addr += SEGMENT_TO_SEGMENT_BUF, len -= SEGMENT_TO_SEGMENT_BUF) 
	{
		segment_read(data_addr, (WORD)buf, SEGMENT_TO_SEGMENT_BUF);
		segment_write(swap_addr, (WORD)buf, SEGMENT_TO_SEGMENT_BUF);
	}
	
	if (len != 0) {
		segment_read(data_addr, (WORD)buf, len);
		segment_write(swap_addr, (WORD)buf, len);
	}
}
#endif

