
/*
 * common/cmd_cpu_temp.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
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
#include <asm/io.h>
#include <asm/arch/efuse.h>
#include <command.h>
#include <asm/arch/secure_apb.h>
#include <asm/arch/mailbox.h>
#include <asm/arch/thermal.h>
#include <asm/cpu_id.h>

//#define HHI_SAR_CLK_CNTL    0xc883c000+0xf6*4 //0xc883c3d8
int temp_base = 27;
#define NUM 30
uint32_t vref_en = 0;
uint32_t trim = 0;
int saradc_vref = -1;


static int get_tsc(int temp)
{
	int vmeasure, TS_C;
	switch (get_cpu_id().family_id) {
	case MESON_CPU_MAJOR_ID_GXBB:
	case MESON_CPU_MAJOR_ID_GXTVBB:
		/*TS_C = (adc-435)/8.25+16*/
		vmeasure = temp-(435+(temp_base-27)*3.4);
		printf("vmeasure=%d\n", vmeasure);
		TS_C = ((vmeasure)/8.25)+16;
		break;
	case MESON_CPU_MAJOR_ID_GXL:
	case MESON_CPU_MAJOR_ID_GXM:
		if (vref_en) {
			/*TS_C = 16-(adc-1655)/37.6*/
			vmeasure = temp-(1655+(temp_base-27)*15.3);
			printf("vmeasure=%d\n", vmeasure);
			TS_C = 16-((vmeasure)/37.6);
			break;
		} else {
			/*TS_C = 16-(adc-1778)/42*/
			vmeasure = temp-(1778+(temp_base-27)*17);
			printf("vmeasure=%d\n", vmeasure);
			TS_C = 16-((vmeasure)/42);
			break;
		}
	case MESON_CPU_MAJOR_ID_TXL:
	case MESON_CPU_MAJOR_ID_TXLX:
		/*TS_C = 16-(adc-1530)/40*/
		vmeasure = temp-(1530+(temp_base-27)*15.5);
		printf("vmeasure=%d\n", vmeasure);
		TS_C = 16-((vmeasure)/40);
		break;
	default:
		printf("cpu family id not support!!!\n");
		return -1;
	}

	if (TS_C > 31)
		TS_C = 31;
	else if (TS_C < 0)
		TS_C = 0;
	printf("TS_C=%d\n", TS_C);
	return TS_C;
}

static int adc_init_chan6(void)
{
	switch (get_cpu_id().family_id) {
	case MESON_CPU_MAJOR_ID_GXBB:
	case MESON_CPU_MAJOR_ID_GXTVBB:
		writel(0x002c2000, SAR_ADC_REG11);/*bit20: test mode disabled*/
		writel(0x00000006, SAR_ADC_CHAN_LIST);
		writel(0x00003000, SAR_ADC_AVG_CNTL);
		writel(0xc3a8500a, SAR_ADC_REG3);
		writel(0x010a000a, SAR_ADC_DELAY);
		writel(0x03eb1a0c, SAR_ADC_AUX_SW);
		writel(0x008c000c, SAR_ADC_CHAN_10_SW);
		writel(0x030e030c, SAR_ADC_DETECT_IDLE_SW);
		writel(0x0c00c400, SAR_ADC_DELTA_10);
		writel(0x00000114, SAR_CLK_CNTL);        /* Clock */
		writel(readl(0xc110868c)|(0x1<<28), SAR_ADC_REG3);
		break;
	case MESON_CPU_MAJOR_ID_GXL:
	case MESON_CPU_MAJOR_ID_GXM:
		writel(0x002c2060, SAR_ADC_REG11);/*bit20 disabled*/
		writel(0x00000006, SAR_ADC_CHAN_LIST);/*channel 6*/
		writel(0x00003000, SAR_ADC_AVG_CNTL);
		writel(0xc8a8500a, SAR_ADC_REG3);/*bit27:0*/
		writel(0x010a000a, SAR_ADC_DELAY);
		writel(0x03eb1a0c, SAR_ADC_AUX_SW);
		writel(0x008c000c, SAR_ADC_CHAN_10_SW);
		writel(0x030e030c, SAR_ADC_DETECT_IDLE_SW);
		writel(0x0c00c400, SAR_ADC_DELTA_10);
		writel(0x00000114, SAR_CLK_CNTL);/*Clock*/
		break;
	case MESON_CPU_MAJOR_ID_TXL:
	case MESON_CPU_MAJOR_ID_TXLX:
		writel(0x00000006, SAR_ADC_CHAN_LIST);/*channel 6*/
		writel(0x00003000, SAR_ADC_AVG_CNTL);
		writel(0xc8a8500a, SAR_ADC_REG3);/*bit27:1 disable*/
		writel(0x010a000a, SAR_ADC_DELAY);
		writel(0x03eb1a0c, SAR_ADC_AUX_SW);
		writel(0x008c000c, SAR_ADC_CHAN_10_SW);
		writel(0x030e030c, SAR_ADC_DETECT_IDLE_SW);
		writel(0x0c00c400, SAR_ADC_DELTA_10);
		writel(0x00000110, SAR_CLK_CNTL);/*Clock*/
		writel(0x002c2060, SAR_ADC_REG11);/*bit20 disabled*/
		break;
	default:
		printf("cpu family id not support!!!\n");
		return -1;
	}
	return 0;
}

