#ifndef __MINI_FS_CONF__
#define __MINI_FS_CONF__

/**< �ļ�����ʹ��ID�ķ�ʽ����ַ��� */
typedef enum {
	FILE1,
	
	F_EPD_CFG = FILE1,	//��һ���ļ�����ǩ������Ϣ
	F_BMP_CURR,				//��ǰ��ʾ��ͼƬ
	F_BMP_BUFF,				//���յ��Ŀ���Ϣ
	F_BMP_DIFF,				//�����õ�����ͼƬ

	FILE_ID_END,
} file_id_t;

/**< ����ÿ���ļ��Ĵ�С��ʹ�ýṹ��ķ�ʽ��������ļ�����ʼ��ַ */
typedef struct FILE_LEN_TABLE {
	BYTE FILE1_SIZE[512];
	BYTE FILE2_SIZE[3096/2 +8];
	BYTE FILE3_SIZE[3096/2 +8];
	BYTE FILE4_SIZE[3096/2 +8];
} FILE_LEN_TABLE;

#define FS_DISK_ROM_FLASH				/**< ʹ��MCU�ڲ���ROM FLASH����Ҫ�����ڶ���ʽ�� */
//#define FS_DISK_SPI_FLASH					/**< ʹ���ⲿ��SPI FLASH */
#define FS_DISK_RAM_FLASH					/**< ʹ���ڴ���Ϊ�ļ�ϵͳ, ��Ҫ���ڲ��� */


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


#define DISK_BLOCK	13						/**< ʹ��10��SEGMENT��Ϊ�ļ�ϵͳ���������2��SEGMENT�ֱ���Ϊ������ͽ����飬����С��3 */
#define SEGMENT_TO_SEGMENT_BUF	32		/**< SPI FLASH��Ҫһ����ʱ�ս���������� */

#endif
