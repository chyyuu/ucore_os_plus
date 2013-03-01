
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

#ifndef ARCH_ARM_MACH_AT91_NANDFLASH_H
#define ARCH_ARM_MACH_AT91_NANDFLASH_H

#include "at91-gpio.h"

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Static Memory Controller Interface
// *****************************************************************************
// *** Register offset in AT91S_SMC structure ***
#define SMC_SETUP0      ( 0)	//  Setup Register for CS 0
#define SMC_PULSE0      ( 4)	//  Pulse Register for CS 0
#define SMC_CYCLE0      ( 8)	//  Cycle Register for CS 0
#define SMC_CTRL0       (12)	//  Control Register for CS 0
#define SMC_SETUP1      (16)	//  Setup Register for CS 1
#define SMC_PULSE1      (20)	//  Pulse Register for CS 1
#define SMC_CYCLE1      (24)	//  Cycle Register for CS 1
#define SMC_CTRL1       (28)	//  Control Register for CS 1
#define SMC_SETUP2      (32)	//  Setup Register for CS 2
#define SMC_PULSE2      (36)	//  Pulse Register for CS 2
#define SMC_CYCLE2      (40)	//  Cycle Register for CS 2
#define SMC_CTRL2       (44)	//  Control Register for CS 2
#define SMC_SETUP3      (48)	//  Setup Register for CS 3
#define SMC_PULSE3      (52)	//  Pulse Register for CS 3
#define SMC_CYCLE3      (56)	//  Cycle Register for CS 3
#define SMC_CTRL3       (60)	//  Control Register for CS 3
#define SMC_SETUP4      (64)	//  Setup Register for CS 4
#define SMC_PULSE4      (68)	//  Pulse Register for CS 4
#define SMC_CYCLE4      (72)	//  Cycle Register for CS 4
#define SMC_CTRL4       (76)	//  Control Register for CS 4
#define SMC_SETUP5      (80)	//  Setup Register for CS 5
#define SMC_PULSE5      (84)	//  Pulse Register for CS 5
#define SMC_CYCLE5      (88)	//  Cycle Register for CS 5
#define SMC_CTRL5       (92)	//  Control Register for CS 5
#define SMC_SETUP6      (96)	//  Setup Register for CS 6
#define SMC_PULSE6      (100)	//  Pulse Register for CS 6
#define SMC_CYCLE6      (104)	//  Cycle Register for CS 6
#define SMC_CTRL6       (108)	//  Control Register for CS 6
#define SMC_SETUP7      (112)	//  Setup Register for CS 7
#define SMC_PULSE7      (116)	//  Pulse Register for CS 7
#define SMC_CYCLE7      (120)	//  Cycle Register for CS 7
#define SMC_CTRL7       (124)	//  Control Register for CS 7
// -------- SMC_SETUP : (SMC Offset: 0x0) Setup Register for CS x -------- 
#define AT91C_SMC_NWESETUP        (0x3F <<  0)	// (SMC) NWE Setup Length
#define AT91C_SMC_NCSSETUPWR      (0x3F <<  8)	// (SMC) NCS Setup Length in WRite Access
#define AT91C_SMC_NRDSETUP        (0x3F << 16)	// (SMC) NRD Setup Length
#define AT91C_SMC_NCSSETUPRD      (0x3F << 24)	// (SMC) NCS Setup Length in ReaD Access
// -------- SMC_PULSE : (SMC Offset: 0x4) Pulse Register for CS x -------- 
#define AT91C_SMC_NWEPULSE        (0x7F <<  0)	// (SMC) NWE Pulse Length
#define AT91C_SMC_NCSPULSEWR      (0x7F <<  8)	// (SMC) NCS Pulse Length in WRite Access
#define AT91C_SMC_NRDPULSE        (0x7F << 16)	// (SMC) NRD Pulse Length
#define AT91C_SMC_NCSPULSERD      (0x7F << 24)	// (SMC) NCS Pulse Length in ReaD Access
// -------- SMC_CYC : (SMC Offset: 0x8) Cycle Register for CS x -------- 
#define AT91C_SMC_NWECYCLE        (0x1FF <<  0)	// (SMC) Total Write Cycle Length
#define AT91C_SMC_NRDCYCLE        (0x1FF << 16)	// (SMC) Total Read Cycle Length
// -------- SMC_CTRL : (SMC Offset: 0xc) Control Register for CS x -------- 
#define AT91C_SMC_READMODE        (0x1 <<  0)	// (SMC) Read Mode
#define AT91C_SMC_WRITEMODE       (0x1 <<  1)	// (SMC) Write Mode
#define AT91C_SMC_NWAITM          (0x3 <<  5)	// (SMC) NWAIT Mode
#define 	AT91C_SMC_NWAITM_NWAIT_DISABLE        (0x0 <<  5)	// (SMC) External NWAIT disabled.
#define 	AT91C_SMC_NWAITM_NWAIT_ENABLE_FROZEN  (0x2 <<  5)	// (SMC) External NWAIT enabled in frozen mode.
#define 	AT91C_SMC_NWAITM_NWAIT_ENABLE_READY   (0x3 <<  5)	// (SMC) External NWAIT enabled in ready mode.
#define AT91C_SMC_BAT             (0x1 <<  8)	// (SMC) Byte Access Type
#define 	AT91C_SMC_BAT_BYTE_SELECT          (0x0 <<  8)	// (SMC) Write controled by ncs, nbs0, nbs1, nbs2, nbs3. Read controled by ncs, nrd, nbs0, nbs1, nbs2, nbs3.
#define 	AT91C_SMC_BAT_BYTE_WRITE           (0x1 <<  8)	// (SMC) Write controled by ncs, nwe0, nwe1, nwe2, nwe3. Read controled by ncs and nrd.
#define AT91C_SMC_DBW             (0x3 << 12)	// (SMC) Data Bus Width
#define 	AT91C_SMC_DBW_WIDTH_EIGTH_BITS     (0x0 << 12)	// (SMC) 8 bits.
#define 	AT91C_SMC_DBW_WIDTH_SIXTEEN_BITS   (0x1 << 12)	// (SMC) 16 bits.
#define 	AT91C_SMC_DBW_WIDTH_THIRTY_TWO_BITS (0x2 << 12)	// (SMC) 32 bits.
#define AT91C_SMC_TDF             (0xF << 16)	// (SMC) Data Float Time.
#define AT91C_SMC_TDFEN           (0x1 << 20)	// (SMC) TDF Enabled.
#define AT91C_SMC_PMEN            (0x1 << 24)	// (SMC) Page Mode Enabled.
#define AT91C_SMC_PS              (0x3 << 28)	// (SMC) Page Size
#define 	AT91C_SMC_PS_SIZE_FOUR_BYTES      (0x0 << 28)	// (SMC) 4 bytes.
#define 	AT91C_SMC_PS_SIZE_EIGHT_BYTES     (0x1 << 28)	// (SMC) 8 bytes.
#define 	AT91C_SMC_PS_SIZE_SIXTEEN_BYTES   (0x2 << 28)	// (SMC) 16 bytes.
#define 	AT91C_SMC_PS_SIZE_THIRTY_TWO_BYTES (0x3 << 28)	// (SMC) 32 bytes.

