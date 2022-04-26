/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _AML_LCD_TCON_H
#define _AML_LCD_TCON_H
#include <amlogic/media/vout/lcd/aml_lcd.h>

#define REG_LCD_TCON_MAX    0xffff

struct lcd_tcon_config_s {
	unsigned char tcon_valid;

	unsigned int core_reg_ver;
	unsigned int core_reg_width;
	unsigned int reg_table_width;
	unsigned int reg_table_len;
	unsigned int core_reg_start;

	unsigned int reg_top_ctrl;
	unsigned int bit_en;

	unsigned int reg_core_od;
	unsigned int bit_od_en;

	unsigned int reg_ctrl_timing_base;
	unsigned int ctrl_timing_offset;
	unsigned int ctrl_timing_cnt;

	unsigned int axi_bank;

	unsigned int rsv_mem_size;
	unsigned int axi_size;
	unsigned int bin_path_size;
	unsigned int vac_size;
	unsigned int demura_set_size;
	unsigned int demura_lut_size;
	unsigned int acc_lut_size;

	unsigned int *axi_reg;
	void (*tcon_axi_mem_config)(void);
	int (*tcon_enable)(struct aml_lcd_drv_s *pdrv);
	int (*tcon_disable)(struct aml_lcd_drv_s *pdrv);
};

struct tcon_rmem_config_s {
	unsigned int mem_paddr;
	unsigned char *mem_vaddr;
	unsigned int mem_size;
};

struct tcon_rmem_s {
	unsigned int flag;

	unsigned int rsv_mem_paddr;
	unsigned int axi_mem_paddr;
	unsigned int rsv_mem_size;
	unsigned int axi_mem_size;

	struct tcon_rmem_config_s *axi_rmem;
	struct tcon_rmem_config_s bin_path_rmem;

	struct tcon_rmem_config_s vac_rmem;
	struct tcon_rmem_config_s demura_set_rmem;
	struct tcon_rmem_config_s demura_lut_rmem;
	struct tcon_rmem_config_s acc_lut_rmem;
};

struct tcon_data_priority_s {
	unsigned int index;
	unsigned int priority;
};

struct tcon_mem_map_table_s {
	unsigned int version;
	unsigned int data_load_level;
	unsigned int block_cnt;
	unsigned int valid_flag;

	unsigned int core_reg_table_size;
	unsigned char *core_reg_table;

	struct tcon_data_priority_s *data_priority;
	unsigned int *data_size;
	unsigned char **data_mem_vaddr;
};

#define TCON_BIN_VER_LEN    9
struct lcd_tcon_local_cfg_s {
	char bin_ver[TCON_BIN_VER_LEN];
};

/* **********************************
 * tcon config
 * ********************************** */

/* TL1 */
#define LCD_TCON_CORE_REG_WIDTH_TL1      8
#define LCD_TCON_TABLE_WIDTH_TL1         8
#define LCD_TCON_TABLE_LEN_TL1           24000
#define LCD_TCON_AXI_BANK_TL1            3

#define BIT_TOP_EN_TL1                   4

#define TCON_CORE_REG_START_TL1          0x0000
#define REG_CORE_OD_TL1                  0x247
#define BIT_OD_EN_TL1                    0
#define REG_CTRL_TIMING_BASE_TL1         0x1b
#define CTRL_TIMING_OFFSET_TL1           12
#define CTRL_TIMING_CNT_TL1              0

/* T5 */
#define LCD_TCON_CORE_REG_WIDTH_T5       32
#define LCD_TCON_TABLE_WIDTH_T5          32
#define LCD_TCON_TABLE_LEN_T5            0x18d4 /* 0x635*4 */
#define LCD_TCON_AXI_BANK_T5             2

#define BIT_TOP_EN_T5                    4

#define TCON_CORE_REG_START_T5           0x0100
#define REG_CORE_OD_T5                   0x263
#define BIT_OD_EN_T5                     31
#define REG_CTRL_TIMING_BASE_T5          0x300
#define CTRL_TIMING_OFFSET_T5            12
#define CTRL_TIMING_CNT_T5               0

/* T5D */
#define LCD_TCON_CORE_REG_WIDTH_T5D       32
#define LCD_TCON_TABLE_WIDTH_T5D          32
#define LCD_TCON_TABLE_LEN_T5D            0x102c /* 0x40b*4 */
#define LCD_TCON_AXI_BANK_T5D             1

#define BIT_TOP_EN_T5D                    4

#define TCON_CORE_REG_START_T5D           0x0100
#define REG_CORE_OD_T5D                   0x263
#define BIT_OD_EN_T5D                     31
#define REG_CTRL_TIMING_BASE_T5D          0x1b
#define CTRL_TIMING_OFFSET_T5D            12
#define CTRL_TIMING_CNT_T5D               0

#ifdef CONFIG_CMD_INI
void *handle_lcd_ext_buf_get(void);
void *handle_tcon_path_mem_get(unsigned int size);
int handle_tcon_vac(unsigned char *vac_data, unsigned int vac_mem_size);
int handle_tcon_demura_set(unsigned char *demura_set_data,
			   unsigned int demura_set_size);
int handle_tcon_demura_lut(unsigned char *demura_lut_data,
			   unsigned int demura_lut_size);
int handle_tcon_acc_lut(unsigned char *acc_lut_data,
			unsigned int acc_lut_size);
int handle_tcon_data_load(unsigned char **buf, unsigned int index);
#endif

#define TCON_VAC_SET_PARAM_NUM    3
#define TCON_VAC_LUT_PARAM_NUM    256

int lcd_tcon_spi_data_probe(struct aml_lcd_drv_s *pdrv);

int lcd_tcon_valid_check(void);
struct lcd_tcon_config_s *get_lcd_tcon_config(void);
struct tcon_rmem_s *get_lcd_tcon_rmem(void);
struct tcon_mem_map_table_s *get_lcd_tcon_mm_table(void);

int lcd_tcon_enable_tl1(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_disable_tl1(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_enable_t5(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_disable_t5(struct aml_lcd_drv_s *pdrv);
int lcd_tcon_disable_t3(struct aml_lcd_drv_s *pdrv);

#endif

