#include <linux/module.h>
#include <linux/init.h>
#include <asm/sizes.h>
#include <asm/io.h>

#include "test.h"

unsigned long phys, virt;
unsigned long *gpecon, *gpedat, *gpeup;
unsigned int reg;

unsigned int ioread(unsigned long *addr)
{
	return *(volatile unsigned long *)(addr);	
}
void iowrite(unsigned int val, unsigned long *addr)
{
	*(volatile unsigned long *)addr = val;	
}



void led_on(void)
{
	iowrite(ioread(gpedat) & ~(3 << 12), gpedat);
}

void led_off(void)
{
	iowrite(ioread(gpedat) | (3 << 12), gpedat);
}


int test_init(void)
{
	phys = 0x56000000;
	virt = ioremap(phys, SZ_4K);
	gpecon = virt + 0x40;	
	gpedat = virt + 0x44;
	gpeup = virt + 0x48;

	//configure gpe13 is output pin
	reg = ioread(gpecon);	
	reg &= ~(0xf << 24);
	reg |= (5 << 24);
	iowrite(reg, gpecon);

	iowrite(ioread(gpeup) | (3 << 12), gpeup);

	//on	
	led_on();
	printk("hello io\n");
	return 0;
}

void test_exit(void)
{
	led_off();
	iounmap(virt);
	printk("[%s]\n", __FUNCTION__);
}

module_init(test_init);
module_exit(test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Richard");
MODULE_VERSION("v0.1");


