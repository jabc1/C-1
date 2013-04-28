/**
Copyright (C) 2012, ������˷�Ƽ����޹�˾
All Rights Reserved World Wide.
Project��		ESL
Description:	FLASH�ļ�ϵͳ
Date			Author			Description
2013-01-31	������			beta�汾
2013-02-20  ������			����directд����
*/

#ifndef __MINI_FS__
#define __MINI_FS__

/** @defgroup mini_fs
*  �ļ�ϵͳ����˵��
*  @{
*/

/**< �����������Ͷ���ͷ�ļ� */
typedef unsigned char BYTE;

/**< �ļ�ϵͳ������Ϣ����ƽ̨�Զ��� */
#include <stdbool.h>
#include "mini_fs_conf.h"

#ifdef FS_DISK_RAM_FLASH
typedef unsigned long WORD;
#else
typedef unsigned int WORD;
#endif

/**<  �����ַ�������ַת�� */
extern WORD DISK;

/**< �ļ�ϵͳ�ṹ�� */
typedef struct fs_t {
	BYTE valid;
	BYTE flag;
	struct file_info_t {
		WORD start_addr;
		WORD file_len;
		WORD file_size;		
	} file[FILE_ID_END];
#ifdef ENABLE_BLOCK_MGMT
	BYTE block_status[FS_USE_SEGMENT_MAX];	/**< ÿ��������״̬ */
	WORD block_map[FS_USE_SEGMENT_MAX];		/**< �ļ�ϵͳʹ�õ��������� */
	WORD block_wc[FS_USE_SEGMENT_MAX];		/**< ��¼ÿ���鱻��д�Ĵ��� */
	#undef MAX_BLOCK_
#endif
} fs_t;
extern fs_t fs;

/**< ����4���겻��Ҫ�Ķ� */
#define FS_BLOCK     ((sizeof(FILE_LEN_TABLE) + SEGMENT_SIZE -1) / SEGMENT_SIZE) 
#define SUPER_BLOCK  ((sizeof(fs) + SEGMENT_SIZE -1) / SEGMENT_SIZE)       
#define SWAP_BLOCK   1  
#define _MAX_(a, b)	((a) > (b) ? (a) : (b))
#define DISK_BLOCK   _MAX_((FS_BLOCK + SUPER_BLOCK + SWAP_BLOCK), FS_USE_SEGMENT_MAX)

/**< ϵͳ����ʱ�����ļ�ϵͳ */
void 	f_init(void);

/**< ϵͳ�ػ�ʱ����Ҫʱ�����ļ�ϵͳ */
void 	f_sync(void);
#ifdef FS_DISK_ROM_FLASH
const BYTE* f_rom_read(file_id_t id, WORD offset);	/**< ���ٶ�������ֱ�ӷ���FLASH��ַ��ֻ����ROM FLASHʹ�� */
#endif

/**
* ��׼������ ��������ȷ��buf�ռ��㹻����len���ֽ�
* @param[in] id �ļ�ID   
* @param[in] offset ���ļ���ƫ�Ƶ�ַoffset��ʼ��   
* @param[out] buf �����ݶ��뵽buf��
* @param[in] len ������ֽ���
* @return ���سɹ�������ֽ���
*/
WORD	f_read(file_id_t id, WORD offset,	BYTE *buf, WORD len);

/**
* ��׼д����
* @param[in] id �ļ�ID   
* @param[in] offset ���ļ���ƫ�Ƶ�ַoffset��ʼд   
* @param[in] data ��д�������
* @param[in] len ��Ҫд����ֽ���
* @return ����д��ɹ����ֽ���
*/
WORD 	f_write(file_id_t id, WORD offset,	const BYTE *data, WORD len);

/**
* ֱ��д����
* ֱ�����ļ���ָ��ƫ����д��ָ�����ȵ����ݡ���������Ҫȷ�������򱻲�������
* ����������Ѿ������ݣ����Һ�Ҫд����������ݲ�һ�£�����ô˺����󣬴��������������δ֪
* @param[in] id �ļ�ID   
* @param[in] offset ���ļ���ƫ�Ƶ�ַoffset��ʼд   
* @param[in] data ��д�������
* @param[in] len ��Ҫд����ֽ���
* @return ����д��ɹ����ֽ���
*/
WORD 	f_write_direct(file_id_t id, WORD offset,	const BYTE *data, WORD len);

/**
* �ļ���������
* @param[in] dst ��Ҫд����ļ�ID   
* @param[in] dst_offset ���ļ���ƫ�Ƶ�ַdst_offset��ʼд   
* @param[in] src ���������ļ�ID
* @param[in] src_offset ���ļ���ƫ�Ƶ�ַsrc_offset��ʼ�� 
* @param[in] len ��Ҫ�������ֽ���
* @return ���سɹ��������ֽ���
*/
WORD f_copy(file_id_t dst, WORD dst_offset, file_id_t src, WORD src_offset, WORD len);

/** �����ļ����� */
WORD	f_len(file_id_t id);

/** �����ļ���С */
WORD	f_size(file_id_t id);

/** �����ļ���ַ */
WORD	f_addr(file_id_t id);

/** ����ļ� */
void	f_erase(file_id_t id);

/** ����һ����С�Ŀ飬������֤��ַaddr�� SEGMENT_SIZE ���� */
extern bool segment_erase(WORD addr);

/**
* IO�������
* @param[in] addr ��Ҫ���ĵ�ַ
* @param[in] buf д����ڴ���������Ӧ��Ϊָ�룬������ʱ��ҪǿתΪWORD����
* @param[in] len ��Ҫ��ȡ���ֽ���, �����߱�֤���ᳬ��buf�ռ�
* @return ��
*/
extern bool segment_read(WORD addr, WORD buf, WORD len);

/**
* IO��д����
* @param[in] addr ��Ҫд�ĵ�ַ
* @param[in] buf Ϊ����Դ������Ӧ��Ϊָ�룬������ʱ��ҪǿתΪWORD����
* @param[in] len ��Ҫд����ֽ����������߱�֤�����MAX_WRITE_UNITд��
* @return ��
*/
extern bool segment_write(WORD addr, WORD buf, WORD len);

/**@}*/ // mini_fs

#endif
