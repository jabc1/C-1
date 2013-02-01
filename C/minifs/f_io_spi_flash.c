#include "mini_fs.h"
#include <msp430f149.h>
#include <stdio.h>
#include <string.h>
#include "flash.h"

FLASHID testid[2]={{0x00112233}};

/*******************************************************
***	�ײ�IO����
***	��Ҫʵ��Flash�����������ڴ浽Flash�ĸ��ƣ�
***  flash��flash�ĸ��ƹ�3������
*******************************************************/

#ifdef FS_DISK_SPI_FLASH

//��Ҫ��ֲ�ĺ���, ������ȷ����ַ�Ѿ���segment����
void segment_erase(WORD seg_addr) {
	FLASH_Erase(testid, FLASH_EREASE_SECTOR, seg_addr);
}

//��Ҫ��ֲ�ĺ�����ʵ�ֽ����ݴ�FLASH�������ڴ���
void segment_read(WORD seg_addr, WORD offset, WORD buf, WORD len) {
	UINT32 addr = FS_DISK_ADDR + seg_addr + offset;
	BYTE *p = (BYTE *)buf;
	FLASH_Read(testid, FLASH_FASTREAD, addr, p, len);
}

//��Ҫ��ֲ�ĺ����������ݴ��ڴ�д��FLASH��
//�����߱�֤���ڵ�FLASH�Ѿ��������Ҳ������������д�뵥Ԫ
void segment_write(WORD seg_addr, WORD offset,  WORD data, WORD len) {
	UINT32 addr = FS_DISK_ADDR + seg_addr + offset;
	BYTE *p = (BYTE *)data;
	FLASH_PageProgram(testid, addr, p, len);
}

#endif // FS_DISK_SPI_FLASH
