#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define LCD_ROW	72
#define LCD_LINE	172

static uint8_t LCD[((LCD_ROW + 7) / 8) * LCD_LINE];
static uint8_t NEW_LCD[LCD_ROW * ((LCD_LINE + 7) / 8)];

//��������ķ�ʽ�洢�ļ�
static void lcd2dot(void) {
	uint8_t c, line_buf[LCD_LINE];	//һ������ȡline���ַ�
	int k, row, line, B, n = 0;

	for (row = 0; row < (LCD_ROW +7) /8; row++) {
		//ȡһ������ֽڵ�line��
		for (line = 0; line < LCD_LINE; line++)
			line_buf[line] = LCD[line * ((LCD_ROW + 7) / 8) + row];
		//�����ǵĸ�λ����ϲ���һ���ֽ�
		for (k = 7; k >= 0; k--) {
			for (B = 0; B < LCD_LINE; B += 8) { //����8��һ��
				c = 0;
				c |= (((line_buf[B + 0] >> k) & 1) << 7);
				c |= (((line_buf[B + 1] >> k) & 1) << 6);
				c |= (((line_buf[B + 2] >> k) & 1) << 5);
				c |= (((line_buf[B + 3] >> k) & 1) << 4);
				c |= (((line_buf[B + 4] >> k) & 1) << 3);
				c |= (((line_buf[B + 5] >> k) & 1) << 2);
				c |= (((line_buf[B + 6] >> k) & 1) << 1);
				c |= (((line_buf[B + 7] >> k) & 1) << 0);
				NEW_LCD[n++] = c;
			}
			//��󼸸�bit���ܴղ���һ���ֽڣ���Ҫ��Ϊ0
			//NEW_LCD[n-1] &= (0xff << (7 - LCD_LINE % 8));
			if (LCD_LINE % 8)
				n--;
		}
	}
}

static void arr_dump(void) {
	int i, j;

	for (j = 0; j < LCD_ROW; j++) {
		for (i = 0; i < (LCD_LINE + 0) / 8; i++)
			printf("%02X, ", NEW_LCD[j * ((LCD_LINE+0) /8) + i]);
		printf("\n");
	}
	
}

int main(void) {
	memset(LCD, 0xf, sizeof(LCD));
	lcd2dot();
	arr_dump();
	return 0;
}
