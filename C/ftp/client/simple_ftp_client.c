//ֻʵ�ֻ�����put��get�ļ���buf�Ĺ���
//ÿ�����ܺ�����������һ��ftp�Ĳ�ѯ

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

#define BUFSIZE		256
#define FTP_USER		"ftp"
#define FTP_PASS		"ftp"
#define FTP_SERVER	"192.168.1.200"
#define FTP_PORT		21
#define FTP_CLIENT_ID	1

typedef struct ftp_t {
	int cmd_sd;
	int data_sd;
	char cmd_buf[BUFSIZE];
} ftp_t;

/****************************************************
	socket ����룺���ӣ��Ͽ�������д
****************************************************/

static int sock_write(int sd, const uint8_t *data, int len) {
	return send(sd, data, len, 0);
}

//����ʵ�ʶ������ֽ���������0��ʾ���ӶϿ�������-1��ʾ����
static int sock_read(int sd, uint8_t *buf, int len) {
	struct timeval tim = {2, 0};
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(sd, &rfds);

	if (select(sd +1, &rfds, NULL, NULL, &tim) > 0)
		return recv(sd, buf, len, 0);
	return -1;
}

//ftp->cmd_buf�е��������������ֵ����cmd_buf��, �������ֵ��������
//�򷵻�-1
static int ftp_cmd_tx(ftp_t *ftp) {
	int n = strlen(ftp->cmd_buf);

	fprintf(stderr, "==> %s", ftp->cmd_buf);

	if (sock_write(ftp->cmd_sd, (uint8_t *)ftp->cmd_buf, n) != n)
		return -1;
	memset(ftp->cmd_buf, 0, BUFSIZE);
	if (sock_read(ftp->cmd_sd, (uint8_t *)ftp->cmd_buf, BUFSIZE) <= 0)
		return -1;

	fprintf(stderr, "<== %s", ftp->cmd_buf);
	if (strstr(ftp->cmd_buf, "Failed") != NULL)
		return -1;
	return 0;
}

static int tcp_connect(const char *serv_addr, int port) {
	int sd, ret = -1;
	struct sockaddr_in addr;
	//��������ͨ��
   if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      goto _ret;
   }   
	
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = inet_addr(serv_addr);
   addr.sin_port = htons(port);

	if (connect(sd, (void *)&addr, sizeof(addr)) == -1)
		goto _connect_err;
	
	return sd;

_connect_err:
	close(sd);
_ret:
	return ret;
}

//��������
static int ftp_connect(ftp_t *ftp) {
	int val[6];
	
	memset(ftp->cmd_buf, 0, BUFSIZE);
	//��������ͨ���׽���
	if ((ftp->cmd_sd = tcp_connect(FTP_SERVER, FTP_PORT)) == -1)
		goto _ret;

	if (sock_read(ftp->cmd_sd, (uint8_t *)ftp->cmd_buf, BUFSIZE) <= 0)
		goto _close_cmd_fd;
	fprintf(stderr, "<== %s", ftp->cmd_buf);
	
	//����
	snprintf(ftp->cmd_buf, BUFSIZE, "USER %s\r\n", FTP_USER);
	if (ftp_cmd_tx(ftp) == -1)
		goto _close_cmd_fd;
	snprintf(ftp->cmd_buf, BUFSIZE, "PASS %s\r\n", FTP_PASS);
	if (ftp_cmd_tx(ftp) == -1)
		goto _close_cmd_fd;
	//��ȡ����ͨ���˿�
	snprintf(ftp->cmd_buf, BUFSIZE, "PASV \r\n");
	if (ftp_cmd_tx(ftp) == -1)
		goto _close_cmd_fd;
	//227 Entering Passive Mode (192,168,1,118,37,164).
	sscanf(ftp->cmd_buf, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d).", \
			 &val[0], &val[1], &val[2], &val[3],&val[4], &val[5]);

	//��������ͨ���׽���
	if ((ftp->data_sd = tcp_connect(FTP_SERVER, val[4] * 256 + val[5])) == -1)
		goto _close_cmd_fd;

	return 0;

_close_cmd_fd:
	close(ftp->cmd_sd);
_ret:
	return -1;
}

static int ftp_disconnect(ftp_t *ftp) {
	close(ftp->cmd_sd);
	close(ftp->data_sd);
	return 0;
}


/*********************************************
	ftp ����������: ����ļ�/ ���ļ� /д�ļ�
*********************************************/

//����ļ��Ƿ����
static int ftp_file_exist(ftp_t *ftp, const char *file_name) {
	int ret = -1;
	if (ftp_connect(ftp) != 0)
		return -1;
	snprintf(ftp->cmd_buf, BUFSIZE, "RETR %s\r\n", file_name);
	ret = ftp_cmd_tx(ftp);
	ftp_disconnect(ftp);
	return ret;
}

//��ȡftp�ļ���buf��
static int ftp_file_get(ftp_t *ftp, const char *file_name, uint8_t *buf) {
	int ret = -1, file_size, ret_code;
	if (ftp_connect(ftp) != 0)
		return -1;
	snprintf(ftp->cmd_buf, BUFSIZE, "SIZE %s\r\n", file_name);
	if ((ret = ftp_cmd_tx(ftp)) == -1) 
		goto _ret;
	//����ļ���С
	file_size = sscanf(ftp->cmd_buf, "%d %d", &ret_code, &file_size);

	//���ͻ�ȡ�ļ�����
	snprintf(ftp->cmd_buf, BUFSIZE, "RETR %s\r\n", file_name);
	if ((ret = ftp_cmd_tx(ftp)) == -1)
		goto _ret;

	//ͨ��data socket��ȡ����
	if (sock_read(ftp->data_sd, buf, file_size) < file_size) {
		ret = -1;
		goto _ret;
	}

	ret = 0;
_ret:
	ftp_disconnect(ftp);
	return ret;
}

//�ϴ�buf���ݵ�ftp���ļ���
static int ftp_file_put(ftp_t *ftp, const char *file_name, const uint8_t *buf, int buf_len) {
	int ret = -1;
	if (ftp_connect(ftp) != 0)
		return -1;

	//�����ϴ�����
	snprintf(ftp->cmd_buf, BUFSIZE, "STOR %s\r\n", file_name);
	if ((ret = ftp_cmd_tx(ftp)) == -1) 
		goto _ret;
		
	//ͨ��data socket��������
	if (sock_write(ftp->data_sd, buf, buf_len) < buf_len) {
		ret = -1;
		goto _ret;
	}

	ret = 0;
_ret:
	ftp_disconnect(ftp);
	return ret;
}



int main(void) {
	ftp_t ftp;

	//ftp_file_exist(&ftp, "test.txt");
	ftp_file_put(&ftp, "remote.file", "aaa111222bbb\n", 13);

	fflush(NULL);
	return 0;
}

