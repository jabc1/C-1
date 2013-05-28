
//for linux header
#include "linux_header.h"

#include "htp.h"

/*
 * ���ܣ�����һ��ָ��������IP��socket���ӣ�TCP/IP��
 * ������ip_addr --> ָ����������IP��ַ
 *		 port    --> ָ���������Ķ˿�
 *		 socket  --> �ɹ����ӵ�socket���
 * ���أ�0  --> �ɹ�
 *		 -1 --> socket����ʧ��
 *		 -x --> connet����ʧ��,����x=errno
 */
INT32 htp_open(htp_socket_t *htp_socket) {
	SOCKET sd;
	struct sockaddr_in svr;
	int ret;

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		ret = errno;
		goto _ret;
	}

	bzero(&svr, sizeof(svr));
	svr.sin_family = AF_INET;
	svr.sin_addr.s_addr = inet_addr(htp_socket->ip_addr);
	svr.sin_port = htons(htp_socket->port);

	if (connect(sd, (void *)&svr, sizeof(svr)) == -1) {
		ret = errno;
		goto _close;
	}

	htp_socket->socket = sd;

	return 0;

_close:
	close(sd);
_ret:
	return ret;
}


static int safe_read(SOCKET sd, void *buf, int len) {
	int n, i, err;

	for (i = 0; i < 3; i++) {
		n = recv(sd, buf, len, 0);
		if (n >= 0)
			return n;
		//readʧ��
		if (n == -1){
			err = errno;
			if (err == EINTR) {//���ж�
				fprintf(stderr, "recv recv sig.\n");
				continue;
			} else if (err == ECONNREFUSED) //���ӶϿ�
				return 0;
		} else
			return -1;	//recvʧ��, errno������������
	}

	return -1;
}

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
INT32 read_socket(SOCKET socket, void* dst, INT32 len, UINT32 timeout) {
	struct timeval tim = {0, timeout * 1000};
	fd_set rfds;
	int n = 0, n1, ret;

	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(socket, &rfds);

		ret = select(socket+1, &rfds, NULL, NULL, &tim);
		//fprintf(stderr, "read_socket: i = %d, ret = %d\n", i, ret);
		if (ret > 0) {	// read ready
			n1 = safe_read(socket, (char *)dst + n, len - n);
			//fprintf(stderr, "safe_read:%d\n", n1);
			if (n1 <= 0) //safe_read ʧ�� 0 �Ͽ���-1��������
				return n;
			n += n1;
			if (n == len)	//�ɹ�������ָ����������
				return n;
			//������
		} else if (ret == -1 && errno == EINTR) { //select interrupt
			fprintf(stderr, "select recv sig.\n");
			continue;
		} else if (ret == 0 && n > 0) {	//ʱ��ľ����յ�������
			return n;
		} else
			break; //select got other error
	}

	return -1;
}

/*
 * ���ܣ�����ָ�����ȵ�����
 * ������socket --> ��Ҫ���͵�socket���
 *		 src    --> �������ݵ��׵�ַ
 *		 len    --> �������ݵĳ���
 * ���أ�>0 --> �ɹ����͵����ݸ���
 *		 0  --> socket���ӶϿ�
 *		 -1 --> �������ݴ���
 */
INT32 write_socket(SOCKET socket, const void* src, INT32 len) {
   INT32 i, n, err;
	int rty_times = 3;

	for (i = 0; i < rty_times; i++) {
		n = send(socket, src, len, 0);
		if (n == len) //ok
			return n;
		err = errno;
		if (n < len && err == EINTR) //interrupt
			continue;
		else if (err == ECONNRESET) //dis connect
			return 0; 
		else
			break;	//other err
	}
   
   return -1;
}

/*
 * ���ܣ���ָ����socket��������(�ȷ�htp_header���ٷ�htp_socket->buf)
 * ������htp_socket->socket --> ��Ҫ���͵�socket���
 *		 htp_socket->buf    --> �������ݵ��׵�ַ
 *		 htp_socket->len    --> �������ݵĳ���
 *		 htp_header			--> htp��������ͷ
 * ���أ�TRUE  --> ���ͳɹ�
 *		 FALSE --> ����ʧ��
 */
