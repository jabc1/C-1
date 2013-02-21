#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "mini_fs.h"


#define SUPER_ADDR	((WORD)SEGMENT_SIZE * FS_BLOCK)
#define SWAP_ADDR		((WORD)SEGMENT_SIZE * (FS_BLOCK + SUPER_BLOCK))

//#define CHECK_ARGC

enum {
	FS_FLAG_CHANGED		= 0x01,
	FS_FLAG_SWAP_CLEAN	= 0x02,
};

enum {
	NORMAL_WRITE,
	DIRECT_WRITE,
};

//static void disk_clean(WORD addr, WORD len);
//static void disk_read(WORD addr, BYTE *data, WORD len);
//static void disk_edit(WORD addr, const BYTE *data, WORD len);
//static void disk_append(WORD addr, const BYTE *data, WORD len);
static void segment_clean(WORD addr, WORD noused, WORD len);
static void segment_edit(WORD addr, WORD data, WORD len);
extern WORD DISK_ADDR;

/*******************************************************
***	�ļ�ϵͳ�㵽���Ĳ���ӳ��
*******************************************************/
typedef void (*op_fun_t)(WORD addr, WORD data, WORD len);
static void __addr_split_opera(WORD addr, WORD data, WORD len, op_fun_t op);
#define disk_edit(addr, data, len) 		__addr_split_opera(VIRT2PHY(addr), (WORD)(data), (len), segment_edit)
#define disk_append(addr, data, len) 	__addr_split_opera(VIRT2PHY(addr), (WORD)data, (len), segment_write)
#define disk_clean(addr, len) 			__addr_split_opera(VIRT2PHY(addr), (WORD)NULL, (len), segment_clean)
#define disk_read(addr, buf, len) 		__addr_split_opera(VIRT2PHY(addr), (WORD)buf, (len), (op_fun_t)segment_read)

/**<  �����ַ�������ַת�� */
#define VIRT2PHY(virt) ((WORD)&(((BYTE *)DISK_ADDR)[virt]))

/*******************************************************
***	�û��ӿڲ����
***	read����ֱ�ӷ���FLASH��ַ����
*** 	write������Ҫ����FLASH��д�������
***	erase��������ָ�����ļ�
*******************************************************/

#ifdef FS_DISK_ROM_FLASH

const BYTE* f_rom_read(file_id_t id, WORD offset) {
#ifdef CHECK_ARGC
	if (id >= FILE_ID_END)
		return NULL;
	if (offset > fs.file[id].file_size)
		return NULL;
#endif
	return (BYTE *)(VIRT2PHY(fs.file[id].start_addr + offset));
}
#endif

BYTE*	f_read(file_id_t id, WORD offset,	BYTE *buf, WORD len) {
#ifdef CHECK_ARGC
	if (id >= FILE_ID_END || buf == NULL)
		return NULL;
	if (offset + len > fs.file[id].file_size)
		len = fs.file[id].file_size - offset;
#endif
	disk_read(fs.file[id].start_addr + offset, buf, len);
	return buf;
}

static WORD _f_write(file_id_t id,	WORD offset, const BYTE *data, WORD len, BYTE write_flag) {
	WORD n, file_addr, file_len;
//	typedef void (*disk_edit_t)(WORD addr, const BYTE *data, WORD len);
//	disk_edit_t disk_edit_p = disk_edit;
//	if (write_flag == DIRECT_WRITE)
//		disk_edit_p = disk_append;

#ifdef CHECK_ARGC
	if (id >= FILE_ID_END || len == 0)
		return 0;
	if (offset + len > fs.file[id].file_size)
		len = fs.file[id].file_size - offset;
#endif
	//ÿ��д�ļ�ʱ���������Ҫ�޸ĵĲ��ֺ���Ҫ׷�ӵĲ���
	//��Ҫ�޸ĵĲ�����Ƶ�����FLASH����׷�ӵĲ��������ļ�����ʱ�Ѿ���д����
	file_addr = fs.file[id].start_addr;
	file_len = fs.file[id].file_len;
	
	if (offset >= file_len) {
		//��Ҫ���������ֻ��Ҫ׷��
		disk_append(file_addr + offset, data, len);
		fs.file[id].file_len = offset + len;
	} else if (offset < file_len && offset + len > file_len) {
		//��Ҫ���������һ����λ�����������ڲ�������һ������Ҫ׷��
		n = file_len - offset;
		//disk_edit_p(file_addr + offset, data, n);
		if (write_flag == DIRECT_WRITE)
			 disk_append(file_addr + offset, data, n);
		else
			disk_edit(file_addr + offset, data, n);
		disk_append(file_addr + file_len, &data[n], len - n);
		fs.file[id].file_len = offset + len;
	} else {
		//��Ҫ�����������ȫλ�����������ڲ�
		//disk_edit_p(file_addr + offset, data, len);
		if (write_flag == DIRECT_WRITE)
			 disk_append(file_addr + offset, data, n);
		else
			disk_edit(file_addr + offset, data, n);
	}

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
	if (dst >= FILE_ID_END || src >= FILE_ID_END || len == 0)
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
	disk_clean(fs.file[id].start_addr, fs.file[id].file_len);
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
	if (fs.flag & FS_FLAG_CHANGED)
		disk_edit(SUPER_ADDR, (BYTE *)&fs, sizeof(fs));
}

