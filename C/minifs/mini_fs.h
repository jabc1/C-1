
#ifndef __MINI_FS__
#define __MINI_FS__

/** @defgroup mini_fs
*  �ļ�ϵͳ����˵��
*  @{
*/

/**< �����������Ͷ���ͷ�ļ� */
typedef unsigned char BYTE;
typedef unsigned long WORD;
#include <stdbool.h>
//#include "datatype.h"

//#define FS_DISK_ROM_FLASH					/**< ʹ��MCU�ڲ���ROM FLASH����Ҫ�����ڶ���ʽ�� ���ڲ�FLASH��ʱֱ�ӷ���FLASH��ַ*/
//#define FS_DISK_SPI_FLASH				/**< ʹ���ⲿ��SPI FLASH */
#define FS_DISK_RAM_FLASH					/**< ʹ���ڴ���Ϊ�ļ�ϵͳ, ��Ҫ���ڲ��� */
//#define FS_USE_MEM_SWAP	
//#define ENABLE_BLOCK_MGMT					/**< ���û������Ԫ */

#ifdef FS_DISK_ROM_FLASH					/**< �ڲ�FLASH�Ķ�д���� */
#define SEGMENT_SIZE		512				/**< FLASH����С������Ԫ��С */
#define MAX_WRITE_UNIT	32					/**< FLASH���������д�뵥Ԫ */
#endif

#ifdef FS_DISK_SPI_FLASH					/**< �ⲿFLASH�Ķ�д���� */

#define SEGMENT_SIZE		4096				/**< FLASH����С������Ԫ��С */
#define MAX_WRITE_UNIT	256				/**< FLASH���������д�뵥Ԫ */
#define FS_DISK_ADDR	0x00					/**< �ļ�ϵͳ��FLASH�е���ʼλ�� */
#endif

#ifdef FS_DISK_RAM_FLASH					/**< �ڲ�FLASH�Ķ�д���� */
#define SEGMENT_SIZE		64				/**< FLASH����С������Ԫ��С */
#define MAX_WRITE_UNIT	32					/**< FLASH���������д�뵥Ԫ */
#endif

/**< ʹ���ڴ���Ϊ������ݽ����ռ�, ������FLASH�ڿ���һ��������segment��Ϊ�����ռ䡣
*    RAM�ռ乻�Ļ��Ƽ����������, �⽫��������ļ�ϵͳ�ٶȣ�����Ҫ���ļ�ϵͳָ��һ��SEGMENT_SIZE�ֽڵ�BUF�� 
*/
					
#ifndef FS_USE_MEM_SWAP
#define SEGMENT_TO_SEGMENT_BUF	32		/**< �����ʹ���ڴ���Ϊ�齻����������һ��32�ֽڵĺ�������ʱ���� */
#else
extern BYTE __FS_SWAP_SPACE[SEGMENT_SIZE];	/**< ��Ҫ�û��ڳ�����ָ������������Ĵ洢λ�� */
#endif

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

/**< ʹ���㹻��SEGMENT��Ϊ�ļ�ϵͳ���������2��SEGMENT�ֱ���Ϊ������ͽ����飬����С��3 */
#define DISK_BLOCK   ((sizeof(FILE_LEN_TABLE) + SEGMENT_SIZE -1) / SEGMENT_SIZE + 2)

/**< ϵͳ����ʱ�����ļ�ϵͳ */
void 	f_init(void);

/**< ϵͳ�ػ�ʱ����Ҫʱ�����ļ�ϵͳ */
void 	f_sync(void);
#ifdef FS_DISK_ROM_FLASH
const BYTE* f_rom_read(file_id_t id, WORD offset, WORD len);	/**< ���ٶ�������ֱ�ӷ���FLASH��ַ��ֻ����ROM FLASHʹ�� */
#endif

/**
* ��׼������ ��������ȷ��buf�ռ��㹻����len���ֽ�
* @param[in] id �ļ�ID   
* @param[in] offset ���ļ���ƫ�Ƶ�ַoffset��ʼ��   
* @param[out] buf �����ݶ��뵽buf��
* @param[in] len ������ֽ���
* @return ������ɹ�������buf��ַ�����ʧ�ܷ���NULL
*/
BYTE*	f_read(file_id_t id, WORD offset,	BYTE *buf, WORD len);

/**
* ��׼д����
* @param[in] id �ļ�ID   
* @param[in] offset ���ļ���ƫ�Ƶ�ַoffset��ʼд   
* @param[in] data ��д�������
* @param[in] len ��Ҫд����ֽ���
* @return ����д��ɹ����ֽ���
*/
WORD 	f_write(file_id_t id, WORD offset,	const BYTE *data, WORD len);

/**< �����ļ����� */
WORD	f_len(file_id_t id);

/**< �����ļ���С */
WORD	f_size(file_id_t id);

/**< ����ļ� */
void	f_erase(file_id_t id);

/**< ����һ����С�Ŀ飬������֤��ַaddr�� SEGMENT_SIZE ���� */
extern bool segment_erase(WORD addr);

/**
* IO�������
* @param[in] seg_addr ��Ҫ���Ŀ���ʼ��ַ
* @param[in] seg_off ����ƫ����
* @param[in] buf д����ڴ���������Ӧ��Ϊָ�룬������ʱ��ҪǿתΪWORD����
* @param[in] len ��Ҫ��ȡ���ֽ���, �����߱�֤���ᳬ��buf�ռ�
* @return ��
*/
extern bool segment_read(WORD seg_addr, WORD seg_off, WORD buf, WORD len);

/**
* IO��д����
* @param[in] seg_addr ��Ҫд�Ŀ���ʼ��ַ
* @param[in] seg_off ����ƫ����
* @param[in] buf Ϊ����Դ������Ӧ��Ϊָ�룬������ʱ��ҪǿתΪWORD����
* @param[in] len ��Ҫд����ֽ����������߱�֤�����MAX_WRITE_UNITд��
* @return ����д�ɹ�����ʧ��
*/
extern bool segment_write(WORD seg_addr, WORD seg_off,  WORD buf, WORD len);

/**@}*/ // mini_fs

#endif
