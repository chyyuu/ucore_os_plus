
//  ----------------------------------------------------------------------------
//          ATMEL Microcontroller Software Support  -  ROUSSET  -
//  ----------------------------------------------------------------------------
//  DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
//  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
//  DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//  OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
//  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  ----------------------------------------------------------------------------

#ifndef ARCH_ARM_MACH_AT91_GPIO_H
#define ARCH_ARM_MACH_AT91_GPIO_H

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Parallel Input Output Controler
// *****************************************************************************
// *** Register offset in AT91S_PIO structure ***
#define PIO_PER         ( 0)	// PIO Enable Register
#define PIO_PDR         ( 4)	// PIO Disable Register
#define PIO_PSR         ( 8)	// PIO Status Register
#define PIO_OER         (16)	// Output Enable Register
#define PIO_ODR         (20)	// Output Disable Registerr
#define PIO_OSR         (24)	// Output Status Register
#define PIO_IFER        (32)	// Input Filter Enable Register
#define PIO_IFDR        (36)	// Input Filter Disable Register
#define PIO_IFSR        (40)	// Input Filter Status Register
#define PIO_SODR        (48)	// Set Output Data Register
#define PIO_CODR        (52)	// Clear Output Data Register
#define PIO_ODSR        (56)	// Output Data Status Register
#define PIO_PDSR        (60)	// Pin Data Status Register
#define PIO_IER         (64)	// Interrupt Enable Register
#define PIO_IDR         (68)	// Interrupt Disable Register
#define PIO_IMR         (72)	// Interrupt Mask Register
#define PIO_ISR         (76)	// Interrupt Status Register
#define PIO_MDER        (80)	// Multi-driver Enable Register
#define PIO_MDDR        (84)	// Multi-driver Disable Register
#define PIO_MDSR        (88)	// Multi-driver Status Register
#define PIO_PPUDR       (96)	// Pull-up Disable Register
#define PIO_PPUER       (100)	// Pull-up Enable Register
#define PIO_PPUSR       (104)	// Pull-up Status Register
#define PIO_ASR         (112)	// Select A Register
#define PIO_BSR         (116)	// Select B Register
#define PIO_ABSR        (120)	// AB Select Status Register
#define PIO_OWER        (160)	// Output Write Enable Register
#define PIO_OWDR        (164)	// Output Write Disable Register
#define PIO_OWSR        (168)	// Output Write Status Register

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Clock Generator Controler
// *****************************************************************************
// *** Register offset in AT91S_CKGR structure ***
#define CKGR_UCKR       ( 0)	// UTMI Clock Configuration Register
#define CKGR_MOR        ( 4)	// Main Oscillator Register
#define CKGR_MCFR       ( 8)	// Main Clock  Frequency Register
#define CKGR_PLLAR      (12)	// PLL A Register
#define CKGR_PLLBR      (16)	// PLL B Register
// -------- CKGR_UCKR : (CKGR Offset: 0x0) UTMI Clock Configuration Register -------- 
#define AT91C_CKGR_UPLLEN         (0x1 << 16)	// (CKGR) UTMI PLL Enable
#define 	AT91C_CKGR_UPLLEN_DISABLED             (0x0 << 16)	// (CKGR) The UTMI PLL is disabled
#define 	AT91C_CKGR_UPLLEN_ENABLED              (0x1 << 16)	// (CKGR) The UTMI PLL is enabled
#define AT91C_CKGR_PLLCOUNT       (0xF << 20)	// (CKGR) UTMI Oscillator Start-up Time
#define AT91C_CKGR_BIASEN         (0x1 << 24)	// (CKGR) UTMI BIAS Enable
#define 	AT91C_CKGR_BIASEN_DISABLED             (0x0 << 24)	// (CKGR) The UTMI BIAS is disabled
#define 	AT91C_CKGR_BIASEN_ENABLED              (0x1 << 24)	// (CKGR) The UTMI BIAS is enabled
#define AT91C_CKGR_BIASCOUNT      (0xF << 28)	// (CKGR) UTMI BIAS Start-up Time
// -------- CKGR_MOR : (CKGR Offset: 0x4) Main Oscillator Register -------- 
#define AT91C_CKGR_MOSCEN         (0x1 <<  0)	// (CKGR) Main Oscillator Enable
#define AT91C_CKGR_OSCBYPASS      (0x1 <<  1)	// (CKGR) Main Oscillator Bypass
#define AT91C_CKGR_OSCOUNT        (0xFF <<  8)	// (CKGR) Main Oscillator Start-up Time
// -------- CKGR_MCFR : (CKGR Offset: 0x8) Main Clock Frequency Register -------- 
#define AT91C_CKGR_MAINF          (0xFFFF <<  0)	// (CKGR) Main Clock Frequency
#define AT91C_CKGR_MAINRDY        (0x1 << 16)	// (CKGR) Main Clock Ready
// -------- CKGR_PLLAR : (CKGR Offset: 0xc) PLL A Register -------- 
#define AT91C_CKGR_DIVA           (0xFF <<  0)	// (CKGR) Divider A Selected
#define 	AT91C_CKGR_DIVA_0                    (0x0)	// (CKGR) Divider A output is 0
#define 	AT91C_CKGR_DIVA_BYPASS               (0x1)	// (CKGR) Divider A is bypassed
#define AT91C_CKGR_PLLACOUNT      (0x3F <<  8)	// (CKGR) PLL A Counter
#define AT91C_CKGR_OUTA           (0x3 << 14)	// (CKGR) PLL A Output Frequency Range
#define 	AT91C_CKGR_OUTA_0                    (0x0 << 14)	// (CKGR) Please refer to the PLLA datasheet
#define 	AT91C_CKGR_OUTA_1                    (0x1 << 14)	// (CKGR) Please refer to the PLLA datasheet
#define 	AT91C_CKGR_OUTA_2                    (0x2 << 14)	// (CKGR) Please refer to the PLLA datasheet
#define 	AT91C_CKGR_OUTA_3                    (0x3 << 14)	// (CKGR) Please refer to the PLLA datasheet
#define AT91C_CKGR_MULA           (0x7FF << 16)	// (CKGR) PLL A Multiplier
#define AT91C_CKGR_SRCA           (0x1 << 29)	// (CKGR)
// -------- CKGR_PLLBR : (CKGR Offset: 0x10) PLL B Register -------- 
#define AT91C_CKGR_DIVB           (0xFF <<  0)	// (CKGR) Divider B Selected
#define 	AT91C_CKGR_DIVB_0                    (0x0)	// (CKGR) Divider B output is 0
#define 	AT91C_CKGR_DIVB_BYPASS               (0x1)	// (CKGR) Divider B is bypassed
#define AT91C_CKGR_PLLBCOUNT      (0x3F <<  8)	// (CKGR) PLL B Counter
#define AT91C_CKGR_OUTB           (0x3 << 14)	// (CKGR) PLL B Output Frequency Range
#define 	AT91C_CKGR_OUTB_0                    (0x0 << 14)	// (CKGR) Please refer to the PLLB datasheet
#define 	AT91C_CKGR_OUTB_1                    (0x1 << 14)	// (CKGR) Please refer to the PLLB datasheet
#define 	AT91C_CKGR_OUTB_2                    (0x2 << 14)	// (CKGR) Please refer to the PLLB datasheet
#define 	AT91C_CKGR_OUTB_3                    (0x3 << 14)	// (CKGR) Please refer to the PLLB datasheet
#define AT91C_CKGR_MULB           (0x7FF << 16)	// (CKGR) PLL B Multiplier
#define AT91C_CKGR_USBDIV         (0x3 << 28)	// (CKGR) Divider for USB Clocks
#define 	AT91C_CKGR_USBDIV_0                    (0x0 << 28)	// (CKGR) Divider output is PLL clock output
#define 	AT91C_CKGR_USBDIV_1                    (0x1 << 28)	// (CKGR) Divider output is PLL clock output divided by 2
#define 	AT91C_CKGR_USBDIV_2                    (0x2 << 28)	// (CKGR) Divider output is PLL clock output divided by 4

