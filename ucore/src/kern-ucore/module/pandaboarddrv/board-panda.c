
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/ti_wilink_st.h>
#include <linux/wl12xx.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

#include "plat/hardware.h"
#include "plat/board.h"
#include "common.h"
#include "control.h"
#include "iomap.h"
#include "mux.h"
#include "common-board-devices.h"

#define MACH_TYPE_OMAP4_PANDA  10001
#define GPIO_HUB_POWER		1
#define GPIO_HUB_NRESET		62
#define GPIO_WIFI_PMENA		43
#define GPIO_WIFI_IRQ		53
#define HDMI_GPIO_CT_CP_HPD 60	/* HPD mode enable/disable */
#define HDMI_GPIO_LS_OE 41	/* Level shifter for HDMI */
#define HDMI_GPIO_HPD  63	/* Hotplug detect */

static struct omap_globals omap4_globals = {
	.class = OMAP443X_CLASS,
	.tap = OMAP2_L4_IO_ADDRESS(OMAP443X_SCM_BASE),
	.ctrl = OMAP2_L4_IO_ADDRESS(OMAP443X_SCM_BASE),
	.ctrl_pad = OMAP2_L4_IO_ADDRESS(OMAP443X_CTRL_BASE),
	.prm = OMAP2_L4_IO_ADDRESS(OMAP4430_PRM_BASE),
	.cm = OMAP2_L4_IO_ADDRESS(OMAP4430_CM_BASE),
	.cm2 = OMAP2_L4_IO_ADDRESS(OMAP4430_CM2_BASE),
	.prcm_mpu = OMAP2_L4_IO_ADDRESS(OMAP4430_PRCM_MPU_BASE),
};

/* wl127x BT, FM, GPS connectivity chip */
static struct ti_st_plat_data wilink_platform_data = {
	.nshutdown_gpio = 46,
	.dev_name = "/dev/ttyO1",
	.flow_cntrl = 1,
	.baud_rate = 3000000,
	.chip_enable = NULL,
	.suspend = NULL,
	.resume = NULL,
};

static struct platform_device wl1271_device = {
	.name = "kim",
	.id = -1,
	.dev = {
		.platform_data = &wilink_platform_data,
		},
};

static struct gpio_led gpio_leds[] = {
	{
	 .name = "pandaboard::status1",
	 .default_trigger = "heartbeat",
	 .gpio = 7,
	 },
	{
	 .name = "pandaboard::status2",
	 .default_trigger = "mmc0",
	 .gpio = 8,
	 },
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds = gpio_leds,
	.num_leds = ARRAY_SIZE(gpio_leds),
};

static struct platform_device leds_gpio = {
	.name = "leds-gpio",
	.id = -1,
	.dev = {
		.platform_data = &gpio_led_info,
		},
};

#if 0
static struct omap_abe_twl6040_data panda_abe_audio_data = {
	/* Audio out */
	.has_hs = ABE_TWL6040_LEFT | ABE_TWL6040_RIGHT,
	/* HandsFree through expansion connector */
	.has_hf = ABE_TWL6040_LEFT | ABE_TWL6040_RIGHT,
	/* PandaBoard: FM TX, PandaBoardES: can be connected to audio out */
	.has_aux = ABE_TWL6040_LEFT | ABE_TWL6040_RIGHT,
	/* PandaBoard: FM RX, PandaBoardES: audio in */
	.has_afm = ABE_TWL6040_LEFT | ABE_TWL6040_RIGHT,
	/* No jack detection. */
	.jack_detection = 0,
	/* MCLK input is 38.4MHz */
	.mclk_freq = 38400000,

};

static struct platform_device panda_abe_audio = {
	.name = "omap-abe-twl6040",
	.id = -1,
	.dev = {
		.platform_data = &panda_abe_audio_data,
		},
};
#endif

static struct platform_device panda_hdmi_audio_codec = {
	.name = "hdmi-audio-codec",
	.id = -1,
};

static struct platform_device btwilink_device = {
	.name = "btwilink",
	.id = -1,
};

