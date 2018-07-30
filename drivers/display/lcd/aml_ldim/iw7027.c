/*
 * drivers/display/lcd/lcd_bl_ldim/iw7027.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the named License,
 * or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <malloc.h>
#include <spi.h>
#include <asm/arch/gpio.h>
#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif
#include <amlogic/aml_lcd.h>
#include <amlogic/aml_ldim.h>
#include "../aml_lcd_reg.h"
#include "../aml_lcd_common.h"
#include "ldim_drv.h"
#include "ldim_dev_drv.h"

#ifdef CONFIG_AML_SPICC

#define NORMAL_MSG            (0<<7)
#define BROADCAST_MSG         (1<<7)
#define BLOCK_DATA            (0<<6)
#define SINGLE_DATA           (1<<6)
#define IW7027_DEV_ADDR        1

static int iw7027_on_flag;

struct iw7027 {
	int cs_hold_delay;
	int cs_clk_delay;
	int cmd_size;
	unsigned char *init_data;
	struct spi_slave *spi;
};
struct iw7027 *bl_iw7027;

static unsigned char *val_brightness;

extern struct ldim_spi_dev_info_s ldim_spi_dev;

//iw7027 register write
static int iw7027_wreg(struct spi_slave *slave, u8 addr, u8 val)
{
	u8 tbuf[3];
	int ret;

	if (lcd_debug_print_flag)
		LDIMPR("%s: 0x%02x = 0x%02x\n", __func__, addr, val);

	ret = spi_claim_bus(slave);
	if (ret) {
		LDIMERR("%s: request spi bus failed\n", __func__);
		goto end;
	}
	if (bl_iw7027->cs_hold_delay)
		udelay(bl_iw7027->cs_hold_delay);
	spi_cs_activate(slave);
	if (bl_iw7027->cs_clk_delay)
		udelay(bl_iw7027->cs_clk_delay);

	tbuf[0] = NORMAL_MSG | SINGLE_DATA | IW7027_DEV_ADDR;
	tbuf[1] = addr & 0x7f;
	tbuf[2] = val;
	ret = spi_xfer(slave, 24, tbuf, 0, 0);
	if (bl_iw7027->cs_clk_delay)
		udelay(bl_iw7027->cs_clk_delay);
	spi_cs_deactivate(slave);

end:
	spi_release_bus(slave);
	return ret;
}

//iw7027 register read
static int iw7027_rreg(struct spi_slave *slave, u8 addr, u8 *val)
{
	u8 tbuf[4], rbuf[4], temp;
	int ret;

	/*set read flag*/
	temp = (addr >= 0x80) ? 0x80 : 0x0;
	iw7027_wreg(slave, 0x78, temp);

	ret = spi_claim_bus(slave);
	if (ret) {
		LDIMERR("%s: request spi bus failed\n", __func__);
		goto end;
	}
	if (bl_iw7027->cs_hold_delay)
		udelay(bl_iw7027->cs_hold_delay);
	spi_cs_activate(slave);
	if (bl_iw7027->cs_clk_delay)
		udelay(bl_iw7027->cs_clk_delay);

	tbuf[0] = NORMAL_MSG | SINGLE_DATA | IW7027_DEV_ADDR;
	tbuf[1] = addr | 0x80;
	ret = spi_xfer(slave, 32, tbuf, rbuf, 0);
	if (ret)
		goto end;
	*val = rbuf[3];
	if (bl_iw7027->cs_clk_delay)
		udelay(bl_iw7027->cs_clk_delay);
	spi_cs_deactivate(slave);

end:
	spi_release_bus(slave);
	return ret;
}

