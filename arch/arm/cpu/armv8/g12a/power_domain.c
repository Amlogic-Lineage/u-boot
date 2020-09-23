/*
 * drivers/amlogic/power/power_domain.c
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
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
 */

#include <common.h>
#include <asm/arch/io.h>
#include <asm/arch/regs.h>
#include <asm/arch/secure_apb.h>
#include <amlogic/power_domain.h>

static void power_switch(int pwr_domain, bool pwr_switch)
{
	unsigned int value;

	value = readl(AO_RTI_GEN_PWR_SLEEP0);
	if (pwr_switch == PWR_ON)
		value &= ~(1 << pwr_domain);
	else
		value |= (1 << pwr_domain);
	writel(value, (AO_RTI_GEN_PWR_SLEEP0));
}

static void mem_pd_switch(int pwr_domain, bool pwr_switch)
{
	unsigned int value;

	if (pwr_switch == PWR_ON) {
		switch (pwr_domain) {
		case PM_DOS_HCODEC:
			writel(0x0, P_DOS_MEM_PD_HCODEC);
			break;
		case PM_DOS_VDEC:
			writel(0x0, P_DOS_MEM_PD_VDEC);
			break;
		case PM_DOS_HEVC:
			writel(0x0, P_DOS_MEM_PD_HEVC);
			break;
		case PM_WAVE420L:
			writel(0x0, (P_DOS_MEM_PD_WAVE420L));
			break;
		case PM_CSI:
			value = readl(HHI_MEM_PD_REG0);
			value &= ~(0x3 << 6);
			writel(value, HHI_MEM_PD_REG0);
			break;
		case PM_VPU:
			writel(0x0, HHI_VPU_MEM_PD_REG0);
			writel(0x0, HHI_VPU_MEM_PD_REG1);
			writel(0x0, HHI_VPU_MEM_PD_REG2);
			writel(0x0, HHI_VPU_MEM_PD_REG3);
			writel(0x0, HHI_VPU_MEM_PD_REG4);
			value = readl(HHI_MEM_PD_REG0);
			value &= ~(0xff << 8);
			writel(value, HHI_MEM_PD_REG0);
			break;
		case PM_NN:
			writel(0x0, HHI_NANOQ_MEM_PD_REG0);
			writel(0x0, HHI_NANOQ_MEM_PD_REG1);
			break;
		case PM_USB:
			value = readl(HHI_MEM_PD_REG0);
			value &= ~(0x3 << 30);
			writel(value, HHI_MEM_PD_REG0);
			break;
		case PM_PCIE0:
			value = readl(HHI_MEM_PD_REG0);
			value &= ~(0xf<<26);
			writel(value, HHI_MEM_PD_REG0);
			break;
		case PM_GE2D:
			value = readl(HHI_MEM_PD_REG0);
			value &= ~(0xff<<18);
			writel(value, HHI_MEM_PD_REG0);
			break;
		}
	} else {
		switch (pwr_domain) {
		case PM_DOS_HCODEC:
			writel(0xffffffff, P_DOS_MEM_PD_HCODEC);
			break;
		case PM_DOS_VDEC:
			writel(0xffffffff, P_DOS_MEM_PD_VDEC);
			break;
		case PM_DOS_HEVC:
			writel(0xffffffff, P_DOS_MEM_PD_HEVC);
			break;
		case PM_WAVE420L:
			writel(0xffffffff, P_DOS_MEM_PD_WAVE420L);
			break;
		case PM_CSI:
			value = readl(HHI_MEM_PD_REG0);
			value |= (0x3 << 6);
			writel(value, HHI_MEM_PD_REG0);
			break;
		case PM_VPU:
			writel(0xffffffff, HHI_VPU_MEM_PD_REG0);
			writel(0xffffffff, HHI_VPU_MEM_PD_REG1);
			writel(0xffffffff, HHI_VPU_MEM_PD_REG2);
			writel(0xffffffff, HHI_VPU_MEM_PD_REG3);
			writel(0xffffffff, HHI_VPU_MEM_PD_REG4);
			value = readl(HHI_MEM_PD_REG0);
			value |= (0xff << 8);
			writel(value, HHI_MEM_PD_REG0);
			break;
		case PM_NN:
			writel(0xffffffff, HHI_NANOQ_MEM_PD_REG0);
			writel(0xffffffff, HHI_NANOQ_MEM_PD_REG1);
			break;
		case PM_USB:
			value = readl(HHI_MEM_PD_REG0);
			value |= (0x3 << 30);
			writel(value, HHI_MEM_PD_REG0);
			break;
		case PM_PCIE0:
			value = readl(HHI_MEM_PD_REG0);
			value |= (0xf<<26);
			writel(value, HHI_MEM_PD_REG0);
			break;
		case PM_GE2D:
			value = readl(HHI_MEM_PD_REG0);
			value |= (0xff<<18);
			writel(value, HHI_MEM_PD_REG0);
			break;
		}
	}
}

