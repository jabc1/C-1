#include "rel_encode.h"

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

int rel8_encode(const unsigned char *in, int insize, unsigned char *out) {
	//�����ҵ�find_c��0x00����0xff, ֻʹ��1���ֽڣ�������ܱ�ʾ�ҵ�256����ͬ���ַ�
	unsigned char find_c = 0;
	int find_key1 = 0, find_key2 = 0; //�ҵ�0x00�����ҵ�0xff��־
	int n = 0;

	while (insize--) {
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

int rel8_decode(const unsigned char *data, int len, unsigned char *out) {
	int i, j;
	int n = 0;

	for (i = 0; i < len;) {
		switch (data[i]) {
			case KEY1:	//����00����ff���������n����n����һ���ֽھ���
			case KEY2:
				for (j = data[i+1]; j > 0; j--)
					*out++ = data[i];
				n += data[i+1];
				i += 2;
				break;
			default:
				*out++ = data[i];
				i++;
				n++;
				break;
		}
	}

	return n;
}

