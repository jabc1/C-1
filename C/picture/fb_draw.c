#include "main.h"

struct fb_st {
     struct fb_fix_screeninfo fix;
     struct fb_var_screeninfo var;
     unsigned long bpp;
     int fd;
     char *fbp;
};

static struct fb_st fb0;
static int otsu(unsigned char *colors, int w, int h);

int fb_open(void)
{
     int ret;

     fb0.fd = open("/dev/fb0", O_RDWR);
     if (-1 == fb0.fd) {
	  perror("open");
	  goto error;
     }

     /* get fb_var_screeninfo */
     ret = ioctl(fb0.fd, FBIOGET_VSCREENINFO, &fb0.var);
     if (-1 == ret) {
	  perror("ioctl(fb0.var)");
	  goto close_fd;
     }

     fb0.bpp = fb0.var.bits_per_pixel / 8;

     /* get fb_fix_screeninfo */
     ioctl(fb0.fd, FBIOGET_FSCREENINFO, &fb0.fix);
     if (-1 == ret) {
	  perror("ioctl(fb0.fix)");
	  goto close_fd;
     }

     /* get framebuffer start address */
     fb0.fbp = mmap(NULL, fb0.fix.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fb0.fd, 0);
     if ((void *)-1 == fb0.fbp) {
	  perror("mmap");
	  goto close_fd;
     }

     return 0;

close_fd:
     close(fb0.fd);
error:
     return -1;
}

void fb_close()
{
     munmap(fb0.fbp, fb0.fix.smem_len);
     close(fb0.fd);
}

void fb_draw_point(int x, int y, struct color_v *color)
{
     unsigned long offet = 0;
	  char *p;

     offet = fb0.bpp * x + y * fb0.fix.line_length;
	  p = fb0.fbp + offet;

	  memcpy(p, color, fb0.bpp);
}


int init_bmp(BMP_FH *bmp_fh, BMP_FI *bmp_fi, FILE *fp) {

	if (fread(bmp_fh, sizeof(*bmp_fh), 1, fp) < 1 || \
		fread(bmp_fi, sizeof(*bmp_fi), 1, fp) < 1 ) 
		return -1;

	if (bmp_fh->bfType != BMPFILE)	
		return -1;

	fseek(fp, bmp_fh->bfOffBits, SEEK_SET);

	return 0;
}

static int read_color(struct color_v *color, int bpp, FILE *fp) {
	short int a;

	switch (bpp) {
	case 4:
		fread(&color->b, 1, 1, fp);
		fread(&color->g, 1, 1, fp);
		fread(&color->r, 1, 1, fp);
		fread(&color->t, 1, 1, fp);
		break;
	case 3:
		fread(&color->b, 1, 1, fp);
		fread(&color->g, 1, 1, fp);
		fread(&color->r, 1, 1, fp);
		break;
	case 2:
		fread(&a, 2, 1, fp);
#if 1
		color->r = (a & 0xFB00) / 800 * 8;
		color->g = (a & 0x07E0) / 20 * 4;
		color->b = (a & 0x001F)* 8;
#else 
		color->r = ((~((~0) << 5)) & (a >> 11)) / 800 * 8 ;
		color->g = ((~((~0) << 6)) & (a >> 5)) / 20 * 4;
		color->b = ((~((~0) << 5)) & a) * 8;
#endif
		break;
	case 1:
		break;
	default:
		break;
	}
	return 0.299 * color->r + 0.587 * color->g + 0.114 * color->b;
}

unsigned char bmp[4096*1024*4];
void draw_pic(BMP_FI *bmp_fi, FILE *fp, int x, int y) {
	int i, j, bpp, offset, thresh;
	struct color_v color = {0,0,0,0};
	struct color_v white = {0xff, 0xff, 0xff, 0xff};
	struct color_v black = {0, 0, 0, 0};

	bpp = bmp_fi->biBitCount / 8;
	offset = (4 - (bmp_fi->biWidth * bpp) % 4) % 4;

	for (i = bmp_fi->biHeight; i > 0; i--) {
		for (j = 0; j < bmp_fi->biWidth; j++)
			bmp[i * bmp_fi->biWidth + j] = read_color(&color, bpp, fp);
		for (j = offset; j > 0; j--)
			fgetc(fp);
	}
	thresh = otsu(bmp, bmp_fi->biWidth, bmp_fi->biHeight);
	for (i = bmp_fi->biHeight; i > 0; i--) 
		for (j = 0; j < bmp_fi->biWidth; j++) 
			fb_draw_point(x+j, y+i, bmp[i * bmp_fi->biWidth + j] > thresh ? &black : &white);
}


/** 
 * OTSU�㷨�����ʷָ���ֵ 
 */ 
static int otsu(unsigned char *colors, int w, int h) { 
    unsigned int pixelNum[256]; // ͼ��Ҷ�ֱ��ͼ[0, 255] 
    int color; // �Ҷ�ֵ 
    int n, n0, n1; //  ͼ���ܵ�����ǰ�������� �󾰵�����n0 + n1 = n�� 
    int w0, w1; // ǰ����ռ������ ����ռ������w0 = n0 / n, w0 + w1 = 1�� 
    double u, u0, u1; // ��ƽ���Ҷȣ�ǰ��ƽ���Ҷȣ���ƽ���Ҷȣ�u = w0 * u0 + w1 * u1�� 
    double g, gMax; // ͼ����䷽������䷽�g = w0*(u0-u)^2+w1*(u1-u)^2 = w0*w1*(u0-u1)^2�� 
    double sum_u, sum_u0, sum_u1; // ͼ��Ҷ��ܺͣ�ǰ���Ҷ��ܺͣ� ��ƽ���ܺͣ�sum_u = n * u�� 
    int thresh; // ��ֵ 
 
    memset(pixelNum, 0, 256 * sizeof(unsigned int)); // ������0 
 
    // ͳ�Ƹ��Ҷ���Ŀ 
    int i, j; 
    for (i = 0; i < h; i++) { 
        for (j = 0; j < w; j++) { 
            color = (colors[w * i + j]) & 0xFF; // ��ûҶ�ֵ 
            pixelNum[color]++; // ��Ӧ�Ҷ���Ŀ��1 
        } 
    } 
 
    // ͼ���ܵ��� 
    n = w * h; 
 
    // �����ܻҶ� 
    int k; 
    for (k = 0; k <= 255; k++) { 
        sum_u += k * pixelNum[k]; 
    } 
 
    // �����ж������䷽��õ������ֵ 
    for (k = 0; k <= 255; k++) { 
        n0 += pixelNum[k]; // ͼ��ǰ������ 
        if (0 == n0) { // δ��ȡǰ����ֱ�Ӽ�������ǰ������ 
            continue; 
        } 
        if (n == n0) { // ǰ������������ȫ��ʱ�������������ӣ��˳�ѭ�� 
            break; 
        } 
        n1 = n - n0; // ͼ��󾰵��� 
 
        sum_u0 += k * pixelNum[k]; // ǰ���Ҷ��ܺ� 
        u0 = sum_u0 / n0; // ǰ��ƽ���Ҷ� 
        u1 = (sum_u - sum_u0) / n1; // ��ƽ���Ҷ� 
 
        g = n0 * n1 * (u0 - u1) * (u0 - u1); // ��䷽��ٳ���n^2�� 
 
        if (g > gMax) { // ���������䷽��ʱ 
            gMax = g; // ���������䷽�� 
            thresh = k; // ȡ�����䷽��ʱ��Ӧ�ĻҶȵ�k���������ֵ 
        } 
    } 
 
    return thresh; 
} 

