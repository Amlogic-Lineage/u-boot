
/*
 * board/khadas/kvim3/kvim3.c
 *
 * Copyright (C) 2019 Khadas, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <environment.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <amlogic/cpu_id.h>
#include <asm/arch/secure_apb.h>
#ifdef CONFIG_SYS_I2C_AML
#include <aml_i2c.h>
#endif
/*
 * I2C and PWM controllers are bound from the device tree in this u-boot, so
 * the board no longer supplies platdata for them and the old
 * <amlogic/i2c.h> / <amlogic/pwm.h> headers are gone.
 */
#ifdef CONFIG_PWM_MESON
#include <pwm.h>
#endif
#ifdef CONFIG_AML_VPU
#include <amlogic/media/vpu/vpu.h>
#endif
#include <amlogic/media/vpp/vpp.h>
#ifdef CONFIG_AML_V2_FACTORY_BURN
#include <amlogic/aml_v2_burning.h>
#endif// #ifdef CONFIG_AML_V2_FACTORY_BURN
#ifdef CONFIG_AML_HDMITX20
#include <amlogic/media/vout/hdmitx/hdmitx.h>
#endif
#ifdef CONFIG_AML_LCD
#include <amlogic/media/vout/lcd/aml_lcd.h>
#endif
#include <asm/arch/eth_setup.h>
#include <phy.h>
#include <linux/mtd/partitions.h>
#include <linux/sizes.h>
#include <emmc_partitions.h>	/* DTB_SIZE */
#include <asm-generic/gpio.h>
#include <dm.h>
#ifdef CONFIG_AML_SPIFC
#include <amlogic/spifc.h>
#endif
#ifdef CONFIG_AML_SPICC
#include <amlogic/spicc.h>
#endif
#ifdef CONFIG_POWER_FUSB302
#include <power/fusb302.h>
#endif
#ifdef CONFIG_TCA6408
#include <khadas_tca6408.h>
#endif
#include <asm/armv8/mmu.h>
#include <asm/arch/timer.h>
#include <amlogic/saradc.h>
DECLARE_GLOBAL_DATA_PTR;

//new static eth setup
struct eth_board_socket*  eth_board_skt;
#define HW_VERSION_ADC_VALUE_TOLERANCE   0x28
#define HW_EXT_BOARD_ADC              0x77

int serial_set_pin_port(unsigned long port_base)
{
    //UART in "Always On Module"
    //GPIOAO_0==tx,GPIOAO_1==rx
    //setbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);
    return 0;
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;
	return 0;
}

/* secondary_boot_func
 * this function should be write with asm, here, is is only for compiling pass
 * */
void secondary_boot_func(void)
{
}

void internalPhyConfig(struct phy_device *phydev)
{
}

static int dwmac_meson_cfg_pll(void)
{
	writel(0x39C0040A, P_ETH_PLL_CTL0);
	writel(0x927E0000, P_ETH_PLL_CTL1);
	writel(0xAC5F49E5, P_ETH_PLL_CTL2);
	writel(0x00000000, P_ETH_PLL_CTL3);
	udelay(200);
	writel(0x19C0040A, P_ETH_PLL_CTL0);
	return 0;
}

static int dwmac_meson_cfg_analog(void)
{
	/*Analog*/
	writel(0x20200000, P_ETH_PLL_CTL5);
	writel(0x0000c002, P_ETH_PLL_CTL6);
	writel(0x00000023, P_ETH_PLL_CTL7);

	return 0;
}

static int dwmac_meson_cfg_ctrl(void)
{
	/*config phyid should between  a 0~0xffffffff*/
	/*please don't use 44000181, this has been used by internal phy*/
	writel(0x33000180, P_ETH_PHY_CNTL0);

	/*use_phy_smi | use_phy_ip | co_clkin from eth_phy_top*/
	writel(0x260, P_ETH_PHY_CNTL2);

	writel(0x74043, P_ETH_PHY_CNTL1);
	writel(0x34043, P_ETH_PHY_CNTL1);
	writel(0x74043, P_ETH_PHY_CNTL1);
	return 0;
}

