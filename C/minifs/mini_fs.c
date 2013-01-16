#include <stdio.h>
#include <string.h>

typedef unsigned char UINT8;
typedef unsigned int UINT16;

typedef enum {
	FILE1,
	FILE2,
	FILE3,

	FILE_ID_END = FILE3,
} file_id_t;

struct file_info_t {
	UINT16 		start_addr;
	UINT16		file_len;
	UINT16		file_size;
};

typedef struct fs_t {
	UINT8 valid;
	UINT8 flag;
	struct file_info_t file[FILE_ID_END + 1];
} fs_t;
static fs_t fs = {
	.flag = 0,
	.file = {
		{0, 	0,	10},
		{10, 	0,	23},
		{33,	0,	150},
	},
};

#define SEGMENT_SIZE	16
#define DISK_SPACE	SEGMENT_SIZE*51
#define SWAP_ADDR		SEGMENT_SIZE*50
#define SUPER_BLOCK	SEGMENT_SIZE* (50 -(sizeof(fs) + SEGMENT_SIZE -1) / SEGMENT_SIZE)

static UINT8 DISK[DISK_SPACE];
static UINT8 DISK_MAP[DISK_SPACE];	//���ڸ���DISKĳ���ֽ����ڵ������Ƿ񱻲�����

void 		f_init(void);
void 		f_sync(void);
UINT8*	f_read(file_id_t id, 	UINT16 offset,	UINT16 len);
UINT16 	f_write(file_id_t id, 	UINT16 offset,	const UINT8 *data, UINT16 len);
void		f_erase(file_id_t id);
static void disk_edit(UINT16 offset, const UINT8 *data, UINT16 len);
static void disk_append(UINT16 offset, const UINT8 *data, UINT16 len);
static void disk_clean(UINT16 offset, UINT16 len);

//��������������Ҫ��ֲ������ĵ�ַΪ�����DISK��ƫ�Ƶ�ַ
static void segment_erase(UINT16 addr);
static void segment_copy_mem(UINT16 addr, UINT16 offset,  const UINT8 *data, UINT16 len);
static void segment_copy_segment(UINT16 des, UINT16 src, UINT16 len);

/*******************************************************
***	�û��ӿڲ����
***	read����ֱ�ӷ���FLASH��ַ����
*** 	write������Ҫ����FLASH��д�������
***	erase��������ָ�����ļ�
*******************************************************/
UINT8* f_read(file_id_t id, UINT16 offset, UINT16 len) {
	if (id > FILE_ID_END)
		return NULL;
	if (offset + len > fs.file[id].file_size)
		len = fs.file[id].file_size - offset;
	return &DISK[fs.file[id].start_addr + offset];
}

UINT16 f_write(file_id_t id,	UINT16 offset,	const UINT8 *data, UINT16 len) {
	UINT16 n;

	if (id > FILE_ID_END)
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
	if ( id > FILE_ID_END)
		return;
	disk_clean(fs.file[id].start_addr, fs.file[id].file_len);
	fs.file[id].file_len = 0;
}

UINT16 f_len(file_id_t id) {
	if ( id > FILE_ID_END)
		return 0;
	return fs.file[id].file_len;
}

void f_sync(void) {
	//segment_erase(SUPER_BLOCK);
	//disk_clean(SUPER_BLOCK, SWAP_ADDR - SUPER_BLOCK);
	disk_edit(SUPER_BLOCK, (UINT8 *)&fs, sizeof(fs));
	//segment_copy_mem(SUPER_BLOCK, 0, (UINT8 *) &fs, sizeof(fs));
}

void f_init(void) {
	int id;
	fs_t *p = (void *)&DISK[SUPER_BLOCK];
	if (p->valid != 0x76) {
		for (id = 0; id <= FILE_ID_END; id++) {
			fs.file[id].file_len = fs.file[id].file_size;
			f_erase(id);
		}
		fs.valid = 0x76;
		f_sync();
	} else {
		memcpy(&fs, &DISK[SUPER_BLOCK], sizeof(fs));
	}
}

