// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <asm/arch/io.h>
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include "lcd_reg.h"
#include "lcd_common.h"
#include "lcd_tcon.h"

#define PR_BUF_MAX    200

#define TCON_IRQ_TIMEOUT_MAX    BIT(17)
static struct lcd_tcon_config_s *lcd_tcon_conf;
static struct tcon_rmem_s tcon_rmem;
static struct tcon_mem_map_table_s tcon_mm_table;
static struct lcd_tcon_local_cfg_s tcon_local_cfg;

int lcd_tcon_valid_check(void)
{
	if (!lcd_tcon_conf) {
		LCDERR("invalid tcon data\n");
		return -1;
	}
	if (lcd_tcon_conf->tcon_valid == 0) {
		LCDERR("invalid tcon\n");
		return -1;
	}

	return 0;
}

struct lcd_tcon_config_s *get_lcd_tcon_config(void)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return NULL;

	return lcd_tcon_conf;
}

struct tcon_rmem_s *get_lcd_tcon_rmem(void)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return NULL;

	return &tcon_rmem;
}

struct tcon_mem_map_table_s *get_lcd_tcon_mm_table(void)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return NULL;

	return &tcon_mm_table;
}

unsigned int lcd_tcon_data_size_align(unsigned int size)
{
	unsigned int new_size;

	/* ready for burst 128bit */
	new_size = ((size + 15) / 16) * 16;

	return new_size;
}

unsigned char lcd_tcon_checksum(unsigned char *buf, unsigned int len)
{
	unsigned int temp = 0;
	unsigned int i;

	if (!buf)
		return 0;
	if (len == 0)
		return 0;
	for (i = 0; i < len; i++)
		temp += buf[i];

	return (unsigned char)(temp & 0xff);
}

unsigned char lcd_tcon_lrc(unsigned char *buf, unsigned int len)
{
	unsigned char temp = 0;
	unsigned int i;

	if (!buf)
		return 0xff;
	if (len == 0)
		return 0xff;
	temp = buf[0];
	for (i = 1; i < len; i++)
		temp = temp ^ buf[i];

	return temp;
}

/* **********************************
 * tcon function api
 * **********************************
 */
static unsigned int lcd_tcon_reg_read(unsigned int addr, unsigned int flag)
{
	unsigned int val;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return 0;

	if (flag)
		val = lcd_tcon_read_byte(addr);
	else
		val = lcd_tcon_read(addr);

	return val;
}

static void lcd_tcon_reg_write(unsigned int addr, unsigned int val,
			       unsigned int flag)
{
	unsigned char temp;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	if (flag) {
		temp = (unsigned char)val;
		lcd_tcon_write_byte(addr, temp);
	} else {
		lcd_tcon_write(addr, val);
	}
}

static void lcd_tcon_reg_table_print(void)
{
	int i, j, cnt;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	if (!tcon_mm_table.core_reg_table) {
		LCDERR("%s: reg_table is null\n", __func__);
		return;
	}

	LCDPR("%s:\n", __func__);
	cnt = tcon_mm_table.core_reg_table_size;
	for (i = 0; i < cnt; i += 16) {
		printf("%04x: ", i);
		for (j = 0; j < 16; j++) {
			if ((i + j) >= cnt)
				break;
			printf(" %02x", tcon_mm_table.core_reg_table[i + j]);
		}
		printf("\n");
	}
}

static void lcd_tcon_reg_readback_print(void)
{
	unsigned int i, j, cnt, offset;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	LCDPR("%s:\n", __func__);
	cnt = tcon_mm_table.core_reg_table_size;
	offset = lcd_tcon_conf->core_reg_start;
	if (lcd_tcon_conf->core_reg_width == 8) {
		for (i = offset; i < cnt; i += 16) {
			printf("%04x: ", i);
			for (j = 0; j < 16; j++) {
				if ((i + j) >= cnt)
					break;
				printf(" %02x", lcd_tcon_read_byte(i + j));
			}
			printf("\n");
		}
	} else {
		if (lcd_tcon_conf->reg_table_width == 32) {
			cnt /= 4;
			for (i = offset; i < cnt; i += 4) {
				printf("%04x: ", i);
				for (j = 0; j < 4; j++) {
					if ((i + j) >= cnt)
						break;
					printf(" %08x", lcd_tcon_read(i + j));
				}
				printf("\n");
			}
		} else {
			for (i = offset; i < cnt; i += 16) {
				printf("%04x: ", i);
				for (j = 0; j < 16; j++) {
					if ((i + j) >= cnt)
						break;
					printf(" %02x", lcd_tcon_read(i + j));
				}
				printf("\n");
			}
		}
	}
}

static unsigned int lcd_tcon_table_read(unsigned int addr)
{
	unsigned char *table8;
	unsigned int *table32, size = 0, val = 0;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return 0;

	if (!tcon_mm_table.core_reg_table) {
		LCDERR("tcon reg_table is null\n");
		return 0;
	}

	if (lcd_tcon_conf->core_reg_width == 8)
		size = tcon_mm_table.core_reg_table_size;
	else
		size = tcon_mm_table.core_reg_table_size / 4;
	if (addr >= size) {
		LCDERR("invalid tcon reg_table addr: 0x%04x\n", addr);
		return 0;
	}

	if (lcd_tcon_conf->core_reg_width == 8) {
		table8 = tcon_mm_table.core_reg_table;
		val = table8[addr];
	} else {
		table32 = (unsigned int *)tcon_mm_table.core_reg_table;
		val = table32[addr];
	}

	return val;
}

static unsigned int lcd_tcon_table_write(unsigned int addr, unsigned int val)
{
	unsigned char *table8;
	unsigned int *table32, size = 0, read_val = 0;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return 0;

	if (!tcon_mm_table.core_reg_table) {
		LCDERR("tcon reg_table is null\n");
		return 0;
	}

	if (lcd_tcon_conf->core_reg_width == 8)
		size = tcon_mm_table.core_reg_table_size;
	else
		size = tcon_mm_table.core_reg_table_size / 4;
	if (addr >= size) {
		LCDERR("invalid tcon reg_table addr: 0x%04x\n", addr);
		return 0;
	}

	if (lcd_tcon_conf->core_reg_width == 8) {
		table8 = tcon_mm_table.core_reg_table;
		table8[addr] = (unsigned char)(val & 0xff);
		read_val = table8[addr];
	} else {
		table32 = (unsigned int *)tcon_mm_table.core_reg_table;
		table32[addr] = val;
		read_val = table32[addr];
	}

	return read_val;
}

static void lcd_tcon_vac_print(void)
{
	int i, j, n, size;
	char *buf;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	if (tcon_mm_table.version) {
		LCDERR("%s: mem map version invalid\n", __func__);
		return;
	}

	if ((tcon_mm_table.valid_flag & LCD_TCON_DATA_VALID_VAC) == 0) {
		LCDERR("%s: vac invalid\n", __func__);
		return;
	}

	buf = (char *)malloc(PR_BUF_MAX * sizeof(char));
	if (buf == NULL) {
		LCDERR("%s: buf malloc error\n", __func__);
		return;
	}

	printf("%s:\n", __func__);
	size = tcon_rmem.vac_rmem.mem_vaddr[0] |
		(tcon_rmem.vac_rmem.mem_vaddr[1] << 8) |
		(tcon_rmem.vac_rmem.mem_vaddr[2] << 16) |
		(tcon_rmem.vac_rmem.mem_vaddr[3] << 24);
	size += 8; /* header for data_cnt & crc */
	for (i = 0; i < size; i += 16) {
		n = snprintf(buf, PR_BUF_MAX, "0x%04x: ", i);
		for (j = 0; j < 16; j++) {
			if ((i + j) >= size)
				break;
			n += snprintf(buf + n, PR_BUF_MAX, " %02x",
				      tcon_rmem.vac_rmem.mem_vaddr[i + j]);
		}
		buf[n] = '\0';
		printf("%s\n", buf);
	}

	free(buf);
}

