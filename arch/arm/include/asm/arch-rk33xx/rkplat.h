/*
 * (C) Copyright 2008 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKPLAT_H
#define __RKPLAT_H


#include <config.h>
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <fdtdec.h>
#include <asm/io.h>
#include <asm/errno.h>

#include <asm/rk-common/typedef.h>
#include <asm/rk-common/cpu.h>
#include <asm/rk-common/uart.h>
#include <asm/rk-common/usbhost.h>

#include "io.h"
#include "pmu.h"
#include "grf.h"
#include "cru.h"
#include "irqs.h"
#include "gpio.h"
#include "iomux.h"
#include "pwm.h"

#include "clock.h"

#ifdef CONFIG_PM_SUBSYSTEM
#include <asm/rk-common/pm.h>
#endif

#ifdef CONFIG_RK_PL330_DMAC
#include "pl330.h"
#include "dma.h"
#endif

/*************************************************************
**                chip hardware configure                    *
**************************************************************/
/* uart chanel */
typedef enum UART_ch {
	UART_CH0,
	UART_CH1,
	UART_CH2,
	UART_CH3,
	UART_CH4,
	UART_CH5,
	UART_CH6,
	UART_CH7,

	UART_CH_MAX
} eUART_ch_t;

/* i2c bus chanel */
typedef enum rk_i2c_bus_ch {
	I2C_CH0,
	I2C_CH1,
	I2C_CH2,
	I2C_CH3,
	I2C_CH4,
	I2C_CH5,
	I2C_CH6,
	I2C_CH7,
	I2C_CH8,

	I2C_BUS_MAX
} eI2C_ch_t;

/* spi chanel */
typedef enum SPI_ch {
	SPI_CH0,
	SPI_CH1,
	SPI_CH2,
	SPI_CH3,
	SPI_CH4,
	SPI_CH5,
	SPI_CH6,
	SPI_CH7,

	SPI_CH_MAX
} eSPI_ch_t;


#if defined(CONFIG_RKCHIP_RK3368)
	/* Loader Flag regiseter */
	#define RKIO_LOADER_FLAG_REG	(RKIO_PMU_GRF_PHYS + PMU_GRF_OS_REG0)

	/* usb otg */
	#define RKIO_USBOTG_BASE	RKIO_USBOTG_PHYS

	/* timer */
	#define RKIO_TIMER_BASE		RKIO_TIMER0_6CH_PHYS
	#define RKIRQ_TIMER0		IRQ_TIMER0_6CH_0
	#define RKIRQ_TIMER1		IRQ_TIMER0_6CH_1
	#define RKIRQ_TIMER2		IRQ_TIMER0_6CH_2
	#define RKIRQ_TIMER3		IRQ_TIMER0_6CH_3

	/* secure timer */
	#define RKIO_SECURE_TIMER_BASE	(RKIO_SECURE_TIMER_2CH_PHYS + 0x20)

	/* uart */
	#define RKIO_UART0_BASE		RKIO_UART0_BT_PHYS
	#define RKIO_UART1_BASE		RKIO_UART1_BB_PHYS
	#define RKIO_UART2_BASE		RKIO_UART2_DBG_PHYS
	#define RKIO_UART3_BASE		RKIO_UART3_GPS_PHYS
	#define RKIO_UART4_BASE		RKIO_UART4_EXP_PHYS
	#define RKIO_UART5_BASE		0
	#define RKIO_UART6_BASE		0
	#define RKIO_UART7_BASE		0

	/* i2c */
	#define RKIO_I2C0_BASE		RKIO_I2C0_PMU_PHYS
	#define RKIO_I2C1_BASE		RKIO_I2C1_AUDIO_PHYS
	#define RKIO_I2C2_BASE		RKIO_I2C2_SENSOR_PHYS
	#define RKIO_I2C3_BASE		RKIO_I2C3_CAM_PHYS
	#define RKIO_I2C4_BASE		RKIO_I2C4_TP_PHYS
	#define RKIO_I2C5_BASE		RKIO_I2C5_HDMI_PHYS
	#define RKIO_I2C6_BASE		0
	#define RKIO_I2C7_BASE		0

	/* spi */
	#define RKIO_SPI0_BASE		RKIO_SPI0_PHYS
	#define RKIO_SPI1_BASE		RKIO_SPI1_PHYS
	#define RKIO_SPI2_BASE		RKIO_SPI2_PHYS
	#define RKIO_SPI3_BASE		0
	#define RKIO_SPI4_BASE		0
	#define RKIO_SPI5_BASE		0
	#define RKIO_SPI6_BASE		0
	#define RKIO_SPI7_BASE		0

	/* pwm */
	#define RKIO_PWM_BASE		RKIO_RK_PWM_PHYS

	/* pwm remote */
	#define RK_PWM_REMOTE_ID	3
	#define RK_PWM_REMOTE_IOBASE	(RKIO_RK_PWM_PHYS + 0x10 * RK_PWM_REMOTE_ID)
	#define RK_PWM_REMOTE_IRQ	IRQ_RK_PWM

	/* saradc */
	#define RKIO_SARADC_BASE	RKIO_SARADC_PHYS

	/* storage */
	#define RKIO_NANDC_BASE		RKIO_NANDC_PHYS
	#define RKIO_SDMMC_BASE		RKIO_SDMMC_PHYS
	#define RKIO_SDIO_BASE		RKIO_SDIO_PHYS
	#define RKIO_EMMC_BASE		RKIO_EMMC_PHYS

	#define RKIRQ_SDMMC		IRQ_SDMMC
	#define RKIRQ_SDIO		IRQ_SDIO
	#define RKIRQ_EMMC		IRQ_EMMC

	/* efuse */
	#define RKIO_FTEFUSE_BASE	RKIO_EFUSE_256BITS_PHYS
	#define RKIO_SECUREEFUSE_BASE	RKIO_EFUSE_1024BITS_PHYS

	/* crypto */
	#define RKIO_CRYPTO_BASE	RKIO_CRYPTO_PHYS

	/* boot information for kernel */
	#define RKIO_BOOTINFO_BASE	(RKIO_NANDC_BASE + 0x1000)
