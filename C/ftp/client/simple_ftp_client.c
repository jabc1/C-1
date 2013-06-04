//ֻʵ�ֻ�����put��get�ļ���buf�Ĺ���
//ÿ�����ܺ�����������һ��ftp�Ĳ�ѯ

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

#include "simple_ftp_client.h"

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
	int n;
#if 1
	for(;;) {
		n = send(sd, data, len, 0);
		if (n == len)
			break;
		else if (n < len && errno == EINTR)
			continue;
		else
			break;
	}

#else
	n = send(sd, data, len, 0);
	if (n > 0 && n < len) {
		fprintf(stderr, "send failed.\n");
		fflush(NULL);
		exit(1);
	}
	if (n <= 0) {
		perror("sock_write:");
		fflush(NULL);
		exit(1);
	}
#endif
	return n;
}

//����ʵ�ʶ������ֽ���������0��ʾ���ӶϿ�������-1��ʾ����
static int sock_read_once(int sd, uint8_t *buf, int len) {
	struct timeval tim = {2, 0};
	fd_set rfds;
	int n;

	FD_ZERO(&rfds);
	FD_SET(sd, &rfds);
	
	if (select(sd +1, &rfds, NULL, NULL, &tim) > 0) {
		n = recv(sd, buf, len, 0);
		if (n == -1)
			perror("read_once:");
	}
	return n;
}

//��������ָ�����ȵ����ݣ�����0��ʾ���ӶϿ�������-1��ʾ����
static int sock_read_wait(int sd, uint8_t *buf, int len) {
	int nc = 0, n;
	while (nc <= len) {
		n = recv(sd, buf + nc, len - nc, 0);
		if (n == 0)
			return nc;
		if (n == -1 && errno == EINTR)
			continue;
		else if (n == -1){
			perror("sock_read_wait:");
			break;
		}
		nc += n;
	}
	return nc;
}


//ftp->cmd_buf�е��������������ֵ����cmd_buf��, �������ֵ��������
//�򷵻�-1
static int ftp_cmd_tx(ftp_t *ftp) {
	int n = strlen(ftp->cmd_buf);
	int ret_code = 500;

	fprintf(stderr, "==> %s", ftp->cmd_buf);

	if (sock_write(ftp->cmd_sd, (uint8_t *)ftp->cmd_buf, n) != n)
		return -1;
	memset(ftp->cmd_buf, 0, BUFSIZE);
	if (sock_read_once(ftp->cmd_sd, (uint8_t *)ftp->cmd_buf, BUFSIZE) <= 0)
		return -1;

	fprintf(stderr, "<== %s", ftp->cmd_buf);
	sscanf(ftp->cmd_buf, "%d ", &ret_code);
	//if (strstr(ftp->cmd_buf, "Failed") != NULL)
	if (ret_code >= 400)	//400���϶��Ǵ��󷵻�ֵ
		return -1;
	return 0;
}

static int tcp_connect(const char *serv_addr, int port) {
	int sd, ret = -1;
	struct sockaddr_in addr;
	struct linger so_linger;
	//��������ͨ��
   if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      goto _ret;
   }   

	so_linger.l_onoff = 1;
	so_linger.l_linger = 300;
	if (setsockopt(sd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof(so_linger)) == -1)
		goto _close_sd;
	
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = inet_addr(serv_addr);
   addr.sin_port = htons(port);

	if (connect(sd, (void *)&addr, sizeof(addr)) == -1)
		goto _close_sd;
	
	return sd;

_close_sd:
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

	if (sock_read_once(ftp->cmd_sd, (uint8_t *)ftp->cmd_buf, BUFSIZE) <= 0)
		goto _close_cmd_fd;
	fprintf(stderr, "<== %s", ftp->cmd_buf);
	
	//����
	snprintf(ftp->cmd_buf, BUFSIZE, "USER %s\r\n", FTP_USER);
	if (ftp_cmd_tx(ftp) == -1)
		goto _close_cmd_fd;
	snprintf(ftp->cmd_buf, BUFSIZE, "PASS %s\r\n", FTP_PASS);
	if (ftp_cmd_tx(ftp) == -1)
		goto _close_cmd_fd;
	//���ö����Ƹ�ʽ
	snprintf(ftp->cmd_buf, BUFSIZE, "TYPE I\r\n");
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
	close(ftp->data_sd);
	snprintf(ftp->cmd_buf, BUFSIZE, "QUIT \r\n");
	ftp_cmd_tx(ftp);
	close(ftp->cmd_sd);
	return 0;
}


/*********************************************
	ftp ����������: ����ļ�/ ���ļ� /д�ļ�
*********************************************/

