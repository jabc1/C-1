#!/bin/bash

#output file
log_file=temp_file
esl_list=esl_list_file
esl_data=esl_data_file

#�ҵ�����Ҫ�������log��Ŀ
grep '^[$]\{3\}' test_log > $log_file
#�����ն�ID�嵥
esl_list=`cut -d':' -f7 $log_file | sort | uniq`

for id in $esl_list
do
	echo -n ID=$id
	#��ǰ�ն����е�ACK��Ŀ��¼���ļ�data��
	grep $id $log_file | grep -E '*:*:*:3:*' > $esl_data
	#ͳ��֡������ACK�ĳɹ�����PACKET��
	awk -F: 'BEGIN{FRAME=0;ACK_OK=0;PACKET=0}{FRAME++;if ($8=="OK") {ACK_OK++;};PACKET+=$9;}END{printf ":FRAME=%d:OK=%d%%:PACKET=%d:", FRAME, ACK_OK*100/FRAME, PACKET}' $esl_data

	#ͳ��ACK����
	cut -d'[' -f2 $esl_data | sed -e 's/\];//' -e 's/,/\n/    g' | cut -d'-' -f3 | sort | uniq -c | awk '{printf "%s=%d,", $2, $1}'
	#������ ESL:֡����:֡�ɹ���:RF������:RF�������ͷֲ�
	echo
done

#ɾ����ʱ�ļ�
rm -rf $log_file $esl_list $esl_data -rf
