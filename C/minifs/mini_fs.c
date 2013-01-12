#include <stdio.h>
#include <string.h>

typedef unsigned char UINT8;
typedef unsigned int UINT16;

typedef enum {
	FILE_ID_BEGIN,

	FILE1	= FILE_ID_BEGIN,
	FILE2,
	FILE3,

	FILE_ID_END = FILE3,
} file_id_t;

struct file_info_t {
	file_id_t 	id;
	UINT16 		start_addr;
	UINT16		file_len;
	UINT16		file_size;
};

static struct file_info_t file_info[] = {
	{FILE1, 0, 0, 10},
	{FILE2, 10, 0, 23},
	{FILE3, 33, 0, 150},
};

#define SEGMENT_SIZE	16
#define DISK_SPACE	SEGMENT_SIZE*50
#define SWAP_ADDR	SEGMENT_SIZE*49
static UINT8 DISK[DISK_SPACE];


UINT8 *	f_read(file_id_t id, 	UINT16 offset,	UINT16 len);
void 		f_write(file_id_t id, 	UINT16 offset,	const UINT8 *data, UINT16 len);
void		f_erase(file_id_t id);
static void disk_edit(UINT16 offset, const UINT8 *data, UINT16 len);
static void disk_append(UINT16 offset, const UINT8 *data, UINT16 len);
static void disk_clean(UINT16 offset, UINT16 len);

/*******************************************************
***	�û��ӿڲ����
***	read����ֱ�ӷ���FLASH��ַ����
*** 	write������Ҫ����FLASH��д�������
***	erase��������ָ�����ļ�
*******************************************************/
UINT8 * f_read(file_id_t id, UINT16 offset, UINT16 len) {
	if (id < FILE_ID_BEGIN || id > FILE_ID_END)
		return NULL;
	if (offset + len > file_info[id].file_size)
		return NULL;
	return (UINT8 *)(file_info[id].start_addr + offset);
}

void f_write(file_id_t id,	UINT16 offset,	const UINT8 *data, UINT16 len) {
	UINT16 n;

	if (id < FILE_ID_BEGIN || id > FILE_ID_END)
		return;
	if (offset + len > file_info[id].file_size)
		return;
	
	//ÿ��д�ļ�ʱ���������Ҫ�޸ĵĲ��ֺ���Ҫ׷�ӵĲ���
	//��Ҫ�޸ĵĲ�����Ƶ�����FLASH����׷�ӵĲ��������ļ�����ʱ�Ѿ���д����

	if (offset >= file_info[id].file_len) {
		//��Ҫ���������ֻ��Ҫ׷��
		disk_append(file_info[id].start_addr + offset, data, len);
		file_info[id].file_len = offset + len;
	} else if (offset < file_info[id].file_len && offset + len > file_info[id].file_len) {
		//��Ҫ���������һ����λ�����������ڲ�������һ������Ҫ׷��
		n = file_info[id].file_len - offset;
		disk_edit(file_info[id].start_addr + offset, data, n);
		disk_append(file_info[id].start_addr + file_info[id].file_len, &data[n], len - n);
		file_info[id].file_len = offset + len;
	} else {
		//��Ҫ�����������ȫλ�����������ڲ�
		disk_edit(file_info[id].start_addr + offset, data, len);
	}
}

void f_erase(file_id_t id) {
	if (id < FILE_ID_BEGIN || id > FILE_ID_END)
		return;
	disk_clean(file_info[id].start_addr, file_info[id].file_len);
	file_info[id].file_len = 0;
}

static void f_dump(void) {
	int i, j, n;
	n = sizeof(file_info) / sizeof(file_info[0]);
	for (i = 0; i < n; i++) {
		printf("FILE %d: addr = %d, len = %d, size = %d\n", i, \
			file_info[i].start_addr, file_info[i].file_len, file_info[i].file_size);
		for (j = 0; j < file_info[i].file_size; j++)
			putchar(DISK[file_info[i].start_addr + j]);
		putchar('\n');
	}
}

/*******************************************************
***	�ײ�IO����
***	��Ҫʵ��Flash�����������ڴ浽Flash�ĸ��ƣ�
***  flash��flash�ĸ��ƹ�3������
*******************************************************/

//��Ҫ��ֲ�ĺ���, ������ȷ����ַ�Ѿ���segment����
static void segment_erase(UINT16 addr) {
	if (addr % SEGMENT_SIZE) {
		fprintf(stderr, "�Ƕ��������\n");
		return;
	}
	memset(&DISK[addr], '0', SEGMENT_SIZE);
}

