/*
 * drivers/display/lcd/lcd_bl_ldim/iw7019.c
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

struct ldim_dev_config_s *ldim_dev_config;

struct ldim_spi_dev_info_s ldim_spi_dev = {
	.modalias = "ldim_dev",
	.mode = SPI_MODE_0,
	.max_speed_hz = 1000000, /* 1MHz */
	.bus_num = 0, /* SPI bus No. */
	.chip_select = 0, /* the device index on the spi bus */
	.wordlen = 8,
};


void ldim_set_gpio(int index, int value)
{
	int gpio;
	char *str;

	if (index >= BL_GPIO_NUM_MAX) {
		LDIMERR("%s: invalid index %d\n", __func__, index);
		return;
	} else {
		str = ldim_dev_config->gpio_name[index];
		gpio = aml_lcd_gpio_name_map_num(str);
	}
	switch (value) {
	case LCD_GPIO_OUTPUT_LOW:
	case LCD_GPIO_OUTPUT_HIGH:
		aml_lcd_gpio_set(gpio, value);
		break;
	case LCD_GPIO_INPUT:
	default:
		value = LCD_GPIO_INPUT;
		aml_lcd_gpio_set(gpio, value);
		break;
	}
	if (lcd_debug_print_flag)
		LDIMPR("set gpio %s[%d] value: %d\n", str, index, value);
}

unsigned int ldim_get_gpio(int index)
{
	int gpio;
	char *str;
	unsigned int value;

	if (index >= BL_GPIO_NUM_MAX) {
		LDIMERR("%s: invalid index %d\n", __func__, index);
		return -1;
	} else {
		str = ldim_dev_config->gpio_name[index];
		gpio = aml_lcd_gpio_name_map_num(str);
	}
	value = aml_lcd_gpio_input_get(gpio);
	return value;
}

static unsigned int pwm_reg[6] = {
	PWM_PWM_A,
	PWM_PWM_B,
	PWM_PWM_C,
	PWM_PWM_D,
	PWM_PWM_E,
	PWM_PWM_F,
};

void ldim_set_duty_pwm(struct bl_pwm_config_s *bl_pwm)
{
	unsigned int pwm_hi = 0, pwm_lo = 0;
	unsigned int port = bl_pwm->pwm_port;
	unsigned int vs[4], ve[4], sw, n, i;

	bl_pwm->pwm_level = bl_pwm->pwm_cnt * bl_pwm->pwm_duty / 100;

	LDIMPR("pwm port %d: duty=%d%%, duty_max=%d, duty_min=%d\n",
		bl_pwm->pwm_port, bl_pwm->pwm_duty,
		bl_pwm->pwm_duty_max, bl_pwm->pwm_duty_min);

	switch (bl_pwm->pwm_method) {
	case BL_PWM_POSITIVE:
		pwm_hi = bl_pwm->pwm_level;
		pwm_lo = bl_pwm->pwm_cnt - bl_pwm->pwm_level;
		break;
	case BL_PWM_NEGATIVE:
		pwm_lo = bl_pwm->pwm_level;
		pwm_hi = bl_pwm->pwm_cnt - bl_pwm->pwm_level;
		break;
	default:
		LDIMERR("port %d: invalid pwm_method %d\n",
			port, bl_pwm->pwm_method);
		break;
	}
	LDIMPR("port %d: pwm_cnt=%d, pwm_hi=%d, pwm_lo=%d\n",
		port, bl_pwm->pwm_cnt, pwm_hi, pwm_lo);

	switch (port) {
	case BL_PWM_A:
	case BL_PWM_B:
	case BL_PWM_C:
	case BL_PWM_D:
	case BL_PWM_E:
	case BL_PWM_F:
		lcd_cbus_write(pwm_reg[port], (pwm_hi << 16) | pwm_lo);
		break;
	case BL_PWM_VS:
		memset(vs, 0xffff, sizeof(unsigned int) * 4);
		memset(ve, 0xffff, sizeof(unsigned int) * 4);
		n = bl_pwm->pwm_freq;
		sw = (bl_pwm->pwm_cnt * 10 / n + 5) / 10;
		pwm_hi = (pwm_hi * 10 / n + 5) / 10;
		pwm_hi = (pwm_hi > 1) ? pwm_hi : 1;
		if (lcd_debug_print_flag)
			LDIMPR("n=%d, sw=%d, pwm_high=%d\n", n, sw, pwm_hi);
		for (i = 0; i < n; i++) {
			vs[i] = 1 + (sw * i);
			ve[i] = vs[i] + pwm_hi - 1;
			if (lcd_debug_print_flag)
				LDIMPR("vs[%d]=%d, ve[%d]=%d\n", i, vs[i], i, ve[i]);
		}
		lcd_vcbus_write(VPU_VPU_PWM_V0, (ve[0] << 16) | (vs[0]));
		lcd_vcbus_write(VPU_VPU_PWM_V1, (ve[1] << 16) | (vs[1]));
		lcd_vcbus_write(VPU_VPU_PWM_V2, (ve[2] << 16) | (vs[2]));
		lcd_vcbus_write(VPU_VPU_PWM_V3, (ve[3] << 16) | (vs[3]));
		break;
	default:
		break;
	}
}