static void setup_net_chip(void)
{
	eth_aml_reg0_t eth_reg0;

	eth_reg0.d32 = 0;
	eth_reg0.b.phy_intf_sel = 4;
	eth_reg0.b.rx_clk_rmii_invert = 0;
	eth_reg0.b.rgmii_tx_clk_src = 0;
	eth_reg0.b.rgmii_tx_clk_phase = 0;
	eth_reg0.b.rgmii_tx_clk_ratio = 4;
	eth_reg0.b.phy_ref_clk_enable = 1;
	eth_reg0.b.clk_rmii_i_invert = 1;
	eth_reg0.b.clk_en = 1;
	eth_reg0.b.adj_enable = 1;
	eth_reg0.b.adj_setup = 0;
	eth_reg0.b.adj_delay = 9;
	eth_reg0.b.adj_skew = 0;
	eth_reg0.b.cali_start = 0;
	eth_reg0.b.cali_rise = 0;
	eth_reg0.b.cali_sel = 0;
	eth_reg0.b.rgmii_rx_reuse = 0;
	eth_reg0.b.eth_urgent = 0;
	setbits_le32(P_PREG_ETH_REG0, eth_reg0.d32);// rmii mode

	dwmac_meson_cfg_pll();
	dwmac_meson_cfg_analog();
	dwmac_meson_cfg_ctrl();

	/* eth core clock */
	setbits_le32(HHI_GCLK_MPEG1, (0x1 << 3));
	/* eth phy clock */
	setbits_le32(HHI_GCLK_MPEG0, (0x1 << 4));

	/* eth phy pll, clk50m */
	setbits_le32(HHI_FIX_PLL_CNTL3, (0x1 << 5));

	/* power on memory */
	clrbits_le32(HHI_MEM_PD_REG0, (1 << 3) | (1<<2));
}

static int dwmac_meson_cfg_drive_strength(void)
{
	writel(0xaaaaaaa5, P_PAD_DS_REG4A);
	return 0;
}

static void setup_net_chip_ext(void)
{
	eth_aml_reg0_t eth_reg0;
	writel(0x11111111, P_PERIPHS_PIN_MUX_6);
	writel(0x111111, P_PERIPHS_PIN_MUX_7);

	eth_reg0.d32 = 0;
	eth_reg0.b.phy_intf_sel = 1;
	eth_reg0.b.rx_clk_rmii_invert = 0;
	eth_reg0.b.rgmii_tx_clk_src = 0;
	eth_reg0.b.rgmii_tx_clk_phase = 1;
	eth_reg0.b.rgmii_tx_clk_ratio = 4;
	eth_reg0.b.phy_ref_clk_enable = 1;
	eth_reg0.b.clk_rmii_i_invert = 0;
	eth_reg0.b.clk_en = 1;
	eth_reg0.b.adj_enable = 0;
	eth_reg0.b.adj_setup = 0;
	eth_reg0.b.adj_delay = 0;
	eth_reg0.b.adj_skew = 0;
	eth_reg0.b.cali_start = 0;
	eth_reg0.b.cali_rise = 0;
	eth_reg0.b.cali_sel = 0;
	eth_reg0.b.rgmii_rx_reuse = 0;
	eth_reg0.b.eth_urgent = 0;
	setbits_le32(P_PREG_ETH_REG0, eth_reg0.d32);// rmii mode

	setbits_le32(HHI_GCLK_MPEG1, 0x1 << 3);
	/* power on memory */
	clrbits_le32(HHI_MEM_PD_REG0, (1 << 3) | (1<<2));
}

extern struct eth_board_socket* eth_board_setup(char *name);
extern int designware_initialize(ulong base_addr, u32 interface);

int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_ETHERNET_NONE
	return 0;
#endif

	run_command("kbi ext_ethernet r", 1);
	char *s = env_get("ext_ethernet");
	if (s != NULL) {
	printf("--------ext_ethernet=%s\n", s);
		if (strcmp(s, "0") == 0) {
		dwmac_meson_cfg_drive_strength();
		setup_net_chip_ext();
		} else {
			setup_net_chip();
		}
	} else {
		dwmac_meson_cfg_drive_strength();
		setup_net_chip_ext();
	}
	udelay(1000);
	designware_initialize(ETH_BASE, PHY_INTERFACE_MODE_RMII);
	return 0;
}

