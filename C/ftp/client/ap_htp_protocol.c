
int htp_recv_job(uint8_t *buf, int len) {
	ftp_t ftp;
	int i, nr;
	
	//���JOB�ļ������ڻ���buf�ռ䲻�����˳�
	if ((nr = ftp_file_get(&ftp, FILE_JOB_NAME, buf, len)) == -1)
		return -1;
	//�ϴ�״̬�ļ�����֪�Ѿ�����������, ����ϴ�3��
	for (i = 0; i < TRY_TIMES; i++)
		if (ftp_file_put(&ftp, FILE_JOB_RCVED, " ", 1) == 0)
			break;
	return i < TRY_TIMES ? nr : -1;
}

int htp_recv_kickoff(void) {
	ftp_t ftp;
	int i;

	//�ȴ�KICKOFF֪ͨ, ����WAIT_SEC��
	for (i = 0; i < WAIT_SEC * 100; i++) {
		if (ftp_file_size(&ftp, FILE_KICKOFF) >= 0)
			break;
		time_sleep_ms(10);
	}
	return i < WAIT_SEC * 100 ? 0 : -1;
}

int htp_send_ack(const uint8_t *ack, int len) {
	ftp_t ftp;
	int i;

	//�ϴ�ACK�ļ�����֪�Ѿ�����������, ����ϴ�3��
	for (i = 0; i < TRY_TIMES; i++)
		if (ftp_file_put(&ftp, FILE_ACK, ack, len) == 0)
			break;
	return i < TRY_TIMES ? 0 : -1;
}

int main_exam(void) {
	//�����˿ڣ��������
	//�˿����ӣ���ʼ����
	int len;
	uint8_t buf[1024], ret[256] = {0};

	if ((len = htp_recv_job(buf, sizeof(buf))) > 0 \ 
		 && htp_recv_kickoff() == 0)
	{
		//do_rf_task,����������buf��, task����ֵ��ret��
		//len = do_rf_task(buf, len, ret);
		;
	}
	
	htp_send_ack(ack, len);
}
