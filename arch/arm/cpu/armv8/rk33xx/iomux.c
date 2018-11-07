/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <asm/arch/rkplat.h>

#ifdef CONFIG_RK_IOMUX


#if defined(CONFIG_RKCHIP_RK3368)
	#include "iomux-rk3368.c"
#elif defined(CONFIG_RKCHIP_RK3366)
	#include "iomux-rk3366.c"
#elif defined(CONFIG_RKCHIP_RK3399)
	#include "iomux-rk3399.c"
#elif defined(CONFIG_RKCHIP_RK322XH)
	#include "iomux-rk322xh.c"
#else
	#error "PLS config iomux-rkxx.c!"
#endif


void rk_iomux_config(int iomux_id)
{
	switch (iomux_id) {
	case RK_PWM0_IOMUX:
	case RK_PWM1_IOMUX:
	case RK_PWM2_IOMUX:
	case RK_PWM3_IOMUX:
	case RK_PWM4_IOMUX:
	case RK_VOP0_PWM_IOMUX:
	case RK_VOP1_PWM_IOMUX:
		rk_pwm_iomux_config(iomux_id);
		break;
	case RK_I2C0_IOMUX:
	case RK_I2C1_IOMUX:
	case RK_I2C2_IOMUX:
	case RK_I2C3_IOMUX:
	case RK_I2C4_IOMUX:
	case RK_I2C6_IOMUX:
	case RK_I2C7_IOMUX:
	case RK_I2C8_IOMUX:
		rk_i2c_iomux_config(iomux_id);
		break;
	case RK_UART0_IOMUX:
	case RK_UART1_IOMUX:
	case RK_UART2_IOMUX:
	case RK_UART3_IOMUX:
	case RK_UART4_IOMUX:
		rk_uart_iomux_config(iomux_id);
		break;
	case RK_LCDC0_IOMUX:
	case RK_LCDC1_IOMUX:
		rk_lcdc_iomux_config(iomux_id);
		break;
	case RK_SPI0_CS0_IOMUX:
	case RK_SPI0_CS1_IOMUX:
	case RK_SPI1_CS0_IOMUX:
	case RK_SPI1_CS1_IOMUX:
	case RK_SPI2_CS0_IOMUX:
	case RK_SPI2_CS1_IOMUX:
		rk_spi_iomux_config(iomux_id);
		break;
	case RK_EMMC_IOMUX:
		rk_emmc_iomux_config(iomux_id);
		break;
	case RK_SDCARD_IOMUX:
		rk_sdcard_iomux_config(iomux_id);
		break;
	case RK_HDMI_IOMUX:
		rk_hdmi_iomux_config(iomux_id);
		break;
	default:
		printf("RK have not this iomux id!\n");
		break;
	}
}

#else

void rk_iomux_config(int iomux_id) {}
#ifdef CONFIG_RK_SDCARD_BOOT_EN
void rk_iomux_sdcard_save(void) {}
void rk_iomux_sdcard_restore(void) {}
#endif /* CONFIG_RK_SDCARD_BOOT_EN */

#endif /* CONFIG_RK_IOMUX */