static int get_adc_sample(int chan)
{
	unsigned value;
	int count = 0;

	if (!(readl(SAR_CLK_CNTL)&(1<<8)))/*check and open clk*/
		writel(readl(SAR_CLK_CNTL)|(1<<8), SAR_CLK_CNTL);
	if (!(readl(SAR_BUS_CLK_EN)&(1<<EN_BIT)))/*check and open clk*/
		writel(readl(SAR_BUS_CLK_EN)|(1<<EN_BIT), HHI_GCLK_MPEG2);

	/*adc reg4 bit14~15: read adc sample value flag*/
	/*0x21a4: bit14: kernel  bit15: bl30*/
	for (; count <= 100; count++) {
		if (count == 100) {
			printf("%s: get flag timeout!\n",__func__);
			return -1;
		}

		if (!((readl(SAR_ADC_DELAY)>>14)&3)) {
			writel(readl(SAR_ADC_DELAY)|(FLAG_BUSY_BL30),
				   SAR_ADC_DELAY);
			if (((readl(SAR_ADC_DELAY)>>14)&3) != 0x2)
				/*maybe kernel set flag, try again*/
				writel(readl(SAR_ADC_DELAY)&(~(FLAG_BUSY_BL30)),
				SAR_ADC_DELAY);
			else
				break;/*set bl30 read flag ok*/
		} else{/*kernel set flag, clear bl30 flag and wait*/
			writel(readl(SAR_ADC_DELAY)&(~(FLAG_BUSY_BL30)),
				SAR_ADC_DELAY);
			udelay(20);
		}
	}
#ifndef CONFIG_CHIP_AML_GXB
	/* if thermal VREF(5 bits) is not zero, write it to SAR_ADC_REG13[13:9]
	 * and set SAR_ADC_REG13[8]:0, chipid >= GXL
	 */
	if (get_cpu_id().family_id >= MESON_CPU_MAJOR_ID_GXL) {
		saradc_vref = (readl(SAR_ADC_REG13)>>8)&0x3f; /*back up SAR_ADC_REG13[13:8]*/
		if ((readl(AO_SEC_SD_CFG12)>>19) & 0x1f) { /*thermal VREF*/
			writel(((readl(SAR_ADC_REG13))&(~(0x3f<<8))) /*SAR_ADC_REG13[8]:0*/
				|(((readl(AO_SEC_SD_CFG12)>>19) & 0x1f)<<9), /*SAR_ADC_REG13[13:9]*/
				SAR_ADC_REG13);
			vref_en = 1;
		} else if ((get_cpu_id().family_id >= MESON_CPU_MAJOR_ID_TXL)&&
			((trim == 1)||
			((((readl(SEC_AO_SEC_SD_CFG12))>>24)&0xff)==0xc0))) {
			writel(((readl(SAR_ADC_REG13))&(~(0x3f<<8))) /*SAR_ADC_REG13[13:8]:0*/
				|(0x1e<<8), /*SAR_ADC_REG13[13:8]:[0x1e]*/
				SAR_ADC_REG13);
		} else {
			writel((readl(SAR_ADC_REG13))&(~(0x3f<<8)), SAR_ADC_REG13);
		}
	}
#endif
	writel(0x00000006, SAR_ADC_CHAN_LIST);/*channel 6*/
	writel(0xc000c|(0x6<<23)|(0x6<<7), SAR_ADC_DETECT_IDLE_SW);/*channel 6*/

	writel((readl(SAR_ADC_REG0)&(~(1<<0))), SAR_ADC_REG0);
	writel((readl(SAR_ADC_REG0)|(1<<0)), SAR_ADC_REG0);
	writel((readl(SAR_ADC_REG0)|(1<<2)), SAR_ADC_REG0);/*start sample*/

	count = 0;
	do {
		udelay(20);
		count++;
	} while ((readl(SAR_ADC_REG0) & (0x7<<28))
		&& (count < 100));/*finish sample?*/
	if (count == 100) {
		writel(readl(SAR_ADC_REG3)&(~(1 << 29)), SAR_ADC_REG3);
		printf("%s: wait finish sample timeout!\n",__func__);
		return -1;
	}

	value = readl(SAR_ADC_FIFO_RD);
#ifndef CONFIG_CHIP_AML_GXB
	if (saradc_vref >= 0) /*write back SAR_ADC_REG13[13:8]*/
		writel(((readl(SAR_ADC_REG13))&(~(0x3f<<8)))|
				((saradc_vref & 0x3f)<<8),
				SAR_ADC_REG13);
#endif
	writel(readl(SAR_ADC_DELAY)&(~(FLAG_BUSY_BL30)), SAR_ADC_DELAY);
	if (((value>>12) & 0x7) == 0x6)
		value = value&SAMPLE_BIT_MASK;
	else{
		value = -1;
		printf("%s:sample value err! ch:%d, flag:%d\n", __func__,
			((value>>12) & 0x7), ((readl(SAR_ADC_DELAY)>>14)&3));
	}
	return value;
}

