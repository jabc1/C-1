#include <common.h>
#include <s3c2440.h>

#define UART_NR UART0

static void _serial_init(int bord_rate, int data, int parity, int stop)
{
	struct uart *const uart = get_base_uart(UART_NR);
	struct gpio *const gpio = get_base_gpio();

	setval(gpio->GPHCON, 0xa, 4, 4 + (UART_NR * 4));
	setbit(gpio->GPHUP, 0x3, 2 + (UART_NR * 2));

	switch(data)
	{
		case 5:
			setval(uart->ULCON, 0x0, 2, 0);
			break;
		case 6:
			setval(uart->ULCON, 0x1, 2, 0);
			break;
		case 7:
			setval(uart->ULCON, 0x2, 2, 0);
			break;
		case 8:
		default:
			setval(uart->ULCON, 0x3, 2, 0);
	}

	switch(parity)
	{
		case 1:
			setval(uart->ULCON, 0x4, 3, 3);
			break;
		case 2:
			setval(uart->ULCON, 0x5, 3, 3);
			break;
		case 0:
		default:
			setval(uart->ULCON, 0x0, 3, 3);
	}

	switch(stop)
	{
		case 2:
			set1(uart->ULCON, 2);
			break;
		case 1:
		default:
			set0(uart->ULCON, 2);
	}

	setval(uart->UCON, 0x5, 4, 0); /* 设置接收和发送为中断或查询模式 */

	setval(uart->UCON, 0x0, 2, 10); /* 设置波特率采用PCLK作为时钟源 */

#define PCLK 50000000
#define UART_CLOCK PCLK
	uart->UBRDIV = UART_CLOCK / (bord_rate * 16) - 1; /* 设置波特率 */

}

void serial_init(void)
{
	/* 波特率115200bps, 8个数据位, 无校验位, 1个停止位 */
	        _serial_init(115200, 8, 0, 1);
}

int serial_putc(const char c)
{
	struct uart *const uart = get_base_uart(UART_NR);

	while(!(uart->UTRSTAT & 2));

	uart->UTXH = c;

	if(c == '\n')
		serial_putc('\r');

	return c;
}

int serial_getc(void)
{
	struct uart *const uart = get_base_uart(UART_NR);
	char c;
		
	while(!(uart->UTRSTAT & 1));

	c = uart->URXH;

	if(c == '\r')
		c =  '\n';
	
	serial_putc(c);

	return c;
}


