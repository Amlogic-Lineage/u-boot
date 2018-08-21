/*
 * drivers/display/lcd/lcd_extern/i2c_DLPC3439.c
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
#include <asm/arch/gpio.h>
#ifdef CONFIG_OF_LIBFDT
#include <libfdt.h>
#endif
#ifdef CONFIG_SYS_I2C_AML
#include <aml_i2c.h>
#else
#include <i2c.h>
#include <dm/device.h>
#endif
#include <amlogic/aml_lcd.h>
#include <amlogic/aml_lcd_extern.h>
#include "lcd_extern.h"
#include "../aml_lcd_common.h"
#include "../aml_lcd_reg.h"

//#define LCD_EXT_DEBUG_INFO

#define LCD_EXTERN_INDEX		2
#define LCD_EXTERN_NAME			"i2c_DLPC3439"
#define LCD_EXTERN_TYPE			LCD_EXTERN_I2C

#define LCD_EXTERN_I2C_ADDR		(0x36 >> 1) //7bit address
#define LCD_EXTERN_I2C_BUS		LCD_EXTERN_I2C_BUS_2

#ifdef CONFIG_SYS_I2C_AML
//#define LCD_EXT_I2C_PORT_INIT     /* no need init i2c port default */
#ifdef LCD_EXT_I2C_PORT_INIT
static unsigned int aml_i2c_bus_tmp = LCD_EXTERN_I2C_BUS_INVALID;
#endif
#endif
static struct lcd_extern_config_s *ext_config;

	/** Write: ImageCrop: 1920x1080
	W 36 10 00 00 00 00 80 07 38 04 **/
static unsigned char data_1[] = {0x10, 0x00, 0x00, 0x00, 0x00,
		0x80, 0x07, 0x38, 0x04};
	/** Write: DisplaySize: 1920x1080
	W 36 12 80 07 38 04 **/
static unsigned char data_2[] = {0x12, 0x80, 0x07, 0x38, 0x04};
	/** Write: InputImageSize: 1920x1080
	W 36 2e 80 07 38 04 **/
static unsigned char data_3[] = {0x2e, 0x80, 0x07, 0x38, 0x04};
	/** Write: InputSourceSelect; 0 = External Video Port
	W 36 05 00 **/
static unsigned char data_4[] = {0x05, 0x00};
	/** Write: VideoSourceFormatSelect: 0x43=RGB888
	W 36 07 43 **/
static unsigned char data_5[] = {0x07, 0x43};

#ifdef CONFIG_SYS_I2C_AML
static int lcd_extern_i2c_write(unsigned i2caddr, unsigned char *buff, unsigned len)
{
	int ret = 0;
#ifdef LCD_EXT_DEBUG_INFO
	int i;
#endif
	struct i2c_msg msg;

	msg.addr = i2caddr;
	msg.flags = 0;
	msg.len = len;
	msg.buf = buff;

#ifdef LCD_EXT_DEBUG_INFO
	printf("%s:", __func__);
	for (i = 0; i < len; i++) {
		printf(" 0x%02x", buff[i]);
	}
	printf(" [addr 0x%02x]\n", i2caddr);
#endif

	ret = aml_i2c_xfer(&msg, 1);
	//ret = aml_i2c_xfer_slow(&msg, 1);
	if (ret < 0)
		EXTERR("i2c write failed [addr 0x%02x]\n", i2caddr);

	return ret;
}
#else
static int lcd_extern_i2c_write(unsigned i2caddr, unsigned char *buff, unsigned len)
{
	int ret;
	unsigned char i2c_bus;
	struct udevice *i2c_dev;
#ifdef LCD_EXT_DEBUG_INFO
	int i;
#endif

	i2c_bus = aml_lcd_extern_i2c_bus_get_sys(ext_config->i2c_bus);
	ret = i2c_get_chip_for_busnum(i2c_bus, i2caddr, &i2c_dev);
	if (ret) {
		EXTERR("no sys i2c_bus %d find\n", i2c_bus);
		return ret;
	}

#ifdef LCD_EXT_DEBUG_INFO
	printf("%s:", __func__);
	for (i = 0; i < len; i++) {
		printf(" 0x%02x", buff[i]);
	}
	printf(" [addr 0x%02x]\n", i2caddr);
#endif

	ret = i2c_write(i2c_dev, i2caddr, buff, len);
	if (ret) {
		EXTERR("i2c write failed [addr 0x%02x]\n", i2caddr);
		return ret;
	}

	return 0;
}
#endif

#ifdef LCD_EXT_I2C_PORT_INIT
static int lcd_extern_change_i2c_bus(unsigned int aml_i2c_bus)
{
	int ret = 0;
	extern struct aml_i2c_platform g_aml_i2c_plat;

	if (aml_i2c_bus == LCD_EXTERN_I2C_BUS_INVALID) {
		EXTERR("%s: invalid sys i2c_bus %d\n", __func__, aml_i2c_bus);
		return -1;
	}
	g_aml_i2c_plat.master_no = aml_i2c_bus;
	ret = aml_i2c_init();

	return ret;
}
#endif

static int lcd_extern_power_on(void)
{
	int ret = 0;
#ifdef LCD_EXT_I2C_PORT_INIT
	extern struct aml_i2c_platform g_aml_i2c_plat;
	unsigned char i2c_bus;

	aml_i2c_bus_tmp = g_aml_i2c_plat.master_no;
	i2c_bus = aml_lcd_extern_i2c_bus_get_sys(ext_config->i2c_bus);
#endif

	aml_lcd_extern_pinmux_set(1);
#ifdef LCD_EXT_I2C_PORT_INIT
	lcd_extern_change_i2c_bus(i2c_bus);
	mdelay(10);
#endif

	lcd_extern_i2c_write(ext_config->i2c_addr, data_1, 9);
	lcd_extern_i2c_write(ext_config->i2c_addr, data_2, 5);
	lcd_extern_i2c_write(ext_config->i2c_addr, data_3, 5);
	lcd_extern_i2c_write(ext_config->i2c_addr, data_4, 2);
	lcd_extern_i2c_write(ext_config->i2c_addr, data_5, 2);

#ifdef LCD_EXT_I2C_PORT_INIT
	lcd_extern_change_i2c_bus(aml_i2c_bus_tmp);
#endif

	EXTPR("%s\n", __func__);
	return ret;
}

static int lcd_extern_power_off(void)
{
	int ret = 0;

	aml_lcd_extern_pinmux_set(0);
	return ret;
}

static int lcd_extern_driver_update(struct aml_lcd_extern_driver_s *ext_drv)
{
	int ret = 0;

	if (ext_drv) {
		ext_drv->power_on  = lcd_extern_power_on;
		ext_drv->power_off = lcd_extern_power_off;
	} else {
		EXTERR("%s driver is null\n", LCD_EXTERN_NAME);
		ret = -1;
	}

	return ret;
}

int aml_lcd_extern_i2c_DLPC3439_get_default_index(void)
{
	return LCD_EXTERN_INDEX;
}

int aml_lcd_extern_i2c_DLPC3439_probe(struct aml_lcd_extern_driver_s *ext_drv)
{
	int ret = 0;

	ext_config = ext_drv->config;
	ret = lcd_extern_driver_update(ext_drv);

	if (lcd_debug_print_flag)
		EXTPR("%s: %d\n", __func__, ret);
	return ret;
}