/* set ldim pwm_vs */
static int ldim_pwm_pinmux_ctrl(int status)
{
	struct bl_pwm_config_s *ld_pwm = &ldim_dev_config->pwm_config;
	int i;

	if (ld_pwm->pwm_port >= BL_PWM_MAX)
		return 0;

	if (lcd_debug_print_flag)
		LDIMPR("%s: %d\n", __func__, status);

	if (status) {
		bl_pwm_ctrl(ld_pwm, 1);
		/* set pinmux */
		ld_pwm->pinmux_flag = 1;
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (ld_pwm->pinmux_clr[i][0] == LCD_PINMUX_END)
				break;
			lcd_pinmux_clr_mask(ld_pwm->pinmux_clr[i][0],
				ld_pwm->pinmux_clr[i][1]);
			if (lcd_debug_print_flag) {
				LDIMPR("%s: port=%d, pinmux_clr=%d,0x%08x\n",
					__func__, ld_pwm->pwm_port,
					ld_pwm->pinmux_clr[i][0],
					ld_pwm->pinmux_clr[i][1]);
			}
			i++;
		}
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (ld_pwm->pinmux_set[i][0] == LCD_PINMUX_END)
				break;
			lcd_pinmux_set_mask(ld_pwm->pinmux_set[i][0],
				ld_pwm->pinmux_set[i][1]);
			if (lcd_debug_print_flag) {
				LDIMPR("%s: port=%d, pinmux_set=%d,0x%08x\n",
					__func__, ld_pwm->pwm_port,
					ld_pwm->pinmux_set[i][0],
					ld_pwm->pinmux_set[i][1]);
			}
			i++;
		}
	} else {
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (ld_pwm->pinmux_set[i][0] == LCD_PINMUX_END)
				break;
			lcd_pinmux_clr_mask(ld_pwm->pinmux_set[i][0],
				ld_pwm->pinmux_set[i][1]);
			if (lcd_debug_print_flag) {
				LDIMPR("%s: port=%d, pinmux_clr=%d,0x%08x\n",
					__func__, ld_pwm->pwm_port,
					ld_pwm->pinmux_set[i][0],
					ld_pwm->pinmux_set[i][1]);
			}
			i++;
		}
		ld_pwm->pinmux_flag = 0;

		bl_pwm_ctrl(ld_pwm, 0);
	}

	return 0;
}