#if CONFIG_AML_SD_EMMC
#include <mmc.h>
#include <asm/arch/sd_emmc.h>
static int  sd_emmc_init(unsigned port)
{
    switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
			//todo add card detect
			/* check card detect */
			clrbits_le32(P_PERIPHS_PIN_MUX_9, 0xF << 24);
			setbits_le32(P_PREG_PAD_GPIO1_EN_N, 1 << 6);
			setbits_le32(P_PAD_PULL_UP_EN_REG1, 1 << 6);
			setbits_le32(P_PAD_PULL_UP_REG1, 1 << 6);
			break;
		case SDIO_PORT_C:
			//enable pull up
			//clrbits_le32(P_PAD_PULL_UP_REG3, 0xff<<0);
			break;
		default:
			break;
	}

	return cpu_sd_emmc_init(port);
}

extern unsigned sd_debug_board_1bit_flag;


static void sd_emmc_pwr_prepare(unsigned port)
{
	cpu_sd_emmc_pwr_prepare(port);
}

static void sd_emmc_pwr_on(unsigned port)
{
    switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
//            clrbits_le32(P_PREG_PAD_GPIO5_O,(1<<31)); //CARD_8
//            clrbits_le32(P_PREG_PAD_GPIO5_EN_N,(1<<31));
			/// @todo NOT FINISH
			break;
		case SDIO_PORT_C:
			break;
		default:
			break;
	}
	return;
}
static void sd_emmc_pwr_off(unsigned port)
{
	/// @todo NOT FINISH
    switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
//            setbits_le32(P_PREG_PAD_GPIO5_O,(1<<31)); //CARD_8
//            clrbits_le32(P_PREG_PAD_GPIO5_EN_N,(1<<31));
			break;
		case SDIO_PORT_C:
			break;
				default:
			break;
	}
	return;
}

// #define CONFIG_TSD      1
static void board_mmc_register(unsigned port)
{
	struct aml_card_sd_info *aml_priv=cpu_sd_emmc_get(port);
    if (aml_priv == NULL)
		return;

	aml_priv->sd_emmc_init=sd_emmc_init;
	aml_priv->sd_emmc_detect=sd_emmc_detect;
	aml_priv->sd_emmc_pwr_off=sd_emmc_pwr_off;
	aml_priv->sd_emmc_pwr_on=sd_emmc_pwr_on;
	aml_priv->sd_emmc_pwr_prepare=sd_emmc_pwr_prepare;
	aml_priv->desc_buf = malloc(NEWSD_MAX_DESC_MUN*(sizeof(struct sd_emmc_desc_info)));

	if (NULL == aml_priv->desc_buf)
		printf(" desc_buf Dma alloc Fail!\n");
	else
		printf("aml_priv->desc_buf = 0x%p\n",aml_priv->desc_buf);

	sd_emmc_register(aml_priv);
}
int board_mmc_init(bd_t	*bis)
{
#ifdef CONFIG_VLSI_EMULATOR
	//board_mmc_register(SDIO_PORT_A);
#else
	//board_mmc_register(SDIO_PORT_B);
#endif
	board_mmc_register(SDIO_PORT_B);
	board_mmc_register(SDIO_PORT_C);
//	board_mmc_register(SDIO_PORT_B1);
	return 0;
}

#ifdef CONFIG_SYS_I2C_AML
static void board_i2c_set_pinmux(void){

	//disable all other pins which share with I2C_SDA_AO & I2C_SCK_AO
	clrbits_le32(P_AO_RTI_PINMUX_REG0, ((1<<8)|(1<<9)|(1<<10)|(1<<11)));
	clrbits_le32(P_AO_RTI_PINMUX_REG0, ((1<<12)|(1<<13)|(1<<14)|(1<<15)));
	//enable I2C MASTER AO pins
	setbits_le32(P_AO_RTI_PINMUX_REG0,
	(MESON_I2C_MASTER_AO_GPIOAO_2_BIT | MESON_I2C_MASTER_AO_GPIOAO_3_BIT));

	udelay(10);
};
struct aml_i2c_platform g_aml_i2c_plat = {
	.wait_count         = 1000000,
	.wait_ack_interval  = 5,
	.wait_read_interval = 5,
	.wait_xfer_interval = 5,
	.master_no          = AML_I2C_MASTER_AO,
	.use_pio            = 0,
	.master_i2c_speed   = AML_I2C_SPPED_400K,
	.master_ao_pinmux = {
		.scl_reg    = (unsigned long)MESON_I2C_MASTER_AO_GPIOAO_2_REG,
		.scl_bit    = MESON_I2C_MASTER_AO_GPIOAO_2_BIT,
		.sda_reg    = (unsigned long)MESON_I2C_MASTER_AO_GPIOAO_3_REG,
		.sda_bit    = MESON_I2C_MASTER_AO_GPIOAO_3_BIT,
	}
};
static void board_i2c_init(void)
{
	//set I2C pinmux with PCB board layout
	board_i2c_set_pinmux();

	//Amlogic I2C controller initialized
	//note: it must be call before any I2C operation
	i2c_plat_init();
	aml_i2c_init();

	udelay(10);
}
#endif
#endif