#define AT91C_NR_PIO               (32 * 4)

/* I/O type */
enum pio_type {
	PIO_PERIPH_A,
	PIO_PERIPH_B,
	PIO_INPUT,
	PIO_OUTPUT
};

/* I/O attributes */
#define PIO_DEFAULT   (0 << 0)
#define PIO_PULLUP    (1 << 0)
#define PIO_DEGLITCH  (1 << 1)
#define PIO_OPENDRAIN (1 << 2)

struct pio_desc {
	const char *pin_name;	/* Pin Name */
	unsigned int pin_num;	/* Pin number */
	unsigned int dft_value;	/* Default value for outputs */
	unsigned char attribute;
	enum pio_type type;
};

/* pio_set_value: assuming the pin is muxed as a gpio output, set its value. */
extern int pio_set_value(unsigned pin, int value);

/* pio_get_value: read the pin's value (works even if it's not muxed as a gpio).
 * !!! PIO Clock must be enabled in the PMC !!! */
extern int pio_get_value(unsigned pin);

/* pio_device_pio_setup: Configure PIO in periph mode according to the platform informations */
extern int pio_setup(const struct pio_desc *pio_desc);

/* these pin numbers double as IRQ numbers, like AT91C_ID_* values */
#define PIO_NB_IO		32	/* Number of IO handled by one PIO controller */
#define	AT91C_PIN_PA(io)	(0 * PIO_NB_IO + io)
#define	AT91C_PIN_PB(io)	(1 * PIO_NB_IO + io)
#define	AT91C_PIN_PC(io)	(2 * PIO_NB_IO + io)
#define	AT91C_PIN_PD(io)	(3 * PIO_NB_IO + io)
#define	AT91C_PIN_PE(io)	(4 * PIO_NB_IO + io)