static void lcd_tcon_demura_print(void)
{
	int i, j, n, size;
	char *buf;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	if (tcon_mm_table.version) {
		LCDERR("%s: mem map version invalid\n", __func__);
		return;
	}

	if ((tcon_mm_table.valid_flag & LCD_TCON_DATA_VALID_DEMURA) == 0) {
		LCDERR("%s: demura invalid\n", __func__);
		return;
	}

	buf = (char *)malloc(PR_BUF_MAX * sizeof(char));
	if (buf == NULL) {
		LCDERR("%s: buf malloc error\n", __func__);
		return;
	}

	printf("%s: demura_set:\n", __func__);
	size = tcon_rmem.demura_set_rmem.mem_vaddr[0] |
		(tcon_rmem.demura_set_rmem.mem_vaddr[1] << 8) |
		(tcon_rmem.demura_set_rmem.mem_vaddr[2] << 16) |
		(tcon_rmem.demura_set_rmem.mem_vaddr[3] << 24);
	size += 8; /* header for data_cnt & crc */
	for (i = 0; i < size; i += 16) {
		n = snprintf(buf, PR_BUF_MAX, "0x%04x: ", i);
		for (j = 0; j < 16; j++) {
			if ((i + j) >= size)
				break;
			n += snprintf(buf + n, PR_BUF_MAX, " %02x",
			 tcon_rmem.demura_set_rmem.mem_vaddr[i + j]);
		}
		buf[n] = '\0';
		printf("%s\n", buf);
	}

	printf("\n%s: demura_lut:\n", __func__);
	size = tcon_rmem.demura_lut_rmem.mem_vaddr[0] |
		(tcon_rmem.demura_lut_rmem.mem_vaddr[1] << 8) |
		(tcon_rmem.demura_lut_rmem.mem_vaddr[2] << 16) |
		(tcon_rmem.demura_lut_rmem.mem_vaddr[3] << 24);
	size += 8; /* header for data_cnt & crc */
	for (i = 0; i < size; i += 16) {
		n = snprintf(buf, PR_BUF_MAX, "0x%04x: ", i);
		for (j = 0; j < 16; j++) {
			if ((i + j) >= size)
				break;
			n += snprintf(buf + n, PR_BUF_MAX, " %02x",
			 tcon_rmem.demura_lut_rmem.mem_vaddr[i + j]);
		}
		buf[n] = '\0';
		printf("%s\n", buf);
	}

	free(buf);
}

static void lcd_tcon_acc_print(void)
{
	int i, j, n, size;
	char *buf;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	if (tcon_mm_table.version) {
		LCDERR("%s: mem map version invalid\n", __func__);
		return;
	}

	if ((tcon_mm_table.valid_flag & LCD_TCON_DATA_VALID_ACC) == 0) {
		LCDERR("%s: acc_lut invalid\n", __func__);
		return;
	}

	buf = (char *)malloc(PR_BUF_MAX * sizeof(char));
	if (!buf) {
		LCDERR("%s: buf malloc error\n", __func__);
		return;
	}

	LCDPR("%s\n", __func__);
	size = tcon_rmem.acc_lut_rmem.mem_vaddr[0] |
		(tcon_rmem.acc_lut_rmem.mem_vaddr[1] << 8) |
		(tcon_rmem.acc_lut_rmem.mem_vaddr[2] << 16) |
		(tcon_rmem.acc_lut_rmem.mem_vaddr[3] << 24);
	size += 8; /* header for data_cnt & crc */
	for (i = 0; i < size; i += 16) {
		n = snprintf(buf, PR_BUF_MAX, "0x%04x: ", i);
		for (j = 0; j < 16; j++) {
			if ((i + j) >= size)
				break;
			n += snprintf(buf + n, PR_BUF_MAX, " %02x",
			 tcon_rmem.acc_lut_rmem.mem_vaddr[i + j]);
		}
		buf[n] = '\0';
		printf("%s\n", buf);
	}

	free(buf);
}

static void lcd_tcon_data_block_print(char *buf, unsigned char *data_mem)
{
	int i, j, n, size;

	size = data_mem[8] |
		(data_mem[9] << 8) |
		(data_mem[10] << 16) |
		(data_mem[11] << 24);
	for (i = 0; i < size; i += 16) {
		n = snprintf(buf, PR_BUF_MAX, "0x%04x: ", i);
		for (j = 0; j < 16; j++) {
			if ((i + j) >= size)
				break;
			n += snprintf(buf + n, PR_BUF_MAX, " %02x",
				      data_mem[i + j]);
		}
		buf[n] = '\0';
		printf("%s\n", buf);
	}
}

static void lcd_tcon_data_print(unsigned char index)
{
	int i;
	char *buf;
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	if (tcon_mm_table.version == 0) {
		LCDERR("%s: mem map version invalid\n", __func__);
		return;
	}

	buf = (char *)malloc(PR_BUF_MAX * sizeof(char));
	if (!buf) {
		LCDERR("%s: buf malloc error\n", __func__);
		return;
	}

	if (index == 0xff) {
		for (i = 0; i < tcon_mm_table.block_cnt; i++) {
			if (!tcon_mm_table.data_mem_vaddr[i])
				continue;

			printf("tcon data[%d] print:\n", i);
			lcd_tcon_data_block_print(buf, tcon_mm_table.data_mem_vaddr[i]);
		}
	} else {
		if (index >= tcon_mm_table.block_cnt) {
			LCDERR("%s: invalid index %d\n", __func__, index);
			free(buf);
			return;
		}
		if (!tcon_mm_table.data_mem_vaddr[index]) {
			LCDERR("%s: invalid data_mem buf\n", __func__);
			free(buf);
			return;
		}

		printf("tcon data[%d] print:\n", index);
		lcd_tcon_data_block_print(buf, tcon_mm_table.data_mem_vaddr[index]);
	}

	free(buf);
}