static void reset_switch(int pwr_domain, bool pwr_switch)
{
	unsigned int value;
	unsigned int tmp;

	if (pwr_switch == PWR_ON) {
		switch (pwr_domain) {
		case PM_DOS_HCODEC:
			value = readl(P_DOS_SW_RESET1);
			value &= ~(0xffff<<2);
			writel(value, P_DOS_SW_RESET1);
			break;
		case PM_DOS_VDEC:
			value = readl(P_DOS_SW_RESET0);
			value &= ~(0x1fff<<2);
			writel(value, P_DOS_SW_RESET0);
			break;
		case PM_DOS_HEVC:
			value = readl(P_DOS_SW_RESET3);
			value &= ~(0x3ffff<<2 | 1<<24);
			writel(value, P_DOS_SW_RESET3);
			break;
		case PM_WAVE420L:
			value = readl(P_DOS_SW_RESET4);
			value &= ~(0xf<<8);
			writel(value, P_DOS_SW_RESET4);
			break;
		case PM_VPU:
			tmp = 0x1 << 5 | 0x1 << 10 | 0x1 << 19 | 0x1 << 13;
			value = readl(P_RESET0_LEVEL);
			value |= tmp;
			writel(value, P_RESET0_LEVEL);
			tmp = 0x1 << 5 | 0x1 << 4;
			value = readl(P_RESET1_LEVEL);
			value |= tmp;
			writel(value, P_RESET1_LEVEL);
			tmp = 0x1 << 15;
			value = readl(P_RESET2_LEVEL);
			value |= tmp;
			writel(value, P_RESET2_LEVEL);
			tmp = 0x1 << 6 | 0x1 << 7 | 0x1 << 13 |
			      0x1 << 5 | 0x1 << 9 | 0x1 << 4 | 0x1 << 12;
			value = readl(P_RESET4_LEVEL);
			value |= tmp;
			writel(value, P_RESET4_LEVEL);
			tmp = 0x1 << 7;
			value = readl(P_RESET7_LEVEL);
			value |= tmp;
			writel(value, P_RESET7_LEVEL);
			break;
		case PM_NN:
			value = readl(P_RESET2_LEVEL);
			value |= (0x1<<12);
			writel(value, P_RESET2_LEVEL);
			break;
		case PM_USB:
			value = readl(P_RESET1_LEVEL);
			value |= (0x1<<2);
			writel(value, P_RESET1_LEVEL);
			break;
		case PM_PCIE0:
			value = readl(P_RESET0_LEVEL);
			value |= ((0x1<<12)|(0x3<<14));
			writel(value, P_RESET0_LEVEL);
			break;
		case PM_GE2D:
			value = readl(P_RESET2_LEVEL);
			value |= (0x1<<6);
			writel(value, P_RESET2_LEVEL);
			break;
		}
	} else {
		switch (pwr_domain) {
		case PM_DOS_HCODEC:
			value = readl(P_DOS_SW_RESET1);
			value |= (0xffff<<2);
			writel(value, P_DOS_SW_RESET1);
			break;
		case PM_DOS_VDEC:
			value = readl(P_DOS_SW_RESET0);
			value |= (0x1fff<<2);
			writel(value, P_DOS_SW_RESET0);
			break;
		case PM_DOS_HEVC:
			value = readl(P_DOS_SW_RESET3);
			value |= (0x3ffff<<2 | 1<<24);
			writel(value, P_DOS_SW_RESET3);
			break;
		case PM_WAVE420L:
			value = readl(P_DOS_SW_RESET4);
			value |= (0xf<<8);
			writel(value, P_DOS_SW_RESET4);
			break;
		case PM_VPU:
			tmp = 0x1 << 5 | 0x1 << 10 | 0x1 << 19 | 0x1 << 13;
			value = readl(P_RESET0_LEVEL);
			value &= ~tmp;
			writel(value, P_RESET0_LEVEL);
			tmp = 0x1 << 5 | 0x1 << 4;
			value = readl(P_RESET1_LEVEL);
			value &= ~tmp;
			writel(value, P_RESET1_LEVEL);
			tmp = 0x1 << 15;
			value = readl(P_RESET2_LEVEL);
			value &= ~tmp;
			writel(value, P_RESET2_LEVEL);
			tmp = 0x1 << 6 | 0x1 << 7 | 0x1 << 13 |
			      0x1 << 5 | 0x1 << 9 | 0x1 << 4 | 0x1 << 12;
			value = readl(P_RESET4_LEVEL);
			value &= ~tmp;
			writel(value, P_RESET4_LEVEL);
			tmp = 0x1 << 7;
			value = readl(P_RESET7_LEVEL);
			value &= ~tmp;
			writel(value, P_RESET7_LEVEL);
			break;
		case PM_NN:
			value = readl(P_RESET2_LEVEL);
			value &= ~(0x1<<12);
			writel(value, P_RESET2_LEVEL);
			break;
		case PM_USB:
			value = readl(P_RESET1_LEVEL);
			value &= ~(0x1<<2);
			writel(value, P_RESET1_LEVEL);
			break;
		case PM_PCIE0:
			value = readl(P_RESET0_LEVEL);
			value &= ~((0x1<<12)|(0x3<<14));
			writel(value, P_RESET0_LEVEL);
			break;
		case PM_GE2D:
			value = readl(P_RESET2_LEVEL);
			value &= ~(0x1<<6);
			writel(value, P_RESET2_LEVEL);
			break;
		}
	}
}

