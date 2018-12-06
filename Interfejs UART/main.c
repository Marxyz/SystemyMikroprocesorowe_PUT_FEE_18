#include "aduc831.h"	/* Definitions of ADuC831 registers name */
#include "stdint.h" 	/* Standard integers */
#include "stdfloat.h"  /* Standard float */
#include "IO.h"				/* Input/output definitions */
#include <stdlib.h> /* for atof() */
#include <string.h> /* for memset() and memcpy() */

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

#define RAMKA_TYP (0)
#define RAMKA_OKRES (2)
#define RAMKA_AMPLITUDA (6)
#define RAMKA_OFFSET (10)
#define RAMKA_ROSNACE (14)
#define RAMKA_OPADAJACE (18)

#define MAX_V = 5;

/*! 
@var char terminal[30] 
@brief Input from user.  
*/
xdata char terminal[30];

/*! 
@var int itr 
@brief Iterator used in UART Interrupt.  
*/
xdata int itr = 0;

/*! 
@var int ready 
@brief Flag setted by UART Interrupt when input from user was read. 
*/
xdata int ready = 0;

/*! 
@var char type 
@brief Represents currently emitted signal. 
 * p - pila
 * t - trojkat
*/
xdata char type = 'p';


/**
 * @brief UART interrupt to fill terminal.
 *
 * line of data specified as:
 * char type of plot, float amplitude, float offset
 * float rosnace, float opadajace. Formatted without ',' delimiter
 */
void uartInterrupt () interrupt 4
{
	char buffer;
	if(RI)
	{
		buffer = _getKey();
		if((buffer == '\r'))
		{	
			terminal[itr] = '\0';			
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
 * @brief Getting parameters from input and setting them to global sygnalParam structure.
 * 
 * @bug using sscanf - probably undefined behaviour, cannot read more than 5 floats, but return value from sscanf is correct.
 * @return void
 */

void getParameters()
{
	xdata float tmp;
	xdata char string[4];
	if(ready == 1)
		{
			TR1=0;
			//sscanf(terminal, "%cO%fA%fF%fR%fP%g" , &type, &sygnalParam.okres, &sygnalParam.amplituda, &sygnalParam.offset, &sygnalParam.rosnace, &sygnalParam.opadajace);
			//tmp = (float)(atoi(&terminal[2])) + (float)(atoi(&terminal[4]))/10 ;
			//sygnalParam.okres = tmp;
			type = terminal[RAMKA_TYP];
			
			memcpy(string, &terminal[RAMKA_OKRES],3);
			string[3] = '\0';
			sygnalParam.okres = atof(string);
			memset(string,0,4);
			
			memcpy(string, &terminal[RAMKA_AMPLITUDA],3);
			string[3] = '\0';
			sygnalParam.amplituda = atof(string);
			memset(string,0,4);
			
			memcpy(string, &terminal[RAMKA_OFFSET],3);
			string[3] = '\0';
			sygnalParam.offset = atof(string);
			memset(string,0,4);
			
			memcpy(string, &terminal[RAMKA_ROSNACE],3);
			string[3] = '\0';
			sygnalParam.rosnace = atof(string);
			memset(string,0,4);
			
			memcpy(string, &terminal[RAMKA_OPADAJACE],3);
			string[3] = '\0';
			sygnalParam.opadajace = atof(string);
			memset(string,0,4);
			
			memset(terminal, 0, sizeof(terminal[0])*30);
			ready = 0;
			TR1 = 1;
		}
}

/**
 * @brief Program entry point.
 * 
 * @return int
 */

int main()
{
	ET1 = 1;
	EA = 1;
	// UART INTERRUPT
	ES = 1;
	DACCON = 0x7F;
	TMOD = 0x10;
	
	
	REN = 1;
	SM0 = 0x00;
	SM1 = 0x01;
	
	//TIMER3 SETTINGS
	T3CON = 0x81;
	T3FD = 0x20;
	
	//INTERRUPT PRIORITY
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
		getParameters();
	};
	
}