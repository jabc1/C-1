#ifndef __DATA_TYPE_H__
#define __DATA_TYPE_H__

//��վ���ն˵��ӳ����ܲ�һ���������Ҫ�����������ǵ���������
typedef uint8_t UINT8;
typedef uint16_t  UINT16;
typedef uint32_t UINT32;

#define FLAG_MASK(x)			((x) & 0x0F)
#define FLAG_SYNC_REQ		FLAG_MASK(0)
#define FLAG_SYNC_ACK		FLAG_MASK(1)
#define FLAG_BCM_REQ			FLAG_MASK(2)
#define FLAG_DATA_REQ		FLAG_MASK(3)

#pragma pack(1)
typedef struct {
	UINT8		flag;
	UINT8		slot;
	UINT16	interval;
	UINT32	start_id;
	UINT8		bitmap[56];
} WK_T;			//�㲥��

typedef union {
	struct {
		UINT32 	id;
		UINT8		flag;
	} req;		//ͬ������
	struct {
		UINT8		flag;
		UINT32	interval;
		UINT32	next_interval;
		UINT8		down_chn;
		UINT8		up_chn;
	} ack;		//ͬ��ACK
} SY_T;			//ͬ�������

typedef struct {
	UINT32	id;
	UINT8		flag;
} PU_T;			//PULL�����

typedef union {
	struct {
		UINT8		flag;
		UINT8 	d[63];
	} data;		//����
	struct {
		UINT32	id;
		UINT8		flag;
	} ack;		//����ACK
} DT_T;			//���ݰ�


typedef union {
	WK_T	wakeup;
	SY_T	sync;
	PU_T	pull;
	DT_T	data;
} PKT_T;
#pragma pack()

typedef enum {
	AP_INIT,        	//AP��ʼ��
   AP_SEND_BEACOM,   //AP���͹㲥��
   AP_RCVD_SYNC,     //AP�յ�ͬ��������
   AP_SEND_SYNC_ACK, //AP�ظ�ͬ��ACK
   AP_RCVD_PULL,     //AP�յ�PULL����
   AP_SEND_DATA,     //AP��������
   AP_RECV_DATA_ACK, //AP�յ�����ACK
   AP_SLEEP,         //AP��������

   AP_TASK_END,
} AP_TASK_T;         //AP��������

typedef enum {
   EP_INIT,          //�ն˳�ʼ��
   EP_RECV_BEACOM,   //�ն˽��չ㲥��
   EP_SEND_SYNC,     //�ն˷���ͬ������
   EP_RECV_SYNC_ACK, //�ն��ܵ�ͬ��ACK
   EP_SEND_PULL,     //�ն˷���PULL������
   EP_RECV_DATA,     //�ն��յ�����
   EP_SEND_DATA_ACK, //�ն˻ظ�����ACK
   EP_SLEEP,         //�ն˽�������

   EP_TASK_END,
} EndPoint_TASK_T;   //�ն˹�������

#endif