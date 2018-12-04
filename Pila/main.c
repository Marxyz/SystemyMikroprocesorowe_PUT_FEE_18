#include "aduc831.h"	//Definitions of ADuC831 registers name
#include "stdint.h" 	//Standard integers
#include "stdfloat.h" //Standard float
#include "IO.h"				//Input/output definitions

#define ca_Vref 5.0
#define ca_Resolution 12
#define ca_Maximum_Value ((0x000001ul<<ca_Resolution)-1)
#define F_OSC 12000000.0
#define pars 12
#define t_resol 16
#define OKRES 10

float32_t SecondsPerHit(float32_t f_osc, int parser)
{
	 return (float32_t)((float32_t)parser/f_osc);
}

uint32_t CalculateTimerHitCount(uint32_t time_ms, float32_t f_osc, int parser)
{
		float32_t sph;
		sph = SecondsPerHit(f_osc,parser);
		return (uint32_t)((float)time_ms * sph / 1000);
}

void SetTimerRegisters(time_ms)
{
		uint32_t N;
		N = CalculateTimerHitCount(time_ms, F_OSC, pars);
		TL1 = (0x000001ul << t_resol) - N;
		TH1 = TL1 >> 8;
}

float32_t modulo(float32_t a, float32_t b)
{
	int16_t result = (int16_t)(a/b);
	float32_t mod = a - (float32_t)(result ) *b;
	return mod;
}

typedef struct
{
	double okres;
	double amplituda;
	double offset;
	double t;
	double delta_t;
}parametry_sygnalu_t;

float32_t pila(parametry_sygnalu_t* syg)
{
	return syg->amplituda*(modulo(syg->t, (syg->okres)))/(syg->okres) + syg->offset;
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
	SetTimerRegisters(OKRES);
	pilaParam.t += pilaParam.delta_t;
	if(pilaParam.t >= pilaParam.okres)pilaParam.t = pilaParam.delta_t;
	probka_napiecie = pila(&pilaParam);
	probka_napiecie = (probka_napiecie>ca_Vref)? ca_Vref : probka_napiecie;
	probka_napiecie = (probka_napiecie < 0)? 0: probka_napiecie;
	probka.wartosc = (uint16_t)(probka_napiecie/ca_Vref * (float32_t)ca_Maximum_Value);
	DAC0H = probka.slowo.bajt_gorny;
	DAC0L = probka.slowo.bajt_dolny;
}

int main()
{
	
	ET1 = 1;
	EA = 1;
	DACCON = 0x7F;
	TMOD = 0x10;
	
	pilaParam.okres = 2.0;
	pilaParam.amplituda = 5.0;
	pilaParam.offset = 0.0;
	pilaParam.t = 0.0;
	pilaParam.delta_t = ((float32_t)OKRES/1000.0);
	
	SetTimerRegisters(OKRES);
	
	TR1 = 1;
  while(1){};
	return 0;
	
}