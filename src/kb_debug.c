/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // naniBox.com

    This file is part of kuroBox / naniBox.

    kuroBox / naniBox is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    kuroBox / naniBox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <ch.h>
#include <hal.h>
#include "chprintf.h"
#include "kb_serial.h"

/*
void **HARDFAULT_PSP;
register void *stack_pointer asm("sp");

void HardFaultVector(void)
{
    // Hijack the process stack pointer to make backtrace work
    asm("mrs %0, psp" : "=r"(HARDFAULT_PSP) : :);
    stack_pointer = HARDFAULT_PSP;
    while(1);
}
*/

// From Joseph Yiu, minor edits by FVH
// hard fault handler in C,
// with stack frame location as input parameter
// called from HardFault_Handler in file xxx.s
void hard_fault_handler_c (unsigned int * hardfault_args)
{
	unsigned int stacked_r0;
	unsigned int stacked_r1;
	unsigned int stacked_r2;
	unsigned int stacked_r3;
	unsigned int stacked_r12;
	unsigned int stacked_lr;
	unsigned int stacked_pc;
	unsigned int stacked_psr;

	stacked_r0 = ((unsigned long) hardfault_args[0]);
	stacked_r1 = ((unsigned long) hardfault_args[1]);
	stacked_r2 = ((unsigned long) hardfault_args[2]);
	stacked_r3 = ((unsigned long) hardfault_args[3]);

	stacked_r12 = ((unsigned long) hardfault_args[4]);
	stacked_lr = ((unsigned long) hardfault_args[5]);
	stacked_pc = ((unsigned long) hardfault_args[6]);
	stacked_psr = ((unsigned long) hardfault_args[7]);

	chprintf(DEBG, "\n\n[Hard fault handler - all numbers in hex]\n");
	chprintf(DEBG, "R0 = %x\n", stacked_r0);
	chprintf(DEBG, "R1 = %x\n", stacked_r1);
	chprintf(DEBG, "R2 = %x\n", stacked_r2);
	chprintf(DEBG, "R3 = %x\n", stacked_r3);
	chprintf(DEBG, "R12 = %x\n", stacked_r12);
	chprintf(DEBG, "LR [R14] = %x  subroutine call return address\n", stacked_lr);
	chprintf(DEBG, "PC [R15] = %x  program counter\n", stacked_pc);
	chprintf(DEBG, "PSR = %x\n", stacked_psr);
	chprintf(DEBG, "BFAR = %x\n", (*((volatile unsigned long *)(0xE000ED38))));
	chprintf(DEBG, "CFSR = %x\n", (*((volatile unsigned long *)(0xE000ED28))));
	chprintf(DEBG, "HFSR = %x\n", (*((volatile unsigned long *)(0xE000ED2C))));
	chprintf(DEBG, "DFSR = %x\n", (*((volatile unsigned long *)(0xE000ED30))));
	chprintf(DEBG, "AFSR = %x\n", (*((volatile unsigned long *)(0xE000ED3C))));
	chprintf(DEBG, "SCB_SHCSR = %x\n", SCB->SHCSR);
	while (1);
}