void lcd_tcon_info_print(void)
{
	unsigned int size, cnt, file_size, n;
	char *str;
	int i, ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	LCDPR("%s:\n", __func__);
	printf("core_reg_width:       %d\n"
		"reg_table_len:        %d\n"
		"tcon_bin_ver:         %s\n"
		"tcon_rmem_flag:       %d\n"
		"rsv_mem addr:         0x%08x\n"
		"rsv_mem size:         0x%08x\n\n",
		lcd_tcon_conf->core_reg_width,
		lcd_tcon_conf->reg_table_len,
		tcon_local_cfg.bin_ver,
		tcon_rmem.flag,
		tcon_rmem.rsv_mem_paddr,
		tcon_rmem.rsv_mem_size);
	if (tcon_rmem.flag) {
		for (i = 0; i < lcd_tcon_conf->axi_bank; i++) {
			printf("axi_mem[%d]_paddr:     0x%lx\n"
				"axi_mem[%d]_vaddr:     0x%p\n"
				"axi_mem[%d]_size:      0x%x\n",
				i,
				(unsigned long)tcon_rmem.axi_rmem[i].mem_paddr,
				i, tcon_rmem.axi_rmem[i].mem_vaddr,
				i, tcon_rmem.axi_rmem[i].mem_size);
		}
	}
	if (tcon_rmem.bin_path_rmem.mem_size) {
		printf("bin_path_mem paddr:   0x%08x\n"
			"bin_path_mem vaddr:   0x%p\n"
			"bin_path_mem size:    0x%08x\n\n",
			tcon_rmem.bin_path_rmem.mem_paddr,
			tcon_rmem.bin_path_rmem.mem_vaddr,
			tcon_rmem.bin_path_rmem.mem_size);
	}
	if (tcon_mm_table.version == 0) {
		if (tcon_rmem.vac_rmem.mem_size) {
			printf("vac_mem vaddr:        0x%p\n"
				"vac_mem size:         0x%08x\n",
				tcon_rmem.vac_rmem.mem_vaddr,
				tcon_rmem.vac_rmem.mem_size);
		}
		if (tcon_rmem.demura_set_rmem.mem_size) {
			printf("demura_set_mem vaddr: 0x%p\n"
				"demura_set_mem size:  0x%08x\n",
				tcon_rmem.demura_set_rmem.mem_vaddr,
				tcon_rmem.demura_set_rmem.mem_size);
		}
		if (tcon_rmem.demura_lut_rmem.mem_size) {
			printf("demura_lut_mem vaddr: 0x%p\n"
				"demura_lut_mem size:  0x%08x\n",
				tcon_rmem.demura_lut_rmem.mem_vaddr,
				tcon_rmem.demura_lut_rmem.mem_size);
		}
		if (tcon_rmem.acc_lut_rmem.mem_size) {
			printf("acc_lut_mem vaddr:    0x%p\n"
				"acc_lut_mem size:     0x%08x\n",
				tcon_rmem.acc_lut_rmem.mem_vaddr,
				tcon_rmem.acc_lut_rmem.mem_size);
		}
	} else {
		printf("data_mem_block_cnt:   %d\n",
		       tcon_mm_table.block_cnt);
		for (i = 0; i < tcon_mm_table.block_cnt; i++) {
			if (tcon_mm_table.data_mem_vaddr[i]) {
				size = tcon_mm_table.data_mem_vaddr[i][8] |
				(tcon_mm_table.data_mem_vaddr[i][9] << 8) |
				(tcon_mm_table.data_mem_vaddr[i][10] << 16) |
				(tcon_mm_table.data_mem_vaddr[i][11] << 24);
				printf("data_mem[%d] vaddr:    0x%p\n"
					"data_mem[%d] size:     0x%08x\n",
					i, tcon_mm_table.data_mem_vaddr[i],
					i, size);
			}
		}
	}
	if (tcon_rmem.bin_path_rmem.mem_vaddr) {
		size = tcon_rmem.bin_path_rmem.mem_vaddr[4] |
			(tcon_rmem.bin_path_rmem.mem_vaddr[5] << 8) |
			(tcon_rmem.bin_path_rmem.mem_vaddr[6] << 16) |
			(tcon_rmem.bin_path_rmem.mem_vaddr[7] << 24);
		cnt = tcon_rmem.bin_path_rmem.mem_vaddr[16] |
			(tcon_rmem.bin_path_rmem.mem_vaddr[17] << 8) |
			(tcon_rmem.bin_path_rmem.mem_vaddr[18] << 16) |
			(tcon_rmem.bin_path_rmem.mem_vaddr[19] << 24);
		if (size < (32 + 256 * cnt))
			return;
		if (cnt > 32)
			return;
		printf("\n");
		for (i = 0; i < cnt; i++) {
			n = 32 + 256 * i;
			file_size = tcon_rmem.bin_path_rmem.mem_vaddr[n] |
			(tcon_rmem.bin_path_rmem.mem_vaddr[n + 1] << 8) |
			(tcon_rmem.bin_path_rmem.mem_vaddr[n + 2] << 16) |
			(tcon_rmem.bin_path_rmem.mem_vaddr[n + 3] << 24);
			str = (char *)&tcon_rmem.bin_path_rmem.mem_vaddr[n + 4];
			printf("tcon_path[%d]: size: 0x%x, %s\n", i,
			       file_size, str);
		}
	}
	printf("\n");
}

#ifdef CONFIG_CMD_INI
static int lcd_tcon_bin_path_resv_mem_set(void)
{
	unsigned char *buf, *mem_vaddr;
	unsigned int data_size, block_size, temp_crc, n, i;

	if (tcon_rmem.flag == 0)
		return 0;

	buf = handle_tcon_path_mem_get(tcon_rmem.bin_path_rmem.mem_size);
	if (!buf) {
		LCDERR("%s: bin_path buf invalid\n", __func__);
		return -1;
	}

	data_size = buf[4] | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);

	if (tcon_mm_table.data_size) {
		for (i = 0; i < tcon_mm_table.block_cnt; i++) {
			block_size = tcon_mm_table.data_size[i];
			if (block_size == 0)
				continue;
			n = 32 + (i * 256);
			buf[n] = block_size & 0xff;
			buf[n + 1] = (block_size >> 8) & 0xff;
			buf[n + 2] = (block_size >> 16) & 0xff;
			buf[n + 3] = (block_size >> 24) & 0xff;
		}

		/* update data check */
		temp_crc = crc32(0, &buf[4], (data_size - 4));
		buf[0] = temp_crc & 0xff;
		buf[1] = (temp_crc >> 8) & 0xff;
		buf[2] = (temp_crc >> 16) & 0xff;
		buf[3] = (temp_crc >> 24) & 0xff;
	}

	mem_vaddr = (unsigned char *)(unsigned long)(tcon_rmem.bin_path_rmem.mem_paddr);
	memcpy(mem_vaddr, buf, data_size);

	return 0;
}
#endif

int lcd_tcon_enable(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return -1;

	if (lcd_tcon_conf->tcon_enable)
		lcd_tcon_conf->tcon_enable(pdrv);

#ifdef CONFIG_CMD_INI
	lcd_tcon_bin_path_resv_mem_set();
#endif

	return 0;
}

void lcd_tcon_disable(struct aml_lcd_drv_s *pdrv)
{
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return;

	LCDPR("%s\n", __func__);

	if (lcd_tcon_conf->tcon_disable)
		lcd_tcon_conf->tcon_disable(pdrv);
}

/* **********************************
 * tcon config
 * **********************************
 */