// *****************************************************************************
//               PIO DEFINITIONS FOR AT91CAP9
// *****************************************************************************
#define AT91C_PIO_PA0             (1 <<  0)	// Pin Controlled by PA0
#define AT91C_PA0_MCI0_DA0        (AT91C_PIO_PA0)	//
#define AT91C_PA0_SPI0_MISO       (AT91C_PIO_PA0)	//
#define AT91C_PIO_PA1             (1 <<  1)	// Pin Controlled by PA1
#define AT91C_PA1_MCI0_CDA        (AT91C_PIO_PA1)	//
#define AT91C_PA1_SPI0_MOSI       (AT91C_PIO_PA1)	//
#define AT91C_PIO_PA10            (1 << 10)	// Pin Controlled by PA10
#define AT91C_PA10_IRQ0           (AT91C_PIO_PA10)	//
#define AT91C_PA10_PWM1           (AT91C_PIO_PA10)	//
#define AT91C_PIO_PA11            (1 << 11)	// Pin Controlled by PA11
#define AT91C_PA11_DMARQ0         (AT91C_PIO_PA11)	//
#define AT91C_PA11_PWM3           (AT91C_PIO_PA11)	//
#define AT91C_PIO_PA12            (1 << 12)	// Pin Controlled by PA12
#define AT91C_PA12_CANTX          (AT91C_PIO_PA12)	//
#define AT91C_PA12_PCK0           (AT91C_PIO_PA12)	//
#define AT91C_PIO_PA13            (1 << 13)	// Pin Controlled by PA13
#define AT91C_PA13_CANRX          (AT91C_PIO_PA13)	//
#define AT91C_PIO_PA14            (1 << 14)	// Pin Controlled by PA14
#define AT91C_PA14_TCLK2          (AT91C_PIO_PA14)	//
#define AT91C_PA14_IRQ1           (AT91C_PIO_PA14)	//
#define AT91C_PIO_PA15            (1 << 15)	// Pin Controlled by PA15
#define AT91C_PA15_DMARQ3         (AT91C_PIO_PA15)	//
#define AT91C_PA15_PCK2           (AT91C_PIO_PA15)	//
#define AT91C_PIO_PA16            (1 << 16)	// Pin Controlled by PA16
#define AT91C_PA16_MCI1_CK        (AT91C_PIO_PA16)	//
#define AT91C_PA16_ISI_D0         (AT91C_PIO_PA16)	//
#define AT91C_PIO_PA17            (1 << 17)	// Pin Controlled by PA17
#define AT91C_PA17_MCI1_CDA       (AT91C_PIO_PA17)	//
#define AT91C_PA17_ISI_D1         (AT91C_PIO_PA17)	//
#define AT91C_PIO_PA18            (1 << 18)	// Pin Controlled by PA18
#define AT91C_PA18_MCI1_DA0       (AT91C_PIO_PA18)	//
#define AT91C_PA18_ISI_D2         (AT91C_PIO_PA18)	//
#define AT91C_PIO_PA19            (1 << 19)	// Pin Controlled by PA19
#define AT91C_PA19_MCI1_DA1       (AT91C_PIO_PA19)	//
#define AT91C_PA19_ISI_D3         (AT91C_PIO_PA19)	//
#define AT91C_PIO_PA2             (1 <<  2)	// Pin Controlled by PA2
#define AT91C_PA2_MCI0_CK         (AT91C_PIO_PA2)	//
#define AT91C_PA2_SPI0_SPCK       (AT91C_PIO_PA2)	//
#define AT91C_PIO_PA20            (1 << 20)	// Pin Controlled by PA20
#define AT91C_PA20_MCI1_DA2       (AT91C_PIO_PA20)	//
#define AT91C_PA20_ISI_D4         (AT91C_PIO_PA20)	//
#define AT91C_PIO_PA21            (1 << 21)	// Pin Controlled by PA21
#define AT91C_PA21_MCI1_DA3       (AT91C_PIO_PA21)	//
#define AT91C_PA21_ISI_D5         (AT91C_PIO_PA21)	//
#define AT91C_PIO_PA22            (1 << 22)	// Pin Controlled by PA22
#define AT91C_PA22_TXD0           (AT91C_PIO_PA22)	//
#define AT91C_PA22_ISI_D6         (AT91C_PIO_PA22)	//
#define AT91C_PIO_PA23            (1 << 23)	// Pin Controlled by PA23
#define AT91C_PA23_RXD0           (AT91C_PIO_PA23)	//
#define AT91C_PA23_ISI_D7         (AT91C_PIO_PA23)	//
#define AT91C_PIO_PA24            (1 << 24)	// Pin Controlled by PA24
#define AT91C_PA24_RTS0           (AT91C_PIO_PA24)	//
#define AT91C_PA24_ISI_PCK        (AT91C_PIO_PA24)	//
#define AT91C_PIO_PA25            (1 << 25)	// Pin Controlled by PA25
#define AT91C_PA25_CTS0           (AT91C_PIO_PA25)	//
#define AT91C_PA25_ISI_HSYNC      (AT91C_PIO_PA25)	//
#define AT91C_PIO_PA26            (1 << 26)	// Pin Controlled by PA26
#define AT91C_PA26_SCK0           (AT91C_PIO_PA26)	//
#define AT91C_PA26_ISI_VSYNC      (AT91C_PIO_PA26)	//
#define AT91C_PIO_PA27            (1 << 27)	// Pin Controlled by PA27
#define AT91C_PA27_PCK1           (AT91C_PIO_PA27)	//
#define AT91C_PA27_ISI_MCK        (AT91C_PIO_PA27)	//
#define AT91C_PIO_PA28            (1 << 28)	// Pin Controlled by PA28
#define AT91C_PA28_SPI0_NPCS3A    (AT91C_PIO_PA28)	//
#define AT91C_PA28_ISI_D8         (AT91C_PIO_PA28)	//
#define AT91C_PIO_PA29            (1 << 29)	// Pin Controlled by PA29
#define AT91C_PA29_TIOA0          (AT91C_PIO_PA29)	//
#define AT91C_PA29_ISI_D9         (AT91C_PIO_PA29)	//
#define AT91C_PIO_PA3             (1 <<  3)	// Pin Controlled by PA3
#define AT91C_PA3_MCI0_DA1        (AT91C_PIO_PA3)	//
#define AT91C_PA3_SPI0_NPCS1      (AT91C_PIO_PA3)	//
#define AT91C_PIO_PA30            (1 << 30)	// Pin Controlled by PA30
#define AT91C_PA30_TIOB0          (AT91C_PIO_PA30)	//
#define AT91C_PA30_ISI_D10        (AT91C_PIO_PA30)	//
#define AT91C_PIO_PA31            (1 << 31)	// Pin Controlled by PA31
#define AT91C_PA31_DMARQ1         (AT91C_PIO_PA31)	//
#define AT91C_PA31_ISI_D11        (AT91C_PIO_PA31)	//
#define AT91C_PIO_PA4             (1 <<  4)	// Pin Controlled by PA4
#define AT91C_PA4_MCI0_DA2        (AT91C_PIO_PA4)	//
#define AT91C_PA4_SPI0_NPCS2A     (AT91C_PIO_PA4)	//
#define AT91C_PIO_PA5             (1 <<  5)	// Pin Controlled by PA5
#define AT91C_PA5_MCI0_DA3        (AT91C_PIO_PA5)	//
#define AT91C_PA5_SPI0_NPCS0      (AT91C_PIO_PA5)	//
#define AT91C_PIO_PA6             (1 <<  6)	// Pin Controlled by PA6
#define AT91C_PA6_AC97FS          (AT91C_PIO_PA6)	//
#define AT91C_PIO_PA7             (1 <<  7)	// Pin Controlled by PA7
#define AT91C_PA7_AC97CK          (AT91C_PIO_PA7)	//
#define AT91C_PIO_PA8             (1 <<  8)	// Pin Controlled by PA8
#define AT91C_PA8_AC97TX          (AT91C_PIO_PA8)	//
#define AT91C_PIO_PA9             (1 <<  9)	// Pin Controlled by PA9
#define AT91C_PA9_AC97RX          (AT91C_PIO_PA9)	//
#define AT91C_PIO_PB0             (1 <<  0)	// Pin Controlled by PB0
#define AT91C_PB0_TF0             (AT91C_PIO_PB0)	//
#define AT91C_PIO_PB1             (1 <<  1)	// Pin Controlled by PB1
#define AT91C_PB1_TK0             (AT91C_PIO_PB1)	//
#define AT91C_PIO_PB10            (1 << 10)	// Pin Controlled by PB10
#define AT91C_PB10_RK1            (AT91C_PIO_PB10)	//
#define AT91C_PB10_PCK1           (AT91C_PIO_PB10)	//
#define AT91C_PIO_PB11            (1 << 11)	// Pin Controlled by PB11
#define AT91C_PB11_RF1            (AT91C_PIO_PB11)	//
#define AT91C_PIO_PB12            (1 << 12)	// Pin Controlled by PB12
#define AT91C_PB12_SPI1_MISO      (AT91C_PIO_PB12)	//
#define AT91C_PIO_PB13            (1 << 13)	// Pin Controlled by PB13
#define AT91C_PB13_SPI1_MOSI      (AT91C_PIO_PB13)	//
#define AT91C_PIO_PB14            (1 << 14)	// Pin Controlled by PB14
#define AT91C_PB14_SPI1_SPCK      (AT91C_PIO_PB14)	//
#define AT91C_PIO_PB15            (1 << 15)	// Pin Controlled by PB15
#define AT91C_PB15_SPI1_NPCS0     (AT91C_PIO_PB15)	//
#define AT91C_PIO_PB16            (1 << 16)	// Pin Controlled by PB16
#define AT91C_PB16_SPI1_NPCS1     (AT91C_PIO_PB16)	//
#define AT91C_PIO_PB17            (1 << 17)	// Pin Controlled by PB17
#define AT91C_PB17_SPI1_NPCS2B    (AT91C_PIO_PB17)	//
#define AT91C_PB17_AD0            (AT91C_PIO_PB17)	//
#define AT91C_PIO_PB18            (1 << 18)	// Pin Controlled by PB18
#define AT91C_PB18_SPI1_NPCS3B    (AT91C_PIO_PB18)	//
#define AT91C_PB18_AD1            (AT91C_PIO_PB18)	//
#define AT91C_PIO_PB19            (1 << 19)	// Pin Controlled by PB19
#define AT91C_PB19_PWM0           (AT91C_PIO_PB19)	//
#define AT91C_PB19_AD2            (AT91C_PIO_PB19)	//
#define AT91C_PIO_PB2             (1 <<  2)	// Pin Controlled by PB2
#define AT91C_PB2_TD0             (AT91C_PIO_PB2)	//
#define AT91C_PIO_PB20            (1 << 20)	// Pin Controlled by PB20
#define AT91C_PB20_PWM1           (AT91C_PIO_PB20)	//
#define AT91C_PB20_AD3            (AT91C_PIO_PB20)	//
#define AT91C_PIO_PB21            (1 << 21)	// Pin Controlled by PB21
#define AT91C_PB21_E_TXCK         (AT91C_PIO_PB21)	//
#define AT91C_PB21_TIOA2          (AT91C_PIO_PB21)	//
#define AT91C_PIO_PB22            (1 << 22)	// Pin Controlled by PB22
#define AT91C_PB22_E_RXDV         (AT91C_PIO_PB22)	//
#define AT91C_PB22_TIOB2          (AT91C_PIO_PB22)	//
#define AT91C_PIO_PB23            (1 << 23)	// Pin Controlled by PB23
#define AT91C_PB23_E_TX0          (AT91C_PIO_PB23)	//
#define AT91C_PB23_PCK3           (AT91C_PIO_PB23)	//
#define AT91C_PIO_PB24            (1 << 24)	// Pin Controlled by PB24
#define AT91C_PB24_E_TX1          (AT91C_PIO_PB24)	//
#define AT91C_PIO_PB25            (1 << 25)	// Pin Controlled by PB25
#define AT91C_PB25_E_RX0          (AT91C_PIO_PB25)	//
#define AT91C_PIO_PB26            (1 << 26)	// Pin Controlled by PB26
#define AT91C_PB26_E_RX1          (AT91C_PIO_PB26)	//
#define AT91C_PIO_PB27            (1 << 27)	// Pin Controlled by PB27
#define AT91C_PB27_E_RXER         (AT91C_PIO_PB27)	//
#define AT91C_PIO_PB28            (1 << 28)	// Pin Controlled by PB28
#define AT91C_PB28_E_TXEN         (AT91C_PIO_PB28)	//
#define AT91C_PB28_TCLK0          (AT91C_PIO_PB28)	//
#define AT91C_PIO_PB29            (1 << 29)	// Pin Controlled by PB29
#define AT91C_PB29_E_MDC          (AT91C_PIO_PB29)	//
#define AT91C_PB29_PWM3           (AT91C_PIO_PB29)	//
#define AT91C_PIO_PB3             (1 <<  3)	// Pin Controlled by PB3
#define AT91C_PB3_RD0             (AT91C_PIO_PB3)	//
#define AT91C_PIO_PB30            (1 << 30)	// Pin Controlled by PB30
#define AT91C_PB30_E_MDIO         (AT91C_PIO_PB30)	//
#define AT91C_PIO_PB31            (1 << 31)	// Pin Controlled by PB31
#define AT91C_PB31_ADTRIG         (AT91C_PIO_PB31)	//
#define AT91C_PB31_E_F100         (AT91C_PIO_PB31)	//
#define AT91C_PIO_PB4             (1 <<  4)	// Pin Controlled by PB4
#define AT91C_PB4_RK0             (AT91C_PIO_PB4)	//
#define AT91C_PB4_TWD             (AT91C_PIO_PB4)	//
#define AT91C_PIO_PB5             (1 <<  5)	// Pin Controlled by PB5
#define AT91C_PB5_RF0             (AT91C_PIO_PB5)	//
#define AT91C_PB5_TWCK            (AT91C_PIO_PB5)	//
#define AT91C_PIO_PB6             (1 <<  6)	// Pin Controlled by PB6
#define AT91C_PB6_TF1             (AT91C_PIO_PB6)	//
#define AT91C_PB6_TIOA1           (AT91C_PIO_PB6)	//
#define AT91C_PIO_PB7             (1 <<  7)	// Pin Controlled by PB7
#define AT91C_PB7_TK1             (AT91C_PIO_PB7)	//
#define AT91C_PB7_TIOB1           (AT91C_PIO_PB7)	//
#define AT91C_PIO_PB8             (1 <<  8)	// Pin Controlled by PB8
#define AT91C_PB8_TD1             (AT91C_PIO_PB8)	//
#define AT91C_PB8_PWM2            (AT91C_PIO_PB8)	//
#define AT91C_PIO_PB9             (1 <<  9)	// Pin Controlled by PB9
#define AT91C_PB9_RD1             (AT91C_PIO_PB9)	//
#define AT91C_PB9_LCDCC           (AT91C_PIO_PB9)	//
#define AT91C_PIO_PC0             (1 <<  0)	// Pin Controlled by PC0
#define AT91C_PC0_LCDVSYNC        (AT91C_PIO_PC0)	//
#define AT91C_PIO_PC1             (1 <<  1)	// Pin Controlled by PC1
#define AT91C_PC1_LCDHSYNC        (AT91C_PIO_PC1)	//
#define AT91C_PIO_PC10            (1 << 10)	// Pin Controlled by PC10
#define AT91C_PC10_LCDD6          (AT91C_PIO_PC10)	//
#define AT91C_PC10_LCDD11B        (AT91C_PIO_PC10)	//
#define AT91C_PIO_PC11            (1 << 11)	// Pin Controlled by PC11
#define AT91C_PC11_LCDD7          (AT91C_PIO_PC11)	//
#define AT91C_PC11_LCDD12B        (AT91C_PIO_PC11)	//
#define AT91C_PIO_PC12            (1 << 12)	// Pin Controlled by PC12
#define AT91C_PC12_LCDD8          (AT91C_PIO_PC12)	//
#define AT91C_PC12_LCDD13B        (AT91C_PIO_PC12)	//
#define AT91C_PIO_PC13            (1 << 13)	// Pin Controlled by PC13
#define AT91C_PC13_LCDD9          (AT91C_PIO_PC13)	//
#define AT91C_PC13_LCDD14B        (AT91C_PIO_PC13)	//
#define AT91C_PIO_PC14            (1 << 14)	// Pin Controlled by PC14
#define AT91C_PC14_LCDD10         (AT91C_PIO_PC14)	//
#define AT91C_PC14_LCDD15B        (AT91C_PIO_PC14)	//
#define AT91C_PIO_PC15            (1 << 15)	// Pin Controlled by PC15
#define AT91C_PC15_LCDD11         (AT91C_PIO_PC15)	//
#define AT91C_PC15_LCDD19B        (AT91C_PIO_PC15)	//
#define AT91C_PIO_PC16            (1 << 16)	// Pin Controlled by PC16
#define AT91C_PC16_LCDD12         (AT91C_PIO_PC16)	//
#define AT91C_PC16_LCDD20B        (AT91C_PIO_PC16)	//
#define AT91C_PIO_PC17            (1 << 17)	// Pin Controlled by PC17
#define AT91C_PC17_LCDD13         (AT91C_PIO_PC17)	//
#define AT91C_PC17_LCDD21B        (AT91C_PIO_PC17)	//
#define AT91C_PIO_PC18            (1 << 18)	// Pin Controlled by PC18
#define AT91C_PC18_LCDD14         (AT91C_PIO_PC18)	//
#define AT91C_PC18_LCDD22B        (AT91C_PIO_PC18)	//
#define AT91C_PIO_PC19            (1 << 19)	// Pin Controlled by PC19
#define AT91C_PC19_LCDD15         (AT91C_PIO_PC19)	//
#define AT91C_PC19_LCDD23B        (AT91C_PIO_PC19)	//
#define AT91C_PIO_PC2             (1 <<  2)	// Pin Controlled by PC2
#define AT91C_PC2_LCDDOTCK        (AT91C_PIO_PC2)	//
#define AT91C_PIO_PC20            (1 << 20)	// Pin Controlled by PC20
#define AT91C_PC20_LCDD16         (AT91C_PIO_PC20)	//
#define AT91C_PC20_E_TX2          (AT91C_PIO_PC20)	//
#define AT91C_PIO_PC21            (1 << 21)	// Pin Controlled by PC21
#define AT91C_PC21_LCDD17         (AT91C_PIO_PC21)	//
#define AT91C_PC21_E_TX3          (AT91C_PIO_PC21)	//
#define AT91C_PIO_PC22            (1 << 22)	// Pin Controlled by PC22
#define AT91C_PC22_LCDD18         (AT91C_PIO_PC22)	//
#define AT91C_PC22_E_RX2          (AT91C_PIO_PC22)	//
#define AT91C_PIO_PC23            (1 << 23)	// Pin Controlled by PC23
#define AT91C_PC23_LCDD19         (AT91C_PIO_PC23)	//
#define AT91C_PC23_E_RX3          (AT91C_PIO_PC23)	//
#define AT91C_PIO_PC24            (1 << 24)	// Pin Controlled by PC24
#define AT91C_PC24_LCDD20         (AT91C_PIO_PC24)	//
#define AT91C_PC24_E_TXER         (AT91C_PIO_PC24)	//
#define AT91C_PIO_PC25            (1 << 25)	// Pin Controlled by PC25
#define AT91C_PC25_LCDD21         (AT91C_PIO_PC25)	//
#define AT91C_PC25_E_CRS          (AT91C_PIO_PC25)	//
#define AT91C_PIO_PC26            (1 << 26)	// Pin Controlled by PC26
#define AT91C_PC26_LCDD22         (AT91C_PIO_PC26)	//
#define AT91C_PC26_E_COL          (AT91C_PIO_PC26)	//
#define AT91C_PIO_PC27            (1 << 27)	// Pin Controlled by PC27
#define AT91C_PC27_LCDD23         (AT91C_PIO_PC27)	//
#define AT91C_PC27_E_RXCK         (AT91C_PIO_PC27)	//
#define AT91C_PIO_PC28            (1 << 28)	// Pin Controlled by PC28
#define AT91C_PC28_PWM0           (AT91C_PIO_PC28)	//
#define AT91C_PC28_TCLK1          (AT91C_PIO_PC28)	//
#define AT91C_PIO_PC29            (1 << 29)	// Pin Controlled by PC29
#define AT91C_PC29_PCK0           (AT91C_PIO_PC29)	//
#define AT91C_PC29_PWM2           (AT91C_PIO_PC29)	//
#define AT91C_PIO_PC3             (1 <<  3)	// Pin Controlled by PC3
#define AT91C_PC3_LCDDEN          (AT91C_PIO_PC3)	//
#define AT91C_PC3_PWM1            (AT91C_PIO_PC3)	//
#define AT91C_PIO_PC30            (1 << 30)	// Pin Controlled by PC30
#define AT91C_PC30_DRXD           (AT91C_PIO_PC30)	//
#define AT91C_PIO_PC31            (1 << 31)	// Pin Controlled by PC31
#define AT91C_PC31_DTXD           (AT91C_PIO_PC31)	//
#define AT91C_PIO_PC4             (1 <<  4)	// Pin Controlled by PC4
#define AT91C_PC4_LCDD0           (AT91C_PIO_PC4)	//
#define AT91C_PC4_LCDD3B          (AT91C_PIO_PC4)	//
#define AT91C_PIO_PC5             (1 <<  5)	// Pin Controlled by PC5
#define AT91C_PC5_LCDD1           (AT91C_PIO_PC5)	//
#define AT91C_PC5_LCDD4B          (AT91C_PIO_PC5)	//
#define AT91C_PIO_PC6             (1 <<  6)	// Pin Controlled by PC6
#define AT91C_PC6_LCDD2           (AT91C_PIO_PC6)	//
#define AT91C_PC6_LCDD5B          (AT91C_PIO_PC6)	//
#define AT91C_PIO_PC7             (1 <<  7)	// Pin Controlled by PC7
#define AT91C_PC7_LCDD3           (AT91C_PIO_PC7)	//
#define AT91C_PC7_LCDD6B          (AT91C_PIO_PC7)	//
#define AT91C_PIO_PC8             (1 <<  8)	// Pin Controlled by PC8
#define AT91C_PC8_LCDD4           (AT91C_PIO_PC8)	//
#define AT91C_PC8_LCDD7B          (AT91C_PIO_PC8)	//
#define AT91C_PIO_PC9             (1 <<  9)	// Pin Controlled by PC9
#define AT91C_PC9_LCDD5           (AT91C_PIO_PC9)	//
#define AT91C_PC9_LCDD10B         (AT91C_PIO_PC9)	//
#define AT91C_PIO_PD0             (1 <<  0)	// Pin Controlled by PD0
#define AT91C_PD0_TXD1            (AT91C_PIO_PD0)	//
#define AT91C_PD0_SPI0_NPCS2D     (AT91C_PIO_PD0)	//
#define AT91C_PIO_PD1             (1 <<  1)	// Pin Controlled by PD1
#define AT91C_PD1_RXD1            (AT91C_PIO_PD1)	//
#define AT91C_PD1_SPI0_NPCS3D     (AT91C_PIO_PD1)	//
#define AT91C_PIO_PD10            (1 << 10)	// Pin Controlled by PD10
#define AT91C_PD10_EBI_CFCE2      (AT91C_PIO_PD10)	//
#define AT91C_PD10_SCK1           (AT91C_PIO_PD10)	//
#define AT91C_PIO_PD11            (1 << 11)	// Pin Controlled by PD11
#define AT91C_PD11_EBI_NCS2       (AT91C_PIO_PD11)	//
#define AT91C_PIO_PD12            (1 << 12)	// Pin Controlled by PD12
#define AT91C_PD12_EBI_A23        (AT91C_PIO_PD12)	//
#define AT91C_PIO_PD13            (1 << 13)	// Pin Controlled by PD13
#define AT91C_PD13_EBI_A24        (AT91C_PIO_PD13)	//
#define AT91C_PIO_PD14            (1 << 14)	// Pin Controlled by PD14
#define AT91C_PD14_EBI_A25_CFRNW  (AT91C_PIO_PD14)	//
#define AT91C_PIO_PD15            (1 << 15)	// Pin Controlled by PD15
#define AT91C_PD15_EBI_NCS3_NANDCS (AT91C_PIO_PD15)	//
#define AT91C_PIO_PD16            (1 << 16)	// Pin Controlled by PD16
#define AT91C_PD16_EBI_D16        (AT91C_PIO_PD16)	//
#define AT91C_PIO_PD17            (1 << 17)	// Pin Controlled by PD17
#define AT91C_PD17_EBI_D17        (AT91C_PIO_PD17)	//
#define AT91C_PIO_PD18            (1 << 18)	// Pin Controlled by PD18
#define AT91C_PD18_EBI_D18        (AT91C_PIO_PD18)	//
#define AT91C_PIO_PD19            (1 << 19)	// Pin Controlled by PD19
#define AT91C_PD19_EBI_D19        (AT91C_PIO_PD19)	//
#define AT91C_PIO_PD2             (1 <<  2)	// Pin Controlled by PD2
#define AT91C_PD2_TXD2            (AT91C_PIO_PD2)	//
#define AT91C_PD2_SPI1_NPCS2D     (AT91C_PIO_PD2)	//
#define AT91C_PIO_PD20            (1 << 20)	// Pin Controlled by PD20
#define AT91C_PD20_EBI_D20        (AT91C_PIO_PD20)	//
#define AT91C_PIO_PD21            (1 << 21)	// Pin Controlled by PD21
#define AT91C_PD21_EBI_D21        (AT91C_PIO_PD21)	//
#define AT91C_PIO_PD22            (1 << 22)	// Pin Controlled by PD22
#define AT91C_PD22_EBI_D22        (AT91C_PIO_PD22)	//
#define AT91C_PIO_PD23            (1 << 23)	// Pin Controlled by PD23
#define AT91C_PD23_EBI_D23        (AT91C_PIO_PD23)	//
#define AT91C_PIO_PD24            (1 << 24)	// Pin Controlled by PD24
#define AT91C_PD24_EBI_D24        (AT91C_PIO_PD24)	//
#define AT91C_PIO_PD25            (1 << 25)	// Pin Controlled by PD25
#define AT91C_PD25_EBI_D25        (AT91C_PIO_PD25)	//
#define AT91C_PIO_PD26            (1 << 26)	// Pin Controlled by PD26
#define AT91C_PD26_EBI_D26        (AT91C_PIO_PD26)	//
#define AT91C_PIO_PD27            (1 << 27)	// Pin Controlled by PD27
#define AT91C_PD27_EBI_D27        (AT91C_PIO_PD27)	//
#define AT91C_PIO_PD28            (1 << 28)	// Pin Controlled by PD28
#define AT91C_PD28_EBI_D28        (AT91C_PIO_PD28)	//
#define AT91C_PIO_PD29            (1 << 29)	// Pin Controlled by PD29
#define AT91C_PD29_EBI_D29        (AT91C_PIO_PD29)	//
#define AT91C_PIO_PD3             (1 <<  3)	// Pin Controlled by PD3
#define AT91C_PD3_RXD2            (AT91C_PIO_PD3)	//
#define AT91C_PD3_SPI1_NPCS3D     (AT91C_PIO_PD3)	//
#define AT91C_PIO_PD30            (1 << 30)	// Pin Controlled by PD30
#define AT91C_PD30_EBI_D30        (AT91C_PIO_PD30)	//
#define AT91C_PIO_PD31            (1 << 31)	// Pin Controlled by PD31
#define AT91C_PD31_EBI_D31        (AT91C_PIO_PD31)	//
#define AT91C_PIO_PD4             (1 <<  4)	// Pin Controlled by PD4
#define AT91C_PD4_FIQ             (AT91C_PIO_PD4)	//
#define AT91C_PIO_PD5             (1 <<  5)	// Pin Controlled by PD5
#define AT91C_PD5_DMARQ2          (AT91C_PIO_PD5)	//
#define AT91C_PD5_RTS2            (AT91C_PIO_PD5)	//
#define AT91C_PIO_PD6             (1 <<  6)	// Pin Controlled by PD6
#define AT91C_PD6_EBI_NWAIT       (AT91C_PIO_PD6)	//
#define AT91C_PD6_CTS2            (AT91C_PIO_PD6)	//
#define AT91C_PIO_PD7             (1 <<  7)	// Pin Controlled by PD7
#define AT91C_PD7_EBI_NCS4_CFCS0  (AT91C_PIO_PD7)	//
#define AT91C_PD7_RTS1            (AT91C_PIO_PD7)	//
#define AT91C_PIO_PD8             (1 <<  8)	// Pin Controlled by PD8
#define AT91C_PD8_EBI_NCS5_CFCS1  (AT91C_PIO_PD8)	//
#define AT91C_PD8_CTS1            (AT91C_PIO_PD8)	//
#define AT91C_PIO_PD9             (1 <<  9)	// Pin Controlled by PD9
#define AT91C_PD9_EBI_CFCE1       (AT91C_PIO_PD9)	//
#define AT91C_PD9_SCK2            (AT91C_PIO_PD9)	//

#endif
