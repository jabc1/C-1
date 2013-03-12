#!/bin/bash

#output file
log_file=temp_file
esl_list=esl_list_file
esl_data=esl_data_file

#�ҵ�����Ҫ�������log��Ŀ
#grep '^[$]\{3\}' $1 > $log_file
grep -E '^*:*:*:3:7:*' $1 > $log_file
#�����ն�ID�嵥
cut -d':' -f7 $log_file | sort | uniq > $esl_list

echo -e 'ID\t\t\tFRAME\tPASS\tPACKET\tACK'
for id in `cat $esl_list`
do
	echo -n -e $id '\t'
	#��ǰ�ն����е�ACK��Ŀ��¼���ļ�data��
	grep $id $log_file > $esl_data
	#ͳ��֡������ACK�ĳɹ�����PACKET��
	awk -F: 'BEGIN{FRAME=0;ACK_OK=0;PACKET=0}{FRAME++;if ($8=="OK") {ACK_OK++;};PACKET+=$9;}END{printf "%d\t%d%%\t%d\t", FRAME, ACK_OK*100/FRAME, PACKET}' $esl_data

	#ͳ��ACK����
	cut -d'[' -f2 $esl_data | cut -d']' -f1 | sed -e 's/,/\n/g' | grep -v '^$'  | cut -d'-' -f2 | sort | uniq -c | awk '{printf "%s=%d,", $2, $1}'
	echo
done

#ɾ����ʱ�ļ�
rm -rf $log_file $esl_list $esl_data -rf
