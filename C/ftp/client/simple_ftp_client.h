#ifndef __SIMPLE_FTP_CLIENT_H
#define __SIMPLE_FTP_CLIENT_H

#include <stdint.h>

#define FILE_JOB_NAME	"ap_jobs.bin"
#define FILE_JOB_RCVED	"ap_jobs_recved.txt"
#define FILE_KICKOFF		"ap_kickoff.txt"
#define FILE_ACK			"ap_ack.bin"

#define TRY_TIMES	3
#define WAIT_SEC	5

//��ȡftp�ļ���buf��,�����ļ���С
int ftp_file_get(const char *file_name, uint8_t *buf, int len);
//�ϴ��ļ������ļ���С������������ش�����
int ftp_file_put(const char *file_name, const uint8_t *buf, int buf_len);
//����ļ���С���ļ��������򷵻�-1
int ftp_file_size(const char *file_name);
//ɾ���ļ���С
int ftp_file_del(const char *file_name);
#endif
