#ifndef __REL_ENCODE_H__
#define __REL_ENCODE_H__

/*	ѹ���㷨���
** �����������0x00����0xff��ʾΪ2���ֽڣ���һ���ֽ�Ϊ0x00����0xff��
** �ڶ����ֽ�Ϊ0x00����0xff�������ֵĴ�����
** ������Ҫѹ����������Ϊ "0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0x34"
**	��ѹ�����������Ϊ"0x00, 0x03, 0xfe, 0xff, 0x02, 0x34"
**	���������д��ڴ�����ox00����0xffʱ��ѹ������Խ��
*/

#define KEY1	0x00	//	0x00��Ҫѹ��
#define KEY2	0xff	//	0xff��Ҫѹ��

int rel8_encode(const unsigned char *in, int insize, unsigned char *out); 
int rel8_decode(const unsigned char *data, int len, unsigned char *out);

#endif