static int lcd_tcon_vac_load(void)
{
	unsigned char *buff = tcon_rmem.vac_rmem.mem_vaddr;
#ifdef CONFIG_CMD_INI
	unsigned int i, data_cnt = 0;
	unsigned char data_checksum, data_lrc, temp_checksum, temp_lrc;
#endif
	int ret = -1;

	if (tcon_rmem.vac_rmem.mem_size == 0 || !buff)
		return -1;

#ifdef CONFIG_CMD_INI
	ret = handle_tcon_vac(buff, tcon_rmem.vac_rmem.mem_size);
	if (ret) {
		LCDPR("%s: no vac data\n", __func__);
		return -1;
	}
	data_cnt = (buff[0] |
		(buff[1] << 8) |
		(buff[2] << 16) |
		(buff[3] << 24));
	if (data_cnt == 0) {
		LCDERR("%s: vac_data data_cnt error\n", __func__);
		return -1;
	}
	data_checksum = buff[4];
	data_lrc = buff[5];
	temp_checksum = lcd_tcon_checksum(&buff[8], data_cnt);
	temp_lrc = lcd_tcon_lrc(&buff[8], data_cnt);
	if (data_checksum != temp_checksum) {
		LCDERR("%s: vac_data checksum error\n", __func__);
		return -1;
	}
	if (data_lrc != temp_lrc) {
		LCDERR("%s: vac_data lrc error\n", __func__);
		return -1;
	}
	if (buff[6] != 0x55 || buff[7] != 0xaa) {
		LCDERR("%s: vac_data pattern error\n", __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		for (i = 0; i < 30; i++)
			LCDPR("vac_data[%d]: 0x%02x\n", i, buff[i * 1]);
	}
#endif
	return ret;
}

static int lcd_tcon_demura_set_load(void)
{
	unsigned char *buff = tcon_rmem.demura_set_rmem.mem_vaddr;
#ifdef CONFIG_CMD_INI
	unsigned int i, data_cnt = 0;
	unsigned char data_checksum, data_lrc, temp_checksum, temp_lrc;
#endif
	int ret = -1;

	if (tcon_rmem.demura_set_rmem.mem_size == 0 || !buff)
		return -1;

#ifdef CONFIG_CMD_INI
	ret = handle_tcon_demura_set(buff, tcon_rmem.demura_set_rmem.mem_size);
	if (ret) {
		LCDPR("%s: no demura_set data\n", __func__);
		return -1;
	}

	data_cnt = (buff[0] |
		(buff[1] << 8) |
		(buff[2] << 16) |
		(buff[3] << 24));
	if (data_cnt == 0) {
		LCDERR("%s: demura_set data_cnt error\n", __func__);
		return -1;
	}
	data_checksum = buff[4];
	data_lrc = buff[5];
	temp_checksum = lcd_tcon_checksum(&buff[8], data_cnt);
	temp_lrc = lcd_tcon_lrc(&buff[8], data_cnt);
	if (data_checksum != temp_checksum) {
		LCDERR("%s: demura_set checksum error\n", __func__);
		return -1;
	}
	if (data_lrc != temp_lrc) {
		LCDERR("%s: demura_set lrc error\n", __func__);
		return -1;
	}
	if (buff[6] != 0x55 || buff[7] != 0xaa) {
		LCDERR("%s: demura_set pattern error\n", __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		for (i = 0; i < 100; i++)
			LCDPR("demura_set[%d]: 0x%x\n", i, buff[i]);
	}
#endif
	return ret;
}

static int lcd_tcon_demura_lut_load(void)
{
	unsigned char *buff = tcon_rmem.demura_lut_rmem.mem_vaddr;
#ifdef CONFIG_CMD_INI
	unsigned int i, data_cnt = 0;
	unsigned char data_checksum, data_lrc, temp_checksum, temp_lrc;
#endif
	int ret = -1;

	if (tcon_rmem.demura_lut_rmem.mem_size == 0 || !buff)
		return -1;

#ifdef CONFIG_CMD_INI
	ret = handle_tcon_demura_lut(buff, tcon_rmem.demura_lut_rmem.mem_size);
	if (ret) {
		LCDPR("%s: no demura_lut data\n", __func__);
		return -1;
	}
	data_cnt = (buff[0] |
		(buff[1] << 8) |
		(buff[2] << 16) |
		(buff[3] << 24));
	if (data_cnt == 0) {
		LCDERR("%s: demura_lut data_cnt error\n", __func__);
		return -1;
	}
	data_checksum = buff[4];
	data_lrc = buff[5];
	temp_checksum = lcd_tcon_checksum(&buff[8], data_cnt);
	temp_lrc = lcd_tcon_lrc(&buff[8], data_cnt);
	if (data_checksum != temp_checksum) {
		LCDERR("%s: demura_lut checksum error\n", __func__);
		return -1;
	}
	if (data_lrc != temp_lrc) {
		LCDERR("%s: demura_lut lrc error\n", __func__);
		return -1;
	}
	if ((buff[6] != 0x55) || (buff[7] != 0xaa)) {
		LCDERR("%s: demura_lut pattern error\n", __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		for (i = 0; i < 100; i++)
			LCDPR("demura_lut[%d]: 0x%02x\n", i, buff[i]);
	}
#endif
	return ret;
}

static int lcd_tcon_acc_lut_load(void)
{
	unsigned char *buff = tcon_rmem.acc_lut_rmem.mem_vaddr;
#ifdef CONFIG_CMD_INI
	unsigned int i, data_cnt = 0;
	unsigned char data_checksum, data_lrc, temp_checksum, temp_lrc;
#endif
	int ret = -1;

	if (tcon_rmem.acc_lut_rmem.mem_size == 0 || !buff)
		return -1;

#ifdef CONFIG_CMD_INI
	ret = handle_tcon_acc_lut(buff, tcon_rmem.acc_lut_rmem.mem_size);
	if (ret) {
		LCDPR("%s: no acc_lut data\n", __func__);
		return -1;
	}
	data_cnt = (buff[0] |
		(buff[1] << 8) |
		(buff[2] << 16) |
		(buff[3] << 24));
	if (data_cnt == 0) {
		LCDERR("%s: acc_lut data_cnt error\n", __func__);
		return -1;
	}
	data_checksum = buff[4];
	data_lrc = buff[5];
	temp_checksum = lcd_tcon_checksum(&buff[8], data_cnt);
	temp_lrc = lcd_tcon_lrc(&buff[8], data_cnt);
	if (data_checksum != temp_checksum) {
		LCDERR("%s: acc_lut checksum error\n", __func__);
		return -1;
	}
	if (data_lrc != temp_lrc) {
		LCDERR("%s: acc_lut lrc error\n", __func__);
		return -1;
	}
	if (buff[6] != 0x55 || buff[7] != 0xaa) {
		LCDERR("%s: acc_lut pattern error\n", __func__);
		return -1;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_ADV) {
		for (i = 0; i < 100; i++)
			LCDPR("acc_lut[%d]: 0x%02x\n", i, buff[i]);
	}
#endif
	return ret;
}

static int lcd_tcon_data_load(struct aml_lcd_drv_s *pdrv)
{
	unsigned char *table;
#ifdef CONFIG_CMD_INI
	struct lcd_tcon_data_block_header_s block_header;
	struct tcon_data_priority_s *data_prio;
	unsigned int i, j, priority, demura_cnt = 0;
#endif
	int ret;

	ret = lcd_tcon_valid_check();
	if (ret)
		return -1;
	table = tcon_mm_table.core_reg_table;
	if (!table)
		return 0;

	if (tcon_mm_table.version == 0) {
		if (pdrv->data->chip_type == LCD_CHIP_TL1 ||
		    pdrv->data->chip_type == LCD_CHIP_TM2) {
			ret = lcd_tcon_vac_load();
			if (ret == 0)
				tcon_mm_table.valid_flag |= LCD_TCON_DATA_VALID_VAC;
			ret = lcd_tcon_demura_set_load();
			if (ret)  {
				table[0x178] = 0x38;
				table[0x17c] = 0x20;
				table[0x181] = 0x00;
				table[0x23d] &= ~(1 << 0);
			} else {
				ret = lcd_tcon_demura_lut_load();
				if (ret) {
					table[0x178] = 0x38;
					table[0x17c] = 0x20;
					table[0x181] = 0x00;
					table[0x23d] &= ~(1 << 0);
				} else {
					tcon_mm_table.valid_flag |= LCD_TCON_DATA_VALID_DEMURA;
				}
			}
		}

		ret = lcd_tcon_acc_lut_load();
		if (ret == 0)
			tcon_mm_table.valid_flag |= LCD_TCON_DATA_VALID_ACC;
	} else {
		if (!tcon_mm_table.data_mem_vaddr) {
			LCDERR("%s: data_mem error\n", __func__);
			return -1;
		}
		if (!tcon_mm_table.data_priority) {
			LCDERR("%s: data_priority error\n", __func__);
			return -1;
		}
		if (!tcon_mm_table.data_size) {
			LCDERR("%s: data_size error\n", __func__);
			return -1;
		}
#ifdef CONFIG_CMD_INI
		data_prio = tcon_mm_table.data_priority;
		for (i = 0; i < tcon_mm_table.block_cnt; i++) {
			ret = handle_tcon_data_load(tcon_mm_table.data_mem_vaddr, i);
			if (ret)
				continue;

			memcpy(&block_header, tcon_mm_table.data_mem_vaddr[i],
			       LCD_TCON_DATA_BLOCK_HEADER_SIZE);
			tcon_mm_table.valid_flag |= block_header.block_flag;
			if (block_header.block_flag == LCD_TCON_DATA_VALID_DEMURA)
				demura_cnt++;

			/* insertion sort for block data init_priority */
			data_prio[i].index = i;
			//data_prio[i].priority = block_header.init_priority;
			/* update init_priority by index */
			priority = i;
			data_prio[i].priority = priority;
			if (i > 0) {
				j = i - 1;
				while (j >= 0) {
					if (priority > data_prio[j].priority)
						break;
					if (priority == data_prio[j].priority) {
						LCDERR("%s: block %d init_priority same as block %d\n",
						__func__,
						data_prio[i].index,
						data_prio[j].index);
						return -1;
					}
					data_prio[j + 1].index = data_prio[j].index;
					data_prio[j + 1].priority = data_prio[j].priority;
					j--;
				}
				data_prio[j + 1].index = i;
				data_prio[j + 1].priority = priority;
			}
			tcon_mm_table.data_size[i] = block_header.block_size;

			if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
				LCDPR("%s %d: block size=0x%x, type=0x%02x, name=%s, init_priority=%d\n",
				      __func__, i,
				      block_header.block_size,
				      block_header.block_type,
				      block_header.name, priority);
			}
		}

		/* specially check demura setting */
		if (pdrv->data->chip_type == LCD_CHIP_TL1 ||
		    pdrv->data->chip_type == LCD_CHIP_TM2) {
			if (demura_cnt < 2) {
				tcon_mm_table.valid_flag &= ~LCD_TCON_DATA_VALID_DEMURA;
				/* disable demura */
				table[0x178] = 0x38;
				table[0x17c] = 0x20;
				table[0x181] = 0x00;
				table[0x23d] &= ~(1 << 0);
			}
		}
#endif
	}

	return 0;
}

static int lcd_tcon_bin_path_update(unsigned int size)
{
#ifdef CONFIG_CMD_INI
	unsigned char *mem_vaddr;
	unsigned int data_size, block_cnt;
	unsigned int data_crc32, temp_crc32;

	/* notice: different with kernel flow: mem_vaddr is not mapping to mem_paddr */
	tcon_rmem.bin_path_rmem.mem_vaddr = handle_tcon_path_mem_get(size);
	if (!tcon_rmem.bin_path_rmem.mem_vaddr) {
		LCDERR("%s: get mem error\n", __func__);
		return -1;
	}
	mem_vaddr = tcon_rmem.bin_path_rmem.mem_vaddr;
	data_size = mem_vaddr[4] |
		(mem_vaddr[5] << 8) |
		(mem_vaddr[6] << 16) |
		(mem_vaddr[7] << 24);
	if (data_size < 32) { /* header size */
		LCDERR("%s: tcon_bin_path data_size error\n", __func__);
		return -1;
	}
	block_cnt = mem_vaddr[16] |
		(mem_vaddr[17] << 8) |
		(mem_vaddr[18] << 16) |
		(mem_vaddr[19] << 24);
	if (block_cnt > 32) {
		LCDERR("%s: tcon_bin_path block_cnt error\n", __func__);
		return -1;
	}
	data_crc32 = mem_vaddr[0] |
		(mem_vaddr[1] << 8) |
		(mem_vaddr[2] << 16) |
		(mem_vaddr[3] << 24);
	temp_crc32 = crc32(0, &mem_vaddr[4], (data_size - 4));
	if (data_crc32 != temp_crc32) {
		LCDERR("%s: tcon_bin_path data crc error\n", __func__);
		return -1;
	}

	tcon_mm_table.version = mem_vaddr[8] |
		(mem_vaddr[9] << 8) |
		(mem_vaddr[10] << 16) |
		(mem_vaddr[11] << 24);
	tcon_mm_table.data_load_level = mem_vaddr[12] |
		(mem_vaddr[13] << 8) |
		(mem_vaddr[14] << 16) |
		(mem_vaddr[15] << 24);
	tcon_mm_table.block_cnt = block_cnt;
#endif

	return 0;
}

static int lcd_tcon_mm_table_config_v0(void)
{
	unsigned int max_size;

	/* reserved memory */
	max_size = lcd_tcon_conf->axi_size +
		lcd_tcon_conf->bin_path_size +
		lcd_tcon_conf->vac_size +
		lcd_tcon_conf->demura_set_size +
		lcd_tcon_conf->demura_lut_size +
		lcd_tcon_conf->acc_lut_size;
	if (tcon_rmem.rsv_mem_size < max_size) {
		LCDERR("%s: tcon mem size 0x%x is not enough, need 0x%x\n",
		       __func__, tcon_rmem.rsv_mem_size, max_size);
		return -1;
	}

	if (tcon_mm_table.block_cnt != 4) {
		LCDERR("%s: tcon data block_cnt %d invalid\n",
		       __func__, tcon_mm_table.block_cnt);
		return -1;
	}

	tcon_rmem.vac_rmem.mem_size = lcd_tcon_conf->vac_size;
	tcon_rmem.vac_rmem.mem_paddr =
		tcon_rmem.bin_path_rmem.mem_paddr +
		tcon_rmem.bin_path_rmem.mem_size;
	tcon_rmem.vac_rmem.mem_vaddr =
		(unsigned char *)(unsigned long)(tcon_rmem.vac_rmem.mem_paddr);
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.vac_rmem.mem_size > 0)
		LCDPR("tcon vac paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.vac_rmem.mem_paddr,
		      tcon_rmem.vac_rmem.mem_size);

	tcon_rmem.demura_set_rmem.mem_size = lcd_tcon_conf->demura_set_size;
	tcon_rmem.demura_set_rmem.mem_paddr =
		tcon_rmem.vac_rmem.mem_paddr + tcon_rmem.vac_rmem.mem_size;
	tcon_rmem.demura_set_rmem.mem_vaddr = (unsigned char *)
			(unsigned long)(tcon_rmem.demura_set_rmem.mem_paddr);
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.demura_set_rmem.mem_size > 0)
		LCDPR("tcon demura set_paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.demura_set_rmem.mem_paddr,
		      tcon_rmem.demura_set_rmem.mem_size);

	tcon_rmem.demura_lut_rmem.mem_size = lcd_tcon_conf->demura_lut_size;
	tcon_rmem.demura_lut_rmem.mem_paddr =
		tcon_rmem.demura_set_rmem.mem_paddr +
		tcon_rmem.demura_set_rmem.mem_size;
	tcon_rmem.demura_lut_rmem.mem_vaddr = (unsigned char *)
		(unsigned long)(tcon_rmem.demura_lut_rmem.mem_paddr);
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.demura_lut_rmem.mem_size > 0)
		LCDPR("tcon demura lut_paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.demura_lut_rmem.mem_paddr,
		      tcon_rmem.demura_lut_rmem.mem_size);

	tcon_rmem.acc_lut_rmem.mem_size = lcd_tcon_conf->acc_lut_size;
	tcon_rmem.acc_lut_rmem.mem_paddr =
		tcon_rmem.demura_lut_rmem.mem_paddr +
		tcon_rmem.demura_lut_rmem.mem_size;
	tcon_rmem.acc_lut_rmem.mem_vaddr = (unsigned char *)
		(unsigned long)(tcon_rmem.acc_lut_rmem.mem_paddr);
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.acc_lut_rmem.mem_size > 0)
		LCDPR("tcon acc lut_paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.acc_lut_rmem.mem_paddr,
		      tcon_rmem.acc_lut_rmem.mem_size);

	return 0;
}