#if defined(CONFIG_BOARD_EARLY_INIT_F)
int board_early_init_f(void){
	/*add board early init function here*/
	return 0;
}
#endif

#ifdef CONFIG_USB_XHCI_AMLOGIC_V2
#include <asm/arch/usb-v2.h>
#include <asm/arch/gpio.h>
#define CONFIG_GXL_USB_U2_PORT_NUM	2

#ifdef CONFIG_USB_XHCI_AMLOGIC_USB3_V2
#define CONFIG_GXL_USB_U3_PORT_NUM	1
#else
#define CONFIG_GXL_USB_U3_PORT_NUM	0
#endif

static void gpio_set_vbus_power(char is_power_on)
{
	int ret;

	ret = gpio_request(CONFIG_USB_GPIO_PWR,
		CONFIG_USB_GPIO_PWR_NAME);
	if (ret && ret != -EBUSY) {
		printf("gpio: requesting pin %u failed\n",
			CONFIG_USB_GPIO_PWR);
		return;
	}

	if (is_power_on) {
		gpio_direction_output(CONFIG_USB_GPIO_PWR, 1);
	} else {
		gpio_direction_output(CONFIG_USB_GPIO_PWR, 0);
	}
}

struct amlogic_usb_config g_usb_config_GXL_skt={
	CONFIG_GXL_XHCI_BASE,
	USB_ID_MODE_HARDWARE,
	gpio_set_vbus_power,//gpio_set_vbus_power, //set_vbus_power
	CONFIG_GXL_USB_PHY2_BASE,
	CONFIG_GXL_USB_PHY3_BASE,
	CONFIG_GXL_USB_U2_PORT_NUM,
	CONFIG_GXL_USB_U3_PORT_NUM,
	.usb_phy2_pll_base_addr = {
		CONFIG_USB_PHY_20,
		CONFIG_USB_PHY_21,
	}
};

#endif /*CONFIG_USB_XHCI_AMLOGIC*/

#ifdef CONFIG_AML_HDMITX20
static void hdmi_tx_set_hdmi_5v(void)
{
}
#endif

/*
 * mtd nand partition table, only care the size!
 * offset will be calculated by nand driver.
 */
#ifdef CONFIG_AML_MTD
static struct mtd_partition normal_partition_info[] = {
#ifdef CONFIG_DISCRETE_BOOTLOADER
    /* MUST NOT CHANGE this part unless u know what you are doing!
     * inherent parition for descrete bootloader to store fip
     * size is determind by TPL_SIZE_PER_COPY*TPL_COPY_NUM
     * name must be same with TPL_PART_NAME
     */
    {
        .name = "tpl",
        .offset = 0,
        .size = 0,
    },
#endif
    {
        .name = "logo",
        .offset = 0,
        .size = 2*SZ_1M,
    },
    {
        .name = "recovery",
        .offset = 0,
        .size = 16*SZ_1M,
    },
    {
        .name = "boot",
        .offset = 0,
        .size = 15*SZ_1M,
    },
    {
        .name = "system",
        .offset = 0,
        .size = 280*SZ_1M,
    },
	/* last partition get the rest capacity */
    {
        .name = "data",
        .offset = MTDPART_OFS_APPEND,
        .size = MTDPART_SIZ_FULL,
    },
};
struct mtd_partition *get_aml_mtd_partition(void)
{
	return normal_partition_info;
}
int get_aml_partition_count(void)
{
	return ARRAY_SIZE(normal_partition_info);
}
#endif /* CONFIG_AML_MTD */

/*
 * SPIFC / SPICC controllers are bound from the device tree in this u-boot;
 * the board no longer supplies platdata for them.
 */