void f_init(void) {
	file_id_t id;
	BYTE p;
	segment_read(VIRT2PHY(SUPER_ADDR) +  offsetof(fs_t, valid), (WORD)&p, sizeof(p));
	
	if (p != 0x76) {
		for (id = FILE1; id < FILE_ID_END; id++) {
			fs.file[id].file_len = fs.file[id].file_size;
			f_erase(id);
		}
		fs.valid = 0x76;
		f_sync();
	} else {
		segment_read(VIRT2PHY(SUPER_ADDR), (WORD)&fs, sizeof(fs));
	}
}

/************************************************
*****************��临�ƺ���********************
************************************************/

#if defined(FS_DISK_ROM_FLASH) || defined(FS_DISK_RAM_FLASH)
void segment_copy_segment(WORD addr, WORD seg_src, WORD len) {\
	segment_write(addr, seg_src, len);
}
#endif

#ifdef FS_DISK_SPI_FLASH
void segment_copy_segment(WORD seg_dst, WORD dst_off, WORD seg_src, WORD len) {
	WORD dst_addr = FS_DISK_ADDR + seg_dst + dst_off;
	WORD src_addr = FS_DISK_ADDR + seg_src;
	BYTE buf[SEGMENT_TO_SEGMENT_BUF];	//SPI FLASH��Ҫһ����ʱ�ս����������
	BYTE i;

	for (i = len / SEGMENT_TO_SEGMENT_BUF; i > 0; i--, dst_addr += SEGMENT_TO_SEGMENT_BUF, \
			src_addr += SEGMENT_TO_SEGMENT_BUF, len -= SEGMENT_TO_SEGMENT_BUF) 
	{
		segment_read(src_addr, 0, (WORD)buf, SEGMENT_TO_SEGMENT_BUF);
		segment_write(dst_addr, 0, (WORD)buf, SEGMENT_TO_SEGMENT_BUF);
	}
	
	if (len != 0) {
		segment_read(src_addr, 0, (WORD)buf, len);
		segment_write(dst_addr, 0, (WORD)buf, len);
	}
}
#endif

/*******************************************************
***	�������������
***	��Ҫ�漰�������Ϳ����ʱ���������������Ĳ���
*******************************************************/

//���������
//typedef void (*op_fun_t)(WORD addr, WORD data, WORD len);
static void __addr_split_opera(WORD addr, WORD data, WORD len, op_fun_t op) {
	int i, temp_off, temp_len = len;
	WORD split_unit = MAX_WRITE_UNIT;
	
	if (op == segment_edit || op == segment_clean) 
		split_unit = SEGMENT_SIZE;
	
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

static void __segment_op(WORD a, WORD b, BYTE step) {
	WORD id, c, d, min, max;
	WORD seg_addr = (a / SEGMENT_SIZE) * SEGMENT_SIZE;

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
			if (step == 0)
				__addr_split_opera(VIRT2PHY(SWAP_ADDR + c - seg_addr), c, min - c, segment_copy_segment);
			else
				__addr_split_opera(c, VIRT2PHY(SWAP_ADDR + c - seg_addr), min - c, segment_copy_segment);
		}
		if (d > max) {
			//fprintf(stderr, "area 2: %d, %d, %d, %d\n", a, b, c, d);
			fs.flag &= ~FS_FLAG_SWAP_CLEAN;
			if (step == 0)
				__addr_split_opera(VIRT2PHY(SWAP_ADDR + max - seg_addr), max, d - max, segment_copy_segment);
			else
				__addr_split_opera(max, VIRT2PHY(SWAP_ADDR + max - seg_addr), d - max, segment_copy_segment);
		}
	}
}

static void segment_clean(WORD addr, WORD noused, WORD len) {
	//fprintf(stderr, "%s(%d,%d,,%d)\n", __FUNCTION__, seg_addr, offset, len);
	if (!(fs.flag & FS_FLAG_SWAP_CLEAN)) {
		segment_erase(VIRT2PHY(SWAP_ADDR));
		fs.flag |= FS_FLAG_SWAP_CLEAN;
	}
	__segment_op(addr, addr + len, 0);
	segment_erase(addr);
	__segment_op(addr, addr + len, 1);
}

static void segment_edit(WORD addr, WORD data, WORD len) {
	segment_clean(addr, data, len);	
	//д���û�����
	__addr_split_opera(addr, (WORD)data, len, segment_write);
}
#if 0
/*******************************************************
***	�ļ�ϵͳ�㵽���Ĳ���ӳ��
*******************************************************/
static void disk_edit(WORD addr, const BYTE *data, WORD len) {
	//segment_edit �Ȳ�����д������Դһ������FLASH��һ�������ڴ�
	__addr_split_opera(VIRT2PHY(addr), (WORD)data, len, segment_edit);
}

static void disk_append(WORD addr, const BYTE *data, WORD len) {
	//segment_write ֻ��Ҫд������Դֻ�����ڴ棬�Ѿ�����������
	__addr_split_opera(VIRT2PHY(addr), (WORD)data, len, segment_write);
}

static void disk_clean(WORD addr, WORD len) {
	//segment_clean ֻ�����, ����Ҫ����Դ
	__addr_split_opera(VIRT2PHY(addr), (WORD)NULL, len, segment_clean);
}

static void disk_read(WORD addr, BYTE *buf, WORD len) {
	//segment_clean ֻ�����, ������û�ж�������
	__addr_split_opera(VIRT2PHY(addr), (WORD)buf, len, (op_fun_t)segment_read);
}
#endif