static int lcd_tcon_mm_table_config_v1(void)
{
	if (tcon_mm_table.block_cnt > 32) {
		LCDERR("%s: tcon data block_cnt %d invalid\n",
		       __func__, tcon_mm_table.block_cnt);
		return -1;
	}

	if (tcon_mm_table.data_mem_vaddr)
		return 0;
	if (tcon_mm_table.block_cnt == 0) {
		if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
			LCDPR("%s: block_cnt is zero\n", __func__);
		return 0;
	}

	tcon_mm_table.data_mem_vaddr = (unsigned char **)malloc
		(tcon_mm_table.block_cnt * sizeof(unsigned char *));
	if (!tcon_mm_table.data_mem_vaddr) {
		LCDERR("%s: Not enough memory\n", __func__);
		return -1;
	}
	memset(tcon_mm_table.data_mem_vaddr, 0,
	       tcon_mm_table.block_cnt * sizeof(unsigned char *));

	tcon_mm_table.data_priority = (struct tcon_data_priority_s *)malloc
		(tcon_mm_table.block_cnt * sizeof(struct tcon_data_priority_s));
	if (!tcon_mm_table.data_priority) {
		LCDERR("%s: Not enough memory\n", __func__);
		return -1;
	}
	memset(tcon_mm_table.data_priority, 0xff,
	       tcon_mm_table.block_cnt * sizeof(struct tcon_data_priority_s));

	tcon_mm_table.data_size = (unsigned int *)malloc
		(tcon_mm_table.block_cnt * sizeof(unsigned int));
	if (!tcon_mm_table.data_size) {
		LCDERR("%s: Not enough memory\n", __func__);
		return -1;
	}
	memset(tcon_mm_table.data_size, 0,
		tcon_mm_table.block_cnt * sizeof(unsigned int));

	return 0;
}

