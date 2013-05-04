#include <stdio.h>
#include <stdint.h>
#include <string.h>

//�ֽ����򣬺���ȡ��
//0x82[10000010] ��������Ϊ*_____*_

#define    ASC_12_OFFS  0
#define    ASC_16_OFFS	 (ASC_12_OFFS + 1536)
#define    ASC_24_OFFS	 (ASC_16_OFFS + 4096)
#define    HZK_16_OFFS	 (ASC_24_OFFS + 12288)
#define    HZK_24_OFFS	 (HZK_16_OFFS + 267616)
#define	  HZK_14_OFFS	 (784064) //size = 189504
#define	  ASC_14_OFFS	 (HZK_14_OFFS + 189504)

#define LCD_ROW	64			//���谴8����
#define LCD_LINE	72
#define LCD_LINE_EMPTY	0	//�ַ�֮���һ������

typedef enum {
    FONT_12	= 12,
    FONT_16	= 16,
    FONT_24	= 24,
	 FONT_14 = 14,

    FONT_MAX	= FONT_24,
} FONT_SIZE_T;

typedef enum {
    ASC_12,
    ASC_16,
    ASC_24,
	 ASC_14,
    HZK_16,
    HZK_24,
	 HZK_14,

    FONT_ERR,
} FONT_TYPE_T;

static uint8_t LCD[LCD_LINE * (LCD_ROW + 7 )/8];	//��Ļ�����ļ�
static void lcd_dump(void) {
	int i, j, k;
	//fwrite(LCD, 1, sizeof(LCD), stderr);
	
	for (i = 0; i < LCD_LINE; i++) {
		for (j = 0; j < LCD_ROW / 8; j++)
			for (k = 7; k >= 0; k--)
				printf("%s", LCD[i * LCD_ROW / 8 + j] & (1 << k) ? "--" : "  ");
		printf("\n");
	}
}

static void spi_read(uint32_t addr, uint8_t *buf, int len) {
	FILE *fp;

	if ((fp = fopen("font.bin", "r")) == NULL) {
		perror("fopen:");
		return;
	}
	fseek(fp, addr, SEEK_SET);
	fread(buf, 1, len, fp);
	fclose(fp);
}

//������������
FONT_TYPE_T get_word_type(FONT_SIZE_T size, uint8_t is_hz) {
  FONT_TYPE_T ret;

  switch (size){
	case FONT_12:
	    ret = !is_hz ? ASC_12: FONT_ERR;
	    break;
	case FONT_16:
	    ret = !is_hz ? ASC_16: HZK_16;
	    break;
	case FONT_24:
	    ret = !is_hz ? ASC_24: HZK_24;
	    break;
	case FONT_14:
	    ret = !is_hz ? ASC_14: HZK_14;
	    break;
	default:
	    ret = FONT_ERR;
	    break;
  }

  return ret;
}

struct __font_bit_size {
	uint8_t r;		//���
	uint8_t l;		//�߶�
	uint8_t s;		//ռ���ֽ���
};
static const struct __font_bit_size font_bit_size[] = {
	{8,12,12},		//ASC_12
	{8,16,16},		//ASC_16
	{16,24,48},		//ASC_24
	{8,14,14},		//ASC_14
	{16,16,32},		//HZK_16
	{24,24,72},		//HZK_24
	{14,14,28},		//HZK_14
	{0,0,0},		//error
};

static uint8_t get_bitmap(FONT_TYPE_T font_type, uint8_t *bit_buf, const uint8_t *str) {
	uint32_t offset;
	int len = font_bit_size[font_type].s;

	switch (font_type) {
		case ASC_12:
			offset = ASC_12_OFFS + (*str) * len;		
	    	break;
		case ASC_16:
	    	offset = ASC_16_OFFS + (*str) * len;		
	    	break;
		case ASC_24:
	    	offset = ASC_24_OFFS + (*str) * len;		
	    	break;
		case ASC_14:
	    	offset = ASC_14_OFFS + (*str - ' ') * len;		
	    	break;
		case HZK_16:
	    	offset = HZK_16_OFFS + (94*(str[0] - 0xa0 -  1) + (str[1] - 0xa0 -1)) * len;		
	    	break;
		case HZK_24:
	    	offset = HZK_24_OFFS + (94*(str[0] - 0xa0  - 15 - 1) + (str[1] - 0xa0 -1)) * len;		
	    	break;
		case HZK_14:
	    	offset = HZK_14_OFFS + (94*(str[0] - 0xa0  - 15 - 1) + (str[1] - 0xa0 -1)) * len;		
	    	break;
		default:
	    	break;	
  }

	spi_read(offset, bit_buf, len);

	return len;
}