/* meson i2c controllers are bound from the device tree now */
#ifdef CONFIG_SYS_I2C_MESON
/*
 *GPIOH_6 I2C_SDA_M1
 *GPIOH_7 I2C_SCK_M1
 *pinmux configuration seperated with i2c controller configuration
 * config it when you use
 */
void set_i2c_m1_pinmux(void)
{
	/*ds =3 */
	clrbits_le32(PAD_DS_REG3A, 0xf << 12);
	setbits_le32(PAD_DS_REG3A, 0x3 << 12 | 0x3 << 14);
	/*pull up en*/
	clrbits_le32(PAD_PULL_UP_EN_REG3, 0x3 << 6);
	setbits_le32(PAD_PULL_UP_EN_REG3, 0x3 << 6 );
	/*pull up*/
	clrbits_le32(PAD_PULL_UP_REG3, 0x3 << 6);
	setbits_le32(PAD_PULL_UP_REG3, 0x3 << 6 );
	/*pin mux to i2cm1*/
	clrbits_le32(PERIPHS_PIN_MUX_B, 0xff << 24);
	setbits_le32(PERIPHS_PIN_MUX_B, 0x4 << 24 | 0x4 << 28);

	return;
}

#endif /*end CONFIG_SYS_I2C_MESON*/

/* meson pwm controllers are bound from the device tree now */

#if (defined (CONFIG_AML_LCD) && defined(CONFIG_TCA6408))
// detect whether the LCD is exist
extern int khadas_mipi_id;
void board_lcd_detect(void)
{
    u8 mask = 0, value = 0;
    int ret = 0;

    // detect RESET pin
    // if the LCD is connected, the RESET pin will be plll high
    // if the LCD is not connected, the RESET pin will be low
    mask = TCA_LCD_RESET_MASK;

    ret = tca6408_get_value(&value, mask);
    if (ret) {
       printf("%s: failed to read LCD_RESET status! error: %d\n", __func__, ret);
       return;
    }

	if (khadas_mipi_id == 2) {//TS101
		value = 1;
	}

    printf("LCD_RESET PIN: %d\n", value);
    env_set_ulong("lcd_exist", value);
}
#endif /* CONFIG_AML_LCD */

void ext_board_detect(void)
{
        unsigned int val = 0;
        int ret;

        /* the legacy saradc helpers are gone; use the DM ADC uclass */
        udelay(100);
        ret = adc_channel_single_shot_mode("adc", ADC_MODE_AVERAGE, 0, &val);
        if (ret) {
                printf("%s: failed to read saradc channel 0: %d\n", __func__, ret);
                env_set("ext_board_exist", "0");
                return;
        }

        if ((val > (HW_EXT_BOARD_ADC - HW_VERSION_ADC_VALUE_TOLERANCE))  && (val < (HW_EXT_BOARD_ADC + HW_VERSION_ADC_VALUE_TOLERANCE)))
                env_set("ext_board_exist", "1");
        else
                env_set("ext_board_exist", "0");

}

extern void aml_pwm_cal_init(int mode);
#ifdef CONFIG_SYS_I2C_AML
extern int i2c_read(uchar chip, uint addr, int alen, uchar *buffer, int len);
static int check_forcebootsd(void)
{
	unsigned char tst_status = 0;
	unsigned char mcu_version[2] = {0};
	int retval;

	retval = i2c_read(0x18, 0x12, 1, mcu_version, 1);
	retval |= i2c_read(0x18, 0x13, 1, mcu_version + 1, 1);

	if (retval < 0) {
		printf("%s i2c_read failed!\n", __func__);
		return -1;
	}

	printf("MCU version: 0x%02x 0x%02x\n", mcu_version[0], mcu_version[1]);

	if (mcu_version[1] < 0x04) {
		printf("MCU version is to low! Doesn't support froce boot from SD card.\n");
		return -1;
	}

	retval = i2c_read(0x18, 0x90, 1, &tst_status, 1);
	if (retval < 0) {
		printf("%s i2c_read failed!\n", __func__);
		return -1;
	}

	if (1 == tst_status) {
		printf("Force boot from SD.\n");
		run_command("kbi tststatus clear", 0);
		run_command("kbi forcebootsd", 0);
	}

	return 0;
}
#endif /* CONFIG_SYS_I2C_AML */