static unsigned int get_cpu_temp(int tsc, int flag)
{
	unsigned value;
	if (flag) {
		value = readl(SAR_ADC_REG11);
	  writel(((value&(~(0x1f<<14)))|((tsc&0x1f)<<14)), SAR_ADC_REG11);
	  /* bit[14-18]:tsc */
	} else{
		value = readl(SAR_ADC_REG11);
	  writel(((value&(~(0x1f<<14)))|(0x10<<14)), SAR_ADC_REG11);
	  /* bit[14-18]:0x16 */
	}
	return  get_adc_sample(6);
}

void quicksort(int a[], int numsize)
{
	int i = 0, j = numsize-1;
	int val = a[0];
	if (numsize > 1) {
		while (i < j) {
			for (; j > i; j--)
				if (a[j] < val) {
					a[i] = a[j];
					break;
				}
			for (; i < j; i++)
				if (a[i] > val) {
					a[j] = a[i];
					break;
				}
		}
	a[i] = val;
	quicksort(a, i);
	quicksort(a+i+1, numsize-1-i);
}
}

static unsigned do_read_calib_data(int *flag, int *temp, int *TS_C)
{
	char buf[2];
	unsigned ret;
	*flag = 0;
	buf[0] = 0; buf[1] = 0;

	char flagbuf;

	ret = readl(AO_SEC_SD_CFG12);
	flagbuf = (ret>>24)&0xff;
	if (((int)flagbuf != 0xA0) && ((int)flagbuf != 0x40)
		&& ((int)flagbuf != 0xC0)) {
		printf("thermal ver flag error!\n");
		printf("flagbuf is 0x%x!\n", flagbuf);
		return 0;
	}

	buf[0] = (ret)&0xff;
	buf[1] = (ret>>8)&0xff;

	*temp = buf[1];
	*temp = (*temp<<8)|buf[0];
	*TS_C =  *temp&0x1f;
	*flag = (*temp&0x8000)>>15;
	*temp = (*temp&0x7fff)>>5;

	if ((get_cpu_id().family_id == MESON_CPU_MAJOR_ID_GXBB)
		&&(0x40 == (int)flagbuf))/*ver2*/
		*TS_C = 16;

	if (get_cpu_id().family_id >= MESON_CPU_MAJOR_ID_GXL)
		*temp = (*temp)<<2; /*adc 12bit sample*/

	printf("adc=%d,TS_C=%d,flag=%d\n", *temp, *TS_C, *flag);
	return ret;
}