static void aml_ldim_device_config_print(void)
{
	struct bl_pwm_config_s *ld_pwm = &ldim_dev_config->pwm_config;

	switch (ldim_dev_config->type) {
	case LDIM_DEV_TYPE_SPI:
		printf("dev_name              = %s\n"
			"cs_hold_delay         = %d\n"
			"cs_clk_delay          = %d\n"
			"en_gpio               = %d\n"
			"en_gpio_on            = %d\n"
			"en_gpio_off           = %d\n"
			"write_check           = %d\n"
			"dim_max               = 0x%03x\n"
			"dim_min               = 0x%03x\n"
			"cmd_size              = %d\n",
			ldim_dev_config->name,
			ldim_dev_config->cs_hold_delay,
			ldim_dev_config->cs_clk_delay,
			ldim_dev_config->en_gpio,
			ldim_dev_config->en_gpio_on,
			ldim_dev_config->en_gpio_off,
			ldim_dev_config->write_check,
			ldim_dev_config->dim_min,
			ldim_dev_config->dim_max,
			ldim_dev_config->cmd_size);
		break;
	case LDIM_DEV_TYPE_I2C:
		break;
	case LDIM_DEV_TYPE_NORMAL:
		printf("dev_name              = %s\n"
			"en_gpio               = %d\n"
			"en_gpio_on            = %d\n"
			"en_gpio_off           = %d\n"
			"dim_max               = %d\n"
			"dim_min               = %d\n",
			ldim_dev_config->name,
			ldim_dev_config->en_gpio,
			ldim_dev_config->en_gpio_on,
			ldim_dev_config->en_gpio_off,
			ldim_dev_config->dim_min,
			ldim_dev_config->dim_max);
		break;
	default:
		break;
	}
	if (ld_pwm->pwm_port < BL_PWM_MAX) {
		printf("pwm_port              = %d\n"
			"pwm_pol               = %d\n"
			"pwm_freq              = %d\n"
			"pwm_duty              = %d%%\n"
			"pinmux_flag           = %d\n",
			ld_pwm->pwm_port, ld_pwm->pwm_method,
			ld_pwm->pwm_freq, ld_pwm->pwm_duty,
			ld_pwm->pinmux_flag);
	}
}

#ifdef CONFIG_OF_LIBFDT
static int aml_ldim_pinmux_load_from_dts(char *dt_addr, struct ldim_dev_config_s *ldev_conf)
{
	int parent_offset;
	char *propdata;
	char propname[30];
	int i, temp, len = 0;
	int ret = 0;
	struct bl_pwm_config_s *ld_pwm = &ldev_conf->pwm_config;

	/* get pinmux */
	sprintf(propname, "/pinmux/%s_pin", ldim_dev_config->pinmux_name);
	parent_offset = fdt_path_offset(dt_addr, propname);
	if (parent_offset < 0) {
		LDIMERR("not find ldim_pwm_pin node\n");
		ld_pwm->pinmux_set[0][0] = LCD_PINMUX_END;
		ld_pwm->pinmux_set[0][1] = 0x0;
		ld_pwm->pinmux_clr[0][0] = LCD_PINMUX_END;
		ld_pwm->pinmux_clr[0][1] = 0x0;
		return -1;
	} else {
		propdata = (char *)fdt_getprop(dt_addr, parent_offset, "amlogic,setmask", &len);
		if (propdata == NULL) {
			LDIMERR("failed to get amlogic,setmask\n");
			ld_pwm->pinmux_set[0][0] = LCD_PINMUX_END;
			ld_pwm->pinmux_set[0][1] = 0x0;
		} else {
			temp = len / 8;
			for (i = 0; i < temp; i++) {
				ld_pwm->pinmux_set[i][0] = be32_to_cpup((((u32*)propdata)+2*i));
				ld_pwm->pinmux_set[i][1] = be32_to_cpup((((u32*)propdata)+2*i+1));
			}
			if (temp < (LCD_PINMUX_NUM - 1)) {
				ld_pwm->pinmux_set[temp][0] = LCD_PINMUX_END;
				ld_pwm->pinmux_set[temp][1] = 0x0;
			}
		}

		propdata = (char *)fdt_getprop(dt_addr, parent_offset, "amlogic,clrmask", &len);
		if (propdata == NULL) {
			LDIMERR("failed to get amlogic,clrmask\n");
			ld_pwm->pinmux_clr[0][0] = LCD_PINMUX_END;
			ld_pwm->pinmux_clr[0][1] = 0x0;
		} else {
			temp = len / 8;
			for (i = 0; i < temp; i++) {
				ld_pwm->pinmux_clr[i][0] = be32_to_cpup((((u32*)propdata)+2*i));
				ld_pwm->pinmux_clr[i][1] = be32_to_cpup((((u32*)propdata)+2*i+1));
			}
			if (temp < (LCD_PINMUX_NUM - 1)) {
				ld_pwm->pinmux_clr[temp][0] = LCD_PINMUX_END;
				ld_pwm->pinmux_clr[temp][1] = 0x0;
			}
		}
		if (lcd_debug_print_flag) {
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (ld_pwm->pinmux_set[i][0] == LCD_PINMUX_END)
					break;
				LDIMPR("pinmux set: %d, 0x%08x\n",
				ld_pwm->pinmux_set[i][0], ld_pwm->pinmux_set[i][1]);
				i++;
			}
			i = 0;
			while (i < LCD_PINMUX_NUM) {
				if (ld_pwm->pinmux_clr[i][0] == LCD_PINMUX_END)
					break;
				LDIMPR("pinmux clr: %d, 0x%08x\n",
				ld_pwm->pinmux_clr[i][0], ld_pwm->pinmux_clr[i][1]);
				i++;
			}
		}
	}

	return ret;
}
#endif

