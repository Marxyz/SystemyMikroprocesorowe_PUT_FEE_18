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

xdata char terminal[30];
xdata int itr = 0;
xdata int ready = 0;
//p - pila, t - trojkat
xdata char type = 'p';

void uartInterrupt () interrupt 4
{
	char buffer;
	if(RI)
	{
		buffer = _getKey();
		if((buffer == '\r'))
		{	
			//terminal[itr] = '\0';			
			itr = 0;
			ready = 1;
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

/**
 * @brief Floating point modulo operation. Takes two float32_t numbers.
 * 
 * @param a The nominator float32_t number
 * @param b The denominator float32_t number
 * @return float32_t The result of operation.
 */
float32_t modulo(float32_t a, float32_t b)
{
	int16_t result = (int16_t)(a/b);
	return  a - (float32_t)(result ) *b;
}


/**
 * @brief Structure representing signal parameters.
 * Okres - singal period interval.
 * Amplituda - signal amplitude.
 * Offset - generating signal offset.
 * t - should not be set, accumulates time for inner calculation
 * delta_t - should not be set, calculated during initialization
 * rosnace - time of signal's rising slope
 * opadajace - time of signal's declining slope.
 */
typedef struct
{
	double okres; 
	double amplituda;
	double offset;
	double t;
	double delta_t;
	double rosnace;
	double opadajace;
}parametry_sygnalu_t;


/**
 * @brief Generates single signal sample.
 * 
 * @param parametry_sygnalu_t* syg Signal parameters pointer
 * @return float32_t Calculated sample.
 */
float32_t GenerateTrojkat(parametry_sygnalu_t* syg)
{
	// Setting values of signal in fixed declaration outside of given reference, in order to save CODE memory.
	float32_t time, result;
	double A = syg->amplituda;
	double T = syg->okres;
	double off = syg->offset;
	double ros = syg->rosnace;
	double opad = syg->opadajace;
	double del = syg->delta_t;
	time = modulo(syg->t,T);
	if(time > ros )
	{
		result = -A  * 1.0 / (opad) *(time - ros) + A + off;
        return result;
	}
	else
	{
		result =   A*time/(ros) + off;
	}
		
	
    return result;
}

/**
 * @brief Generates single signal sample.
 * 
 * @param parametry_sygnalu_t* syg Signal parameters pointer
 * @return float32_t Calculated sample.
 */

float32_t pila(parametry_sygnalu_t* syg)
{
	return syg->amplituda*(modulo(syg->t, (syg->okres)))/(syg->okres) + syg->offset;
}

/**
 * @brief Union representing sample value in register memory.
 * 
 */
typedef union
{
	uint16_t wartosc;
	struct
	{
		uint8_t bajt_gorny;
		uint8_t bajt_dolny;
	}slowo;
}probka_t;

xdata probka_t probka = {0};
xdata float32_t probka_napiecie = 0;
xdata parametry_sygnalu_t sygnalParam;



/**
 * @brief Timer interrupt function.
 * Generates signal samples and instructs analog outputs to emit signal.
 *
 * 
 */
void timer1() interrupt 3
{
	T1_Set(OKRES);
	sygnalParam.t += sygnalParam.delta_t;
	if(sygnalParam.t > sygnalParam.okres)sygnalParam.t = sygnalParam.delta_t;
	switch (type){
		case 'p':
		{
			probka_napiecie = pila(&sygnalParam);
			probka_napiecie = (probka_napiecie>ca_Vref)? ca_Vref : probka_napiecie;
			probka_napiecie = (probka_napiecie < 0)? 0: probka_napiecie;
			probka.wartosc = (uint16_t)(probka_napiecie/ca_Vref * (float32_t)ca_Maximum_Value);
			break;
		}
		case 't':
		{
			probka_napiecie = GenerateTrojkat(&sygnalParam);
			probka_napiecie = (probka_napiecie>ca_Vref)? ca_Vref : probka_napiecie;
			probka.wartosc = (uint16_t)(probka_napiecie* (1.0 / (1.0 * ca_Vref ))* (float32_t)ca_Maximum_Value);
			break;
		}
	}
	//probka_napiecie = GenerateTrojkat(&sygnalParam);
	
	DAC0H = probka.slowo.bajt_gorny;
	DAC0L = probka.slowo.bajt_dolny;
}


/**
 * @brief Program entry point.
 * 
 * @return int
 */

int main()
{
	float tmp;
	int scan;
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
	sygnalParam.okres = 3.0;
	sygnalParam.amplituda = 3.0;
	sygnalParam.offset = 1;
	sygnalParam.t = 0.0;
	sygnalParam.rosnace = 1.5;
	sygnalParam.opadajace = 1.5;
	sygnalParam.delta_t = ((float32_t)OKRES/1000.0);
	
	T1_Set(OKRES)
	TR1 = 1;
	while(1)
	{
		if(ready == 1)
		{
			TR1=0;
			sscanf(terminal, "%cO%fA%fF%fR%f" , &type, &sygnalParam.okres, &sygnalParam.amplituda, &sygnalParam.offset, &sygnalParam.rosnace);
//			sscanf(terminal, "%c", &type);
	//		sscanf(terminal, "O%f", &sygnalParam.okres);
//			sscanf(terminal, "A%f", &sygnalParam.amplituda);
//			sscanf(terminal, "F%f", &sygnalParam.offset);
//			sscanf(terminal, "R%f", &sygnalParam.rosnace);
			scan = sscanf(terminal, "P%f", &sygnalParam.opadajace);
			//sscanf(terminal, "T%f", &tmp);
			memset(terminal, 0, sizeof(terminal[0])*30);
			ready = 0;
			TR1 = 1;
		}
	};
	
}