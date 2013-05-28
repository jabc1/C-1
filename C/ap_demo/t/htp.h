#ifndef _HTP_H_
#define _HTP_H_

#define IP_ADDRESS_LEN	16
#define VERSION_LEN		8

#define HTP_VERSION 16
#define HTP_VERSION_S "16"

typedef struct htp_socket_t
{
	SOCKET socket; // ����ͨ���׽���
	char	ip_addr[IP_ADDRESS_LEN]; // IP��ַ
	UINT16 port; // �˿ں�
	UINT8 *buf; // �շ����ݵ��׵�ַ
	UINT32 len; // �շ����ݵĳ���
} htp_socket_t;

typedef struct htp_header_t
{
	UINT16 version;
	UINT16 version_s;
	char vsrt[VERSION_LEN];
	UINT16 opcode;
	UINT16 opcode_s;
	UINT16 para;
	UINT16 para_s;
	UINT32 len;
	UINT32 len_s;
	UINT16 reserved;
	UINT16 reserved_s;
} htp_header_t;

/*
 * ���ܣ�����һ��ָ��������IP��socket���ӣ�TCP/IP��
 * ������ip_addr --> ָ����������IP��ַ
 *		 port    --> ָ���������Ķ˿�
 *		 socket  --> �ɹ����ӵ�socket���
 * ���أ�0  --> �ɹ�
 *		 -1 --> socket����ʧ��
 *		 -x --> connet����ʧ��,����x=errno
 */
INT32 htp_open(htp_socket_t *htp_socket);

/*
 * ���ܣ�����ָ�����ȵ�����
 * ������socket --> ��Ҫ���͵�socket���
 *		 dst    --> �������ݵ��׵�ַ
 *		 len    --> �������ݵĳ���
 *		 timeout --> ��ʱʱ��,��λ ms
 * ���أ�>0 --> �ɹ����յ����ݸ���
 *		 0  --> socket���ӶϿ�
 *		 -1 --> �������ݴ���
 */
INT32 read_socket(SOCKET socket, void* dst, INT32 len, UINT32 timeout);

/*
 * ���ܣ�����ָ�����ȵ�����
 * ������socket --> ��Ҫ���͵�socket���
 *		 src    --> �������ݵ��׵�ַ
 *		 len    --> �������ݵĳ���
 * ���أ�>0 --> �ɹ����͵����ݸ���
 *		 0  --> socket���ӶϿ�
 *		 -1 --> �������ݴ���
 */
INT32 write_socket(SOCKET socket, const void* src, INT32 len);

/*
 * ���ܣ���ָ����socket��������(�ȷ�htp_header���ٷ�htp_socket->buf)
 * ������htp_socket->socket --> ��Ҫ���͵�socket���
 *		 htp_socket->buf    --> �������ݵ��׵�ַ
 *		 htp_socket->len    --> �������ݵĳ���
 *		 htp_header			--> htp��������ͷ
 * ���أ�TRUE  --> ���ͳɹ�
 *		 FALSE --> ����ʧ��
 */
bool htp_send(htp_socket_t *htp_socket, htp_header_t *htp_header);

/*
 * ���ܣ�����ָ����socket����(�����ճɹ���htp����ͷ����htp_header�����������htp_socket->buf��)
 * ������htp_socket->socket --> ָ�����յ�socket���
 *		 htp_socket->buf    --> �������ݵ��׵�ַ,�ڲ�����ռ�
 *		 htp_socket->len    --> �������ݵĳ���
 *		 htp_header			--> htp��������ͷ
 * ���أ�TRUE  --> ���ճɹ�(�������ͷ�)
 *		 FALSE --> ����ʧ��(�ڲ��ͷ�)
 */
bool htp_recv(htp_socket_t *htp_socket, htp_header_t *htp_header);

/*
 * ���ܣ��ر�ָ��������IP��socket���ӣ�TCP/IP��
 * ������socket  --> ��Ҫ�رյ�socket���
 * ���أ�TRUE  --> �رճɹ�
 *		 FALSE --> �ر�ʧ��
 */
bool htp_close(htp_socket_t *htp_socket);

/*
 * ���ܣ���htp��ͷ
 * ������opcode --> htp����������
 *		 para	--> htp�������ֵĲ���,Ĭ��Ϊ0
 *		 len	--> htp��������ĳ���,Ĭ��Ϊ0
 * ���أ�����ͷ���׵�ַ	--> �ɹ�(�ṹ��ռ�õĴ洢�ռ��ڲ����룬�������ͷ�)
 *		 NULL			--> ʧ��(�ڲ��ͷ�)
 */
htp_header_t *htp_ass_header(UINT16 opcode, UINT16 para, UINT32 len);

#endif