static int do_write_trim(cmd_tbl_t *cmdtp, int flag1,
	int argc, char * const argv[])
{
	int temp = 0;
	int temp1[NUM];
	char buf[2];
	unsigned int data;
	int i, TS_C;
	int ret;
	int flag;

	memset(temp1, 0, NUM);

	ret = adc_init_chan6();
	if (ret)
		goto err;

	ret = do_read_calib_data(&flag, &temp, &TS_C);
	if (ret) {
		printf("chip has trimed!!!\n");
		return -1;
	} else {
		printf("chip is not triming! triming now......\n");
		flag = 0;
		temp = 0;
		TS_C = 0;
		trim = 1;
	}
	for (i = 0; i < NUM; i++) {
		udelay(10000);
		/*adc sample value*/
		temp1[i] = get_cpu_temp(16, 0);
	}

	printf("raw data\n");
	for (i = 0; i < NUM; i++)
		printf("%d ", temp1[i]);

	printf("\nsort  data\n");

	quicksort(temp1, NUM);
	for (i = 0; i < NUM; i++)
		printf("%d ", temp1[i]);
	printf("\n");
	for (i = 2; i < NUM-2; i++)
		temp += temp1[i];
	temp = temp/(NUM-4);
	printf("the adc cpu adc=%d\n", temp);

/**********************************/
	TS_C = get_tsc(temp);
	if ((TS_C == 31) || (TS_C <= 0)) {
		printf("TS_C: %d NO Trim! Bad chip!Please check!!!\n", TS_C);
		goto err;
	}
/**********************************/
	temp = 0;
	memset(temp1, 0, NUM);
	/* flag=1; */
	for (i = 0; i < NUM; i++) {
		udelay(10000);
		temp1[i] = get_cpu_temp(TS_C, 1);
	}
	printf("use triming  data\n");
	quicksort(temp1, NUM);
	for (i = 0; i < NUM; i++)
		printf("%d ", temp1[i]);
	printf("\n");
	for (i = 2; i < NUM-2; i++)
		temp += temp1[i];
	temp = temp/(NUM-4);
	printf("the adc cpu adc=%d\n", temp);

/**************recalculate to 27******/
	switch (get_cpu_id().family_id) {
	case MESON_CPU_MAJOR_ID_GXBB:
	case MESON_CPU_MAJOR_ID_GXTVBB:
		temp = temp - 3.4*(temp_base - 27);
		break;
	case MESON_CPU_MAJOR_ID_GXL:/*12bit*/
	case MESON_CPU_MAJOR_ID_GXM:
		if (vref_en) {
			temp = temp - 15.3*(temp_base - 27);
			temp = temp>>2;/*efuse only 10bit adc*/
			break;
		} else {
			temp = temp - 17*(temp_base - 27);
			temp = temp>>2;/*efuse only 10bit adc*/
			break;
		}
	case MESON_CPU_MAJOR_ID_TXL:
	case MESON_CPU_MAJOR_ID_TXLX:
		temp = temp - 15.5*(temp_base - 27);
		temp = temp>>2;/*efuse only 10bit adc*/
		break;
	default:
		printf("cpu family id not support!!!\n");
		goto err;
	}
/**********************************/
	temp = ((temp<<5)|(TS_C&0x1f))&0xffff;
/* write efuse tsc,flag */
	buf[0] = (char)(temp&0xff);
	buf[1] = (char)((temp>>8)&0xff);
	buf[1] |= 0x80;
	printf("buf[0]=%x,buf[1]=%x\n", buf[0], buf[1]);
	data = buf[1]<<8 | buf[0];
	ret = thermal_calibration(0, data);
	return ret;
err:
	return -1;
}