//��Ҫ��ֲ�ĺ����������ݴ��ڴ�д��FLASH�������߱�֤���ڵ�FLASH�Ѿ���������.
static void segment_copy_mem(UINT16 addr, UINT16 offset,  const UINT8 *data, UINT16 len) {
	memcpy(&DISK[addr + offset], data, len);
}

//��Ҫ��ֲ�ĺ����������ݴ�FLASH������FLASH�������߱�֤Ŀ������FLASH�Ѿ���������.
static void segment_copy_segment(UINT16 des, UINT16 src, UINT16 len) {
	memcpy(&DISK[des], &DISK[src], len);
}

static void segment_clean(UINT16 addr, UINT16 offset, const UINT8 *noused, UINT16 len) {
	UINT16 n = offset + len;

	//�������棬��Ŀ�����ڿ���������ݰ�����
	segment_erase(SWAP_ADDR);
	segment_copy_segment(SWAP_ADDR, addr, offset); //ǰ
	segment_copy_segment(SWAP_ADDR + n, addr + n, SEGMENT_SIZE - n);	//��

	//����Ŀ��飬���ղŻ��������д��
	segment_erase(addr);
	segment_copy_segment(addr, SWAP_ADDR, offset);	//ǰ
	segment_copy_segment(addr + n, SWAP_ADDR + n, SEGMENT_SIZE - n); //��
}

static void segment_write(UINT16 addr, UINT16 offset, const UINT8 *data, UINT16 len) {
	segment_clean(addr, offset, data, len);	
	segment_copy_mem(addr, offset, data, len); 	//д���û�����
}

//����������
typedef void (*op_fun_t)(UINT16 addr, UINT16 offset, const UINT8 *data, UINT16 len);
static void __addr_split_opera(UINT16 offset, const UINT8 *data, UINT16 len, op_fun_t op) {
	UINT16 temp_addr, temp_off, temp_len;
	int i, n;
	
	//��һ����offset���¶��� - offset, ֪����Ҫ��д����һ���ַ
	temp_addr = (offset / SEGMENT_SIZE ) * SEGMENT_SIZE;
	temp_off = offset - temp_addr;
	temp_len = SEGMENT_SIZE - temp_off > len ? len : SEGMENT_SIZE - temp_off;
	
	if (temp_len != 0) 
		op(temp_addr, temp_off, data, temp_len);
	
	//�ڶ���, copy �պö���Ĳ���
	offset = temp_addr + SEGMENT_SIZE;
	data += temp_len;
	len -= temp_len;
	
	n = len / SEGMENT_SIZE;
	//printf("block:%d / %d")
	for (i = 0; i < n; i++) 
		op(offset + i * SEGMENT_SIZE, 0, data + i * SEGMENT_SIZE, SEGMENT_SIZE);
	
	//������
	offset += n * SEGMENT_SIZE;
	data += n * SEGMENT_SIZE;
	len -= n * SEGMENT_SIZE;
	if (len != 0) 
		op(offset, 0, data, len);
}

static void disk_edit(UINT16 offset, const UINT8 *data, UINT16 len) {
	//segment_write �Ȳ�����д������Դһ������FLASH��һ�������ڴ�
	__addr_split_opera(offset, data, len, segment_write);
}

static void disk_append(UINT16 offset, const UINT8 *data, UINT16 len) {
	//segment_copy ֻ��Ҫд������Դֻ�����ڴ棬�Ѿ�����������
	__addr_split_opera(offset, data, len, segment_copy_mem);
}

static void disk_clean(UINT16 offset, UINT16 len) {
	//segment_clean ֻ�����, ����Ҫ����Դ
	__addr_split_opera(offset, NULL, len, segment_clean);
}


/*******************************************************
***	�û�����Ժ���
*******************************************************/

int main(void) {
	UINT8 i;
	//UINT8 tmp[17] = "this is a test";

	memset(DISK, '0', sizeof(DISK));
	for (i = 0; i < 1; i++) {
		//f_write(FILE2, 1, tmp, sizeof(tmp));
		//f_write(FILE3, 20, (UINT8 *)"abcdefg", 7);
		//f_write(FILE3, 35, (UINT8 *)"1234567", 7);
		//f_write(FILE3, 2, (UINT8 *)"ABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFG", 63);
		//f_write(FILE3, 14, (UINT8 *)"ZZZ", 3);
		//f_dump();
		f_write(FILE3, 14, (UINT8 *)"DDDDDDDDDDDDDDDDDDD", 19);
		f_dump();
		//f_write(FILE1, 2, (UINT8 *)"ABCDEFGABC", 8);
		//f_write(FILE3, 7 * i + i * 2, (UINT8 *)"ABCDEFG", 7);
	}

	return 0;
}


