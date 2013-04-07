#ifndef __MINI_FS_CONF__
#define __MINI_FS_CONF__

/**< �ļ�����ʹ��ID�ķ�ʽ����ַ��� */
typedef enum {
	FILE1,
	
	F_EPD_CFG = FILE1,	//��һ���ļ�����ǩ������Ϣ
	F_BMP_CURR,				//��ǰ��ʾ��ͼƬ
	F_BMP_BUFF,				//���յ��Ŀ���Ϣ
	F_BMP_DIFF,				//�����õ�����ͼƬ
   F_BMP_MEGR,				//�洢�鲢����ļ�
	
	FILE_ID_END,
} file_id_t;

/**< ����ÿ���ļ��Ĵ�С��ʹ�ýṹ��ķ�ʽ��������ļ�����ʼ��ַ */

#include "epd.h"

typedef struct FILE_LEN_TABLE {
	BYTE FILE1_SIZE[512];
	//�ļ��ܴ�С=ͼƬ�ļ���С+�ļ�ͷ+һ���հ׵��������ݰ�
	BYTE FILE2_SIZE[PIC_SIZE + 13 + 64];	
	BYTE FILE3_SIZE[PIC_SIZE + 13 + 64];
	BYTE FILE4_SIZE[PIC_SIZE + 13 + 64];
	BYTE FILE5_SIZE[PIC_SIZE + 13 + 64];
} FILE_LEN_TABLE;

//#define FS_DISK_ROM_FLASH					/**< ʹ��MCU�ڲ���ROM FLASH����Ҫ�����ڶ���ʽ�� */
#define FS_DISK_SPI_FLASH				/**< ʹ���ⲿ��SPI FLASH */
//#define FS_DISK_RAM_FLASH				/**< ʹ���ڴ���Ϊ�ļ�ϵͳ, ��Ҫ���ڲ��� */

//#define F_COPY_USE_EXT_MEM      			/**< f_copyʹ���ⲿ���� */
#define F_COPY_CACHE_SIZE	32

#ifdef  F_COPY_USE_EXT_MEM
extern BYTE *F_COPY_CACHE;
#endif

#ifdef FS_DISK_ROM_FLASH					/**< �ڲ�FLASH�Ķ�д���� */
#define SEGMENT_SIZE		512				/**< FLASH����С������Ԫ��С */
#define MAX_WRITE_UNIT	32					/**< FLASH�����д�뵥Ԫ */
#endif

#ifdef FS_DISK_SPI_FLASH					/**< �ⲿFLASH�Ķ�д���� */
#define SEGMENT_SIZE		4096				/**< FLASH����С������Ԫ��С */
#define MAX_WRITE_UNIT	256				/**< FLASH�����д�뵥Ԫ */
#endif

#ifdef FS_DISK_RAM_FLASH					/**< �ڲ�FLASH�Ķ�д���� */
#define SEGMENT_SIZE		64				/**< FLASH����С������Ԫ��С */
#define MAX_WRITE_UNIT	32					/**< FLASH���������д�뵥Ԫ */
#endif

#endif