#elif defined(CONFIG_RKCHIP_RK3366)
	/* Loader Flag regiseter */
	#define RKIO_LOADER_FLAG_REG	(RKIO_PMU_GRF_PHYS + PMU_GRF_OS_REG0)

	/* usb otg */
	#define RKIO_USBOTG_BASE	RKIO_USBOTG_PHYS
	/* usb3 */
	#define RKIO_USB3_BASE		RKIO_USB3_PHYS

	/* timer */
	#define RKIO_TIMER_BASE		RKIO_TIMER_6CH_PHYS
	#define RKIRQ_TIMER0		IRQ_TIMER_6CH_0
	#define RKIRQ_TIMER1		IRQ_TIMER_6CH_1
	#define RKIRQ_TIMER2		IRQ_TIMER_6CH_2
	#define RKIRQ_TIMER3		IRQ_TIMER_6CH_3

	/* secure timer */
	#define RKIO_SECURE_TIMER_BASE	(RKIO_SECURE_TIMER_2CH_PHYS + 0x20)

	/* uart */
	#define RKIO_UART0_BASE		RKIO_UART0_BT_PHYS
	#define RKIO_UART1_BASE		0
	#define RKIO_UART2_BASE		RKIO_UART2_DBG_PHYS
	#define RKIO_UART3_BASE		RKIO_UART3_GPS_PHYS
	#define RKIO_UART4_BASE		0
	#define RKIO_UART5_BASE		0
	#define RKIO_UART6_BASE		0
	#define RKIO_UART7_BASE		0

	/* i2c */
	#define RKIO_I2C0_BASE		RKIO_I2C0_PMU_PHYS
	#define RKIO_I2C1_BASE		RKIO_I2C1_AUDIO_PHYS
	#define RKIO_I2C2_BASE		RKIO_I2C2_SENSOR_PHYS
	#define RKIO_I2C3_BASE		RKIO_I2C3_CAM_PHYS
	#define RKIO_I2C4_BASE		RKIO_I2C4_TP_PHYS
	#define RKIO_I2C5_BASE		RKIO_I2C5_HDMI_PHYS
	#define RKIO_I2C6_BASE		0
	#define RKIO_I2C7_BASE		0

	/* spi */
	#define RKIO_SPI0_BASE		RKIO_SPI0_PHYS
	#define RKIO_SPI1_BASE		RKIO_SPI1_PHYS
	#define RKIO_SPI2_BASE		RKIO_SPI2_PHYS
	#define RKIO_SPI3_BASE		0
	#define RKIO_SPI4_BASE		0
	#define RKIO_SPI5_BASE		0
	#define RKIO_SPI6_BASE		0
	#define RKIO_SPI7_BASE		0

	/* pwm */
	#define RKIO_PWM_BASE		RKIO_RK_PWM_PHYS

	/* pwm remote */
	#define RK_PWM_REMOTE_ID	3
	#define RK_PWM_REMOTE_IOBASE	(RKIO_RK_PWM_PHYS + 0x10 * RK_PWM_REMOTE_ID)
	#define RK_PWM_REMOTE_IRQ	IRQ_RK_PWM

	/* saradc */
	#define RKIO_SARADC_BASE	RKIO_SARADC_PHYS

	/* storage */
	#define RKIO_NANDC_BASE		RKIO_NANDC_PHYS
	#define RKIO_SDMMC_BASE		RKIO_SDMMC_PHYS
	#define RKIO_SDIO_BASE		RKIO_SDIO_PHYS
	#define RKIO_EMMC_BASE		RKIO_EMMC_PHYS

	#define RKIRQ_SDMMC		IRQ_SDMMC
	#define RKIRQ_SDIO		IRQ_SDIO
	#define RKIRQ_EMMC		IRQ_EMMC

	/* efuse */
	#define RKIO_FTEFUSE_BASE	RKIO_EFUSE_256BITS_PHYS
	#define RKIO_SECUREEFUSE_BASE	RKIO_EFUSE_1024BITS_PHYS

	/* crypto */
	#define RKIO_CRYPTO_BASE	RKIO_CRYPTO_PHYS

	/* boot information for kernel */
	#define RKIO_BOOTINFO_BASE	(RKIO_NANDC_BASE + 0x1000)
