/*
* Copyright (C) 2013, ������˷�Ƽ����޹�˾
* All Rights Reserved World Wide.
* Project��		ESL
* Description:	HTP SOCKET �㺯��
*				��Ҫʵ����socket��Ĳ���������������socket��ĸ����쳣����

* Date			Author			Description
* 20130529		mmyu			����
*
* BUGS��
*	1���ṹ��htp_header_t�ĳ�Ա������ַ���룬�Ժ�������޸ģ���ȷ������Ա����ַ���롣
*
*   2��ͨ��htp_new()���ص���һ��ָ�����ȿ��õ��ڴ�ռ䡣���Ժ�������汾�У����صĿռ���ܻ����������ͣ�
*      �������ʹ�ö��ƻ���ԭ�нṹ���ܻᵼ��htp_free()ʧ�ܡ�
*      ���統htp_new()�������½ṹʱ:
*			struct mem{
*				int version;
*				int len;
*				void* buf;
*				int buf_crc;
*			};
*		����˽ṹ����֪��ĺ���ʹ�ö���д��bufָ���ֵ��������htp_free()��ʵ��Ϊ
*			void htp_free(UINT8 *pMem) {
*				struct mem *p = pMem;
*				free(p->buf);
*				free(p);
*			}
*		��ʱ������free�쳣��
*
*   3������htp_send()���͵�buf����len��UINT32���ͣ�����htp_send()���õײ�write_socket()�����ǣ�len��INT32���͡�
*	   ��htp_send()��buf����len�����λΪ1ʱ���˺����Ľ��δ���塣
*
* INFO��
*	1��htp_send()�����Ĳ�����û��ʹ��const���η���Ҳ��Ĭ����ʵ���߿����޸�dst�����ݡ�htp_close()Ҳһ����
*
*/

#ifndef _HTP_H_
#define _HTP_H_

#define IP_ADDRESS_LEN	16
#define VERSION_LEN		8

#define HTP_VERSION		16		//Э��汾��
#define HTP_VERSION_S	"16"	//Э��汾�ַ���ֵ

#define READ_TIMEOUT_MS	2000	//socket read�ĳ�ʱʱ�䣬��λΪ����

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
 * ���ܣ���̬����ռ�
 * ������len --> ����ռ�Ĵ�С
 * ���أ�����ռ���׵�ַ
 */
UINT8 *htp_new(UINT32 len);

/*
 * ���ܣ��ͷŶ�̬����Ŀռ�
 * ������pMem --> ��̬����ռ���׵�ַ
 * ���أ���
 */
void htp_free(UINT8 *pMem); // ע��htp_new��htp_free��ɶ�ʹ��

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
INT32 read_socket(SOCKET socket, void* dst, INT32 len, UINT32 timeout); // �ڲ�����

/*
 * ���ܣ�����ָ�����ȵ�����
 * ������socket --> ��Ҫ���͵�socket���
 *		 src    --> �������ݵ��׵�ַ
 *		 len    --> �������ݵĳ���
 * ���أ�>0 --> �ɹ����͵����ݸ���
 *		 0  --> socket���ӶϿ�
 *		 -1 --> �������ݴ���
 */
INT32 write_socket(SOCKET socket, const void* src, INT32 len); // �ڲ�����

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
 * ������htp_header --> ���İ�ͷ���׵�ַ(�����߷���ռ�)
 *		 opcode --> htp����������
 *		 para	--> htp�������ֵĲ���,Ĭ��Ϊ0
 *		 len	--> htp��������ĳ���,Ĭ��Ϊ0
 * ���أ�TRUE  --> �رճɹ�
 *		 FALSE --> �ر�ʧ��
 */
bool htp_ass_header(htp_header_t *htp_header, UINT16 opcode, UINT16 para = 0, UINT32 len = 0);


#endif