static int aml_ldim_pinmux_load_from_bsp(struct ldim_dev_config_s *ldev_conf)
{
	char propname[50] = "ldim_pwm_vs_pin";
	unsigned int i, j;
	int set_cnt = 0, clr_cnt = 0;
	struct bl_pwm_config_s *ld_pwm = &ldev_conf->pwm_config;

	for (i = 0; i < 2; i++) {
		if (strncmp(ldev_conf->ldim_pinmux->name, "invalid", 7) == 0)
			break;
		if (strncmp(ldev_conf->ldim_pinmux->name, propname, strlen(propname)) == 0) {
			for (j = 0; j < LCD_PINMUX_NUM; j++ ) {
				if (ldev_conf->ldim_pinmux->pinmux_set[j][0] == LCD_PINMUX_END)
					break;
				ld_pwm->pinmux_set[j][0] = ldev_conf->ldim_pinmux->pinmux_set[j][0];
				ld_pwm->pinmux_set[j][1] = ldev_conf->ldim_pinmux->pinmux_set[j][1];
				set_cnt++;
			}
			for (j = 0; j < LCD_PINMUX_NUM; j++ ) {
				if (ldev_conf->ldim_pinmux->pinmux_clr[j][0] == LCD_PINMUX_END)
					break;
				ld_pwm->pinmux_clr[j][0] = ldev_conf->ldim_pinmux->pinmux_clr[j][0];
				ld_pwm->pinmux_clr[j][1] = ldev_conf->ldim_pinmux->pinmux_clr[j][1];
				clr_cnt++;
			}
			break;
		}
		ldev_conf->ldim_pinmux++;
	}
	if (set_cnt < LCD_PINMUX_NUM) {
		ld_pwm->pinmux_set[set_cnt][0] = LCD_PINMUX_END;
		ld_pwm->pinmux_set[set_cnt][1] = 0x0;
	}
	if (clr_cnt < LCD_PINMUX_NUM) {
		ld_pwm->pinmux_clr[clr_cnt][0] = LCD_PINMUX_END;
		ld_pwm->pinmux_clr[clr_cnt][1] = 0x0;
	}

	if (lcd_debug_print_flag) {
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (ld_pwm->pinmux_set[i][0] == LCD_PINMUX_END)
				break;
			LDIMPR("ldim_pinmux set: %d, 0x%08x\n",
				ld_pwm->pinmux_set[i][0], ld_pwm->pinmux_set[i][1]);
			i++;
		}
		i = 0;
		while (i < LCD_PINMUX_NUM) {
			if (ld_pwm->pinmux_clr[i][0] == LCD_PINMUX_END)
				break;
			LDIMPR("ldim_pinmux clr: %d, 0x%08x\n",
				ld_pwm->pinmux_clr[i][0], ld_pwm->pinmux_clr[i][1]);
			i++;
		}
	}

	return 0;
}