bool htp_send(htp_socket_t *htp_socket, htp_header_t *htp_header) {
	htp_socket_t *s = htp_socket;
	htp_header_t *h = htp_header;

	if (write_socket(s->socket, h, sizeof(htp_header_t)) <= 0)
		return false;
	if (write_socket(s->socket, s->buf, s->len) <= 0)
		return false;
	
	return true;
}

static int htp_header_check(const htp_header_t *h) {
	
	if (h->version != HTP_VERSION \
		|| strncmp(h->vsrt, HTP_VERSION_S, strlen(HTP_VERSION_S) != 0))
		return -1;

	if (  (~(h->version)) != h->version_s \
		|| (~(h->opcode))  != h->opcode_s \
		|| (~(h->para))    != h->para_s \
		|| (~(h->len))     != h->len_s) {
			return -1;
		}
	
	return 0;
}

/*
 * ���ܣ�����ָ����socket����(�����ճɹ���htp����ͷ����htp_header�����������htp_socket->buf��)
 * ������htp_socket->socket --> ָ�����յ�socket���
 *		 htp_socket->buf    --> �������ݵ��׵�ַ,�ڲ�����ռ�
 *		 htp_socket->len    --> �������ݵĳ���
 *		 htp_header			--> htp��������ͷ
 * ���أ�TRUE  --> ���ճɹ�(�������ͷ�)
 *		 FALSE --> ����ʧ��(�ڲ��ͷ�)
 */
bool htp_recv(htp_socket_t *htp_socket, htp_header_t *htp_header) {
	htp_socket_t *s = htp_socket;
	htp_header_t *h = htp_header;
	UINT8 *data_buf;
	
	//����ͷʧ��
	if (read_socket(s->socket, h,\
		sizeof(htp_header_t), 100) != sizeof(htp_header_t))
		goto _clean_sock_buf;

	//Э��ͷ����
	if (htp_header_check(h) != 0) {
		goto _clean_sock_buf;
	}

	switch (h->opcode) {
		case HTP_OPCODE_WRITEDATA:
			data_buf = malloc(h->len);
			if (data_buf == NULL)
				goto _clean_sock_buf;
			s->buf = data_buf;
			s->len = h->len;
			//������������
			//BUGS: s->lenΪUINT32, ������Ҫ����INT32
			if (read_socket(s->socket, s->buf, s->len, 100) != s->len) {
				goto _clean_data_buf;
			}
			break;
		case HTP_OPCODE_KICKOFF:
		case HTP_OPCODE_CANCAL:
		case HTP_OPCODE_PING:
		case HTP_OPCODE_ACK:
		case HTP_OPCODE_NAK:
			//ֻ��ͷ��Ϣ
			break;
		default:
			goto _clean_sock_buf;
			break;
	}

	return true;

_clean_data_buf:
	free(data_buf);

_clean_sock_buf:
	return false;
}

/*
 * ���ܣ��ر�ָ��������IP��socket���ӣ�TCP/IP��
 * ������socket  --> ��Ҫ�رյ�socket���
 * ���أ�TRUE  --> �رճɹ�
 *		 FALSE --> �ر�ʧ��
 */
bool htp_close(htp_socket_t *htp_socket) {
	return close(htp_socket->socket) == 0 ? true : false;
}

/*
 * ���ܣ���htp��ͷ
 * ������opcode --> htp����������
 *		 para	--> htp�������ֵĲ���,Ĭ��Ϊ0
 *		 len	--> htp��������ĳ���,Ĭ��Ϊ0
 * ���أ�����ͷ���׵�ַ	--> �ɹ�(�ṹ��ռ�õĴ洢�ռ��ڲ����룬�������ͷ�)
 *		 NULL			--> ʧ��(�ڲ��ͷ�)
 */
htp_header_t *htp_ass_header(UINT16 opcode, UINT16 para, UINT32 len) {
	return NULL;
}

