#ifndef __DATA_TYPE_H
#define __DATA_TYPE_H

#include <stdint.h>

/** opcode ��� */
enum {
	HS_OPCODE_MIN,
	HS_OPCODE_WRITEDATA,
	HS_OPCODE_KICKOFF,
	HS_OPCODE_CANCAL,
	HS_OPCODE_PING,
	HS_OPCODE_ACK = 0x0100,
	HS_OPCODE_NACK = 0x0200,

	HS_OPCODE_MAX,
};

//pc->bs ctrl
//�����ǩ������
#define CTRL_NORMAL_UPDATA		0x10
#define CTRL_NORMAL_NETLINK	0x11
#define CTRL_NORMAL_QUERY		0x12

#define CTRL_DOT20_UPDATA		0x20
#define CTRL_DOT24_UPDATA		0x21
#define CTRL_DOT29_UPDATA		0x22
#define CTRL_DOT43_UPDATA		0x23

#define CTRL_DOT20_NETLINK		0x30
#define CTRL_DOT20_QUERY		0x31
#define CTRL_DOT24_NETLINK		0x32
#define CTRL_DOT24_QUERY		0x33
#define CTRL_DOT29_NETLINK		0x34
#define CTRL_DOT29_QUERY		0x35
#define CTRL_DOT43_NETLINK		0x36
#define CTRL_DOT43_QUERY		0x37


#pragma pack(1)
/** �������ݰ���ʽ  */
typedef struct HS_PKT_T {

	/** ���ݰ�ͷ  */
	struct HS_PKT_HEADER_T {
		uint16_t version;
		uint16_t version_s;
		uint8_t version_str[8];

		uint16_t opcode;
		uint16_t opcode_s;

		uint16_t para;
		uint16_t para_s;
		
		uint32_t len;
		uint32_t len_s;

		uint16_t reserved0;
		uint16_t reserved1;

	} header;

	/** ���ݰ�����  */
	union {
		struct HS_PKT_OP_WRITEDATA_T {
			uint8_t ctrl;
			uint8_t para;
			uint8_t powermode;
			uint32_t wakeup_id;
			uint8_t rf_ch;
			uint16_t data_esl_num;
			uint16_t sleep_esl_num;

			//���������ն�����
			struct data_area_t {
				uint32_t id;
				uint32_t len;
				uint8_t data[0];
			} data[0];
			struct sleep_area_t {
				uint32_t id;
			} sleep[0];
		} write;

		struct HS_PKT_OP_KICKOFF {
			uint8_t d;
		} kickoff;

		struct HS_PKT_OP_CANCL {
			uint8_t d;
		} cancel;

		struct HS_PKT_OP_PING {
			uint8_t d;
		} ping;

		struct HS_PKT_OP_ACK {
			uint8_t d;
		} ack;

		struct HS_PKT_OP_NACK {
			uint8_t d;
		} nack;
	} buf;
	

} HS_PKT_T;

#define PRODUCT_NAME_MAX_LEN	128
#define PORDUCT_ORIGIN_MAX_LEN	24
struct dot_info_t {
   int type;    //����?2.0?2.9��4.3?
   uint32_t dot_id;   //���ID

   int product_id;   //��ID
   float price;   //��
   char name[PRODUCT_NAME_MAX_LEN];  //��?
   char origin[PORDUCT_ORIGIN_MAX_LEN];   //��   
   //������
};

#define AP_NAME_MAX_LEN	256
#define PKT_MAX_SIZE	4*1024*1024	//4M

struct AP_TASK_T {
	char ap_name[AP_NAME_MAX_LEN];
	int data_len;	//ack_flag����1��ʾ�յ���վ��ACK��ACKֵд�뵽data.ack��
	union {
		HS_PKT_T tosend;
		HS_PKT_T ack;
		uint8_t _buf[PKT_MAX_SIZE];
	} data;
};

#pragma pack()

int fill_header_data(struct HS_PKT_HEADER_T *header, int op_code, int para, int data_len);
int fill_write_data(struct HS_PKT_OP_WRITEDATA_T *buf, int buf_len, \
							const struct dot_info_t * data_ids, int data_n, \
                    	const uint32_t *sleep_ids, int sleep_n);
void write_dump(const HS_PKT_T *pkg, FILE *fp);
void esl_data_dump(const struct dot_info_t *info, FILE *fp, int len);
int lcd_display(const struct dot_info_t *info, uint8_t *out_buf, int out_len);
int assign_ap_task(struct AP_TASK_T *task, int task_n);

#endif