//iw7027 block write
static int iw7027_wregs(struct spi_slave *slave, u8 addr, u8 *val, int len)
{
	u8 tbuf[30];
	int size, i;
	int ret;

	if (lcd_debug_print_flag) {
		LDIMPR("%s: ", __func__);
		for (i = 0; i < len; i++)
			printf("0x%02x ", val[i]);
		printf("\n");
	}

	ret = spi_claim_bus(slave);
	if (ret) {
		LDIMERR("%s: request spi bus failed\n", __func__);
		goto end;
	}
	if (bl_iw7027->cs_hold_delay)
		udelay(bl_iw7027->cs_hold_delay);
	spi_cs_activate(slave);
	if (bl_iw7027->cs_clk_delay)
		udelay(bl_iw7027->cs_clk_delay);

	tbuf[0] = NORMAL_MSG | BLOCK_DATA | IW7027_DEV_ADDR;
	tbuf[1] = len;
	tbuf[2] = addr & 0x7f;
	size = (len + 3) * 8;
	memcpy(&tbuf[3], val, len);
	ret = spi_xfer(slave, size, tbuf, 0, 0);
	if (bl_iw7027->cs_clk_delay)
		udelay(bl_iw7027->cs_clk_delay);
	spi_cs_deactivate(slave);

end:
	spi_release_bus(slave);
	return ret;
}

static int iw7027_power_on_init(void)
{
	unsigned char addr, val;
	int i, ret = 0;

	for (i = 0; i < LDIM_SPI_INIT_ON_SIZE; i += bl_iw7027->cmd_size) {
		if (bl_iw7027->init_data[i] == 0xff) {
			if (bl_iw7027->init_data[i+3] > 0)
				mdelay(bl_iw7027->init_data[i+3]);
			break;
		} else if (bl_iw7027->init_data[i] == 0x0) {
			addr = bl_iw7027->init_data[i+1];
			val = bl_iw7027->init_data[i+2];
			ret = iw7027_wreg(bl_iw7027->spi, addr, val);
			udelay(1);
		}
		if (bl_iw7027->init_data[i+3] > 0)
			mdelay(bl_iw7027->init_data[i+3]);
	}

	return ret;
}

static int iw7027_hw_init_on(void)
{
	int i;
	unsigned char  reg_duty_chk = 0 , reg_chk = 0;
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	/* step 1: system power_on */
	LDIMPR("%s: iw7027 system power_on\n", __func__);
	ldim_set_gpio(ldim_drv->ldev_conf->en_gpio, ldim_drv->ldev_conf->en_gpio_on);
	ldim_set_duty_pwm(&(ldim_drv->ldev_conf->pwm_config));

	/* step 2: delay for internal logic stable */
	mdelay(10);

	/* step 3: SPI communication check */
	LDIMPR("%s: SPI Communication Check\n", __func__);
	for (i = 0; i <= 10; i++) {
		iw7027_wreg(bl_iw7027->spi, 0x00, 0x06);
		iw7027_rreg(bl_iw7027->spi, 0x00, &reg_chk);
		if (reg_chk == 0x06)
			break;
		if (i == 10) {
			LDIMERR("%s: SPI communication check error\n",
				__func__);
		}
	}

	/* step 4: configure initial registers */
	LDIMPR("%s: Write initial control registers\n", __func__);
	iw7027_power_on_init();

	/* step 5: supply stable vsync */
	LDIMPR("%s: open Vsync\n", __func__);
	ldim_drv->pinmux_ctrl(1);

	/* step 6: delay for system clock and light bar PSU stable */
	mdelay(550);

	/* step 7: start calibration */
	LDIMPR("%s: start calibration\n", __func__);
	iw7027_wreg(bl_iw7027->spi, 0x00, 0x07);
	mdelay(200);

	/* step 8: calibration done or not */
	i = 0;
	while (i++ < 1000) {
		iw7027_rreg(bl_iw7027->spi, 0xb3, &reg_duty_chk);
		/*VDAC statue reg :FB1=[0x5] FB2=[0x50]*/
		/*The current platform using FB1*/
		if ((reg_duty_chk & 0xf) == 0x05)
			break;
		mdelay(1);
	}
	LDIMPR("%s: calibration done: [%d] = %x\n", __func__, i, reg_duty_chk);

	return 0;
}

static int iw7027_hw_init_off(void)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	ldim_drv->pinmux_ctrl(0);
	ldim_set_gpio(ldim_drv->ldev_conf->en_gpio, ldim_drv->ldev_conf->en_gpio_off);

	return 0;
}

static unsigned int dim_max, dim_min;
static unsigned int iw7027_get_value(unsigned int level)
{
	unsigned int val;

	val = dim_min + ((level * (dim_max - dim_min)) / LD_DATA_MAX);

	return val;
}

