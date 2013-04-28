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

//��Ҫ��ֲ�ĺ���, ������ȷ����ַ�Ѿ���segment����
bool segment_erase(WORD seg_addr) {
	memset((char *)seg_addr, 0xff, SEGMENT_SIZE);
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
	WORD i;
	//fprintf(stderr, "%s(%lu, data, %lu)\n", __FUNCTION__, addr, len);
	for (i = 0; i < len; i++)
		((BYTE *)addr)[i] &= ((BYTE *)data)[i];
	return true;
}

#endif // FS_DISK_ROM_FLASH
