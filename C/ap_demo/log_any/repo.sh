#!/bin/bash

#output file
log_file=temp_file
esl_list=esl_list_file
esl_data=esl_data_file

echo -e '\nData Packet'
#�ҵ�����Ҫ�������log��Ŀ $$$:x:x:3:7:*, ���ų�case13
grep -a -E '^[^:]{3}:[^:]{1,}:[^:]{1,}:3:7:*' $1 | grep -v -E '^[^:]{3}:[^:]{1,}:13:3:7:*' > $log_file
#�����ն�ID�嵥
cut -d':' -f7 $log_file | sort | uniq > $esl_list

echo -e 'ID\t\t\tFRAME\tPASS\t\tPACKET\tACK[0:DEFAULT, 1:TIMOUT, 2:CRCERR, 3:OK]'
for id in `cat $esl_list`
do
	echo -n -e $id '\t'
	#��ǰ�ն����е�ACK��Ŀ��¼���ļ�data��
	grep $id $log_file > $esl_data
	#ͳ��֡������ACK�ĳɹ�����PACKET��
	awk -F: 'BEGIN{FRAME=0;ACK_OK=0;PACKET=0}{FRAME++;if ($8=="PASS") {ACK_OK++;};PACKET+=$9;}END{printf "%d\t%04f\t%d\t", FRAME, ACK_OK/FRAME, PACKET}' $esl_data

	#ͳ��ACK����
	cut -d'[' -f2 $esl_data | cut -d']' -f1 | sed -e 's/,/\n/g' | grep -v '^$'  | cut -d'-' -f2 | sort | uniq -c | awk '{printf "%s=%d,", $2, $1}'
	echo
done

echo -e '\nQueue Packet'
grep -a -E '^[^:]{3}:[^:]{1,}:[^:]{1,}:3:8:*' $1 > $log_file
echo -e 'ID\t\t\tFRAME\tPASS'
for id in `cat $esl_list`
do
	echo -n -e $id '\t'
	grep $id $log_file > $esl_data
	awk -F: 'BEGIN{FRAME=0;ACK_OK=0}{FRAME++; if (match($8, "PASS")) {ACK_OK++;};}END{printf "%d\t%04f\n", FRAME, ACK_OK/FRAME}' $esl_data
done

echo -e '\nNetlink Packet'
grep -a -E '^[^:]{3}:[^:]{1,}:[^:]{1,}:3:9:*' $1 > $log_file
echo -e 'ID\t\t\tFRAME\tPASS'
for id in `cat $esl_list`
do
	echo -n -e $id '\t'
	grep $id $log_file > $esl_data
	awk -F: 'BEGIN{FRAME=0;ACK_OK=0}{FRAME++; if (match($8, "PASS")) {ACK_OK++;};}END{printf "%d\t%04f\n", FRAME, ACK_OK/FRAME}' $esl_data
done

#ͳ��ÿ��case�ĳɹ���, ��3����ӡ�㣬��6��ͳ����
#$$$:23:10:3:6:ESL000:ID=0xa1,0x01,0x02,0x01:PASS(0x04);355;
echo -e '\nCase'
for ((i = 0; i < 15; i++))
do
	echo -n -e $i: '\t'
	grep -a -E "^[^:]{3}:[^:]{1,}:$i:3:6:*" $1 | awk -F: 'BEGIN{OK=0;TOTAL=0}{TOTAL++; if (match($8,"PASS")) {OK++;}; } END{printf "%4f\n", OK/TOTAL}'
done

#ͳ��ÿ�ִ���ĸ��ʣ���2����ӡ�㣬��6��ͳ������ΪFAIL��
#��Ҫ�ų�case13��������ݲ���
echo -e '\nFailed Reason'
grep -a -E "^[^:]{3}:[^:]{1,}:[^:]{1,}:3:6:*" $1 | grep -v -E '^[^:]{3}:[^:]{1,}:13:3:6:*' | grep "FAIL" | cut -d':' -f8 | cut -b6-9 | sort | uniq -c

#ɾ����ʱ�ļ�
rm -rf $log_file $esl_list $esl_data -rf
