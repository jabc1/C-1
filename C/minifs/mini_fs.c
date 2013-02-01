#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include "mini_fs.h"

#ifndef FS_USE_MEM_SWAP
#define SWAP_ADDR    ((WORD)SEGMENT_SIZE)*(DISK_BLOCK-1)
#else
#define SWAP_ADDR 	((WORD)&__FS_SWAP_SPACE[0])
#endif

#define SUPER_BLOCK  ((WORD)SEGMENT_SIZE)*(DISK_BLOCK-2-(sizeof(fs) + SEGMENT_SIZE -1) / SEGMENT_SIZE)

#define SIZEOF(s,m) ((size_t) sizeof(((s *)0)->m)) 
fs_t fs = { 
   .flag = 0,
   .file = { 
      {offsetof(FILE_LEN_TABLE, FILE1_SIZE),   0, SIZEOF(FILE_LEN_TABLE, FILE1_SIZE)},
		{offsetof(FILE_LEN_TABLE, FILE2_SIZE),   0, SIZEOF(FILE_LEN_TABLE, FILE2_SIZE)},
		{offsetof(FILE_LEN_TABLE, FILE3_SIZE),   0, SIZEOF(FILE_LEN_TABLE, FILE3_SIZE)},
		{offsetof(FILE_LEN_TABLE, FILE4_SIZE),   0, SIZEOF(FILE_LEN_TABLE, FILE4_SIZE)},
		{offsetof(FILE_LEN_TABLE, FILE5_SIZE),   0, SIZEOF(FILE_LEN_TABLE, FILE5_SIZE)},
		{offsetof(FILE_LEN_TABLE, FILE6_SIZE),   0, SIZEOF(FILE_LEN_TABLE, FILE6_SIZE)},
		{offsetof(FILE_LEN_TABLE, FILE7_SIZE),   0, SIZEOF(FILE_LEN_TABLE, FILE7_SIZE)},
		{offsetof(FILE_LEN_TABLE, FILE8_SIZE),   0, SIZEOF(FILE_LEN_TABLE, FILE8_SIZE)},
		{offsetof(FILE_LEN_TABLE, FILE9_SIZE),   0, SIZEOF(FILE_LEN_TABLE, FILE9_SIZE)},
   },  
};

#define fprintf(...) 
//#define printf(...) 

static void disk_clean(WORD addr, WORD len);
static void disk_read(WORD addr, BYTE *data, WORD len);
static void disk_edit(WORD addr, const BYTE *data, WORD len);
static void disk_append(WORD addr, const BYTE *data, WORD len);

/*******************************************************
***	�û��ӿڲ����
***	read����ֱ�ӷ���FLASH��ַ����
*** 	write������Ҫ����FLASH��д�������
***	erase��������ָ�����ļ�
*******************************************************/

#ifdef FS_DISK_ROM_FLASH
extern const BYTE *f_disk_addr(void);
const BYTE* f_rom_read(file_id_t id, WORD offset, WORD len) {
	if (id >= FILE_ID_END)
		return NULL;
	if (offset + len > fs.file[id].file_size)
		len = fs.file[id].file_size - offset;
	return &f_disk_addr()[fs.file[id].start_addr + offset];
}
#endif

BYTE*	f_read(file_id_t id, WORD offset,	BYTE *buf, WORD len) {
	if (id >= FILE_ID_END || buf == NULL)
		return NULL;
	if (offset + len > fs.file[id].file_size)
		len = fs.file[id].file_size - offset;
	disk_read(fs.file[id].start_addr + offset, buf, len);
	return buf;
}

WORD f_write(file_id_t id,	WORD offset,	const BYTE *data, WORD len) {
	WORD n;

	if (id >= FILE_ID_END)
		return 0;
	if (offset + len > fs.file[id].file_size)
		len = fs.file[id].file_size - offset;

	//ÿ��д�ļ�ʱ���������Ҫ�޸ĵĲ��ֺ���Ҫ׷�ӵĲ���
	//��Ҫ�޸ĵĲ�����Ƶ�����FLASH����׷�ӵĲ��������ļ�����ʱ�Ѿ���д����

	if (offset >= fs.file[id].file_len) {
		//��Ҫ���������ֻ��Ҫ׷��
		disk_append(fs.file[id].start_addr + offset, data, len);
		fs.file[id].file_len = offset + len;
	} else if (offset < fs.file[id].file_len && offset + len > fs.file[id].file_len) {
		//��Ҫ���������һ����λ�����������ڲ�������һ������Ҫ׷��
		n = fs.file[id].file_len - offset;
		disk_edit(fs.file[id].start_addr + offset, data, n);
		disk_append(fs.file[id].start_addr + fs.file[id].file_len, &data[n], len - n);
		fs.file[id].file_len = offset + len;
	} else {
		//��Ҫ�����������ȫλ�����������ڲ�
		disk_edit(fs.file[id].start_addr + offset, data, len);
	}

	return len;
}

void f_erase(file_id_t id) {
	if ( id >= FILE_ID_END)
		return;
	disk_clean(fs.file[id].start_addr, fs.file[id].file_len);
	fs.file[id].file_len = 0;
}

WORD f_len(file_id_t id) {
	if ( id >= FILE_ID_END)
		return 0;
	return fs.file[id].file_len;
}

void f_sync(void) {
	disk_edit(SUPER_BLOCK, (BYTE *)&fs, sizeof(fs));
}