static void lcd_tcon_axi_mem_config_tl1(void)
{
	unsigned int size[3] = {4162560, 4162560, 1960440};
	unsigned int total_size = 0, temp_size = 0;
	int i;

	for (i = 0; i < lcd_tcon_conf->axi_bank; i++)
		total_size += size[i];
	if (total_size > tcon_rmem.axi_mem_size) {
		LCDERR("%s: tcon axi_mem size 0x%x is not enough, need 0x%x\n",
		       __func__, tcon_rmem.axi_mem_size, total_size);
		return;
	}

	tcon_rmem.axi_rmem = (struct tcon_rmem_config_s *)
		malloc(lcd_tcon_conf->axi_bank * sizeof(struct tcon_rmem_config_s));
	if (!tcon_rmem.axi_rmem)
		return;
	memset(tcon_rmem.axi_rmem, 0,
	       lcd_tcon_conf->axi_bank * sizeof(struct tcon_rmem_config_s));

	for (i = 0; i < lcd_tcon_conf->axi_bank; i++) {
		tcon_rmem.axi_rmem[i].mem_paddr = tcon_rmem.axi_mem_paddr + temp_size;
		tcon_rmem.axi_rmem[i].mem_vaddr = (unsigned char *)
			(unsigned long)tcon_rmem.axi_rmem[i].mem_paddr;
		tcon_rmem.axi_rmem[i].mem_size = size[i];
		temp_size += size[i];
	}
}

static void lcd_tcon_axi_mem_config_t5(void)
{
	unsigned int size[2] = {0x00800000, 0x100000};
	unsigned int reg[2] = {0x261, 0x1a9};
	unsigned int total_size = 0, temp_size = 0;
	int i;

	for (i = 0; i < lcd_tcon_conf->axi_bank; i++)
		total_size += size[i];
	if (total_size > tcon_rmem.axi_mem_size) {
		LCDERR("%s: tcon axi_mem size 0x%x is not enough, need 0x%x\n",
		       __func__, tcon_rmem.axi_mem_size, total_size);
		return;
	}

	temp_size = lcd_tcon_conf->axi_bank * sizeof(struct tcon_rmem_config_s);
	tcon_rmem.axi_rmem = (struct tcon_rmem_config_s *)malloc(temp_size);
	if (!tcon_rmem.axi_rmem)
		return;
	memset(tcon_rmem.axi_rmem, 0, temp_size);

	temp_size = lcd_tcon_conf->axi_bank * sizeof(unsigned int);
	lcd_tcon_conf->axi_reg = (unsigned int *)malloc(temp_size);
	if (!lcd_tcon_conf->axi_reg) {
		free(tcon_rmem.axi_rmem);
		return;
	}
	memset(lcd_tcon_conf->axi_reg, 0, temp_size);

	temp_size = 0;
	for (i = 0; i < lcd_tcon_conf->axi_bank; i++) {
		tcon_rmem.axi_rmem[i].mem_paddr = tcon_rmem.axi_mem_paddr + temp_size;
		tcon_rmem.axi_rmem[i].mem_vaddr = (unsigned char *)
			(unsigned long)tcon_rmem.axi_rmem[i].mem_paddr;
		tcon_rmem.axi_rmem[i].mem_size = size[i];
		temp_size += size[i];

		lcd_tcon_conf->axi_reg[i] = reg[i];
	}
}

static void lcd_tcon_axi_mem_config_t5d(void)
{
	unsigned int size = 0x00500000;
	unsigned int reg = 0x261;
	unsigned int temp_size = 0;

	if (size > tcon_rmem.axi_mem_size) {
		LCDERR("%s: tcon axi_mem size 0x%x is not enough, need 0x%x\n",
			__func__, tcon_rmem.axi_mem_size, size);
		return;
	}

	temp_size = sizeof(struct tcon_rmem_config_s);
	tcon_rmem.axi_rmem = (struct tcon_rmem_config_s *)malloc(temp_size);
	if (!tcon_rmem.axi_rmem)
		return;
	memset(tcon_rmem.axi_rmem, 0, temp_size);

	temp_size = sizeof(unsigned int);
	lcd_tcon_conf->axi_reg = (unsigned int *)malloc(temp_size);
	if (!lcd_tcon_conf->axi_reg) {
		free(tcon_rmem.axi_rmem);
		return;
	}
	memset(lcd_tcon_conf->axi_reg, 0, temp_size);

	tcon_rmem.axi_rmem->mem_paddr = tcon_rmem.axi_mem_paddr;
	tcon_rmem.axi_rmem->mem_vaddr =
		(unsigned char *)(unsigned long)tcon_rmem.axi_rmem->mem_paddr;
	tcon_rmem.axi_rmem->mem_size = size;

	*lcd_tcon_conf->axi_reg = reg;
}

