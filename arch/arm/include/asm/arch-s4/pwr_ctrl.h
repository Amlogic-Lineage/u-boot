/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _PWRC_H_
#define _PWRC_H_

#define PWR_ON    1
#define PWR_OFF   0

#define PM_DOS_HEVC		0
#define PM_DOS_VDEC		1
#define PM_VPU_HDMI		2
#define PM_USB_COMB		3
#define PM_GE2D			4
#define PM_ETH			5
#define PM_DEMOD		6
#define PM_AUDIO		7

#define PM_MAX			8
//static int pm_max = 8;
/*
char* domain_name[] =
{
	"PM_DOS_HEVC",
	"PM_DOS_VDEC",
	"PM_VPU_HDMI",
	"PM_USB_COMB",
	"PM_GE2D",
	"PM_ETH",
	"PM_DEMOD",
	"PM_AUDIO",
};
*/
unsigned long pwr_ctrl_psci_smc(unsigned int power_domain, bool power_control);
#endif