#ifdef CONFIG_OF_LIBFDT
static int ldim_dev_get_config_from_dts(char *dt_addr, int index)
{
	int parent_offset, child_offset;
	char propname[30];
	char *propdata;
	char *p;
	const char *str;
	unsigned char cmd_size;
	int temp;
	struct bl_pwm_config_s *ld_pwm = &ldim_dev_config->pwm_config;
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();
	int i, j;
	int ret = 0;

	strcpy(ldim_dev_config->name, "ldim_dev");
	memset(ldim_dev_config->init_on, 0, LDIM_SPI_INIT_ON_SIZE);
	memset(ldim_dev_config->init_off, 0, LDIM_SPI_INIT_OFF_SIZE);
	ldim_dev_config->init_on[0] = 0xff;
	ldim_dev_config->init_off[0] = 0xff;

	if (dt_addr == NULL) {
		LDIMERR("%s: dt_addr is NULL\n", __func__);
		return -1;
	}

	parent_offset = fdt_path_offset(dt_addr, "/local_dimming_device");
	if (parent_offset < 0) {
		parent_offset = fdt_path_offset(dt_addr, "/local_diming_device");
		if (parent_offset < 0) {
			LDIMERR("not find /local_dimming_device node: %s\n",
				fdt_strerror(parent_offset));
			return -1;
		}
	}
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (propdata == NULL) {
		LDIMERR("not find local_dimming_device status, default to disabled\n");
		return -1;
	} else {
		if (strncmp(propdata, "okay", 2)) {
			LDIMPR("local_dimming_device status disabled\n");
			return -1;
		}
	}

	/* init gpio */
	i = 0;
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "ldim_dev_gpio_names", NULL);
	if (propdata == NULL) {
		LDIMERR("failed to get ldim_dev_gpio_names\n");
	} else {
		p = propdata;
		while (i < BL_GPIO_NUM_MAX) {
			if (i > 0)
				p += strlen(p) + 1;
			str = p;
			if (strlen(str) == 0)
				break;
			strcpy(ldim_dev_config->gpio_name[i], str);
			if (lcd_debug_print_flag)
				LDIMPR("i=%d, gpio=%s\n", i, ldim_dev_config->gpio_name[i]);
			i++;
		}
	}
	for (j = i; j < BL_GPIO_NUM_MAX; j++) {
		strcpy(ldim_dev_config->gpio_name[j], "invalid");
	}

	/* get device config */
	sprintf(propname,"/local_dimming_device/ldim_dev_%d", index);
	child_offset = fdt_path_offset(dt_addr, propname);
	if (child_offset < 0) {
		sprintf(propname,"/local_diming_device/ldim_dev_%d", index);
		child_offset = fdt_path_offset(dt_addr, propname);
		if (child_offset < 0) {
			LDIMERR("not find %s node: %s\n",
				propname, fdt_strerror(child_offset));
			return -1;
		}
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_dev_name", NULL);
	if (propdata == NULL)
		LDIMERR("failed to get ldim_dev_name\n");
	else
		strcpy(ldim_dev_config->name, propdata);
	LDIMPR("get config: %s(%d)\n", ldim_dev_config->name, index);

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_pwm_port", NULL);
	if (propdata == NULL) {
		LDIMERR("failed to get ldim_pwm_port\n");
		ld_pwm->pwm_port = BL_PWM_MAX;
	} else {
		ld_pwm->pwm_port = bl_pwm_str_to_pwm(propdata);
	}
	LDIMPR("pwm_port: %s(%u)\n", propdata, ld_pwm->pwm_port);
	if (ld_pwm->pwm_port < BL_PWM_MAX) {
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_pwm_attr", NULL);
		if (propdata == NULL) {
			LDIMERR("failed to get ldim_pwm_attr\n");
			ld_pwm->pwm_method = BL_PWM_POSITIVE;
			if (ld_pwm->pwm_port == BL_PWM_VS)
				ld_pwm->pwm_freq = 1;
			else
				ld_pwm->pwm_freq = 60;
			ld_pwm->pwm_duty = 50;
		} else {
			ld_pwm->pwm_method = be32_to_cpup((u32*)propdata);
			ld_pwm->pwm_freq = be32_to_cpup((((u32*)propdata)+1));
			ld_pwm->pwm_duty = be32_to_cpup((((u32*)propdata)+2));
		}
		if (ld_pwm->pwm_port == BL_PWM_VS) {
			if (ld_pwm->pwm_freq > 4) {
				LDIMERR("pwm_vs wrong freq %d\n", ld_pwm->pwm_freq);
				ld_pwm->pwm_freq = BL_FREQ_VS_DEFAULT;
			}
		} else {
			if (ld_pwm->pwm_freq > XTAL_HALF_FREQ_HZ)
				ld_pwm->pwm_freq = XTAL_HALF_FREQ_HZ;
		}
		LDIMPR("get pwm pol = %d, freq = %d, duty = %d%%\n",
			ld_pwm->pwm_method, ld_pwm->pwm_freq, ld_pwm->pwm_duty);
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "en_gpio_on_off", NULL);
	if (propdata == NULL) {
		LDIMERR("failed to get en_gpio_on_off\n");
	} else {
		ldim_dev_config->en_gpio = be32_to_cpup((u32*)propdata);
		ldim_dev_config->en_gpio_on = be32_to_cpup((((u32*)propdata)+1));
		ldim_dev_config->en_gpio_off = be32_to_cpup((((u32*)propdata)+2));
	}
	if (lcd_debug_print_flag) {
		LDIMPR("en_gpio=%s(%d), en_gpio_on=%d, en_gpio_off=%d\n",
		ldim_dev_config->gpio_name[ldim_dev_config->en_gpio],
		ldim_dev_config->en_gpio, ldim_dev_config->en_gpio_on,
		ldim_dev_config->en_gpio_off);
	}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "dim_max_min", NULL);
	if (propdata == NULL) {
		LDIMERR("failed to get dim_max_min\n");
	} else {
		ldim_dev_config->dim_max = be32_to_cpup((u32*)propdata);
		ldim_dev_config->dim_min = be32_to_cpup((((u32*)propdata)+1));
	}
	if (lcd_debug_print_flag) {
		LDIMPR("dim_max=0x%03x, dim_min=0x%03x\n",
		ldim_dev_config->dim_max, ldim_dev_config->dim_min);
	}

	temp = ldim_drv->ldim_conf->row * ldim_drv->ldim_conf->col;
	ldim_dev_config->bl_regnum = (unsigned short)temp;

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "type", NULL);
	if (propdata == NULL)
		LDIMERR("failed to get type\n");
	else {
		ldim_dev_config->type = be32_to_cpup((u32*)propdata);
		LDIMPR("type: %d\n", ldim_dev_config->type);
		}

	propdata = (char *)fdt_getprop(dt_addr, child_offset, "ldim_pwm_pinmux_sel", NULL);
	if (propdata == NULL)
		LDIMERR("failed to get ldim_pwm_name\n");
	else
		strcpy(ldim_dev_config->pinmux_name, propdata);
	LDIMPR("ldim_pwm_pinmux_sel: %s\n", ldim_dev_config->pinmux_name);

	if (ldim_dev_config->type >= LDIM_DEV_TYPE_MAX) {
		LDIMERR("type num is out of support\n");
		return -1;
	}

	switch (ldim_dev_config->type) {
	case LDIM_DEV_TYPE_SPI:
		ldim_drv->spi_dev = &ldim_spi_dev;
		/* get spi config */
		/*
		propdata = (char *)fdt_getprop(dt_addr, parent_offset, "spi_bus_num", NULL);
		if (propdata == NULL)
			LDIMERR("failed to get spi_bus_num\n");
		else
			ldim_spi_dev.bus_num = be32_to_cpup((u32*)propdata);
		*/

		ldim_spi_dev.bus_num = 0; /* fix value */
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_chip_select", NULL);
		if (propdata == NULL)
			LDIMERR("failed to get spi_chip_select\n");
		else
			ldim_spi_dev.chip_select = be32_to_cpup((u32*)propdata);

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_max_frequency", NULL);
		if (propdata == NULL)
			LDIMERR("failed to get spi_max_frequency\n");
		else
			ldim_spi_dev.max_speed_hz = be32_to_cpup((u32*)propdata);

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_mode", NULL);
		if (propdata == NULL)
			LDIMERR("failed to get spi_mode\n");
		else
			ldim_spi_dev.mode = be32_to_cpup((u32*)propdata);

		if (lcd_debug_print_flag) {
			LDIMPR("spi bus_num=%d, chip_select=%d, max_frequency=%d, mode=%d\n",
				ldim_spi_dev.bus_num, ldim_spi_dev.chip_select,
				ldim_spi_dev.max_speed_hz, ldim_spi_dev.mode);
		}

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_cs_delay", NULL);
		if (propdata == NULL) {
			LDIMERR("failed to get spi_cs_delay\n");
		} else {
			ldim_dev_config->cs_hold_delay = be32_to_cpup((u32*)propdata);
			ldim_dev_config->cs_clk_delay = be32_to_cpup((((u32*)propdata)+1));
		}
		if (lcd_debug_print_flag) {
			LDIMPR("cs_hold_delay=%dus, cs_clk_delay=%dus\n",
				ldim_dev_config->cs_hold_delay, ldim_dev_config->cs_clk_delay);
		}

		propdata = (char *)fdt_getprop(dt_addr, child_offset, "spi_write_check", NULL);
		if (propdata == NULL)
			LDIMERR("failed to get spi_write_check\n");
		else
			ldim_dev_config->write_check = (unsigned char)(be32_to_cpup((u32*)propdata));
		if (lcd_debug_print_flag)
			LDIMPR("write_check=%d\n", ldim_dev_config->write_check);

		/* get init_cmd */
		propdata = (char *)fdt_getprop(dt_addr, child_offset, "cmd_size", NULL);
		if (propdata == NULL) {
			LDIMERR("failed to get cmd_size\n");
		} else {
			temp = be32_to_cpup((u32*)propdata);
			if (temp > 1)
				ldim_dev_config->cmd_size = (unsigned char)temp;
			else
				ldim_dev_config->cmd_size = 1;
		}
		if (lcd_debug_print_flag)
			LDIMPR("cmd_size=%d\n", ldim_dev_config->cmd_size);
		cmd_size = ldim_dev_config->cmd_size;
		if (cmd_size > 1) {
			propdata = (char *)fdt_getprop(dt_addr, child_offset, "init_on", NULL);
			if (propdata == NULL) {
				LDIMPR("no init_on\n");
				ldim_dev_config->init_on[0] = 0xff;
			} else {
				i = 0;
				while (i < LDIM_SPI_INIT_ON_SIZE) {
					for (j = 0; j < cmd_size; j++) {
						ldim_dev_config->init_on[i+j] =
							(unsigned char)(be32_to_cpup((((u32*)propdata)+i+j)));
					}
					if (ldim_dev_config->init_on[i] == 0xff)
						break;
					else
						i += cmd_size;
				}
			}
			propdata = (char *)fdt_getprop(dt_addr, child_offset, "init_off", NULL);
			if (propdata == NULL) {
				LDIMPR("no init_off\n");
				ldim_dev_config->init_off[0] = 0xff;
			} else {
				i = 0;
				while (i < LDIM_SPI_INIT_OFF_SIZE) {
					for (j = 0; j < cmd_size; j++) {
						ldim_dev_config->init_off[i+j] =
							(unsigned char)(be32_to_cpup((((u32*)propdata)+i+j)));
					}
					if (ldim_dev_config->init_off[i] == 0xff)
						break;
					else
						i += cmd_size;
				}
			}
		}
		break;
	case LDIM_DEV_TYPE_I2C:
		break;
	case LDIM_DEV_TYPE_NORMAL:
	default:
		break;
	}

	/* pinmux */
	/* new kernel dts pinctrl detect */
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "pinctrl_version", NULL);
	if (propdata)
		ldim_dev_config->pinctrl_ver = (unsigned char)(be32_to_cpup((u32*)propdata));
	LDIMPR("pinctrl_version: %d\n", ldim_dev_config->pinctrl_ver);

	switch (ldim_dev_config->pinctrl_ver) {
	case 0:
		ret = aml_ldim_pinmux_load_from_dts(dt_addr, ldim_drv->ldev_conf);
		break;
	case 1:
		ret = aml_ldim_pinmux_load_from_bsp(ldim_drv->ldev_conf);
		break;
	default:
		ret = aml_ldim_pinmux_load_from_dts(dt_addr, ldim_drv->ldev_conf);
		break;
	}

	return ret;
}
#endif

