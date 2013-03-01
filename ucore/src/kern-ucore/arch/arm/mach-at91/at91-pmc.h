#ifndef ARCH_ARM_MACH_AT91_PMC_H
#define ARCH_ARM_MACH_AT91_PMC_H

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Power Management Controler
// *****************************************************************************
// *** Register offset in AT91S_PMC structure ***
#define PMC_SCER        ( 0)	// System Clock Enable Register
#define PMC_SCDR        ( 4)	// System Clock Disable Register
#define PMC_SCSR        ( 8)	// System Clock Status Register
#define PMC_PCER        (16)	// Peripheral Clock Enable Register
#define PMC_PCDR        (20)	// Peripheral Clock Disable Register
#define PMC_PCSR        (24)	// Peripheral Clock Status Register
#define PMC_UCKR        (28)	// UTMI Clock Configuration Register
#define PMC_MOR         (32)	// Main Oscillator Register
#define PMC_MCFR        (36)	// Main Clock  Frequency Register
#define PMC_PLLAR       (40)	// PLL A Register
#define PMC_PLLBR       (44)	// PLL B Register
#define PMC_MCKR        (48)	// Master Clock Register
#define PMC_PCKR        (64)	// Programmable Clock Register
#define PMC_IER         (96)	// Interrupt Enable Register
#define PMC_IDR         (100)	// Interrupt Disable Register
#define PMC_SR          (104)	// Status Register
#define PMC_IMR         (108)	// Interrupt Mask Register

#define		AT91_PMC_MDIV		(3 <<  8)	/* Master Clock Division */
#define			AT91SAM9_PMC_MDIV_3		(3 << 8)	/* [some SAM9 only] */
#define		AT91_PMC_PLLADIV2	(1 << 12)	/* PLLA divisor by 2 [some SAM9 only] */
#endif
