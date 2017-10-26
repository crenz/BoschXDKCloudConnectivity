/*
* Licensee agrees that the example code provided to Licensee has been developed and released by Bosch solely as an example to be used as a potential reference for Licensee�s application development. 
* Fitness and suitability of the example code for any use within Licensee�s applications need to be verified by Licensee on its own authority by taking appropriate state of the art actions and measures (e.g. by means of quality assurance measures).
* Licensee shall be responsible for conducting the development of its applications as well as integration of parts of the example code into such applications, taking into account the state of the art of technology and any statutory regulations and provisions applicable for such applications. Compliance with the functional system requirements and testing there of (including validation of information/data security aspects and functional safety) and release shall be solely incumbent upon Licensee. 
* For the avoidance of doubt, Licensee shall be responsible and fully liable for the applications and any distribution of such applications into the market.
* 
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are 
* met:
* 
*     (1) Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer. 
* 
*     (2) Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.  
*     
*     (3)The name of the author may not be used to
*     endorse or promote products derived from this software without
*     specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
*  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
*  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
*  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
*  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
*  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
*  POSSIBILITY OF SUCH DAMAGE.
*/


/* module includes ********************************************************** */

/* standard header files */

/* additional interface header files */
#include "BCDS_BSP_Board.h"
#include "stdio.h"
/* constant definitions ***************************************************** */

/*System Handler Control and State Register */
#define SYSHND_CTRL (*(volatile unsigned int*) (0xE000ED24u))
/*Memory Management Fault Status Register */
#define NVIC_MFSR (*(volatile unsigned char*) (0xE000ED28u))
/*Bus Fault Status Register */
#define NVIC_BFSR (*(volatile unsigned char*) (0xE000ED29u))
/* Usage Fault Status Register */
#define NVIC_UFSR (*(volatile unsigned short*)(0xE000ED2Au))
/*Hard Fault Status Register */
#define NVIC_HFSR (*(volatile unsigned int*) (0xE000ED2Cu))
/* Debug Fault Status Register */
#define NVIC_DFSR (*(volatile unsigned int*) (0xE000ED30u))
/* Bus Fault Manage Address Register */
#define NVIC_BFAR (*(volatile unsigned int*) (0xE000ED38u))
/* Auxiliary Fault Status Register */
#define NVIC_AFSR (*(volatile unsigned int*) (0xE000ED3Cu))


/* local variables ********************************************************** */

/* global variables ********************************************************* */


static volatile unsigned int _Continue; /* Set this variable to 1 to run further */

static struct {
	struct {
		volatile unsigned int r0; /* Register R0 */
		volatile unsigned int r1; /* Register R1*/
		volatile unsigned int r2; /* Register R2*/
		volatile unsigned int r3; /* Register R3*/
		volatile unsigned int r12; /* Register R12*/
		volatile unsigned int lr; /* Link register*/
		volatile unsigned int pc; /*Program counter*/
		union {
			volatile unsigned int byte;
			struct {
				unsigned int IPSR :8; /*Interrupt Program Status register (IPSR) */
				unsigned int EPSR :19; /* Execution Program Status register (EPSR)*/
				unsigned int APSR :5; /* Application Program Status register (APSR)*/
			} bits;
		} psr; /* Program status register.*/
	} SavedRegs;
	union {
		volatile unsigned int byte;
		struct {
			unsigned int MEMFAULTACT :1; /* Read as 1 if memory management fault is active */
			unsigned int BUSFAULTACT :1; /* Read as 1 if bus fault exception is active */
			unsigned int UnusedBits1 :1;
			unsigned int USGFAULTACT :1; /*Read as 1 if usage fault exception is active*/
			unsigned int UnusedBits2 :3;
			unsigned int SVCALLACT :1; /* Read as 1 if SVC exception is active*/
			unsigned int MONITORACT :1; /* Read as 1 if debug monitor exception is active*/
			unsigned int UnusedBits3 :1;
			unsigned int PENDSVACT :1; /* Read as 1 if PendSV exception is active*/
			unsigned int SYSTICKACT :1; /* Read as 1 if SYSTICK exception is active*/
			unsigned int USGFAULTPENDED :1; /*Usage fault pended; usage fault started but was replaced by a higher-priority exception*/
			unsigned int MEMFAULTPENDED :1; /* Memory management fault pended; memory management fault started but was replaced by a higher-priority exception */
			unsigned int BUSFAULTPENDED :1; /* Bus fault pended; bus fault handler was started but was replaced by a higher-priority exception */
			unsigned int SVCALLPENDED :1; /* SVC pended; SVC was started but was replaced by a higher-priority exception */
			unsigned int MEMFAULTENA :1; /* Memory management fault handler enable */
			unsigned int BUSFAULTENA :1; /*Bus fault handler enable */
			unsigned int USGFAULTENA :1; /*Usage fault handler enable */
		} bits;
	} syshndctrl; /*System Handler Control and State Register (0xE000ED24)*/

