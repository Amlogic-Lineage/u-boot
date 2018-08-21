/*
 * drivers/display/lcd/bl_extern/i2c_lp8556.c
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
#include <amlogic/aml_bl_extern.h>
#include "bl_extern.h"
#include "../aml_lcd_common.h"
#include "../aml_lcd_reg.h"

//#define BL_EXT_DEBUG_INFO

#define BL_EXTERN_INDEX			1
#define BL_EXTERN_NAME			"i2c_lp8556"
#define BL_EXTERN_TYPE			BL_EXTERN_I2C

#define BL_EXTERN_I2C_ADDR		(0x58 >> 1) //7bit address
#define BL_EXTERN_I2C_BUS		BL_EXTERN_I2C_BUS_C

#ifdef CONFIG_SYS_I2C_AML
//#define BL_EXT_I2C_PORT_INIT     /* no need init i2c port default */
#ifdef BL_EXT_I2C_PORT_INIT
static unsigned int aml_i2c_bus_tmp = BL_EXTERN_I2C_BUS_INVALID;
#endif
#endif

static unsigned int bl_status = 0;

#define BL_EXTERN_CMD_SIZE        4
static unsigned char init_on_table[] = {
	0x00, 0xa2, 0x20, 0x00,
	0x00, 0xa5, 0x54, 0x00,
	0x00, 0x00, 0xff, 0x00,
	0x00, 0x01, 0x05, 0x00,
	0x00, 0xa2, 0x20, 0x00,
	0x00, 0xa5, 0x54, 0x00,
	0x00, 0xa1, 0xb7, 0x00,
	0x00, 0xa0, 0xff, 0x00,
	0x00, 0x00, 0x80, 0x00,
	0xff, 0x00, 0x00, 0x00, //ending
};

static unsigned char init_off_table[] = {
	0xFF, 0x00, 0x00, 0x00, //ending
};

#ifdef CONFIG_SYS_I2C_AML
static int i2c_lp8556_write(unsigned i2caddr, unsigned char *buff, unsigned len)
{
	int ret = 0;
	struct i2c_msg msg;
#ifdef BL_EXT_DEBUG_INFO
	int i;
#endif

	msg.addr = i2caddr;
	msg.flags = 0;
	msg.len = len;
	msg.buf = buff;

#ifdef BL_EXT_DEBUG_INFO
	printf("%s:", __func__);
	for (i = 0; i < len; i++)
		printf(" 0x%02x", buff[i]);
	printf(" [addr 0x%02x]\n", i2caddr);
#endif

	ret = aml_i2c_xfer(&msg, 1);
	//ret = aml_i2c_xfer_slow(&msg, 1);
	if (ret < 0)
		BLEXERR("i2c write failed [addr 0x%02x]\n", i2caddr);

	return ret;
}
#else
static int i2c_lp8556_write(unsigned i2caddr, unsigned char *buff, unsigned len)
{
	int ret;
	struct aml_bl_extern_driver_s *bl_extern = aml_bl_extern_get_driver();
	struct udevice *i2c_dev;
	unsigned char i2c_bus;
#ifdef LCD_EXT_DEBUG_INFO
	int i;
#endif

	i2c_bus = aml_bl_extern_i2c_bus_get_sys(bl_extern->config->i2c_bus);
	ret = i2c_get_chip_for_busnum(i2c_bus, i2caddr, &i2c_dev);
	if (ret) {
		EXTERR("no sys i2c_bus %d find\n", i2c_bus);
		return ret;
	}

#ifdef LCD_EXT_DEBUG_INFO
	printf("%s:", __func__);
	for (i = 0; i < len; i++)
		printf(" 0x%02x", buff[i]);
	printf(" [addr 0x%02x]\n", i2caddr);
#endif

	ret = i2c_write(i2c_dev, i2caddr, buff, len);
	if (ret) {
		BLEXERR("i2c write failed [addr 0x%02x]\n", i2caddr);
		return ret;
	}

	return 0;
}
#endif

static int i2c_lp8556_power_cmd(unsigned char *init_table)
{
	int len;
	int i = 0;
	int ret = 0;
	struct aml_bl_extern_driver_s *bl_extern = aml_bl_extern_get_driver();

	len = BL_EXTERN_CMD_SIZE;
	while (i <= BL_EXTERN_INIT_TABLE_MAX) {
		if (init_table[i] == BL_EXTERN_INIT_END) {
			break;
		} else if (init_table[i] == BL_EXTERN_INIT_NONE) {
			//do nothing, only for delay
		} else if (init_table[i] == BL_EXTERN_INIT_CMD) {
			ret = i2c_lp8556_write(bl_extern->config->i2c_addr,
				&init_table[i+1], (len-2));
		} else {
			BLEXERR("%s(%d: %s): pwoer_type %d is invalid\n",
				__func__, bl_extern->config->index,
				bl_extern->config->name, bl_extern->config->type);
		}
		if (init_table[i+len-1] > 0)
			mdelay(init_table[i+len-1]);
		i += len;
	}

	return ret;
}