static int ldim_dev_add_driver(struct ldim_dev_config_s *ldev_conf, int index)
{
	int ret = -1;

#ifdef CONFIG_AML_SPICC
	if (strcmp(ldev_conf->name, "iw7019") == 0) {
#ifdef CONFIG_AML_LOCAL_DIMMING_IW7019
		ret = ldim_dev_iw7019_probe();
#endif
		goto ldim_dev_add_driver_next;
	}

	if (strcmp(ldev_conf->name, "iw7027") == 0) {
#ifdef CONFIG_AML_LOCAL_DIMMING_IW7027
		ret = ldim_dev_iw7027_probe();
#endif

		goto ldim_dev_add_driver_next;
	}

#else
	LDIMERR("%s: no AML_SPICC config\n", __func__);
	ret = -1;
#endif

	if (strcmp(ldev_conf->name, "ob3350") == 0) {
#ifdef CONFIG_AML_LOCAL_DIMMING_OB3350
		ret = ldim_dev_ob3350_probe();
#endif
		goto ldim_dev_add_driver_next;
	}

	if (strcmp(ldev_conf->name, "global") == 0) {
#ifdef CONFIG_AML_LOCAL_DIMMING_GLOBAL
		ret = ldim_dev_global_probe();
#endif
		goto ldim_dev_add_driver_next;
	}

	LDIMERR("invalid device name: %s\n", ldev_conf->name);
	ret = -1;

ldim_dev_add_driver_next:
	if (ret) {
		LDIMERR("add device driver failed %s(%d)\n",
			ldev_conf->name, index);
	} else {
		LDIMPR("add device driver %s(%d)\n", ldev_conf->name, index);
	}

	return ret;
}

