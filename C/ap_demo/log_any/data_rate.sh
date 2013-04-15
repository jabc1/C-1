#!/bin/bash

#output file
log_file=temp_file
esl_list=esl_list_file
esl_data=esl_data_file

#�ҵ�����Ҫ�������log��Ŀ
grep -a -E '^[^:]{3}:[^:]{1,}:[^:]{1,}:3:6:[^:]{1,}:ID=0xa1,0x01,0x02,0x05' $1 | cut -d':' -f8-| awk -F\; '{if (match($1, "0x40")) {printf "%d OK\n", $2} else {printf "%d fa\n", $2}}' > $log_file

vimdiff $log_file $2

#ɾ����ʱ�ļ�
rm -rf $log_file $esl_list $esl_data -rf
