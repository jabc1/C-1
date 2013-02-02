#include "mini_fs.h"
#include <msp430f149.h>
#include <string.h>

#ifdef FS_DISK_ROM_FLASH
/*******************************************************
***	�ײ�IO����
***	��Ҫʵ��Flash�����������ڴ浽Flash�ĸ��ƣ�
***  flash��flash�ĸ��ƹ�3������
*******************************************************/

#define DISK_SPACE   SEGMENT_SIZE*DISK_BLOCK

static const BYTE __DISK[DISK_SPACE] @ "MINI_FS";
const BYTE *DISK = __DISK;
const BYTE *f_disk_addr(void) {
	return DISK;
}

//��Ҫ��ֲ�ĺ���, ������ȷ����ַ�Ѿ���segment����
bool segment_erase(WORD seg_addr) {
	_DINT();
	char *flash_ptr = (char *)&DISK[seg_addr];
	
	while (FCTL3 & BUSY)		// busy = 1 ��ʾ��û������	
		;	
	FCTL1 = FWKEY + ERASE;                    // Set Erase bit
	//FCTL2 = FWKEY + FSSEL_1 + FN4 + FN2;      // MCLK/20 for Flash Timing Generator
	FCTL2 = FWKEY + FSSEL_1 + 9;
	FCTL3 = FWKEY;                            // Clear Lock bit
	*flash_ptr = 0;                           // Dummy write to erase Flash segment
	while (FCTL3 & BUSY)								// busy = 1 ��ʾ��û������	
		;
	FCTL1 = FWKEY;                            // Clear WRT bit
	FCTL3 = FWKEY + LOCK;                     // Set LOCK bit	
		
	_EINT();
}

//��Ҫ��ֲ�ĺ�����ʵ�ֽ����ݴ�FLASH�������ڴ���
bool segment_read(WORD seg_addr, WORD offset, WORD buf, WORD len) {
	memcpy((BYTE *)buf, &DISK[seg_addr+offset], len);
}

//��Ҫ��ֲ�ĺ����������ݴ��ڴ�д��FLASH�������߱�֤���ڵ�FLASH�Ѿ��������Ҳ��������
#if 1
bool segment_write(WORD seg_addr, WORD offset,  WORD data, WORD len) {
	int i;
	
	_DINT();
	char *flash_ptr = (char *)&DISK[seg_addr + offset];
	char *flash_dst = (char *)data;
	
	while (FCTL3 & BUSY)		// busy = 1 ��ʾ��û������	
		;	
	FCTL1 = FWKEY + WRT;                    // Set WRT bit for write operation
	//FCTL2 = FWKEY + FSSEL_1 + FN4 + FN2;   		 // MCLK/20 for Flash Timing Generator	
	FCTL2 = FWKEY + FSSEL_1 + 9;   		 // MCLK/20 for Flash Timing Generator
	FCTL3 = FWKEY; 
	
	for (i = 0; i < len; i++) {
		flash_ptr[i] = flash_dst[i];                   // Write value to flash
		while (!(FCTL3 & WAIT))		// WAIT = 0 ��ʾ��ûд��
			;
	}
	while (FCTL3 & BUSY)		// busy = 1 ��ʾ��û������	
		;
	FCTL1 = FWKEY;                            // Clear WRT bit
	FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
	_EINT();		
}
#else
__ramfunc void segment_write(WORD seg_addr, WORD offset, WORD data, WORD len) {
	int i;
	
	_DINT();
	char *flash_ptr = (char *)&DISK[seg_addr + offset];
	char *value = (char *)data;
	
	while (FCTL3 & BUSY)		// busy = 1 ��ʾ��û������	
		;	
	//FCTL2 = FWKEY + FSSEL_1 + FN4 + FN2;   		 // MCLK/20 for Flash Timing Generator	
	FCTL2 = FWKEY + FSSEL_1 + 9;
	FCTL3 = FWKEY; 
	FCTL1 = FWKEY + BLKWRT + WRT;                    // Set BLOCK, WRT bit for write operation
	
	for (i = 0; i < len; i++) {
		flash_ptr[i] = value[i];                   // Write value to flash
		while (!(FCTL3 & WAIT))		// WAIT = 0 ��ʾ��ûд��
			;
	}
	FCTL1 = FWKEY;           // Clear WRT bit
	while (FCTL3 & BUSY)		// busy = 1 ��ʾ��û������	
		;
	FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
	_EINT();		
}

#endif

#if 0
//��Ҫ��ֲ�ĺ����������ݴ�FLASH������FLASH��
//�����߱�֤Ŀ������FLASH�Ѿ��������Ҳ������������д�뵥Ԫ
void segment_copy_segment(WORD seg_des,WORD dst_off, WORD seg_src, WORD len) {
#if 1
	segment_write(seg_des + dst_off, 0, (WORD)&DISK[seg_src], len);
#else
	int i;
	
	_DINT();
	char *flash_ptr = (char *)&DISK[seg_des + dst_off];
	char *flash_dst = (char *)&DISK[seg_src];
	
	while (FCTL3 & BUSY)		// busy = 1 ��ʾ��û������	
		;	
	FCTL1 = FWKEY + WRT;                    // Set WRT bit for write operation
	//FCTL2 = FWKEY + FSSEL_1 + FN4 + FN2;   		 // MCLK/20 for Flash Timing Generator	
	FCTL2 = FWKEY + FSSEL_1 + 9;   		 // MCLK/20 for Flash Timing Generator
	FCTL3 = FWKEY; 
	
	for (i = 0; i < len; i++) {
		flash_ptr[i] = flash_dst[i];                   // Write value to flash
		while (!(FCTL3 & WAIT))		// WAIT = 0 ��ʾ��ûд��
			;
	}
	while (FCTL3 & BUSY)		// busy = 1 ��ʾ��û������	
		;
	FCTL1 = FWKEY;                            // Clear WRT bit
	FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
	_EINT();		
#endif
}
#endif

#endif // FS_DISK_ROM_FLASH