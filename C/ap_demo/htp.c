// htp.cpp: ����Ŀ�ļ���

//#include "stdafx.h"

//for windows header
//#include "windows_header.h"

//for linux header
#include "linux_header.h"

#include "htp.h"

INT32 htp_open(htp_socket_t *htp_socket) {
	SOCKET sd;
	struct sockaddr_in svr;
	int ret;
	int i;

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		ret = errno;
		goto _ret;
	}

	memset(&svr, 0, sizeof(svr));
	svr.sin_family = AF_INET;
	svr.sin_addr.s_addr = inet_addr(htp_socket->ip_addr);
	svr.sin_port = htons(htp_socket->port);

	for (i = 0; i < 100; i++) {
		if (connect(sd, (void *)&svr, sizeof(svr)) == -1) {
			ret = errno;
			if (ret != EINTR)
				goto _close;
		} else
			break;	//connect ok
	}

	htp_socket->socket = sd;
	htp_socket->buf = NULL;
	htp_socket->len = 0;

	return 0;

_close:
	close(sd);
_ret:
	return ret;
}

static int safe_read(SOCKET sd, void *buf, int len) {
	int n, i, err;

	for (i = 0; i < 3; i++) {
		n = recv(sd, (char *)buf, len, 0);
		if (n >= 0)
			return n;
		//readʧ��
		if (n == -1){
			err = errno;
			if (err == EINTR) {//���ж�
				continue;
			} else if (err == ECONNREFUSED ) //���ӶϿ�
				return 0;
		} else
			return -1;	//recvʧ��, errno������������
	}

	return -1;
}

INT32 read_socket(SOCKET socket, void* dst, INT32 len, UINT32 timeout) {
	struct timeval tim = {0, timeout * 1000};
	fd_set rfds;
	int n = 0, n1, ret;

	//double start_tim;
	//long els_tim; 

	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(socket, &rfds);

		//start_tim = GetTickCount();
		ret = select(socket+1, &rfds, NULL, NULL, &tim);

		//��ȥ���ŵ�ʱ��
		//els_tim = (long)(GetTickCount() - start_tim);
		//tim.tv_usec -= els_tim;

		if (ret > 0) {	// read ready
			n1 = safe_read(socket, (char *)dst + n, len - n);
			if (n1 <= 0) //safe_read ʧ�� 0 �Ͽ���-1��������
				return n;
			n += n1;
			if (n == len)	//�ɹ�������ָ����������
				return n;
			//������
		} else if (ret == -1 && errno == EINTR) { //select interrupt
			//ֻ���ڱ��жϴ�ϵ�����²Ż�һֱѭ��
			continue;
		} else if (ret == 0 && n > 0) {	//ʱ��ľ����յ�������
			return n;
		} else
			break; //select got other error
	}

	return -1;
}

INT32 write_socket(SOCKET socket, const void* src, INT32 len) {
   INT32 i, n, err;
	int rty_times = 3;

	for (i = 0; i < rty_times; i++) {
		n = send(socket, (const char *)src, len, 0);
		if (n == len) //ok or disconnect
			return n;
		err = errno;
		if (n < len && err == EINTR) //interrupt
			continue;
		else
			break;	//other err
	}
   
   return -1;
}

bool htp_send(htp_socket_t *htp_socket, htp_header_t *htp_header) {
	htp_socket_t *s = htp_socket;
	htp_header_t *h = htp_header;

	if (write_socket(s->socket, h, sizeof(htp_header_t)) <= 0)
		return false;
	if (s->len != 0 && write_socket(s->socket, s->buf, s->len) <= 0)
		return false;
	
	static int n = 0;
	n++;
	char buf[10];

	sprintf(buf, "file%d", n);
	FILE *fp = fopen(buf, "wb");
	fwrite(h, 1, sizeof(htp_header_t), fp);
	fwrite(s->buf, 1, s->len, fp);
	fclose(fp);

	return true;
}

static int htp_header_check(const htp_header_t *h) {
	
	if (h->version != HTP_VERSION \
		|| strncmp(h->vsrt, HTP_VERSION_S, strlen(HTP_VERSION_S) != 0))
		return -1;

	if (   ((~(h->version)) & 0xFFFF) != h->version_s \
		|| ((~(h->opcode)) & 0xFFFF) != h->opcode_s \
		|| ((~(h->para))   & 0xFFFF) != h->para_s \
		|| ((~(h->len))    & 0xFFFFFFFF) != h->len_s) {
			return -1;
		}
	
	return 0;
}

UINT8 *htp_new(UINT32 len) {
	return (UINT8 *)malloc(len);
}

void htp_free(UINT8 *pMem) {
	free(pMem);
}

bool htp_recv(htp_socket_t *htp_socket, htp_header_t *htp_header) {
	htp_socket_t *s = htp_socket;
	htp_header_t *h = htp_header;
	UINT8 *data_buf = NULL;
	
	//����ͷʧ��
	if ( read_socket(s->socket, h,\
		sizeof(htp_header_t), READ_TIMEOUT_MS) != sizeof(htp_header_t)) {
		goto _clean_sock_buf;
	}

	//Э��ͷ����
	if (htp_header_check(h) != 0) {
		goto _clean_sock_buf;
	}

	if (h->len == 0) {
		s->buf = NULL;
		s->len = 0;
		return true;	//����û��������
	}

	//�����ڴ�
	data_buf = htp_new(h->len);
	if (data_buf == NULL)
		goto _clean_sock_buf;
	s->buf = data_buf;
	s->len = h->len;

	//������������
	//BUGS: s->lenΪUINT32, ������Ҫ����INT32
	if (read_socket(s->socket, s->buf, s->len, READ_TIMEOUT_MS) != s->len) {
		goto _clean_data_buf;
	}

	return true;

_clean_data_buf:
	htp_free(data_buf);

_clean_sock_buf:
	//����ʣ��Ĵ�������
	while (read_socket(s->socket, htp_header, sizeof(htp_header_t), 1) > 0)
		;
	return false;
}


bool htp_close(htp_socket_t *htp_socket) {
	return close(htp_socket->socket) == 0 ? true : false;
}

bool htp_ass_header(htp_header_t *htp_header, UINT16 opcode, UINT16 para , UINT32 len ) {

   htp_header_t *h = htp_header;

   h->version = HTP_VERSION;
   h->version_s = ~h->version;
   strncpy((char *)h->vsrt, HTP_VERSION_S, sizeof(h->vsrt));
   
   h->opcode = opcode;
   h->opcode_s = ~h->opcode;

   h->para = para;
   h->para_s = ~h->para;

   h->len = len;
   h->len_s = ~h->len;

   return true;
}




