#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/sizes.h>

#include "common.h"

#define LCD_WIDTH			480
#define LCD_HEIGHT		272
#define LCD_REG_BASE		0x4D000000
#define GPIO_REG_BASE	0x56000000

struct __lcd_st {
	unsigned long reg_virt, gpio_virt;
	unsigned long lcdcon[5];
	unsigned long lcdadd[3];
	unsigned long gpbcon, gpbdat;
	unsigned long gpccon, gpcup;
	unsigned long gpdcon, gpdup;

	int	width, height;
	void	*fb_buf;

	int (*config)(struct __lcd_st *);
	int (*enable)(struct __lcd_st *);
	int (*disable)(struct __lcd_st *);
};

static struct __lcd_st *lcd = NULL;

static int lcd_config(struct __lcd_st *l) {

	/* GPIO 设置*/
	setval(l->gpbcon, 0x01, 2, 0); /*lcd 灯光控制*/
	*(unsigned int *)(l->gpccon) = 0xaaaaaaaa;
	*(unsigned int *)(l->gpcup) = 0xffff;
	*(unsigned int *)(l->gpdcon) = 0xaaaaaaaa;
	*(unsigned int *)(l->gpdup) = 0xffff;

	/*lcd 设置*/
	setval(l->lcdcon[0],0x0,1,0);
	setval(l->lcdcon[0],12,4,1);
	setval(l->lcdcon[0],0x3,2,5);
	setval(l->lcdcon[0],0x4,10,8);

	setval(l->lcdcon[1], 0xa, 6, 0);
	setval(l->lcdcon[1], 0x2, 8, 6);
	setval(l->lcdcon[1], 271, 10, 14);
	setval(l->lcdcon[1], 0x2, 8, 24);

	setval(l->lcdcon[2], 0x2, 8, 0);
	setval(l->lcdcon[2], 479, 11, 8);
	setval(l->lcdcon[2], 0x2, 7, 19);

	setval(l->lcdcon[3], 41, 8, 0);

	setval(l->lcdcon[4], 0x1, 1, 0);
	setval(l->lcdcon[4], 0x3, 2, 8);
	setval(l->lcdcon[4], 0x1, 1, 11);

	*(unsigned int *)(l->lcdadd[0]) = (unsigned long)(l->fb_buf) >> 1;
	*(unsigned int *)(l->lcdadd[1]) = ((unsigned long)(l->fb_buf) \
												+ l->width * l->height * 2) >> 1;
	*(unsigned int *)(l->lcdadd[2]) = LCD_WIDTH;

	return 0;
}

static int lcd_enable(struct __lcd_st *l){
	/*启用*/
	setval(l->gpbdat, 0x1, 1, 0);
	setval(l->lcdcon[4], 0x1, 1, 3);
	setval(l->lcdcon[0], 0x1, 1, 0);

	return 0;
}

static int lcd_disable(struct __lcd_st *l){
	setval(l->gpbdat, 0x0, 1, 0);
	setval(l->lcdcon[4], 0x0, 1, 3);
	setval(l->lcdcon[0], 0x0, 1, 0);

	return 0;
}

static int fill_struct_value(struct __lcd_st *l) {
	int ret = 0;

	l->reg_virt = (unsigned long) ioremap(LCD_REG_BASE, SZ_4K);
	l->gpio_virt = (unsigned long) ioremap(GPIO_REG_BASE, SZ_4K);

	l->lcdcon[0] = l->reg_virt + 0x00;
	l->lcdcon[1] = l->reg_virt + 0x04;
	l->lcdcon[2] = l->reg_virt + 0x08;
	l->lcdcon[3] = l->reg_virt + 0x0c;
	l->lcdcon[4] = l->reg_virt + 0x10;
	
	l->lcdadd[0] = l->reg_virt + 0x14;
	l->lcdadd[1] = l->reg_virt + 0x18;
	l->lcdadd[2] = l->reg_virt + 0x1c;

	l->gpbcon = l->gpio_virt + 0x10;
	l->gpbdat = l->gpio_virt + 0x14;

	l->gpccon = l->gpio_virt + 0x20;
	l->gpcup  = l->gpio_virt + 0x28;

	l->gpdcon = l->gpio_virt + 0x30;
	l->gpdup  = l->gpio_virt + 0x38;

	l->width	 = LCD_WIDTH;
	l->height = LCD_HEIGHT;

	l->fb_buf = kmalloc(LCD_WIDTH * LCD_HEIGHT * 2, GFP_KERNEL);
	if (l->fb_buf == NULL) {
		printk("alloc memory for fb_buf failed.\n");
		ret = -ENOMEM;
		goto malloc_fb_err;
	}

	l->config = lcd_config;
	l->enable = lcd_enable;
	l->disable = lcd_disable;

	return ret;

/*if error*/
malloc_fb_err:
	return ret;
}

static int lcd_init(void){
	int ret;

	if ((lcd = kmalloc(sizeof(struct __lcd_st), GFP_KERNEL)) == NULL) {
		printk("malloc alloc memery for lcd failed.\n");
		ret = -ENOMEM;
		goto malloc_lcd_err;
	}

	if ((ret = fill_struct_value(lcd)))
		goto fill_struct_err;

	lcd->config(lcd);
	lcd->enable(lcd);

	return 0;

/*if error*/
fill_struct_err:
	kfree(lcd);
malloc_lcd_err:
	return ret;
}

static void lcd_exit(void){
	kfree(lcd);
}

module_init(lcd_init);
module_exit(lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YU");
MODULE_VERSION("v0.5");
