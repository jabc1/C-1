#ifndef __RF_DOT_MODULE_T__
#define __RF_DOT_MODULE_T__

#include <stdint.h>

#define RF_CTRL_TYPE	0x2	/**< ctrl�ֶθ�3λΪ2��ʾ����֡ */

typedef enum {
	rf_ret_default = 0,
	rf_ret_ok,
	rf_ret_timeout,
	rf_ret_crcerr,
} rf_ret_t;


/** ��ѯ֡���ؽ��ֵ */
enum {
	//Э��ACKֵ
	RF_ACK_DATA_ERRCRC 	= 1 << 0,	 		/**< RF����CRC����  */
	RF_ACK_DATA_ERRBAT 	= 1 << 1,	 		/**< �ն˵͵�ѹ */
	RF_ACK_DATA_ERROK	= 1 << 6,	 		/**< RF�������� */

	//�����ӵ�ACKֵ
	RF_ACK_DATA_ERR_FILE_CRC	= 1 << 2,	/**< �ļ�CRC���� */
	RF_ACK_DATA_ERR_EPD			= 1 << 3,	/**< EPD��ĻͨѶ���� */
	RF_ACK_DATA_ERR_FILE_ZIP	= 1 << 4,	/**< �ļ�ѹ����ʽ���� */
	RF_ACK_DATA_ERR_FS_OVER		= 1 << 5,	/**< �ն��ļ�ϵͳ��� */
	RF_ACK_DATA_ERR_LOSS_END	= 1 << 7,	/**< δ�յ�����֡ */
} ;

/**
* ���䵥���ն˵�����
* @param[in]	rcv_id �ն˵�ID, ����Ϊ4�ֽ�
* @param[in]	data ��Ҫ���͵����ݵ�ַ
* @param[in]	len ��Ҫ���͵����ݳ���
* @param[out]	ack_buf ����ģ�黺��ÿ���ն˵�ACKֵ,��2λ��ʾ�ش�������
*				�ε�2λ��ʾ��һ�δ��������θ�2λ��ʾ��2�δ���������2λ��ʾ�����δ�����
* @return 		�������һ�����Ĵ��ͽ��
*/
rf_ret_t rf_dot_transfer(const uint8_t *rcv_id, const uint8_t *data, int len, uint8_t *ack_buf);

/**
* ���Ͳ�ѯ֡��ѯ�ն˽������ݵĽ��
* @param[in]	rcv_id �ն˵�ID, ����Ϊ4�ֽ�
* @return		������, ����0��ʾRF�����������������enumֵ
*/
uint8_t rf_dot_transfer_queue(const uint8_t *rcv_id);

/**
* ����Ҫ���õ��ⲿ���������ڷ��ͣ����գ�һ�����ݰ������ӣ�ָ���նˣ���ʱʱ��Ϊtimeout ms������RF����/����״̬
* �Լ���ʱ��������Ҫ���ڷ��ͽ���֡ʱ�ļ����ʱ
*/
extern rf_ret_t RF_TxBuf(const uint8_t *rcv_id, const uint8_t* data, uint8_t len, uint16_t timeout);
extern rf_ret_t RF_RxBuf(const uint8_t *rcv_id, uint8_t* buf, uint8_t len, uint16_t timeout);
extern void RF_Delay100us(uint16_t num);
#endif