static void f_dump(void) {
	int i, j, n;
	n = sizeof(fs.file) / sizeof(fs.file[0]);
	for (i = 0; i < n; i++) {
		printf("FILE %d: addr = %d, len = %d, size = %d\n", i + 1, \
			fs.file[i].start_addr, fs.file[i].file_len, fs.file[i].file_size);
		for (j = 0; j < fs.file[i].file_size; j++)
			putchar(DISK[fs.file[i].start_addr + j]);
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
	fprintf(stderr, "%s(%d)\n", __FUNCTION__, addr);
	if (addr % SEGMENT_SIZE != 0)
		fprintf(stderr, "%s:segment\n", __FUNCTION__);
	memset(&DISK[addr], '0', SEGMENT_SIZE);
	memset(&DISK_MAP[addr], 1, SEGMENT_SIZE);
}

static int erase_byte_count(UINT16 addr, UINT16 len) {
	int i, ret = 0;
	for (i = 0; i < len; i++)
		ret += DISK_MAP[addr + i];
	return ret;
}

//��Ҫ��ֲ�ĺ����������ݴ��ڴ�д��FLASH�������߱�֤���ڵ�FLASH�Ѿ��������Ҳ��������
static void segment_copy_mem(UINT16 addr, UINT16 offset,  const UINT8 *data, UINT16 len) {
	fprintf(stderr, "%s(%d, %d, %p, %d)\n", __FUNCTION__, addr, offset, data, len);
	if (offset + len > SEGMENT_SIZE || addr % SEGMENT_SIZE != 0)
		fprintf(stderr, "%s:segment not split\n", __FUNCTION__);
	if (erase_byte_count(addr + offset, len) != len)
		fprintf(stderr, "%s:write before erase:%d+%d, %d\n", __FUNCTION__, \
				addr, offset, len);
	memcpy(&DISK[addr + offset], data, len);
	memset(&DISK_MAP[addr + offset], 0, len);
}

//��Ҫ��ֲ�ĺ����������ݴ�FLASH������FLASH�������߱�֤Ŀ������FLASH�Ѿ��������Ҳ��������
static void segment_copy_segment(UINT16 des, UINT16 src, UINT16 len) {
	fprintf(stderr, "%s(%d, %d, %d)\n", __FUNCTION__, des, src, len);
	if (des % SEGMENT_SIZE + len > SEGMENT_SIZE)
		fprintf(stderr, "%s:segment not split\n", __FUNCTION__);
	if (src % SEGMENT_SIZE + len > SEGMENT_SIZE)
		fprintf(stderr, "%s:segment not split\n", __FUNCTION__);
	if (erase_byte_count(des, len) != len)
		fprintf(stderr, "%s:write before erase:%d, %d\n", __FUNCTION__, des, len);
	memcpy(&DISK[des], &DISK[src], len);
	memset(&DISK_MAP[des], 0, len);
}

#define is_contain(a, b, c,d ) ((d) >= (a) && (c) < (b))
#define MIN(a, b) (a) < (b) ? (a) : (b)
#define MAX(a, b) (a) > (b) ? (a) : (b)

static void __segment_op(UINT16 seg_addr, UINT16 a, UINT16 b, UINT8 step) {
	UINT16 id, c, d, min, max;

	for (id = FILE1; id <= FILE_ID_END; id++) {
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
			if (step == 0)
				segment_copy_segment(SWAP_ADDR + c - seg_addr, c, min - c);
			else
				segment_copy_segment(c, SWAP_ADDR + c - seg_addr, min - c);
		}
		if (d > max) {
			//fprintf(stderr, "area 2: %d, %d, %d, %d\n", a, b, c, d);
			if (step == 0)
				segment_copy_segment(SWAP_ADDR + max - seg_addr, max, d - max);
			else
				segment_copy_segment(max, SWAP_ADDR + max - seg_addr, d - max);
		}
	}
}

static void segment_clean(UINT16 seg_addr, UINT16 offset, const UINT8 *noused, UINT16 len) {
	fprintf(stderr, "%s(%d,%d,,%d)\n", __FUNCTION__, seg_addr, offset, len);
	segment_erase(SWAP_ADDR);
	__segment_op(seg_addr, seg_addr + offset, seg_addr + offset + len, 0);
	segment_erase(seg_addr);
	__segment_op(seg_addr, seg_addr + offset, seg_addr + offset + len, 1);
}

