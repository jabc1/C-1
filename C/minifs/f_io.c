#include <stdio.h>
#include <string.h>
#include "mini_fs.h"

#define fprintf(...)

/*******************************************************
***	�ײ�IO����
***	��Ҫʵ��Flash�����������ڴ浽Flash�ĸ��ƣ�
***  flash��flash�ĸ��ƹ�3������
*******************************************************/

#define DISK_SPACE   SEGMENT_SIZE*DISK_BLOCK
extern UINT8 DISK[DISK_SPACE];
extern UINT8 DISK_MAP[DISK_SPACE];

//��Ҫ��ֲ�ĺ���, ������ȷ����ַ�Ѿ���segment����
void segment_erase(UINT16 addr) {
	fprintf(stderr, "%s(%d)\n", __FUNCTION__, addr);
	if (addr % SEGMENT_SIZE != 0)
		fprintf(stderr, "%s:segment\n", __FUNCTION__);
	memset(&DISK[addr], '0', SEGMENT_SIZE);
	memset(&DISK_MAP[addr], 1, SEGMENT_SIZE);
}

int erase_byte_count(UINT16 addr, UINT16 len) {
	int i, ret = 0;
	for (i = 0; i < len; i++)
		ret += DISK_MAP[addr + i];
	return ret;
}

//��Ҫ��ֲ�ĺ����������ݴ��ڴ�д��FLASH�������߱�֤���ڵ�FLASH�Ѿ��������Ҳ��������
void segment_copy_mem(UINT16 addr, UINT16 offset,  const UINT8 *data, UINT16 len) {
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
void segment_copy_segment(UINT16 des, UINT16 src, UINT16 len) {
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

