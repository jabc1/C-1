#ifndef __SIMPLE_FTP_CLIENT_H
#define __SIMPLE_FTP_CLIENT_H

#include <stdint.h>

#define FILE_JOB_NAME	"ap_jobs.bin"
#define FILE_JOB_RCVED	"ap_jobs_recved.txt"
#define FILE_KICKOFF		"ap_kickoff.txt"
#define FILE_ACK			"ap_ack.bin"

#define AP_LISTEN_PORT	20000

#define CHECK_ACK_10MS_N	(5000/10)
#define TRY_TIMES	3
#define WAIT_SEC	5

//��ȡftp�ļ���buf��,�ɹ������ļ���С��ʧ�ܷ���-1
int ftp_file_get(const char *file_name, uint8_t *buf, int len);
//�ϴ��ļ���ftp�У��ɹ������ļ���С��ʧ�ܷ���-1
int ftp_file_put(const char *file_name, const uint8_t *buf, int buf_len);
//�����ļ���С���ļ��������򷵻�-1
int ftp_file_size(const char *file_name);
//ɾ��ָ���ļ�
int ftp_file_del(const char *file_name);

#endif