static int do_read_temp(cmd_tbl_t *cmdtp, int flag1,
	int argc, char * const argv[])
{
	int temp;
	int TS_C;
	int flag, adc, count, tempa;
	unsigned ret;
	flag = 0;
	char buf[100] = {};

	setenv("tempa", " ");
	adc_init_chan6();
	ret = do_read_calib_data(&flag, &temp, &TS_C);
	if (ret) {
		adc = 0;
		count = 0;
		while (count < 64) {
			adc += get_cpu_temp(TS_C, flag);
			count++;
			udelay(200);
		}
		adc /= count;
		tempa = 0;
		printf("adc=%d\n", adc);
		if (flag) {
			switch (get_cpu_id().family_id) {
			case MESON_CPU_MAJOR_ID_GXBB:
			case MESON_CPU_MAJOR_ID_GXTVBB:
				tempa = (10*(adc-temp))/34+27;
				break;
			case MESON_CPU_MAJOR_ID_GXL:
			case MESON_CPU_MAJOR_ID_GXM:
				if (vref_en)/*thermal VREF*/
					tempa = (10*(adc-temp))/153+27;
				else
					tempa = (10*(adc-temp))/171+27;
				break;
			case MESON_CPU_MAJOR_ID_TXL:
			case MESON_CPU_MAJOR_ID_TXLX:
				tempa = (10*(adc-temp))/155+27;
				break;
			}
			printf("tempa=%d\n", tempa);

			sprintf(buf, "%d", tempa);
			setenv("tempa", buf);
			memset(buf, 0, sizeof(buf));
			sprintf(buf, "temp:%d, adc:%d, tsc:%d, dout:%d", tempa, adc, TS_C, temp);
			setenv("err_info_tempa", buf);
		} else {
			printf("This chip is not trimmed\n");
			sprintf(buf, "%s", "This chip is not trimmed");
			setenv("err_info_tempa", buf);
			return -1;
		}
	} else {
		printf("read calibrated data failed\n");
		sprintf(buf, "%s", "read calibrated data failed");
		setenv("err_info_tempa", buf);
		return -1;
	}

	return 0;
}

static int do_write_version(cmd_tbl_t *cmdtp, int flag1,
	int argc, char * const argv[])
{
	int ret;
	unsigned int val = simple_strtoul(argv[1], NULL, 16);

	ret = thermal_calibration(1, val);
	return ret;
}

static int do_set_trim_base(cmd_tbl_t *cmdtp, int flag1,
	int argc, char * const argv[])
{
	int temp = simple_strtoul(argv[1], NULL, 10);
	temp_base = temp;
	printf("set base temperature: %d\n", temp_base);
	return 0;
}

static int do_temp_triming(cmd_tbl_t *cmdtp, int flag1,
	int argc, char * const argv[])
{
	int cmd_result;
	int temp = simple_strtoul(argv[1], NULL, 10);
	temp_base = temp;
	printf("set base temperature: %d\n", temp_base);

	cmd_result = run_command("write_trim", 0);
	if (cmd_result == CMD_RET_SUCCESS) {
		/*FB calibration v5: 1010 0000*/
		/*manual calibration v2: 0100 0000*/
		printf("manual calibration v3: 1100 0000\n");
		cmd_result = run_command("write_version 0xc0", 0);
		if (cmd_result != CMD_RET_SUCCESS)
			printf("write version error!!!\n");
	} else {
		printf("trim FAIL!!!Please check!!!\n");
	}
	run_command("read_temp", 0);
	return 0;
}

U_BOOT_CMD(
	write_trim,	5,	0,	do_write_trim,
	"cpu temp-system",
	"write_trim data"
);

U_BOOT_CMD(
	read_temp,	5,	0,	do_read_temp,
	"cpu temp-system",
	"read_temp pos"
);

U_BOOT_CMD(
	write_version,	5,	0,	do_write_version,
	"cpu temp-system",
	"write_flag"
);

U_BOOT_CMD(
	temp_triming,	5,	1,	do_temp_triming,
	"cpu temp-system",
	"write_trim 502 and write flag"
);

U_BOOT_CMD(
	set_trim_base,	5,	1,	do_set_trim_base,
	"cpu temp-system",
	"set triming base temp"
);
