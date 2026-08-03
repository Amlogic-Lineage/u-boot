// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <asm/arch/stick_mem.h>

uint32_t get_stick_reboot_flag(void)
{
	/* g12a/g12b/sm1 have no AOCPU stick memory */
	return 0;
}