#ifdef BL_EXT_I2C_PORT_INIT
static int bl_extern_change_i2c_bus(unsigned int aml_i2c_bus)
{
	int ret = 0;
	extern struct aml_i2c_platform g_aml_i2c_plat;

	if (aml_i2c_bus == BL_EXTERN_I2C_BUS_INVALID) {
		BLEXERR("%s: invalid sys i2c_bus %d\n", __func__, aml_i2c_bus);
		return -1;
	}
	g_aml_i2c_plat.master_no = aml_i2c_bus;
	ret = aml_i2c_init();

	return ret;
}
#endif

static int i2c_lp8556_power_ctrl(int flag)
{
#ifdef BL_EXT_I2C_PORT_INIT
	extern struct aml_i2c_platform g_aml_i2c_plat;
	unsigned char i2c_bus;
#endif
	struct aml_bl_extern_driver_s *bl_extern = aml_bl_extern_get_driver();
	int ret = 0;

	if (bl_status) {
		/* step 1: power prepare */
#ifdef BL_EXT_I2C_PORT_INIT
		aml_i2c_bus_tmp = g_aml_i2c_plat.master_no;
		i2c_bus = aml_bl_extern_i2c_bus_get_sys(bl_extern->config->i2c_bus);
		bl_extern_change_i2c_bus(i2c_bus);
#endif

		/* step 2: power cmd */
		if (flag)
			ret = i2c_lp8556_power_cmd(init_on_table);
		else
			ret = i2c_lp8556_power_cmd(init_off_table);

		/* step 3: power finish */
#ifdef BL_EXT_I2C_PORT_INIT
		bl_extern_change_i2c_bus(aml_i2c_bus_tmp);
#endif
	}
	BLEX("%s(%d: %s): %d\n",
		__func__, bl_extern->config->index,
		bl_extern->config->name, flag);
	return ret;
}

static int i2c_lp8556_power_on(void)
{
	int ret;

	bl_status = 1;
	ret = i2c_lp8556_power_ctrl(1);

	return ret;

}

static int i2c_lp8556_power_off(void)
{
	int ret;

	bl_status = 0;
	ret = i2c_lp8556_power_ctrl(0);
	return ret;

}

static int i2c_lp8556_set_level(unsigned int level)
{
	unsigned char tData[3];
	int ret = 0;
	struct aml_lcd_drv_s *bl_drv = aml_lcd_get_driver();
	struct aml_bl_extern_driver_s *bl_extern = aml_bl_extern_get_driver();
	unsigned int level_max, level_min;
	unsigned int dim_max, dim_min;

	if (bl_drv == NULL)
		return -1;
	level_max = bl_drv->bl_config->level_max;
	level_min = bl_drv->bl_config->level_min;
	dim_max = bl_extern->config->dim_max;
	dim_min = bl_extern->config->dim_min;
	level = dim_min - ((level - level_min) * (dim_min - dim_max)) /
			(level_max - level_min);
	level &= 0xff;

	if (bl_status) {
		tData[0] = 0x0;
		tData[1] = level;
		ret = i2c_lp8556_write(bl_extern->config->i2c_addr, tData, 2);
	}
	return ret;
}

static int i2c_lp8556_update(void)
{
	struct aml_bl_extern_driver_s *bl_extern = aml_bl_extern_get_driver();

	if (bl_extern == NULL) {
		BLEXERR("%s driver is null\n", BL_EXTERN_NAME);
		return -1;
	}

	if (bl_extern->config->type == BL_EXTERN_MAX) {
		bl_extern->config->index = BL_EXTERN_INDEX;
		bl_extern->config->type = BL_EXTERN_TYPE;
		strcpy(bl_extern->config->name, BL_EXTERN_NAME);
		bl_extern->config->i2c_addr = BL_EXTERN_I2C_ADDR;
		bl_extern->config->i2c_bus = BL_EXTERN_I2C_BUS;
	}

	bl_extern->device_power_on = i2c_lp8556_power_on;
	bl_extern->device_power_off = i2c_lp8556_power_off;
	bl_extern->device_bri_update = i2c_lp8556_set_level;

	return 0;
}

int i2c_lp8556_probe(void)
{
	int ret = 0;

	ret = i2c_lp8556_update();
	if (lcd_debug_print_flag)
		BLEX("%s: %d\n", __func__, ret);

	return ret;
}

