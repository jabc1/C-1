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

static BYTE __DISK[DISK_SPACE];
WORD DISK = (WORD) &__DISK[0];
BYTE	__DISK_MAP[DISK_SPACE];

//��Ҫ��ֲ�ĺ���, ������ȷ����ַ�Ѿ���segment����
bool segment_erase(WORD seg_addr) {
	//memset((char *)&DISK[seg_addr], 0, SEGMENT_SIZE);
	memset((char *)seg_addr, 0, SEGMENT_SIZE);
	memset((char *)&__DISK_MAP[seg_addr - (WORD)DISK], 0, SEGMENT_SIZE);
	//fprintf(stderr, "erase %lu\n", seg_addr);

	return true;
}

//��Ҫ��ֲ�ĺ�����ʵ�ֽ����ݴ�FLASH�������ڴ���
bool segment_read(WORD addr, WORD buf, WORD len) {
	//fprintf(stderr, "%s(%lu, %lu, data, %lu)\n", __FUNCTION__, seg_addr, offset, len);
	memcpy((char *)buf, (char *)(addr), (int)len);
	return true;
}

//��Ҫ��ֲ�ĺ����������ݴ��ڴ�д��FLASH�������߱�֤���ڵ�FLASH�Ѿ��������Ҳ��������
bool segment_write(WORD addr,  WORD data, WORD len) {
	int i;
	char *flash_ptr = (char *)(addr);
	//fprintf(stderr, "%s(%lu, data, %lu)\n", __FUNCTION__, addr, len);

	for (i = 0; i < len; i++) {
		if (__DISK_MAP[(addr+ i) - (WORD)DISK] == 1) {
			//fprintf(stderr, "write %lu + %lu before erase\n", addr, len);
			return false;
		}
	}

	memcpy(flash_ptr, (char *)data, (int)len);
	memset(&__DISK_MAP[addr - (WORD)DISK], 1, (int)len);
	//fprintf(stderr, "set %lu + %lu has used\n", addr, len);
	return true;
}

#endif // FS_DISK_ROM_FLASH
