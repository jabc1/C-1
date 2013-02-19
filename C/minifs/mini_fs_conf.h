#ifndef __MINI_FS_CONF__
#define __MINI_FS_CONF__

/**< �ļ�����ʹ��ID�ķ�ʽ����ַ��� */
typedef enum {
   FILE1,
   FILE2,
   FILE3,
   FILE4,
   FILE5,
   FILE6,
   FILE7,
   FILE8,
   FILE9,

   FILE_ID_END,
} file_id_t;

/**< ����ÿ���ļ��Ĵ�С��ʹ�ýṹ��ķ�ʽ��������ļ�����ʼ��ַ */
typedef struct FILE_LEN_TABLE {
   BYTE FILE1_SIZE[10];
   BYTE FILE2_SIZE[23];
   BYTE FILE3_SIZE[150];
   BYTE FILE4_SIZE[9];
   BYTE FILE5_SIZE[43];
   BYTE FILE6_SIZE[120];
   BYTE FILE7_SIZE[10];
   BYTE FILE8_SIZE[300];
   BYTE FILE9_SIZE[600];
} FILE_LEN_TABLE;


//#define FS_DISK_ROM_FLASH				/**< ʹ��MCU�ڲ���ROM FLASH����Ҫ�����ڶ���ʽ�� */
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

#define SEGMENT_TO_SEGMENT_BUF	32		/**< SPI FLASH��Ҫһ����ʱ�ս���������� */

#endif