#elif defined(CONFIG_RKCHIP_RK3399)
	/* Loader Flag regiseter */
	#define RKIO_LOADER_FLAG_REG	(RKIO_PMU_GRF_PHYS + PMU_GRF_OS_REG0)

	/* usb otg */
	#define RKIO_USBOTG_BASE	RKIO_USBOTG0_PHYS
	/*usb3*/
	#define RKIO_USB3_BASE		RKIO_USBOTG0_PHYS

	/* timer */
	#define RKIO_TIMER_BASE		RKIO_TIMER0_6CH_PHYS
	#define RKIRQ_TIMER0		(IRQ_TIMER + 0)
	#define RKIRQ_TIMER1		(IRQ_TIMER + 1)
	#define RKIRQ_TIMER2		(IRQ_TIMER + 2)
	#define RKIRQ_TIMER3		(IRQ_TIMER + 3)

	/* secure timer */
	#define RKIO_SECURE_TIMER_BASE	(RKIO_STIMER0_6CH_PHYS + 0x20)

	/* uart */
	#define RKIO_UART0_BASE		RKIO_UART0_PHYS
	#define RKIO_UART1_BASE		RKIO_UART1_PHYS
	#define RKIO_UART2_BASE		RKIO_UART2_PHYS
	#define RKIO_UART3_BASE		RKIO_UART3_PHYS
	#define RKIO_UART4_BASE		RKIO_UART4_PHYS
	#define RKIO_UART5_BASE		0
	#define RKIO_UART6_BASE		0
	#define RKIO_UART7_BASE		0

	/* i2c */
	#define RKIO_I2C0_BASE		RKIO_I2C0_PHYS
	#define RKIO_I2C1_BASE		RKIO_I2C1_PHYS
	#define RKIO_I2C2_BASE		RKIO_I2C2_PHYS
	#define RKIO_I2C3_BASE		RKIO_I2C3_PHYS
	#define RKIO_I2C4_BASE		RKIO_I2C4_PHYS
	#define RKIO_I2C5_BASE		RKIO_I2C5_PHYS
	#define RKIO_I2C6_BASE		RKIO_I2C6_PHYS
	#define RKIO_I2C7_BASE		RKIO_I2C7_PHYS
	#define RKIO_I2C8_BASE		RKIO_I2C8_PHYS

	/* spi */
	#define RKIO_SPI0_BASE		RKIO_SPI0_PHYS
	#define RKIO_SPI1_BASE		RKIO_SPI1_PHYS
	#define RKIO_SPI2_BASE		RKIO_SPI2_PHYS
	#define RKIO_SPI3_BASE		RKIO_SPI3_PHYS
	#define RKIO_SPI4_BASE		RKIO_SPI4_PHYS
	#define RKIO_SPI5_BASE		RKIO_SPI5_PHYS
	#define RKIO_SPI6_BASE		0
	#define RKIO_SPI7_BASE		0

        /* pwm */
        #define RKIO_PWM_BASE           RKIO_PWM_PHYS

        /* pwm remote */
        #define RK_PWM_REMOTE_ID        3
        #define RK_PWM_REMOTE_IOBASE    (RKIO_PWM_BASE + 0x10 * RK_PWM_REMOTE_ID)
        #define RK_PWM_REMOTE_IRQ       IRQ_PWM

	/* saradc */
	#define RKIO_SARADC_BASE	RKIO_SARADC_PHYS

	/* storage */
	#define RKIO_SDMMC_BASE		RKIO_SDMMC_PHYS
	#define RKIO_SDIO_BASE		RKIO_SDIO_PHYS
	#define RKIO_EMMC_BASE		RKIO_EMMC_PHYS

	#define RKIRQ_SDMMC		IRQ_SDMMC
	#define RKIRQ_SDIO		IRQ_SDIO
	#define RKIRQ_EMMC		IRQ_EMMCCORE

	/* efuse */
	#define RKIO_FTEFUSE_BASE	RKIO_EFUSE0_PHYS
	#define RKIO_SECUREEFUSE_BASE	RKIO_EFUSE1_PHYS

	/* crypto */
	#define RKIO_CRYPTO_BASE	RKIO_CRYPTO0_PHYS

	/* boot information for kernel */
	#define RKIO_BOOTINFO_BASE	RKIO_IMEM1_PHYS
