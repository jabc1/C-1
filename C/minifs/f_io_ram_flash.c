#include "mini_fs.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef FS_DISK_RAM_FLASH
/*******************************************************
***	�ײ�IO����
***	ʹ��RAMģ��Ӳ�̣���Ҫ���ڲ��ԣ�
***   ʹ��һ���ֽ�ӳ�������ж�д��ǰ���ڵ��ֽ��Ƿ񱻲�����
*******************************************************/
#define SIZEOF(s,m) ((size_t) sizeof(((s *)0)->m)) 

#define DISK_SPACE   SEGMENT_SIZE*DISK_BLOCK

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


static const BYTE __DISK[DISK_SPACE];
const BYTE *DISK = &__DISK[0];
BYTE	__DISK_MAP[DISK_SPACE];
const BYTE *f_disk_addr(void) {
	return __DISK;
}

//��Ҫ��ֲ�ĺ���, ������ȷ����ַ�Ѿ���segment����
void segment_erase(WORD seg_addr) {
	memset((char *)&DISK[seg_addr], 0, SEGMENT_SIZE);
	memset((char *)&__DISK_MAP[seg_addr], 0, SEGMENT_SIZE);
	fprintf(stderr, "erase %lu\n", seg_addr);
}

//��Ҫ��ֲ�ĺ�����ʵ�ֽ����ݴ�FLASH�������ڴ���
void segment_read(WORD seg_addr, WORD offset, WORD buf, WORD len) {
	//fprintf(stderr, "%s(%lu, %lu, data, %lu)\n", __FUNCTION__, seg_addr, offset, len);
	memcpy((char *)buf, (char *)&DISK[seg_addr+offset], (int)len);
}

//��Ҫ��ֲ�ĺ����������ݴ��ڴ�д��FLASH�������߱�֤���ڵ�FLASH�Ѿ��������Ҳ��������
void segment_write(WORD seg_addr, WORD offset,  WORD data, WORD len) {
	int i;
	char *flash_ptr = (char *)&DISK[seg_addr + offset];
	//fprintf(stderr, "%s(%lu, %lu, data, %lu)\n", __FUNCTION__, seg_addr, offset, len);

	for (i = 0; i < len; i++) {
		if (__DISK_MAP[seg_addr + offset + i] == 1) {
			fprintf(stderr, "write %lu + %lu + %lu before erase\n", seg_addr, offset, len);
		}
	}

	memcpy(flash_ptr, (char *)data, (int)len);
	memset(&__DISK_MAP[seg_addr + offset], 1, (int)len);
	//fprintf(stderr, "set %lu + %lu + %lu has used\n", seg_addr, offset, len);
}

#endif // FS_DISK_ROM_FLASH
