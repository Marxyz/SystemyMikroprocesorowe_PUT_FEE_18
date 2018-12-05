/**
 * @author Dominik Luczak
 * @date 2016-03-09
 * @brief IO.
 */

#include "IO.h"
char _getKey()
{
	char c;
	while(!RI);
	c = SBUF;
	RI = 0;
	return c;
}

char putchar(char c)
{
	while(!TI);
	TI = 0;
	return c;
}

char UART_putchar(char c)
{
	SBUF = c;
	while(!TI);
	TI = 0;
	return c;
}
void UART_puts(char * str)
{
	while(*str != '\0')
	{
		UART_putchar(*str);
		str++;
	}
}
void UART_gets(char* str)
{
	//while(SBUF==0x00);
	while(SBUF != 0x0D)
	{
		*str = _getKey();
		//*str = 'x';
		str++;
	}
}