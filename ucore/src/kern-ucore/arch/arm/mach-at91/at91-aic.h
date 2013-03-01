/*
 * =====================================================================================
 *
 *       Filename:  at91_aic.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2012 12:31:45 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __AT91_AIC_H
#define __AT91_AIC_H
// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Advanced Interrupt Controller
// *****************************************************************************
// *** Register offset in AT91S_AIC structure ***
#define AIC_SMR         ( 0)	// Source Mode Register
#define AIC_SVR         (128)	// Source Vector Register
#define AIC_IVR         (256)	// IRQ Vector Register
#define AIC_FVR         (260)	// FIQ Vector Register
#define AIC_ISR         (264)	// Interrupt Status Register
#define AIC_IPR         (268)	// Interrupt Pending Register
#define AIC_IMR         (272)	// Interrupt Mask Register
#define AIC_CISR        (276)	// Core Interrupt Status Register
#define AIC_IECR        (288)	// Interrupt Enable Command Register
#define AIC_IDCR        (292)	// Interrupt Disable Command Register
#define AIC_ICCR        (296)	// Interrupt Clear Command Register
#define AIC_ISCR        (300)	// Interrupt Set Command Register
#define AIC_EOICR       (304)	// End of Interrupt Command Register
#define AIC_SPU         (308)	// Spurious Vector Register
#define AIC_DCR         (312)	// Debug Control Register (Protect)
#define AIC_FFER        (320)	// Fast Forcing Enable Register
#define AIC_FFDR        (324)	// Fast Forcing Disable Register
#define AIC_FFSR        (328)	// Fast Forcing Status Register
// -------- AIC_SMR : (AIC Offset: 0x0) Control Register -------- 
#define AT91C_AIC_PRIOR           (0x7 <<  0)	// (AIC) Priority Level
#define 	AT91C_AIC_PRIOR_LOWEST               (0x0)	// (AIC) Lowest priority level
#define 	AT91C_AIC_PRIOR_HIGHEST              (0x7)	// (AIC) Highest priority level
#define AT91C_AIC_SRCTYPE         (0x3 <<  5)	// (AIC) Interrupt Source Type
#define 	AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE  (0x0 <<  5)	// (AIC) Internal Sources Code Label Level Sensitive
#define 	AT91C_AIC_SRCTYPE_INT_EDGE_TRIGGERED   (0x1 <<  5)	// (AIC) Internal Sources Code Label Edge triggered
#define 	AT91C_AIC_SRCTYPE_EXT_HIGH_LEVEL       (0x2 <<  5)	// (AIC) External Sources Code Label High-level Sensitive
#define 	AT91C_AIC_SRCTYPE_EXT_POSITIVE_EDGE    (0x3 <<  5)	// (AIC) External Sources Code Label Positive Edge triggered
// -------- AIC_CISR : (AIC Offset: 0x114) AIC Core Interrupt Status Register -------- 
#define AT91C_AIC_NFIQ            (0x1 <<  0)	// (AIC) NFIQ Status
#define AT91C_AIC_NIRQ            (0x1 <<  1)	// (AIC) NIRQ Status
// -------- AIC_DCR : (AIC Offset: 0x138) AIC Debug Control Register (Protect) -------- 
#define AT91C_AIC_DCR_PROT        (0x1 <<  0)	// (AIC) Protection Mode
#define AT91C_AIC_DCR_GMSK        (0x1 <<  1)	// (AIC) General Mask

#define AT91C_SYSC_PITIEN     ((unsigned int) 0x1 << 25)	// (PITC) Periodic Interval Timer Interrupt Enable
#define AT91C_SYSC_PITEN      ((unsigned int) 0x1 << 24)	// (PITC) Periodic Interval Timer Enabled

#endif
