#include "aduc831.h"	//Definitions of ADuC831 registers name
#include "stdint.h" 	//Standard integers
#include "stdfloat.h" //Standard float
#include "IO.h"				//Input/output definitions
#include <stdlib.h>
#define ca_Vref 5.0
#define ca_Resolution 12
#define ca_Maximum_Value ((0x000001ul<<ca_Resolution)-1)
#define F_OSC 11058000
#define pars 12
#define t_resol 16
#define OKRES 50

#define Tx_Tau(dzielnik) (float)((float)dzielnik/F_OSC)
#define Tx_N(czas_ms, dzielnik) (unsigned int)((float)czas_ms/Tx_Tau(dzielnik)/1000.0)
#define T1_Rejestr(czas_ms) ((0x000001ul<<t_resol)-Tx_N(czas_ms,pars))
#define T1_Set(czas_ms) TL1 = T1_Rejestr(czas_ms);TH1 = T1_Rejestr(czas_ms)>>8;

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
    probka_napiecie = (probka_napiecie < 0) ? 0 : probka_napiecie;
	probka.wartosc = (uint16_t)(probka_napiecie* (1.0 / (1.0 * ca_Vref ))* (float32_t)ca_Maximum_Value);
	DAC0H = probka.slowo.bajt_gorny;
	DAC0L = probka.slowo.bajt_dolny;
}

int main()
{
	
	ET1 = 1;
	EA = 1;
	DACCON = 0x7F;
	TMOD = 0x10;
	
	pilaParam.okres = 4.0;
	pilaParam.amplituda = 5.0;
	pilaParam.offset = 0.0;
	pilaParam.t = 0.0;
	pilaParam.param = 2.0;
	pilaParam.delta_t = ((float32_t)OKRES/1000.0);
	
	T1_Set(OKRES)
	TR1 = 1;
	while(1){};
	
}