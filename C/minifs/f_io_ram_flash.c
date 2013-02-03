#include "mini_fs.h"
#include <stdio.h>
#include <string.h>

#ifdef FS_DISK_RAM_FLASH
/*******************************************************
***	�ײ�IO����
***	ʹ��RAMģ��Ӳ�̣���Ҫ���ڲ��ԣ�
***   ʹ��һ���ֽ�ӳ�������ж�д��ǰ���ڵ��ֽ��Ƿ񱻲�����
*******************************************************/

#define DISK_SPACE   SEGMENT_SIZE*DISK_BLOCK

static const BYTE __DISK[DISK_SPACE];
BYTE	__DISK_MAP[DISK_SPACE];
const BYTE *DISK = __DISK;

//��Ҫ��ֲ�ĺ���, ������ȷ����ַ�Ѿ���segment����
bool segment_erase(WORD seg_addr) {
	memset((char *)&DISK[seg_addr], 0, SEGMENT_SIZE);
	memset((char *)&__DISK_MAP[seg_addr], 0, SEGMENT_SIZE);
	//fprintf(stderr, "erase %lu\n", seg_addr);
	return true;
}

//��Ҫ��ֲ�ĺ�����ʵ�ֽ����ݴ�FLASH�������ڴ���
bool segment_read(WORD seg_addr, WORD offset, WORD buf, WORD len) {
	memcpy((char *)buf, (char *)&DISK[seg_addr+offset], (int)len);
	return true;
}

//��Ҫ��ֲ�ĺ����������ݴ��ڴ�д��FLASH�������߱�֤���ڵ�FLASH�Ѿ��������Ҳ��������
bool segment_write(WORD seg_addr, WORD offset,  WORD data, WORD len) {
	int i;
	char *flash_ptr = (char *)&DISK[seg_addr + offset];
	
	for (i = 0; i < len; i++) {
		if (__DISK_MAP[seg_addr + offset + i] == 1) {
			fprintf(stderr, "write %lu + %lu + %lu before erase\n", seg_addr, offset, len);
			return false;
		}
	}

	memcpy(flash_ptr, (char *)data, (int)len);
	memset(&__DISK_MAP[seg_addr + offset], 1, (int)len);
	//fprintf(stderr, "set %lu + %lu + %lu has used\n", seg_addr, offset, len);
	return true;
}

#endif // FS_DISK_ROM_FLASH