static void iso_switch(int pwr_domain, bool pwr_switch)
{
	unsigned int value;

	value = readl(AO_RTI_GEN_PWR_ISO0);
	if (pwr_switch == PWR_ON)
		value &= ~(1 << pwr_domain);
	else
		value |= (1 << pwr_domain);
	writel(value, AO_RTI_GEN_PWR_ISO0);
}

void power_domain_switch(int pwr_domain, bool pwr_switch)
{
	if (pwr_switch == PWR_ON) {
		/* Powerup Power Domain */
		power_switch(pwr_domain, PWR_ON);
		udelay(50);

		/* Powerup memories */
		mem_pd_switch(pwr_domain, PWR_ON);
		udelay(150);

		reset_switch(pwr_domain, PWR_OFF);

		/* remove isolations */
		iso_switch(pwr_domain, PWR_ON);

		/* deassert reset */
		reset_switch(pwr_domain, PWR_ON);

	} else {
		/* reset */
		reset_switch(pwr_domain, PWR_OFF);

		/* add isolation to domain */
		iso_switch(pwr_domain, PWR_OFF);

		/* Power down memories */
		mem_pd_switch(pwr_domain, PWR_OFF);
		udelay(50);

		/* Power off  domain */
		power_switch(pwr_domain, PWR_OFF);
	}
}