	union {
		volatile unsigned char byte;
		struct {
			unsigned char IACCVIOL :1; /* Instruction access violation */
			unsigned char DACCVIOL :1; /* Data access violation */
			unsigned char UnusedBits :1;
			unsigned char MUNSTKERR :1; /* Unstacking error */
			unsigned char MSTKERR :1; /* Stacking error */
			unsigned char UnusedBits2 :2;
			unsigned char MMARVALID :1; /*Indicates the MMAR is valid */
		} bits;
	} mfsr; /* Memory Management Fault Status Register (0xE000ED28) */

	union {
		volatile unsigned int byte;
		struct {
			unsigned int IBUSERR :1; /* Instruction access violation  */
			unsigned int PRECISERR :1; /* Precise data access violation  */
			unsigned int IMPREISERR :1; /* Imprecise data access violation  */
			unsigned int UNSTKERR :1; /* Unstacking error  */
			unsigned int STKERR :1; /* Stacking error  */
			unsigned int UnusedBits :2;
			unsigned int BFARVALID :1; /*Indicates BFAR is valid  */
		} bits;
	} bfsr; /* Bus Fault Status Register (0xE000ED29)  */

	volatile unsigned int bfar; /*Bus Fault Manage Address Register   (0xE000ED38) */

	union {
		volatile unsigned short byte;
		struct {
			unsigned short UNDEFINSTR :1; /* Attempts to execute an undefined instruction */
			unsigned short INVSTATE :1; /* Attempts to switch to an invalid state (e.g., ARM)*/
			unsigned short INVPC :1; /* Attempts to do an exception with a bad value in the EXC_RETURN number */
			unsigned short NOCP :1; /*Attempts to execute a coprocessor instruction */
			unsigned short UnusedBits :4;
			unsigned short UNALIGNED :1; /* Indicates that an unaligned access fault has taken place */
			unsigned short DIVBYZERO :1; /* Indicates a divide by zero has taken place (can be set only if DIV_0_TRP is set) */

		} bits;
	} ufsr; /*Usage Fault Status Register (0xE000ED2A) */
	union {
		volatile unsigned int byte;
		struct {
			unsigned int UnusedBits :1;
			unsigned int VECTBL :1; /* Indicates hard fault is caused by failed vector fetch */
			unsigned int UnusedBits2 :28;
			unsigned int FORCED :1; /*Indicates hard fault is taken because of bus fault/memory management fault/usage fault*/
			unsigned int DEBUGEVT :1; /* Indicates hard fault is triggered by debug event*/
		} bits;
	} hfsr; /*Hard Fault Status Register (0xE000ED2C) */
	union {
		volatile unsigned int byte;
		struct {
			unsigned int HALTED :1; /* Halt requested in NVIC */
			unsigned int BKPT :1; /* BKPT instruction executed */
			unsigned int DWTTRAP :1; /* DWT match occurred */
			unsigned int VCATCH :1; /* Vector fetch occurred */
			unsigned int EXTERNAL :1; /* EDBGRQ signal asserted */
		} bits;
	} dfsr; /*Debug Fault Status Register (0xE000ED30) */
	volatile unsigned int afsr; /*Auxiliary Fault Status Register (0xE000ED3C) Vendor controlled (optional) */
}HardFaultRegs;



/* inline functions ********************************************************* */

/* local functions ********************************************************** */

static void extractFaultInfo(unsigned int* pStack);
/* global functions ********************************************************* */

/** Debug function which is entered whenever an unrecoverable system fault occurs.
 *
 * @param pulFaultStackAddress Pointer to the saved fault stack.
 */



/** Hard Fault exception handler */

__attribute__((naked))void HardFault_Handler(void)
{
    __asm volatile
    (
            " tst lr, #4                                                \n"
            " ite eq                                                    \n"
            " mrseq r0, msp                                             \n"
            " mrsne r0, psp                                             \n"
            " ldr r2, handler2_address_const                            \n"
            " bx r2                                                     \n"
            " handler2_address_const: .word Hard_Fault_Handler    \n"
    );
}



/** Usage Fault exception handler */
__attribute__((naked)) void UsageFault_Handler(void)
{
    __asm volatile
    (
            " tst lr, #4                                                \n"
            " ite eq                                                    \n"
            " mrseq r0, msp                                             \n"
            " mrsne r0, psp                                             \n"
            " ldr r2, handler2_address_const1                            \n"
            " bx r2                                                     \n"
            " handler2_address_const1: .word Usage_Fault_Handler    \n"
    );
}