static int ldim_dev_remove_driver(struct ldim_dev_config_s *ldev_conf,
		int index)
{
	int ret = -1;

#ifdef CONFIG_AML_SPICC
	if (strcmp(ldev_conf->name, "iw7019") == 0) {
#ifdef CONFIG_AML_LOCAL_DIMMING_IW7019
		ret = ldim_dev_iw7019_remove();
#endif
		goto ldim_dev_remove_driver_next;
	}

	if (strcmp(ldev_conf->name, "iw7027") == 0) {
#ifdef CONFIG_AML_LOCAL_DIMMING_IW7027
		ret = ldim_dev_iw7027_remove();
#endif
		goto ldim_dev_remove_driver_next;
	}

#else
	LDIMERR("%s: no AML_SPICC config\n", __func__);
	ret = -1;
#endif

	if (strcmp(ldev_conf->name, "ob3350") == 0) {
#ifdef CONFIG_AML_LOCAL_DIMMING_OB3350
		ret = ldim_dev_ob3350_remove();
#endif
		goto ldim_dev_remove_driver_next;
	}

	if (strcmp(ldev_conf->name, "global") == 0) {
#ifdef CONFIG_AML_LOCAL_DIMMING_GLOBAL
		ret = ldim_dev_global_remove();
#endif
		goto ldim_dev_remove_driver_next;
	}

	LDIMERR("invalid device name: %s\n", ldev_conf->name);
	ret = -1;

ldim_dev_remove_driver_next:
	if (ret) {
		LDIMERR("add device driver failed %s(%d)\n",
			ldev_conf->name, index);
	} else {
		LDIMPR("add device driver %s(%d)\n", ldev_conf->name, index);
	}

	return ret;
}

int aml_ldim_device_probe(char *dt_addr)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();
	int ret = 0;

	/* get configs */
	ldim_dev_config = &ldim_config_dft;
	ldim_drv->ldev_conf = ldim_dev_config;
	ldim_drv->pinmux_ctrl = ldim_pwm_pinmux_ctrl;
	ldim_drv->device_config_print = aml_ldim_device_config_print;

#ifdef CONFIG_OF_LIBFDT
	ret = ldim_dev_get_config_from_dts(dt_addr, ldim_drv->dev_index);
	if (ret)
		return -1;
#endif

	/* add device driver */
	ret = ldim_dev_add_driver(ldim_drv->ldev_conf, ldim_drv->dev_index);

	LDIMPR("%s is ok\n", __func__);

	return ret;
}

int aml_ldim_device_remove(void)
{
	struct aml_ldim_driver_s *ldim_drv = aml_ldim_get_driver();
	int ret = 0;

	ldim_dev_remove_driver(ldim_drv->ldev_conf, ldim_drv->dev_index);

	return ret;
}