//����ļ���С���ļ��������򷵻�-1
int ftp_file_size(const char *file_name) {
	int ret = -1, ret_code = 0, file_size = 0;
	ftp_t ftp;

	if (ftp_connect(&ftp) != 0)
		return -1;
	snprintf(ftp.cmd_buf, BUFSIZE, "SIZE %s\r\n", file_name);
	if ((ret = ftp_cmd_tx(&ftp)) == -1) 
		goto _ret;
	//����ļ���С
	sscanf(ftp.cmd_buf, "%d %d", &ret_code, &file_size);
	ret = file_size;
_ret:
	ftp_disconnect(&ftp);
	return ret;
}

//��ȡftp�ļ���buf��,�����ļ���С
int ftp_file_get(const char *file_name, uint8_t *buf, int len) {
	int ret = -1, file_size = 0;
	ftp_t ftp;
	//�ļ������ڻ���buf�ռ䲻����������ʧ��
	if ((file_size = ftp_file_size(file_name)) == -1 \
		|| len < file_size || ftp_connect(&ftp) != 0)
		return -1;

	//���ͻ�ȡ�ļ�����
	snprintf(ftp.cmd_buf, BUFSIZE, "RETR %s\r\n", file_name);
	if ((ret = ftp_cmd_tx(&ftp)) == -1)
		goto _ret;

	//ͨ��data socket��ȡ����
	if (sock_read_wait(ftp.data_sd, buf, file_size) < file_size) 
		goto _ret;

	ret = file_size;
_ret:
	ftp_disconnect(&ftp);
	return ret;
}

//�ϴ�buf���ݵ�ftp���ļ���
static int _ftp_file_put(const char *file_name, const uint8_t *buf, int buf_len) {
	int ret = -1;
	ftp_t ftp;

	if (ftp_connect(&ftp) != 0)
		return -1;

	//�����ϴ�����
	snprintf(ftp.cmd_buf, BUFSIZE, "STOR %s\r\n", file_name);
	if ((ret = ftp_cmd_tx(&ftp)) == -1) 
		goto _ret;
		
	//ͨ��data socket��������
	if (sock_write(ftp.data_sd, buf, buf_len) < buf_len)
		goto _ret;

	ret = 0;
_ret:
	ftp_disconnect(&ftp);
	return ret;
}

//�ϴ��ļ������ļ���С������������ش�����
int ftp_file_put(const char *file_name, const uint8_t *buf, int buf_len) {
	int i, rty_times = 3, ret;

	for (i = 0; i < rty_times; i++) {
		if (_ftp_file_put(file_name, buf, buf_len) == 0 \
			 && ftp_file_size(file_name) == buf_len)
		{
			ret = 0;
			break;	 
		} else {
			ret = -1;
			printf("ftp_file_put failed.\n");
		}
	}

	return ret;
}

int ftp_file_del(const char *file_name) {
	int ret = -1;
	ftp_t ftp;

	if (ftp_connect(&ftp) != 0)
		return -1;
	//����ɾ������
	snprintf(ftp.cmd_buf, BUFSIZE, "DELE %s\r\n", file_name);
	if ((ret = ftp_cmd_tx(&ftp)) == -1) 
		goto _ret;

	ret = 0;
_ret:
	ftp_disconnect(&ftp);
	return ret;
}

#if 0
static void rand_file(uint8_t *buf, int len) {
	int i;
	for (i = 0; i < len; i++)
		buf[i] = rand() % 256;
}

#define SEND_SIZE	10240
bool test(const char *TEST_FILE) {
	uint8_t buf[SEND_SIZE], ran[SEND_SIZE];

	rand_file(ran, SEND_SIZE);
	memset(buf, 0, sizeof(buf));

	if (ftp_file_put(TEST_FILE, ran, sizeof(ran)) == -1) {
		fprintf(stderr, "ftp_file_put failed\n");
		return false;
	}
	if (ftp_file_size(TEST_FILE) != sizeof(ran)) {
		fprintf(stderr, "ftp_file_exist failed\n");
		return false;
	}
	if (ftp_file_get(TEST_FILE, buf, sizeof(ran)) == -1) {
		fprintf(stderr, "ftp_file_get failed\n");
		return false;
	}
	if (memcmp(buf, ran, sizeof(ran)) != 0) {
		fprintf(stderr, "memcmp failed\n");
		return false;
	}
	if (ftp_file_del(TEST_FILE) == -1){
		fprintf(stderr, "ftp_file_del failed\n");
		return false;
	}
	return true;
}

int main(int arg, char **argv) {
	int i, n = 0;
	bool ok = true;
	srand(getpid());
	while (ok) {
		for (i = 0; i < 100 && ok; i++)
			ok = test(argv[1]);
		if (!ok)
			printf("test %d:\t%s\n", n++, "FAILED");
	}
	return 0;
}
#endif