/** BUS Fault exception handler */
__attribute__((naked)) void BusFault_Handler(void)
{
    __asm volatile
    (
            " tst lr, #4                                                \n"
            " ite eq                                                    \n"
            " mrseq r0, msp                                             \n"
            " mrsne r0, psp                                             \n"
            " ldr r2, handler2_address_const2                            \n"
            " bx r2                                                     \n"
            " handler2_address_const2: .word Bus_Fault_Handler    \n"
    );
}

/** Memory Manager Fault exception handler */
__attribute__((naked))void MemManage_Handler(void)
{
    __asm volatile
    (
            " tst lr, #4                                                \n"
            " ite eq                                                    \n"
            " mrseq r0, msp                                             \n"
            " mrsne r0, psp                                             \n"
            " ldr r2, handler2_address_const3                            \n"
            " bx r2                                                     \n"
            " handler2_address_const3: .word Mem_Fault_Handler    \n"
    );
}

void Hard_Fault_Handler(unsigned int *pulHardFaultStackAddress)
{
	extractFaultInfo(pulHardFaultStackAddress);
}

void Usage_Fault_Handler(unsigned int *pulUsageFaultStackAddress)
{
	extractFaultInfo(pulUsageFaultStackAddress);
}

void Bus_Fault_Handler(unsigned int *pulBusFaultStackAddress)
{
	extractFaultInfo(pulBusFaultStackAddress);
}

void Mem_Fault_Handler(unsigned int *pulMemFaultStackAddress)
{
	extractFaultInfo(pulMemFaultStackAddress);
}


static void extractFaultInfo(unsigned int* pStack)
{
	printf("HardFault_Occured_going to reset the processor  \r\n");

/* Read NVIC registers */

	HardFaultRegs.syshndctrl.byte = SYSHND_CTRL; /* System Handler Control and State Register */
	printf("syshndctrl_register_byteValue %d \r\n",HardFaultRegs.syshndctrl.byte);

	HardFaultRegs.mfsr.byte = NVIC_MFSR; /* Memory Fault Status Register */
	printf("HardFaultRegs.mfsr.byte Value %d \r\n",HardFaultRegs.mfsr.byte);

	HardFaultRegs.bfsr.byte = NVIC_BFSR; /* Bus Fault Status Register*/
	printf(" HardFaultRegs.bfsr.byte Value %d \r\n",HardFaultRegs.bfsr.byte);

	HardFaultRegs.bfar = NVIC_BFAR; /* Bus Fault Manage Address Register*/
	printf(" HardFaultRegs.bfar Value %d \r\n",HardFaultRegs.bfar);

	HardFaultRegs.ufsr.byte = NVIC_UFSR; /*Usage Fault Status Register*/
	printf(" HardFaultRegs.ufsr.byte Value %d \r\n",HardFaultRegs.ufsr.byte);

	HardFaultRegs.hfsr.byte = NVIC_HFSR; /* Hard Fault Status Register*/
	printf(" HardFaultRegs.hfsr.byte Value %d \r\n",HardFaultRegs.hfsr.byte);

	HardFaultRegs.dfsr.byte = NVIC_DFSR; /* Debug Fault Status Register*/
	printf(" HardFaultRegs.dfsr.byte Value %d \r\n",HardFaultRegs.dfsr.byte);

	HardFaultRegs.afsr = NVIC_AFSR; /* Auxiliary Fault Status Register*/
	printf(" HardFaultRegs.afsr Value %d \r\n",HardFaultRegs.afsr);

/* Read saved registers from the stack*/
	HardFaultRegs.SavedRegs.r0 = pStack[0]; /* Register R0*/
	HardFaultRegs.SavedRegs.r1 = pStack[1]; /* Register R1 */
	HardFaultRegs.SavedRegs.r2 = pStack[2]; /* Register R2*/
	HardFaultRegs.SavedRegs.r3 = pStack[3]; /* Register R3 */
	HardFaultRegs.SavedRegs.r12 = pStack[4]; /* Register R12 */
	HardFaultRegs.SavedRegs.lr = pStack[5]; /* Link register LR */
	printf(" HardFaultRegs.SavedRegs.lr Value %d \r\n",HardFaultRegs.SavedRegs.lr);
	HardFaultRegs.SavedRegs.pc = pStack[6]; /*Program counter PC */
	printf(" HardFaultRegs.SavedRegs.pc Value %d \r\n",HardFaultRegs.SavedRegs.pc);
	HardFaultRegs.SavedRegs.psr.byte = pStack[7]; /* Program status word PSR */
	printf(" HardFaultRegs.SavedRegs.psr.byte Value %d \r\n",HardFaultRegs.SavedRegs.psr.byte);
	BSP_Board_SoftReset();
}

/** ************************************************************************* */
