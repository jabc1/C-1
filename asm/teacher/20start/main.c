
int main(void)
{
	char buf[1024];

	serial_init();
	
	puts("hello start....");

	while(1)
	{
		puts("[u-boot]# ");
		gets(buf);
		puts(buf);
	}


	return 0;
}