/* ******************************************************************* */
/* NandFlash Settings                                                  */
/*                                                                     */
/* ******************************************************************* */

#define AT91_SMART_MEDIA_ALE    (1 << 21)	/* our ALE is AD21 */
#define AT91_SMART_MEDIA_CLE    (1 << 22)	/* our CLE is AD22 */

/* ******************************************************************** */
/* SMC Chip Select 3 Timings for NandFlash for MASTER_CLOCK = 133000000.*/
/* Please refer to SMC section in AT91SAM9 datasheet to learn how 	*/
/* to generate these values. 						*/
/* ******************************************************************** */
#define AT91C_SM_NWE_SETUP	(2 << 0)
#define AT91C_SM_NCS_WR_SETUP	(0 << 8)
#define AT91C_SM_NRD_SETUP	(2 << 16)
#define AT91C_SM_NCS_RD_SETUP	(0 << 24)

#define AT91C_SM_NWE_PULSE 	(4 << 0)
#define AT91C_SM_NCS_WR_PULSE	(4 << 8)
#define AT91C_SM_NRD_PULSE	(4 << 16)
#define AT91C_SM_NCS_RD_PULSE	(4 << 24)

#define AT91C_SM_NWE_CYCLE 	(7 << 0)
#define AT91C_SM_NRD_CYCLE	(7 << 16)

#define AT91C_SM_TDF	        (3 << 16)

typedef struct SNandInitInfo {
	unsigned short uNandID;	/* Nand Chip ID */
	unsigned short uNandNbBlocks;
	unsigned int uNandBlockSize;
	unsigned short uNandSectorSize;
	unsigned char uNandSpareSize;
	unsigned char uNandBusWidth;
	struct NandSpareScheme *pSpareScheme;
	char name[32];		/* Nand Name */
} SNandInitInfo, *PSNandInitInfo;