static int lcd_tcon_mem_config(void)
{
	unsigned char *mem_vaddr;
	unsigned int max_size;
	int ret;

	/* reserved memory */
	max_size = lcd_tcon_conf->axi_size + lcd_tcon_conf->bin_path_size;
	if (tcon_rmem.rsv_mem_size < max_size) {
		LCDERR("%s: tcon mem size 0x%x is not enough, need 0x%x\n",
		       __func__, tcon_rmem.rsv_mem_size, max_size);
		return -1;
	}

	tcon_rmem.axi_mem_size = lcd_tcon_conf->axi_size;
	tcon_rmem.axi_mem_paddr = tcon_rmem.rsv_mem_paddr;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("tcon axi_mem paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.axi_mem_paddr, tcon_rmem.axi_mem_size);
	lcd_tcon_conf->tcon_axi_mem_config();

	tcon_rmem.bin_path_rmem.mem_size = lcd_tcon_conf->bin_path_size;
	tcon_rmem.bin_path_rmem.mem_paddr =
		tcon_rmem.axi_mem_paddr + tcon_rmem.axi_mem_size;
	/* don't set bin_path_rmem.mem_vaddr here */
	if ((lcd_debug_print_flag & LCD_DBG_PR_NORMAL) && tcon_rmem.bin_path_rmem.mem_size > 0)
		LCDPR("tcon bin_path paddr: 0x%08x, size: 0x%x\n",
		      tcon_rmem.bin_path_rmem.mem_paddr,
		      tcon_rmem.bin_path_rmem.mem_size);

	/* default clear tcon rmem */
	mem_vaddr = (unsigned char *)(unsigned long)(tcon_rmem.rsv_mem_paddr);
	memset(mem_vaddr, 0, tcon_rmem.rsv_mem_size);

	ret = lcd_tcon_bin_path_update(tcon_rmem.bin_path_rmem.mem_size);
	if (ret)
		return -1;

	/* allocated memory, memory map table config */
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("tcon mm_table version: %d\n", tcon_mm_table.version);
	if (tcon_mm_table.version == 0)
		ret = lcd_tcon_mm_table_config_v0();
	else
		ret = lcd_tcon_mm_table_config_v1();

	return ret;
}

static void lcd_tcon_config_axi_offset_default(void)
{

	tcon_rmem.rsv_mem_paddr = env_get_ulong("tcon_mem_addr", 16, 0);
	if (tcon_rmem.rsv_mem_paddr) {
		tcon_rmem.rsv_mem_size = lcd_tcon_conf->rsv_mem_size;
		LCDPR("get lcd_tcon mem_addr from default\n");
	} else {
		LCDERR("can't find env tcon_mem_addr\n");
	}
}

static int lcd_tcon_load_init_data_from_unifykey(void)
{
	int key_len, data_len, ret;

	data_len = tcon_mm_table.core_reg_table_size;
	if (!tcon_mm_table.core_reg_table) {
		tcon_mm_table.core_reg_table = (unsigned char *)malloc
					(sizeof(unsigned char) * data_len);
		if (!tcon_mm_table.core_reg_table)
			return -1;
	}
	memset(tcon_mm_table.core_reg_table, 0, (sizeof(unsigned char) * data_len));
	key_len = data_len;
	ret = lcd_unifykey_get_no_header("lcd_tcon",
					 tcon_mm_table.core_reg_table,
					 &key_len);
	if (ret)
		goto lcd_tcon_load_init_data_err;
	if (key_len != data_len)
		goto lcd_tcon_load_init_data_err;

	memset(tcon_local_cfg.bin_ver, 0, TCON_BIN_VER_LEN);
	LCDPR("tcon: load init data len: %d\n", data_len);
	return 0;

lcd_tcon_load_init_data_err:
	free(tcon_mm_table.core_reg_table);
	tcon_mm_table.core_reg_table = NULL;
	LCDERR("%s: !!!!!!tcon unifykey load error!!!!!!\n", __func__);
	return -1;
}

static int lcd_tcon_load_init_data_from_unifykey_new(void)
{
	int key_len, data_len;
	unsigned char *buf, *p;
	struct lcd_tcon_init_block_header_s *data_header;
	int ret;

	data_len = tcon_mm_table.core_reg_table_size + LCD_TCON_DATA_BLOCK_HEADER_SIZE;
	buf = (unsigned char *)malloc(data_len * sizeof(unsigned char));
	if (!buf)
		return -1;

	key_len = data_len;
	ret = lcd_unifykey_get_tcon("lcd_tcon", buf, &key_len);
	if (ret)
		goto lcd_tcon_load_init_data_new_err;
	if (key_len != data_len)
		goto lcd_tcon_load_init_data_new_err;

	data_header = (struct lcd_tcon_init_block_header_s *)buf;
	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		LCDPR("unifykey header:\n");
		LCDPR("crc32             = 0x%08x\n", data_header->crc32);
		LCDPR("block_size        = %d\n", data_header->block_size);
		LCDPR("chipid            = %d\n", data_header->chipid);
		LCDPR("name              = %s\n", data_header->name);
	}
	memcpy(tcon_local_cfg.bin_ver, data_header->version,
	       LCD_TCON_INIT_BIN_VERSION_SIZE);
	tcon_local_cfg.bin_ver[TCON_BIN_VER_LEN - 1] = '\0';

	data_len = tcon_mm_table.core_reg_table_size;
	if (!tcon_mm_table.core_reg_table) {
		tcon_mm_table.core_reg_table = (unsigned char *)
				malloc(data_len * sizeof(unsigned char));
		if (!tcon_mm_table.core_reg_table)
			goto lcd_tcon_load_init_data_new_err;
		memset(tcon_mm_table.core_reg_table, 0,
		       (sizeof(unsigned char) * data_len));
	}
	p = buf + LCD_TCON_DATA_BLOCK_HEADER_SIZE;
	memcpy(tcon_mm_table.core_reg_table, p, data_len);
	free(buf);

	LCDPR("tcon: load init data len: %d, ver: %s\n",
	      data_len, tcon_local_cfg.bin_ver);
	return 0;

lcd_tcon_load_init_data_new_err:
	free(buf);
	LCDERR("%s: !!!!!!tcon unifykey load error!!!!!!\n", __func__);
	return -1;
}

static int lcd_tcon_get_config(char *dt_addr, struct aml_lcd_drv_s *pdrv, int load_id)
{
	int parent_offset, size;
	char *propdata;
	unsigned int mem_size;

	if (load_id & 0x1) {
		parent_offset = fdt_path_offset(dt_addr, "/reserved-memory");
		size = fdt_address_cells(dt_addr, parent_offset);
		parent_offset = fdt_path_offset(dt_addr, "/reserved-memory/linux,lcd_tcon");
		if (parent_offset < 0) {
			LCDERR("can't find node: /reserved-memory/linux,lcd_tcon\n");
		} else {
			propdata = (char *)fdt_getprop(dt_addr, parent_offset,
						       "alloc-ranges", NULL);
			if (!propdata) {
				LCDERR("failed to get lcd_tcon reserved-memory from dts\n");
				lcd_tcon_config_axi_offset_default();
			} else {
				if (size == 2)
					tcon_rmem.rsv_mem_paddr = be32_to_cpup((((u32 *)propdata) + 1));
				else
					tcon_rmem.rsv_mem_paddr = be32_to_cpup(((u32 *)propdata));
			}

			propdata = (char *)fdt_getprop(dt_addr, parent_offset, "size", NULL);
			if (!propdata) {
				LCDERR("failed to get tcon size from dts\n");
				lcd_tcon_config_axi_offset_default();
			} else {
				if (size == 2)
					mem_size = be32_to_cpup((((u32 *)propdata) + 1));
				else
					mem_size = be32_to_cpup(((u32 *)propdata));

				if (mem_size < lcd_tcon_conf->rsv_mem_size) {
					LCDERR("tcon mem_size is not enough\n");
					tcon_rmem.rsv_mem_paddr = 0;
					tcon_rmem.flag = 0;
				} else {
					tcon_rmem.rsv_mem_size = lcd_tcon_conf->rsv_mem_size;
				}
			}
		}
	} else {
		lcd_tcon_config_axi_offset_default();
	}
	if (tcon_rmem.rsv_mem_paddr) {
		tcon_rmem.flag = 1;
		lcd_tcon_mem_config();
		LCDPR("tcon: rsv_mem addr: 0x%x\n", tcon_rmem.rsv_mem_paddr);
	}

	tcon_mm_table.core_reg_table_size = lcd_tcon_conf->reg_table_len;
	if (lcd_tcon_conf->core_reg_ver)
		lcd_tcon_load_init_data_from_unifykey_new();
	else
		lcd_tcon_load_init_data_from_unifykey();

	lcd_tcon_data_load(pdrv);

	return 0;
}

