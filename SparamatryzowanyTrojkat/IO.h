#ifndef _IO_H_
#define _IO_H_
/**
 * @author Dominik Luczak
 * @date 2016-03-09
 * @brief IO.
 */

//input/output macro definitions
#define BitSet(variable, bit_number) 			variable|=0x0001<<bit_number 			//set 1 to bit in variable
#define BitClear(variable, bit_number) 		variable&=~(0x0001<<bit_number) 	//set 0 to bit in variable
#define BitCheck(variable, bit_number) 		(variable&(0x0001<<bit_number)) 	//checked bit in variable - bit=0->0/false or bit=1->non-zero/true
#define BitToggle(variable, bit_number) 	variable^=0x0001<<bit_number 			//toggle bit in variable

#endif