/* Group all usefull sizes for the nandflash chip */
typedef struct _NandInfo {
	unsigned int uDataNbBytes;	/* Nb of bytes in data section */
	unsigned int uSpareNbBytes;	/* Nb of bytes in spare section */
	unsigned int uSectorNbBytes;	/* Total nb of bytes in a sector */

	unsigned int uNbBlocks;	/* Nb of blocks in device */
	unsigned int uBlockNbData;	/* Nb of DataBytes in a block */
#if 0
	unsigned int uBlockNbSectors;	/* Nb of sector in a block */
	unsigned int uBlockNbSpares;	/* Nb of SpareBytes in a block */
	unsigned int uBlockNbBytes;	/* Total nb of bytes in a block */

	unsigned int uNbSectors;	/* Total nb of sectors in device */
#endif
	unsigned int uNbData;	/* Nb of DataBytes in device */
	unsigned int uNbSpares;	/* Nb of SpareBytes in device */
	unsigned int uNbBytes;	/* Total nb of bytes in device */

	unsigned int uOffset;

	unsigned int uDataBusWidth;	/* Data Bus Width (8/16 bits) */

	unsigned int uBadBlockInfoOffset;	/* Bad block info offset in spare zone (in bytes) */
	struct NandSpareScheme *pSpareScheme;
} SNandInfo, *PSNandInfo;

/* Sector Info Structure */
//typedef struct  __attribute__((__packed__)) _SectorInfo
//{
//    unsigned int dwReserved1;       /* Reserved - used by FAL */
//    char  bOEMReserved;             /* For use by OEM */
//    char  bBadBlock;              /* Indicates if block is BAD */
//    short wReserved2;               /* Reserved - used by FAL */
//}SSectorInfo, *PSSectorInfo;

#ifdef NANDFLASH_SMALL_BLOCKS
#define BAD_BLOCK_INFO_OFFSET 5
#define SPARE_DATA_SIZE      64
#else
#define BAD_BLOCK_INFO_OFFSET 0
#define SPARE_DATA_SIZE      16
#endif

#define BAD_BLOCK_TAG   0xFF

typedef struct _SectorInfo {
	unsigned char spare[SPARE_DATA_SIZE];
} SSectorInfo, *PSSectorInfo;

/* Wait mode */
#define WAIT_POOL			POOL_ON_READYBUSY	/* Default pool mode */
#define WAIT_INTERRUPT			INTERRUPT_ON_READYBUSY	/* Default interrupt mode */
#define	ERASE_TIMEOUT			10	/* erase maximum time in ms */
#define RESET_TIMEOUT			10	/* reset maximum time in ms */
#define READ_TIMEOUT			10	/* read maximum time in ms */
#define WRITE_TIMEOUT			10	/* write maximum time in ms */

#define POOL_ON_READYBUSY		0x01	/* Pool on ReadyBusy PIO */
#define INTERRUPT_ON_READYBUSY		0x02	/* Interrupt on ReadyBusy PIO */
#define POOL_ON_STATUS			0x04	/* Pool on Status byte */

/* Sector zones */
#define SPARE_VALUE			0xFF

#define ZONE_DATA			0x01	/* Sector data zone */
#define ZONE_INFO			0x02	/* Sector info zone */

/* Nand flash chip status codes */
#define STATUS_READY                (0x01<<6)	/* Status code for Ready */
#define STATUS_ERROR                (0x01<<0)	/* Status code for Error */

/* NandFlash functions */

extern void nandflash_cfg_16bits_dbw_init(void);

extern void nandflash_cfg_8bits_dbw_init(void);

//extern int load_nandflash(unsigned int img_addr, unsigned int img_size, unsigned int img_dest);

extern int AT91F_NandEraseBlock0(void);

extern int read_nandflash(unsigned char *dst, unsigned long offset, int len);

extern int nandflash_hw_init(void);

struct sam9_nand_config {
	uint32_t smc_setup;
	uint32_t smc_pulse;
	uint32_t smc_cycle;
	uint32_t smc_mode;

	unsigned char ale;
	unsigned char cle;
	uint32_t rdy_pin_mask;
	uint32_t enable_pin_mask;
	unsigned char bus_width_16;

	uintptr_t io_addr_base;
	uintptr_t io_addr_phy_base;

	uintptr_t io_addr_ecc_base;
	uintptr_t io_addr_ecc_phy_base;

	SNandInitInfo *info;

};

#endif
