#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>

#define KEY1 0xff
#define KEY2 0x00

//������ɸ�0x00����0xff
//���ú�ķ�ʽ�����������ΪҪͬʱ�޸ĺü���ֵ
#define out_00ff do {\
						if (find_key1 || find_key2) {\
							*out = find_key1 ? KEY1 : KEY2;\
							out++;\
							*out = find_c;\
							out++;\
							n += 2;\
							find_c = 0;\
							find_key1 = find_key2 = 0;\
						}\
					} while(0);

static int rel8_encode(const unsigned char *in, int insize, unsigned char *out, int out_size) {
	//�����ҵ�find_c��0x00����0xff, ֻʹ��1���ֽڣ�������ܱ�ʾ�ҵ�256����ͬ���ַ�
	unsigned char find_c = 0;
	int find_key1 = 0, find_key2 = 0; //�ҵ�0x00�����ҵ�0xff��־
	int n = 0;

	while (insize--) {
		//��ֹ���
		if (n >= out_size)
			return 0;

		switch (*in) {
			case KEY1:
			case KEY2:
				if (*in == KEY1) {
					if (find_key2)	//����ϸ��ַ���0xff
						out_00ff;
					find_key1 = 1;
				} else if (*in == KEY2) {
					if (find_key1)	//����ϸ��ַ���0x00
						out_00ff;
					find_key2 = 1;
				}

				find_c++;
				if (find_c == 0)
					out_00ff;	//find_c��������255��ض�,���¼���
				in++;
				break;
			default:
				out_00ff;
				*out = *in;
				in++;
				out++;
				n += 1;
				break;
		}
	}
	out_00ff;
	return n;
}

static uint16_t cal_crc(const unsigned char *data, int len) {
   unsigned short crc = 0;
   int i, j;
   
   for (j = 0; j < len; j++) {
      for(i = 0x80; i != 0; i /= 2) {
         crc *= 2;
         if((crc & 0x8000) != 0)
            crc ^= 0x1021;
         if((data[j] & i) != 0)  
            crc ^= 0x1021;
      }    
   }   
	//fprintf(stderr, "crc %d bytes = %02X.\n", len, crc);
	return crc;
}


#pragma pack(1)
struct protocal_t {
	uint16_t crc;
	uint32_t size;
	uint8_t	flag;
	uint8_t	content[0];
};
#pragma pack()

int protocal_data(const uint8_t *content, int content_len, uint8_t *buf, int buf_len) {
	int len, n;
	struct protocal_t *prt_data = (void *)buf;
	
	n = buf_len - offsetof(struct protocal_t, content);
	if (n <= 0)
		return 0;
	len = rel8_encode(content, content_len, prt_data->content, n);
	if (len == 0)
		return 0;
	else if (len < content_len)
		prt_data->flag = 0x40;	//REL-8ѹ����ʽ
	else {
		prt_data->flag = 0x00;	//ѹ��Ч�����ã���ѹ��
		memcpy(prt_data->content, content, content_len);
		len = content_len;
	}

	prt_data->size = len + sizeof(prt_data->size) + sizeof(prt_data->flag);
	prt_data->crc = cal_crc((void *)&prt_data->size, prt_data->size);

	//fprintf(stderr, "CRC_VAL = 0x%02X, CRC_LEN = %d\n", prt_data->crc, prt_data->size);
	return prt_data->size + sizeof(prt_data->crc);
}