static int start_x, start_y, end_x, end_y;
static uint8_t byte_rev(uint8_t data) {
	uint8_t val = 0;
	int i;
	for (i = 7; i >= 0; i--) {
		val |= (((data >> i) & 0x1) << (7 - i));
	}
	return ~val;
}

static void set_arr_bit(uint8_t *arr, int bitn, int val) {
	uint8_t *p = &arr[bitn / 8];
	//printf("set arr bit %d to %d\n", bitn, val);
	*p &= ~(1 << (7 - bitn % 8));
	*p |= ((val & 1) << (7 - bitn % 8));	//��LCD�ĵ�n��bit
}

static void send_bitmap(FONT_TYPE_T font_type, uint8_t *tmp) {
	uint8_t c;
	int bit, line, row, k;
	int font_row = font_bit_size[font_type].r, font_low_align = (font_row + 7) / 8;

	for (line = 0; line < font_bit_size[font_type].l; line++) {
		//ÿ�δ�tmp��ȡ��һ���ֵ�һ��
		for (row = 0, bit = 0; row < font_low_align; row++) {
				c = byte_rev(tmp[line * font_low_align + row]);
				//printf("tmp[%d * (%d + 7) / 8 + %d = %d ] = %d\n", \
					line, font_row, row, line * ((font_row + 7) / 8) + row, c);
				for (k = 0; k < 8; k++) { //������λ
					//��Ҫ�����������λ������14���ؿ�����壬��2���ֽڵĸ�2λ�ǲ���Ҫ��
					if (row == (font_low_align -1) && font_row % 8 != 0 && k >= font_row % 8) {
						//printf("skip %d, k = %d\n", row, k);
						continue;
					}
					set_arr_bit(LCD, (start_y + line) * LCD_ROW + start_x + bit, (c >> k) & 1);
					//printf("set_arr_bit (%d + %d) * %d + %d + %d = %d = %d\n", \
						start_y , line , LCD_ROW , start_x , bit, (start_y + line) * LCD_ROW + start_x + bit, (c >> k) & 1);
					bit++;
				}
		}
	}
}

static void MainLCD_Window_Set(int cur_row, int cur_line, int next_row, int next_line) {
	//printf("set (%d,%d) (%d,%d)\n", cur_row, cur_line, next_row, next_line);
	//x����ֹ��ַ��8bit����
	start_x = cur_row;
	start_y = cur_line;
	end_x = next_row;
	end_y = next_line;
}

//������Ļ��ʼ�ͽ���Ϊֹ,����
//row, lineָ����һ���հ�λ��,���ܻ���Ҳ������������
static void set_lcd_row_line(FONT_TYPE_T font_type, int *rows, int *lines) {
	int font_size_r, font_size_l, cur_row = *rows, cur_line = *lines, next_row, next_line;

	font_size_r = font_bit_size[font_type].r;
	font_size_l = font_bit_size[font_type].l;

	if (cur_row + font_size_r > LCD_ROW) {
		cur_row = 0;
		cur_line += font_size_l + LCD_LINE_EMPTY;
	}

	if (cur_line + font_size_l > LCD_LINE) {
		cur_line = 0;
		cur_row = 0;
	}

	next_row = 	 cur_row + font_size_r;
	next_line =  cur_line + font_size_l;

	MainLCD_Window_Set(cur_row, cur_line, next_row-1, next_line-1);

	next_row += LCD_LINE_EMPTY;
	next_line += LCD_LINE_EMPTY;

 	*rows = next_row;
	*lines = cur_line;

}

static void lcd_print(FONT_SIZE_T size, int row, int lines, const uint8_t *str) {
	unsigned char is_hz;
	unsigned char bit_buf[FONT_MAX * (FONT_MAX/8)];
	FONT_TYPE_T font_type;

	while (*str != '\0') {
		is_hz = (*str) > 0xa0 ? 1 : 0;	//�ж��Ƿ�Ϊ����	
		//������������
		font_type = get_word_type(size, is_hz);
		//������Ļ�������ʼλ��
		set_lcd_row_line(font_type, &row, &lines);

		//���ֿ���ȡ����ǰ�ֵĵ���
		memset(bit_buf, 0x0, sizeof(bit_buf));
		get_bitmap(font_type, bit_buf, str);
		send_bitmap(font_type, bit_buf);
		//row, lineʼ��ָ����һ���հ�λ��,���ܻ���Ҳ������������
		str = is_hz ? str + 2 : str + 1;	//ָ����һ���ַ�
	}
}

int main(int argc, char **argv) {
	memset(LCD, 0xff, sizeof(LCD));
	lcd_print(FONT_14, 0, 0, (uint8_t *)"abcһ������@!�õ��Ӷ�����Զ�����");
	//lcd_print(FONT_14, 0, 0, (uint8_t *)"����һ����");
	lcd_dump();
	return 0;
}