#elif defined(CONFIG_RKCHIP_RK322XH)
	/* Loader Flag regiseter */
	#define RKIO_LOADER_FLAG_REG	(RKIO_GRF_PHYS + GRF_OS_REG0)

	/* usb otg */
	#define RKIO_USBOTG_BASE	RKIO_USB2_OTG_PHYS

	/* timer */
	#define RKIO_TIMER_BASE		RKIO_TIMER0_6CH_PHYS
	#define RKIRQ_TIMER0		IRQ_TIMER0_6CH_0
	#define RKIRQ_TIMER1		IRQ_TIMER0_6CH_1
	#define RKIRQ_TIMER2		IRQ_TIMER0_6CH_2
	#define RKIRQ_TIMER3		IRQ_TIMER0_6CH_3

	/* secure timer */
	#define RKIO_SECURE_TIMER_BASE	(RKIO_SECURE_TIMER_2CH_PHYS + 0x20)

	/* uart */
	#define RKIO_UART0_BASE		RKIO_UART0_PHYS
	#define RKIO_UART1_BASE		RKIO_UART1_PHYS
	#define RKIO_UART2_BASE		RKIO_UART2_PHYS
	#define RKIO_UART3_BASE		0
	#define RKIO_UART4_BASE		0
	#define RKIO_UART5_BASE		0
	#define RKIO_UART6_BASE		0
	#define RKIO_UART7_BASE		0

	/* i2c */
	#define RKIO_I2C0_BASE		RKIO_I2C0_PHYS
	#define RKIO_I2C1_BASE		RKIO_I2C1_PHYS
	#define RKIO_I2C2_BASE		RKIO_I2C2_PHYS
	#define RKIO_I2C3_BASE		RKIO_I2C3_PHYS
	#define RKIO_I2C4_BASE		0
	#define RKIO_I2C5_BASE		0
	#define RKIO_I2C6_BASE		0
	#define RKIO_I2C7_BASE		0

	/* spi */
	#define RKIO_SPI0_BASE		RKIO_SPI_PHYS
	#define RKIO_SPI1_BASE		0
	#define RKIO_SPI2_BASE		0
	#define RKIO_SPI3_BASE		0
	#define RKIO_SPI4_BASE		0
	#define RKIO_SPI5_BASE		0
	#define RKIO_SPI6_BASE		0
	#define RKIO_SPI7_BASE		0

	/* pwm */
	#define RKIO_PWM_BASE		RKIO_PWM_PHYS

	/* pwm remote */
	#define RK_PWM_REMOTE_ID	3
	#define RK_PWM_REMOTE_IOBASE	(RKIO_PWM_PHYS + 0x10 * RK_PWM_REMOTE_ID)
	#define RK_PWM_REMOTE_IRQ	IRQ_RK_PWM

	/* saradc */
	#define RKIO_SARADC_BASE	RKIO_SARADC_PHYS

	/* storage */
	#define RKIO_SDMMC_BASE		RKIO_SDMMC_PHYS
	#define RKIO_SDIO_BASE		RKIO_SDIO_PHYS
	#define RKIO_EMMC_BASE		RKIO_EMMC_PHYS

	#define RKIRQ_SDMMC		IRQ_SDMMC
	#define RKIRQ_SDIO		IRQ_SDIO
	#define RKIRQ_EMMC		IRQ_EMMC

	/* efuse */
	#define RKIO_FTEFUSE_BASE	RKIO_EFUSE_NS_PHYS
	#define RKIO_SECUREEFUSE_BASE	RKIO_EFUSE_S_PHYS

	/* crypto */
	#define RKIO_CRYPTO_BASE	RKIO_CRYPTO_PHYS

	/* boot information for kernel */
	#define RKIO_BOOTINFO_BASE	RKIO_IMEM_PHYS
#else
	#error "PLS config chiptype for hardware!"
#endif


/*************************************************************
**                     api function                          *
**************************************************************/
int rk_get_chiptype(void);
uint8 rk_get_cpu_version(void);
int rk_get_bootrom_chip_version(unsigned int chip_info[]);
void rk_module_deinit(void);
#ifdef CONFIG_RK_MCU
void rk_mcu_init(void);
#endif

#ifdef CONFIG_RK_SDHCI
int rk_sdhci_init(u32 regbase, u32 emmc_freq);
#endif

#endif /* __RKPLAT_H */