static struct platform_device *panda_devices[] __initdata = {
	&leds_gpio,
	&wl1271_device,
	//&panda_abe_audio,
	&panda_hdmi_audio_codec,
	&btwilink_device,
};

static struct wl12xx_platform_data omap_panda_wlan_data __initdata = {
	/* PANDA ref clock is 38.4 MHz */
	.board_ref_clock = 2,
};

void __init __omap2_set_globals(struct omap_globals *omap2_globals)
{
	omap2_set_globals_tap(omap2_globals);
	//omap2_set_globals_sdrc(omap2_globals);
	//omap2_set_globals_control(omap2_globals);
	//omap2_set_globals_prcm(omap2_globals);
}

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	/* WLAN IRQ - GPIO 53 */
	OMAP4_MUX(GPMC_NCS3, OMAP_MUX_MODE3 | OMAP_PIN_INPUT),
	/* WLAN POWER ENABLE - GPIO 43 */
	OMAP4_MUX(GPMC_A19, OMAP_MUX_MODE3 | OMAP_PIN_OUTPUT),
	/* WLAN SDIO: MMC5 CMD */
	OMAP4_MUX(SDMMC5_CMD, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	/* WLAN SDIO: MMC5 CLK */
	OMAP4_MUX(SDMMC5_CLK, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	/* WLAN SDIO: MMC5 DAT[0-3] */
	OMAP4_MUX(SDMMC5_DAT0, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	OMAP4_MUX(SDMMC5_DAT1, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	OMAP4_MUX(SDMMC5_DAT2, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	OMAP4_MUX(SDMMC5_DAT3, OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP),
	/* gpio 0 - TFP410 PD */
	OMAP4_MUX(KPD_COL1, OMAP_PIN_OUTPUT | OMAP_MUX_MODE3),
	/* dispc2_data23 */
	OMAP4_MUX(USBB2_ULPITLL_STP, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data22 */
	OMAP4_MUX(USBB2_ULPITLL_DIR, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data21 */
	OMAP4_MUX(USBB2_ULPITLL_NXT, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data20 */
	OMAP4_MUX(USBB2_ULPITLL_DAT0, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data19 */
	OMAP4_MUX(USBB2_ULPITLL_DAT1, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data18 */
	OMAP4_MUX(USBB2_ULPITLL_DAT2, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data15 */
	OMAP4_MUX(USBB2_ULPITLL_DAT3, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data14 */
	OMAP4_MUX(USBB2_ULPITLL_DAT4, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data13 */
	OMAP4_MUX(USBB2_ULPITLL_DAT5, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data12 */
	OMAP4_MUX(USBB2_ULPITLL_DAT6, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data11 */
	OMAP4_MUX(USBB2_ULPITLL_DAT7, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data10 */
	OMAP4_MUX(DPM_EMU3, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data9 */
	OMAP4_MUX(DPM_EMU4, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data16 */
	OMAP4_MUX(DPM_EMU5, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data17 */
	OMAP4_MUX(DPM_EMU6, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_hsync */
	OMAP4_MUX(DPM_EMU7, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_pclk */
	OMAP4_MUX(DPM_EMU8, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_vsync */
	OMAP4_MUX(DPM_EMU9, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_de */
	OMAP4_MUX(DPM_EMU10, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data8 */
	OMAP4_MUX(DPM_EMU11, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data7 */
	OMAP4_MUX(DPM_EMU12, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data6 */
	OMAP4_MUX(DPM_EMU13, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data5 */
	OMAP4_MUX(DPM_EMU14, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data4 */
	OMAP4_MUX(DPM_EMU15, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data3 */
	OMAP4_MUX(DPM_EMU16, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data2 */
	OMAP4_MUX(DPM_EMU17, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data1 */
	OMAP4_MUX(DPM_EMU18, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* dispc2_data0 */
	OMAP4_MUX(DPM_EMU19, OMAP_PIN_OUTPUT | OMAP_MUX_MODE5),
	/* NIRQ2 for twl6040 */
	OMAP4_MUX(SYS_NIRQ2, OMAP_MUX_MODE0 |
		  OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_WAKEUPENABLE),
	{.reg_offset = OMAP_MUX_TERMINATOR},
};

#else
#define board_mux	NULL
#endif

static void omap4_panda_init_rev(void)
{
#if 0
	if (cpu_is_omap443x()) {
		/* PandaBoard 4430 */
		/* ASoC audio configuration */
		panda_abe_audio_data.card_name = "PandaBoard";
		panda_abe_audio_data.has_hsmic = 1;
	} else {
		/* PandaBoard ES */
		/* ASoC audio configuration */
		panda_abe_audio_data.card_name = "PandaBoardES";
	}
#endif
}

#ifdef	CONFIG_ARCH_OMAP4
static struct map_desc omap44xx_io_desc[] __initdata = {
	{
	 .virtual = L3_44XX_VIRT,
	 .pfn = __phys_to_pfn(L3_44XX_PHYS),
	 .length = L3_44XX_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	 .virtual = L4_44XX_VIRT,
	 .pfn = __phys_to_pfn(L4_44XX_PHYS),
	 .length = L4_44XX_SIZE,
	 .type = MT_DEVICE,
	 },
	{
	 .virtual = L4_PER_44XX_VIRT,
	 .pfn = __phys_to_pfn(L4_PER_44XX_PHYS),
	 .length = L4_PER_44XX_SIZE,
	 .type = MT_DEVICE,
	 },

};
#endif

void __init omap_barriers_init(void)
{
}

void __init omap44xx_map_common_io(void)
{
	iotable_init(omap44xx_io_desc, ARRAY_SIZE(omap44xx_io_desc));
	omap_barriers_init();
}

void __init omap2_set_globals_443x(void)
{
	__omap2_set_globals(&omap4_globals);
}

void __init omap4_map_io(void)
{
	omap44xx_map_common_io();
}

static void __init omap_common_init_early(void)
{
	//omap_init_consistent_dma_size();
}

#ifdef CONFIG_ARCH_OMAP4
void __init omap4430_init_early(void)
{
	omap2_set_globals_443x();
	omap4xxx_check_revision();
	omap4xxx_check_features();
	omap_common_init_early();
#if 0
	omap44xx_voltagedomains_init();
	omap44xx_powerdomains_init();
	omap44xx_clockdomains_init();
	omap44xx_hwmod_init();
	omap_hwmod_init_postsetup();
	omap4xxx_clk_init();
#endif
}

void __init omap4430_init_late(void)
{
	omap_mux_late_init();
#if 0
	omap2_common_pm_late_init();
	omap4_pm_init();
#endif
}
#endif

static void __init omap4_panda_init(void)
{
	int package = OMAP_PACKAGE_CBS;
	int ret;

	if (omap_rev() == OMAP4430_REV_ES1_0)
		package = OMAP_PACKAGE_CBL;
	omap4_mux_init(board_mux, NULL, package);

#if 0
	omap_panda_wlan_data.irq = gpio_to_irq(GPIO_WIFI_IRQ);
	ret = wl12xx_set_platform_data(&omap_panda_wlan_data);
	if (ret)
		pr_err("error setting wl12xx data: %d\n", ret);
#endif

	omap4_panda_init_rev();
#if 0
	omap4_panda_i2c_init();
#endif
	platform_add_devices(panda_devices, ARRAY_SIZE(panda_devices));

#if 0
	platform_device_register(&omap_vwlan_device);
	omap_serial_init();
	omap_sdrc_init(NULL, NULL);
	omap4_twl6030_hsmmc_init(mmc);
	omap4_ehci_init();
	usb_musb_init(&musb_board_data);
	omap4_panda_display_init();
#endif
}

static void omap44xx_init_machine()
{
	omap4430_init_early();
	omap4_panda_init();
	omap4430_init_late();
}

MACHINE_START(OMAP4_PANDA, "OMAP4 Panda board")
    .map_io = omap4_map_io,.init_machine = omap44xx_init_machine, MACHINE_END