static int iw7027_smr(unsigned short *buf, unsigned char len)
{
	unsigned int i, temp;
	unsigned short *mapping, num;
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	if (iw7027_on_flag == 0) {
		LDIMPR("%s: on_flag=%d\n", __func__, iw7027_on_flag);
		return 0;
	}
	num = ldim_drv->ldev_conf->bl_regnum;
	if (len != num) {
		LDIMERR("%s: data len %d invalid\n", __func__, len);
		return -1;
	}
	if (val_brightness == NULL) {
		LDIMERR("%s: val_brightness is null\n", __func__);
		return -1;
	}

	dim_max = ldim_drv->ldev_conf->dim_max;
	dim_min = ldim_drv->ldev_conf->dim_min;

	for (i = 0; i < num; i++) {
		temp = iw7027_get_value(buf[i]);
		val_brightness[2*i] = (temp >> 8) & 0xf;
		val_brightness[2*i+1] = temp & 0xff;
	}

	iw7027_wregs(bl_iw7027->spi, 0x40, val_brightness, (num * 2));

	return 0;
}

static int iw7027_power_on(void)
{
	iw7027_hw_init_on();
	iw7027_on_flag = 1;

	LDIMPR("%s: ok\n", __func__);
	return 0;
}

static int iw7027_power_off(void)
{
	iw7027_on_flag = 0;
	iw7027_hw_init_off();

	LDIMPR("%s: ok\n", __func__);
	return 0;
}

static int iw7027_ldim_driver_update(void)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();

	ldim_drv->device_power_on = iw7027_power_on;
	ldim_drv->device_power_off = iw7027_power_off;
	ldim_drv->device_bri_update = iw7027_smr;
	return 0;
}

int ldim_dev_iw7027_probe(void)
{
	char name[30], *str;
	struct udevice *dev;
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();
	int ret;
	printf("%s:%d\n",__func__,__LINE__);

	bl_iw7027 = (struct iw7027 *)malloc(sizeof(struct iw7027));
	if (bl_iw7027 == NULL) {
		LDIMERR("iw7027 malloc error\n");
		return -1;
	}
	memset(bl_iw7027, 0, sizeof(struct iw7027));

	iw7027_on_flag = 0;

	/* register spi */
	snprintf(name, sizeof(name), "generic_%d:%d",
		ldim_drv->spi_dev->bus_num, ldim_drv->spi_dev->chip_select);
	str = strdup(name);
	ret = spi_get_bus_and_cs(ldim_drv->spi_dev->bus_num,
				ldim_drv->spi_dev->chip_select,
				ldim_drv->spi_dev->max_speed_hz,
				ldim_drv->spi_dev->mode, "spi_generic_drv",
				str, &dev, &ldim_drv->spi_dev->spi);
	if (ret) {
		LDIMERR("register ldim_dev spi driver failed\n");
		free(bl_iw7027);
		return -1;
	}
	ldim_drv->spi_dev->spi->wordlen = ldim_drv->spi_dev->wordlen;
	spi_cs_deactivate(ldim_drv->spi_dev->spi);

	bl_iw7027->spi = ldim_drv->spi_dev->spi;
	bl_iw7027->cs_hold_delay = ldim_drv->ldev_conf->cs_hold_delay;
	bl_iw7027->cs_clk_delay = ldim_drv->ldev_conf->cs_clk_delay;
	bl_iw7027->cmd_size = ldim_drv->ldev_conf->cmd_size;
	bl_iw7027->init_data = ldim_drv->ldev_conf->init_on;

	val_brightness = (unsigned char *)malloc(
		ldim_drv->ldev_conf->bl_regnum * 2 * sizeof(unsigned char));
	if (val_brightness == NULL) {
		LDIMERR("malloc val_brightness failed\n");
		free(bl_iw7027);
		return -1;
	}

	iw7027_ldim_driver_update();

	printf("%s: ok\n", __func__);

	return 0;
}

int ldim_dev_iw7027_remove(void)
{
	if (val_brightness) {
		free(val_brightness);
		val_brightness = NULL;
	}

	if (bl_iw7027) {
		free(bl_iw7027);
		bl_iw7027 = NULL;
	}
	return 0;
}
#endif