static int lcd_tcon_core_flag(enum lcd_chip_e chip_type)
{
	int ret = 0;

	switch (chip_type) {
	case LCD_CHIP_TL1:
	case LCD_CHIP_TM2:
		ret = (readl(TCON_CORE_FLAG_LIC2) >> 17) & 0x1;
		break;
	default:
		break;
	}

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL) {
		if (ret)
			LCDPR("%s: tcon invalid\n", __func__);
	}

	return ret;
}

/* **********************************
 * tcon match data
 * **********************************
 */
static struct lcd_tcon_config_s tcon_data_tl1 = {
	.tcon_valid = 0,

	.core_reg_ver = 0,
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_TL1,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_TL1,
	.reg_table_len = LCD_TCON_TABLE_LEN_TL1,
	.core_reg_start = TCON_CORE_REG_START_TL1,

	.reg_top_ctrl = TCON_TOP_CTRL,
	.bit_en = BIT_TOP_EN_TL1,

	.reg_core_od = REG_CORE_OD_TL1,
	.bit_od_en = BIT_OD_EN_TL1,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_TL1,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_TL1,

	.axi_bank = LCD_TCON_AXI_BANK_TL1,

	.rsv_mem_size    = 0x00c00000, /* 12M */
	.axi_size        = 0x00a00000, /* 10M */
	.bin_path_size   = 0x00002800, /* 10K */
	.vac_size        = 0x00002000, /* 8K */
	.demura_set_size = 0x00001000, /* 4K */
	.demura_lut_size = 0x00120000, /* 1152K */
	.acc_lut_size    = 0x00001000, /* 4K */

	.axi_reg = NULL,
	.tcon_axi_mem_config = lcd_tcon_axi_mem_config_tl1,
	.tcon_enable = lcd_tcon_enable_tl1,
	.tcon_disable = lcd_tcon_disable_tl1,
};

static struct lcd_tcon_config_s tcon_data_t5 = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T5,
	.core_reg_start = TCON_CORE_REG_START_T5,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5,

	.reg_core_od = REG_CORE_OD_T5,
	.bit_od_en = BIT_OD_EN_T5,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5,

	.axi_bank = LCD_TCON_AXI_BANK_T5,

	.rsv_mem_size    = 0x00c00000, /* 12M */
	.axi_size        = 0x00a00000, /* 9M */
	.bin_path_size   = 0x00002800, /* 10K */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_reg = NULL,
	.tcon_axi_mem_config = lcd_tcon_axi_mem_config_t5,
	.tcon_enable = lcd_tcon_enable_t5,
	.tcon_disable = lcd_tcon_disable_t5,
};

static struct lcd_tcon_config_s tcon_data_t5d = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5D,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5D,
	.reg_table_len = LCD_TCON_TABLE_LEN_T5D,
	.core_reg_start = TCON_CORE_REG_START_T5D,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5D,

	.reg_core_od = REG_CORE_OD_T5D,
	.bit_od_en = BIT_OD_EN_T5D,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5D,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5D,

	.axi_bank = LCD_TCON_AXI_BANK_T5D,

	.rsv_mem_size    = 0x00800000, /* 8M */
	.axi_size        = 0x00500000, /* 5M */
	.bin_path_size   = 0x00002800, /* 10K */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_reg = NULL,
	.tcon_axi_mem_config = lcd_tcon_axi_mem_config_t5d,
	.tcon_enable = lcd_tcon_enable_t5,
	.tcon_disable = lcd_tcon_disable_t5,
};

static struct lcd_tcon_config_s tcon_data_t3 = {
	.tcon_valid = 0,

	.core_reg_ver = 1, /* new version with header */
	.core_reg_width = LCD_TCON_CORE_REG_WIDTH_T5,
	.reg_table_width = LCD_TCON_TABLE_WIDTH_T5,
	.reg_table_len = LCD_TCON_TABLE_LEN_T5,
	.core_reg_start = TCON_CORE_REG_START_T5,

	.reg_top_ctrl = REG_LCD_TCON_MAX,
	.bit_en = BIT_TOP_EN_T5,

	.reg_core_od = REG_CORE_OD_T5,
	.bit_od_en = BIT_OD_EN_T5,

	.reg_ctrl_timing_base = REG_LCD_TCON_MAX,
	.ctrl_timing_offset = CTRL_TIMING_OFFSET_T5,
	.ctrl_timing_cnt = CTRL_TIMING_CNT_T5,

	.axi_bank = LCD_TCON_AXI_BANK_T5,

	.rsv_mem_size    = 0x00c00000, /* 12M */
	.axi_size        = 0x00a00000, /* 9M */
	.bin_path_size   = 0x00002800, /* 10K */
	.vac_size        = 0,
	.demura_set_size = 0,
	.demura_lut_size = 0,
	.acc_lut_size    = 0,

	.axi_reg = NULL,
	.tcon_axi_mem_config = lcd_tcon_axi_mem_config_t5,
	.tcon_enable = lcd_tcon_enable_t5,
	.tcon_disable = lcd_tcon_disable_t3,
};

int lcd_tcon_probe(char *dt_addr, struct aml_lcd_drv_s *pdrv, int load_id)
{
	int ret = 0;
	struct lcd_config_s *pconf = &pdrv->config;

	lcd_tcon_conf = NULL;
	switch (pdrv->data->chip_type) {
	case LCD_CHIP_TL1:
	case LCD_CHIP_TM2:
		if (lcd_tcon_core_flag(pdrv->data->chip_type) == 0)
			lcd_tcon_conf = &tcon_data_tl1;
		break;
	case LCD_CHIP_T5:
		lcd_tcon_conf = &tcon_data_t5;
		break;
	case LCD_CHIP_T5D:
		lcd_tcon_conf = &tcon_data_t5d;
		break;
	case LCD_CHIP_T3:
		lcd_tcon_conf = &tcon_data_t3;
		break;
	default:
		break;
	}
	if (!lcd_tcon_conf)
		return 0;

	switch (pconf->basic.lcd_type) {
	case LCD_MLVDS:
		lcd_tcon_conf->tcon_valid = 1;
		break;
	case LCD_P2P:
		if (pdrv->data->chip_type == LCD_CHIP_T5D)
			lcd_tcon_conf->tcon_valid = 0;
		else
			lcd_tcon_conf->tcon_valid = 1;
		break;
	default:
		break;
	}
	if (lcd_tcon_conf->tcon_valid == 0)
		return 0;

	if (lcd_debug_print_flag & LCD_DBG_PR_NORMAL)
		LCDPR("%s\n", __func__);

	memset(&tcon_rmem, 0, sizeof(struct tcon_rmem_s));
	memset(&tcon_mm_table, 0, sizeof(struct tcon_mem_map_table_s));
	/*must before tcon_config, for memory alloc*/
	lcd_tcon_spi_data_probe(pdrv);
	ret = lcd_tcon_get_config(dt_addr, pdrv, load_id);

	pdrv->tcon_reg_print = lcd_tcon_reg_readback_print;
	pdrv->tcon_table_print = lcd_tcon_reg_table_print;
	pdrv->tcon_vac_print = lcd_tcon_vac_print;
	pdrv->tcon_demura_print = lcd_tcon_demura_print;
	pdrv->tcon_acc_print = lcd_tcon_acc_print;
	pdrv->tcon_data_print = lcd_tcon_data_print;
	pdrv->tcon_reg_read = lcd_tcon_reg_read;
	pdrv->tcon_reg_write = lcd_tcon_reg_write;
	pdrv->tcon_table_read = lcd_tcon_table_read;
	pdrv->tcon_table_write = lcd_tcon_table_write;

	return ret;
}