int board_init(void)
{
	struct udevice *pinctrl;

	printf("board init\n");

	/*
	 * initr_dm() re-scans the device tree after relocation, and the meson
	 * pinctrl driver only binds its UCLASS_GPIO child when it is probed.
	 * board_init() runs straight after initr_dm() and before initr_serial(),
	 * so without this no GPIO device is bound yet and a lookup by name
	 * fails with "GPIO: 'gpioao_6' not found".
	 */
	for (uclass_first_device(UCLASS_PINCTRL, &pinctrl);
	     pinctrl;
	     uclass_next_device(&pinctrl))
		;

	run_command("gpio s gpioao_6", 1);
    //Please keep CONFIG_AML_V2_FACTORY_BURN at first place of board_init
    //As NOT NEED other board init If USB BOOT MODE
#ifdef CONFIG_AML_V2_FACTORY_BURN
	if ((0x1b8ec003 != readl(P_PREG_STICKY_REG2)) && (0x1b8ec004 != readl(P_PREG_STICKY_REG2))) {
				aml_try_factory_usb_burning(0, gd->bd);
	}
#endif// #ifdef CONFIG_AML_V2_FACTORY_BURN
#ifdef CONFIG_USB_XHCI_AMLOGIC_V2
	board_usb_pll_disable(&g_usb_config_GXL_skt);
	board_usb_init(&g_usb_config_GXL_skt,BOARD_USB_MODE_HOST);
#endif /*CONFIG_USB_XHCI_AMLOGIC*/

#if 0
	aml_pwm_cal_init(0);
#endif//
#ifdef CONFIG_AML_NAND
	extern int amlnf_init(unsigned char flag);
	amlnf_init(0);
#endif
#ifdef CONFIG_SYS_I2C_MESON
	set_i2c_m1_pinmux();
#endif
#ifdef CONFIG_SYS_I2C_AML
	board_i2c_init();
	check_forcebootsd();
#endif
#ifdef CONFIG_TCA6408
	tca6408_gpio_init();
#endif
	/* power on GPIOZ_5 : CMD_VDD_EN */
	clrbits_le32(PREG_PAD_GPIO4_EN_N, (1 << 5));
	clrbits_le32(PREG_PAD_GPIO4_O, (1 << 5));
	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	printf("board late init\n");
		//update env before anyone using it
		run_command("get_rebootmode; echo reboot_mode=${reboot_mode}; "\
						"if test ${reboot_mode} = factory_reset; then "\
						"defenv_reserv;save; fi;", 0);
		run_command("if itest ${upgrade_step} == 1; then "\
						"defenv_reserv; setenv upgrade_step 2; saveenv; fi;", 0);
		/*add board late init function here*/
		run_command("kbi check_panel", 0);//kbi check_panel - check TS050 or TS101
		run_command("kbi check_camera", 0);//kbi check_camera - check OS08A10 or IMX415
		run_command("kbi check_m2x", 0);//check M2X
#ifndef DTB_BIND_KERNEL
		{
				/*
				 * 2015.01 had a "store dtb read <addr> [size]" subcommand that
				 * expanded to "emmc dtb_read <addr> 0x40000". In 2019.01 the
				 * reserved-area accessors were folded into "store rsv", so the
				 * dtb is read with "store rsv read dtb <addr> <size>" - which
				 * takes exactly 6 argv entries and needs an explicit size
				 * (DTB_SIZE, see include/emmc_partitions.h).
				 */
				char cmd[64];
				int ret;

				sprintf(cmd, "store rsv read dtb ${dtb_mem_addr} 0x%x",
						DTB_SIZE);
				ret = run_command(cmd, 1);
				if (ret) {
						printf("%s(): [%s] fail\n", __func__, cmd);
#ifdef CONFIG_DTB_MEM_ADDR
						printf("load dtb to %x\n", CONFIG_DTB_MEM_ADDR);
						sprintf(cmd, "store rsv read dtb 0x%x 0x%x",
								CONFIG_DTB_MEM_ADDR, DTB_SIZE);
						ret = run_command(cmd, 1);
						if (ret)
								printf("%s(): %s fail\n", __func__, cmd);
#endif
				}
		}
#elif defined(CONFIG_DTB_MEM_ADDR)
		{
				char cmd[128];
				int ret;
                if (!env_get("dtb_mem_addr")) {
						sprintf(cmd, "setenv dtb_mem_addr 0x%x", CONFIG_DTB_MEM_ADDR);
						run_command(cmd, 0);
				}
				sprintf(cmd, "imgread dtb boot ${dtb_mem_addr}");
				ret = run_command(cmd, 0);
                if (ret) {
						printf("%s(): cmd[%s] fail, ret=%d\n", __func__, cmd, ret);
				}
		}
#endif// #ifndef DTB_BIND_KERNEL

#ifdef CONFIG_POWER_FUSB302
	fusb302_init();
#endif

		/* load unifykey */
		run_command("keyunify init 0x1234", 0);
#ifdef CONFIG_AML_VPU
	vpu_probe();
#endif
	vpp_init();
#ifdef CONFIG_AML_HDMITX20
	hdmi_tx_set_hdmi_5v();
	hdmi_tx_init();
#endif
#ifdef CONFIG_AML_CVBS
	run_command("cvbs init", 0);
#endif
#ifdef CONFIG_AML_LCD
#ifdef CONFIG_TCA6408
        board_lcd_detect();
#endif
	lcd_probe();
#endif
	ext_board_detect();

#ifdef CONFIG_AML_V2_FACTORY_BURN
	if (0x1b8ec003 == readl(P_PREG_STICKY_REG2))
		aml_try_factory_usb_burning(1, gd->bd);
	aml_try_factory_sdcard_burning(0, gd->bd);
#endif// #ifdef CONFIG_AML_V2_FACTORY_BURN

	cpu_id_t cpu_id = get_cpu_id();
	if (cpu_id.family_id == MESON_CPU_MAJOR_ID_G12B) {
		char cmd[16];
		env_set("maxcpus","6");
		sprintf(cmd, "%X", cpu_id.chip_rev);
		env_set("chiprev", cmd);
	}

	return 0;
}
#endif

