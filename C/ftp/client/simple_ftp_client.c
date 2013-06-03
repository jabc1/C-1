//ֻʵ�ֻ�����put��get�ļ���buf�Ĺ���
//ÿ�����ܺ�����������һ��ftp�Ĳ�ѯ

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define BUFSIZE		256
#define FTP_USER		"yu"
#define FTP_PASS		"yu"
#define FTP_SERVER	127.0.0.1
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
	return recv(sd, buf, len, 0);
}

//��������
static int ftp_connect(ftp_t *ftp) {
	
	return 0;
}

static int ftp_disconnect(ftp_t *ftp) {
	close(ftp->cmd_sd);
	close(ftp->data_sd);
	return 0;
}

//ftp->cmd_buf�е��������������ֵ����cmd_buf��, �������ֵ��������
//�򷵻�-1
static int ftp_cmd_rx(ftp_t *ftp) {
	int n = strlen(ftp->cmd_buf);

	fprintf(stderr, "==> %s", ftp->cmd_buf);

	if (sock_write(ftp->cmd_sd, (uint8_t *)ftp->cmd_buf, n) != n)
		return -1;
	if (sock_read(ftp->cmd_sd, (uint8_t *)ftp->cmd_buf, BUFSIZE) <= 0)
		return -1;

	fprintf(stderr, "<== %s", ftp->cmd_buf);
	if (strstr(ftp->cmd_buf, "Failed") != NULL)
		return -1;
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
	ret = ftp_cmd_rx(ftp);
	ftp_disconnect(ftp);
	return ret;
}

//��ȡftp�ļ���buf��
static int ftp_file_get(ftp_t *ftp, const char *file_name, uint8_t *buf) {
	int ret = -1, file_size, ret_code;
	if (ftp_connect(ftp) != 0)
		return -1;
	snprintf(ftp->cmd_buf, BUFSIZE, "SIZE %s\r\n", file_name);
	if ((ret = ftp_cmd_rx(ftp)) == -1) 
		goto _ret;
	//����ļ���С
	file_size = sscanf(ftp->cmd_buf, "%d %d", &ret_code, &file_size);

	//���ͻ�ȡ�ļ�����
	snprintf(ftp->cmd_buf, BUFSIZE, "RETR %s\r\n", file_name);
	if ((ret = ftp_cmd_rx(ftp)) == -1)
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
	if ((ret = ftp_cmd_rx(ftp)) == -1) 
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

	ftp_file_exist(&ftp, "test.txt");


	return 0;
}