void f_init(void) {
	file_id_t id;
	fs_t p;
	segment_read(SUPER_BLOCK, 0, (WORD)&p, sizeof(p));
	
	if (p.valid != 0x76) {
		for (id = FILE1; id < FILE_ID_END; id++) {
			fs.file[id].file_len = fs.file[id].file_size;
			f_erase(id);
		}
		fs.valid = 0x76;
		f_sync();
		printf("FS init again.\n");
	} else {
		memcpy(&fs, &p, sizeof(fs));
		printf("FS Load.\n");
	}
}

/*******************************************************
***	������
***	��Ҫ�漰�������Ϳ����ʱ���������������Ĳ���
*******************************************************/

//���������
typedef void (*op_fun_t)(WORD seg_addr, WORD offset, WORD data, WORD len);
static void __addr_split_opera(WORD addr, WORD data, WORD len, WORD split_unit, op_fun_t op) {
	int i, temp_off, temp_len;
	//��һ��������δ����Ĳ���
	if ((temp_off = addr % split_unit) != 0) {
		temp_len = temp_off + len > split_unit ? split_unit - temp_off : len;
		op(addr - temp_off, temp_off, data, temp_len);
		addr += temp_len; data += temp_len; len -= temp_len;
	}
	//�ڶ���, ����պö���Ĳ���
	for (i = len / split_unit; i > 0; i--, addr += split_unit, data += split_unit, len -= split_unit)
		op(addr, 0, data, split_unit);
	//������,�������ʣ�µ�
	if (len != 0)
		op(addr, 0, data, len);
}

/************************************************
*****************��临�ƺ���********************
************************************************/

#ifndef FS_USE_MEM_SWAP	//��ʹ���ڴ潻���������

#define SEGMENT_TO_SWAP	segment_copy_segment
#define SWAP_TO_SEGMENT	segment_copy_segment

#ifdef FS_DISK_ROM_FLASH
void segment_copy_segment(WORD seg_dst, WORD dst_off, WORD seg_src, WORD len) {\
	segment_write(seg_dst, dst_off, seg_src, len);
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

#else	//ʹ���ڴ���Ϊ������ݽ���ʱ

#define SEGMENT_TO_SWAP	segment_to_mem 
#define SWAP_TO_SEGMENT	segment_write
static void segment_to_mem(WORD mem_addr, WORD mem_offset, WORD flash_addr, WORD len){
	WORD seg_addr = flash_addr / SEGMENT_SIZE * SEGMENT_SIZE;
	segment_read(seg_addr , flash_addr - seg_addr, mem_addr + mem_offset, len);
}
#endif


#define is_contain(a, b, c,d ) ((d) >= (a) && (c) < (b))
#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

static void __segment_op(WORD seg_addr, WORD a, WORD b, BYTE step) {
	WORD id, c, d, min, max;

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
			if (step == 0)	//�����ݴ�FLASH������������
				__addr_split_opera(SWAP_ADDR + c - seg_addr, c, min - c, MAX_WRITE_UNIT, SEGMENT_TO_SWAP);
			else				//�����ݴӻ�����������FLASH
				__addr_split_opera(c, SWAP_ADDR + c - seg_addr, min - c, MAX_WRITE_UNIT, SWAP_TO_SEGMENT);
		}
		if (d > max) {
			//fprintf(stderr, "area 2: %d, %d, %d, %d\n", a, b, c, d);
			if (step == 0)
				__addr_split_opera(SWAP_ADDR + max - seg_addr, max, d - max, MAX_WRITE_UNIT, SEGMENT_TO_SWAP);
			else
				__addr_split_opera(max, SWAP_ADDR + max - seg_addr, d - max, MAX_WRITE_UNIT, SWAP_TO_SEGMENT);
		}
	}
}

static void segment_clean(WORD seg_addr, WORD offset, WORD noused, WORD len) {
	fprintf(stderr, "%s(%d,%d,,%d)\n", __FUNCTION__, seg_addr, offset, len);
#ifndef FS_USE_MEM_SWAP
	segment_erase(SWAP_ADDR);
#endif
	__segment_op(seg_addr, seg_addr + offset, seg_addr + offset + len, 0);
	segment_erase(seg_addr);
	__segment_op(seg_addr, seg_addr + offset, seg_addr + offset + len, 1);
}

static void segment_edit(WORD seg_addr, WORD offset, WORD data, WORD len) {
	if (len == 0)
		return;
	segment_clean(seg_addr, offset, data, len);	
	//д���û�����
	__addr_split_opera(seg_addr + offset, (WORD)data, len, MAX_WRITE_UNIT, segment_write);
}

static void disk_edit(WORD addr, const BYTE *data, WORD len) {
	//segment_edit �Ȳ�����д������Դһ������FLASH��һ�������ڴ�
	__addr_split_opera(addr, (WORD)data, len, SEGMENT_SIZE, segment_edit);
}

static void disk_append(WORD addr, const BYTE *data, WORD len) {
	//segment_write ֻ��Ҫд������Դֻ�����ڴ棬�Ѿ�����������
	__addr_split_opera(addr, (WORD)data, len, MAX_WRITE_UNIT, segment_write);
}

static void disk_clean(WORD addr, WORD len) {
	//segment_clean ֻ�����, ����Ҫ����Դ
	__addr_split_opera(addr, (WORD)NULL, len, SEGMENT_SIZE, segment_clean);
}

static void disk_read(WORD addr, BYTE *buf, WORD len) {
	//segment_clean ֻ�����, ������û�ж�������
	__addr_split_opera(addr, (WORD)buf, len, MAX_WRITE_UNIT, (op_fun_t)segment_read);
}
