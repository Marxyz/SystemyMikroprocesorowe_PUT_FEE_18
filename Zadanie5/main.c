#include "aduc831.h"	//Definitions of ADuC831 registers name
#include "stdint.h" 	//Standard integers
#include "stdfloat.h" //Standard float
#include "IO.h"				//Input/output definitions
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define ca_Vref 5.0
#define ca_Resolution 12
#define ca_Maximum_Value ((0x000001ul<<ca_Resolution)-1)
#define F_OSC 11058000
#define pars 12
#define t_resol 16
#define OKRES 1

#define Tx_Tau(dzielnik) (float)((float)dzielnik/F_OSC)
#define Tx_N(czas_ms, dzielnik) (unsigned int)((float)czas_ms/Tx_Tau(dzielnik)/1000.0)
#define T1_Rejestr(czas_ms) ((0x000001ul<<t_resol)-Tx_N(czas_ms,pars))
#define T1_Set(czas_ms) TL1 = T1_Rejestr(czas_ms);TH1 = T1_Rejestr(czas_ms)>>8;

char terminal[10];
int itr = 0;
char ready = '0';
char type = '0';

void toneUartInterrupt () interrupt 4
{
	char buffer;
	if(RI)
	{
		buffer = _getKey();
		if((buffer == '\r') || itr == 9)
		{	
			terminal[itr] = '\0';			
			itr = 0;
			ready = '1';
			//strncpy(values, terminal, 6);
			//UART_puts(terminal);
			//memset(terminal, 0, sizeof(terminal[0])*20);
		}
		else
		{
			terminal[itr] = buffer;
			SBUF = terminal[itr];
			itr++;
		}
		RI=0;
	}
	if(TI){
		TI = 0;
	}
	return;
}

float32_t modulo(float32_t a, float32_t b)
{
	int16_t result = (int16_t)(a/b);
	return  a - (float32_t)(result ) *b;
}

typedef struct
{
	double okres;
	double amplituda;
	double offset;
	double t;
	double delta_t;
	double param;
}parametry_sygnalu_t;

//float32_t pila(parametry_sygnalu_t* syg)
//{
//	
//	return syg->amplituda*(modulo(syg->t, (syg->okres)))/(syg->okres) + syg->offset;
//}


float32_t trojkat(parametry_sygnalu_t* syg)
{
	float32_t time, result;
	double A = syg->amplituda;
	double T = syg->okres;
	double off = syg->offset;
	double param = syg->param;
	double del = syg->delta_t;
	time = modulo(syg->t,T);
	if(time > param + del)
	{
		result = -A  * 1.0 / (T - param) *(time - param) + A + off;
        return result;
	}
		
	result =   A*time/(param) + off;
    return result;
}

typedef union
{
	uint16_t wartosc;
	struct
	{
		uint8_t bajt_gorny;
		uint8_t bajt_dolny;
	}slowo;
}probka_t;

probka_t probka = {0};
float32_t probka_napiecie = 0;
parametry_sygnalu_t pilaParam;

void timer1() interrupt 3
{
	T1_Set(OKRES);
	pilaParam.t += pilaParam.delta_t;
	if(pilaParam.t > pilaParam.okres)pilaParam.t = pilaParam.delta_t;
	probka_napiecie = trojkat(&pilaParam);
	probka_napiecie = (probka_napiecie>ca_Vref)? ca_Vref : probka_napiecie;
	probka.wartosc = (uint16_t)(probka_napiecie* (1.0 / (1.0 * ca_Vref ))* (float32_t)ca_Maximum_Value);
	DAC0H = probka.slowo.bajt_gorny;
	DAC0L = probka.slowo.bajt_dolny;
}

int main()
{
	
	ET1 = 1;
	EA = 1;
	ES = 1;
	DACCON = 0x7F;
	TMOD = 0x10;
	
	//UART
	REN = 1;
	SM0 = 0x00;
	SM1 = 0x01;
	
	//TIMER3 SETTINGS
	T3CON = 0x81;
	T3FD = 0x20;
	//PRIORYTETY
	PS=1;
	PT1=0;
	
	//DEFAULTS
	pilaParam.okres = 4.0;
	pilaParam.amplituda = 3.0;
	pilaParam.offset = 1.5;
	pilaParam.t = 0.0;
	pilaParam.param = 1.5;
	pilaParam.delta_t = ((float32_t)OKRES/1000.0);
	
	T1_Set(OKRES);
	TR1 = 1;
	while(1)
	{
			
	}
	
}