static void segment_write(UINT16 addr, UINT16 offset, const UINT8 *data, UINT16 len) {
	if (len == 0)
		return;
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
	if (temp_len != 0) {
		fprintf(stderr, "step 1:\n");
		op(temp_addr, temp_off, data, temp_len);
	}
	
	//�ڶ���, copy �պö���Ĳ���
	offset = temp_addr + SEGMENT_SIZE;
	data += temp_len;
	len -= temp_len;
	
	n = len / SEGMENT_SIZE;
	for (i = 0; i < n; i++) {
		fprintf(stderr, "step 2:\n");
		op(offset + i * SEGMENT_SIZE, 0, data + i * SEGMENT_SIZE, SEGMENT_SIZE);
	}

	//������
	offset += n * SEGMENT_SIZE;
	data += n * SEGMENT_SIZE;
	len -= n * SEGMENT_SIZE;
	if (len != 0) {
		fprintf(stderr, "step 3:\n");
		op(offset, 0, data, len);
	}
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

void f_test(void) {
	UINT8 tmp_data[20] = "this is a test line";
	//UINT8 tmp_read[20];

	//erase ����
	f_erase(FILE1);
	fprintf(stderr, "f_erase(FILE1) Finished.\n");
	f_erase(FILE2);
	fprintf(stderr, "f_erase(FILE2) Finished.\n");
	f_erase(FILE3);
	fprintf(stderr, "f_erase(FILE3) Finished.\n");

#if 1
	f_write(FILE1, 2, tmp_data, 5);
	fprintf(stderr, "f_write(FILE1) Finished.\n");
	if (memcmp(f_read(FILE1, 2, 5), tmp_data, 5) != 0)
		fprintf(stderr, "%s:%d erase failed.\n", __FUNCTION__, __LINE__);
	//��д����
	fprintf(stderr, "f_write(FILE1) Begin.\n");
	f_write(FILE1, 2, tmp_data, 6);
	fprintf(stderr, "f_write(FILE1) Finished.\n");
	if (memcmp(f_read(FILE1, 2, 6), tmp_data, 6) != 0)
		fprintf(stderr, "%s:%d f_write failed.\n", __FUNCTION__, __LINE__);
	//����д����
	fprintf(stderr, "f_write continue mode test.\n");
	f_write(FILE2, 0, tmp_data, 15);
	f_write(FILE1, 9, tmp_data, 1);
	f_write(FILE2, 10, tmp_data, 10);
	if (memcmp(f_read(FILE2, 10, 10), tmp_data, 10) != 0) 
		fprintf(stderr, "%s:%d more write failed.\n", __FUNCTION__, __LINE__);
	fprintf(stderr, "f_write continue mode test comp.\n");
	//����д����
#endif
}


int main(void) {
	UINT8 i;
	UINT8 tmp[] = "this is a test";

	memset(DISK_MAP, 1, sizeof(DISK_MAP));
	memset(DISK, '0', sizeof(DISK));
	f_init();
	fprintf(stderr, "f_init comp.\n");
	//f_test();
	#if 1
	f_sync();
	fprintf(stderr, "f_sync comp.\n");
	//f_init();
	fprintf(stderr, "f_init comp.\n");
	for (i = 0; i < 10; i++) {
		fprintf(stderr, "1.\n");
		f_write(FILE2, 2, tmp, sizeof(tmp) -1);
		#if 1
		fprintf(stderr, "2.\n");
		f_write(FILE1, 2, (UINT8 *)"ABCDEFGABC", 8);
		fprintf(stderr, "3.\n");
		f_write(FILE3, 35, (UINT8 *)"1234567", 7);
		fprintf(stderr, "4.\n");
		f_write(FILE3, 2, (UINT8 *)"ABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFG", 63);
		fprintf(stderr, "5.\n");
		f_write(FILE3, 14, (UINT8 *)"ZZZ", 3);
		fprintf(stderr, "6.\n");
		f_write(FILE3, 14, (UINT8 *)"DDDDDDDDDDDDDDDDDDD", 19);
		#endif
		fprintf(stderr, "7.\n");
		f_write(FILE2, 1, (UINT8 *)"abcdefg", 7);
		fprintf(stderr, "8.\n");
		f_write(FILE3, 7 * i + i * 2, (UINT8 *)"ABCDEFG", 7);
	}
	
	f_dump();
	#endif
	return 0;
}