#ifdef CONFIG_AML_TINY_USBTOOL
int usb_get_update_result(void)
{
	unsigned long upgrade_step;
	upgrade_step = simple_strtoul (getenv ("upgrade_step"), NULL, 16);
	printf("upgrade_step = %d\n", (int)upgrade_step);
	if (upgrade_step == 1)
	{
		run_command("defenv", 1);
		run_command("setenv upgrade_step 2", 1);
		run_command("saveenv", 1);
		return 0;
	}
	else
	{
		return -1;
	}
}
#endif

phys_size_t get_effective_memsize(void)
{
	// >>16 -> MB, <<20 -> real size, so >>16<<20 = <<4
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	return (((readl(AO_SEC_GP_CFG0)) & 0xFFFF0000) << 4) - CONFIG_SYS_MEM_TOP_HIDE;
#else
	return (((readl(AO_SEC_GP_CFG0)) & 0xFFFF0000) << 4);
#endif
}

#ifdef CONFIG_MULTI_DTB
int checkhw(char * name)
{
	/*
	 * use rev id to identify revA/revB
	 */
	cpu_id_t cpu_id;
	cpu_id = get_cpu_id();
	char loc_name[64] = {0};

	printf("cpu_id.chip_rev: %x\n", cpu_id.chip_rev);

	switch (cpu_id.chip_rev) {
		case 0xA:
			/* revA */
			strcpy(loc_name, "g12b_w400_a\0");
			break;
		case 0xB:
			/* revB */
			strcpy(loc_name, "g12b_kvim3\0");
			break;
		default:
			strcpy(loc_name, "g12b_w400_unsupport\0");
			break;
	}
	strcpy(name, loc_name);
	env_set("aml_dt", loc_name);
	return 0;
}
#endif

const char * const _env_args_reserve_[] =
{
		"aml_dt",
		"firstboot",
		"lock",
		"upgrade_step",
		"bootloader_version",

		NULL//Keep NULL be last to tell END
};

/*
 * u-boot 2019.01 requires the board to provide the MMU memory map and the
 * OF_BOARD_SETUP hook; both used to live in SoC code in the older tree.
 */
static struct mm_region bd_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = bd_mem_map;

int print_cpuinfo(void)
{
	printf("print_cpuinfo\n");
	return 0;
}

int mach_cpu_init(void)
{
	printf("mach_cpu_init\n");
	return 0;
}

int ft_board_setup(void *blob, bd_t *bd)
{
	/* eg: bl31/32 rsv */
	return 0;
}
