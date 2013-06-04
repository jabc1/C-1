#include "simple_ftp_client.h"

int htp_recv_job(uint8_t *buf, int len) {
	//���JOB�ļ������ڻ���buf�ռ䲻�����˳�
	if (ftp_file_get(FILE_JOB_NAME, buf, len) == -1)
		return -1;
	//�ϴ�״̬�ļ�����֪�Ѿ�����������
	return ftp_file_put(FILE_JOB_RCVED, (uint8_t *)" ", 1);
}

int htp_recv_kickoff(void) {
	int i;
	//�ȴ�KICKOFF֪ͨ, ����WAIT_SEC��
	for (i = 0; i < WAIT_SEC * 100; i++) {
		if (ftp_file_size(FILE_KICKOFF) >= 0)
			break;
	}
	return i < WAIT_SEC * 100 ? 0 : -1;
}

int htp_send_ack(const uint8_t *ack, int len) {
	//�ϴ�ACK�ļ�����֪�Ѿ�����������
	return ftp_file_put(FILE_ACK, ack, len);
}

#include <unistd.h>
#include <stdlib.h>
//�����rf�����������������2��10��
int do_rf_task(uint8_t *buf, int len, uint8_t *ret) {
	int sec = rand() % 8 + 2;
	sleep(sec);
	return 0;
}

int main(void) {
	//�����˿ڣ��������
	//�˿����ӣ���ʼ����
	int len;
	uint8_t buf[1024], ack[256] = {0};

	if ((len = htp_recv_job(buf, sizeof(buf))) > 0 \
		 && htp_recv_kickoff() == 0)
	{
		//do_rf_task,����������buf��, task����ֵ��ret��
		len = do_rf_task(buf, len, ack);
		htp_send_ack(ack, len);
	}
	
	return 0;
}
