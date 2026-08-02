/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
 * Must match the bl30/bl301 split that fip/g12a/build.sh actually packs:
 * blx_bin_limit=40960 (bl30 padded to 40K) and blx01_bin_limit=13312, i.e.
 * bl301 is loaded at 0x1000A000. Commit abc1ccc86bb ("bl301: change bl30 size
 * to 46k [2/3]") moved this to 46K/15K, but parts [1/3] and [3/3] - the new
 * bl30.bin and the matching padding - never landed here: bl30/bin/g12a/bl30.bin
 * (g12a_v1.1.3522) still hard-codes 0x1000A000 and 0x1000D400. Linking bl301
 * for 0x1000B800 while BL30 copies it to 0x1000A000 leaves every absolute data
 * reference 6K short, and the AOCPU dies right after "Inits done" with
 * "PROCESS EXCEPTION: 06 ... pc :00000000 / Invalid state".
 */
#define CONFIG_RAM_BASE        (0x10000000 + 40 * 1024)
#define CONFIG_RAM_SIZE         (13 * 1024)
#define CONFIG_RAM_END		(CONFIG_RAM_BASE+CONFIG_RAM_SIZE)

#define CONFIG_TASK_STACK_SIZE	512
#define TASK_SHARE_MEM_SIZE	1024

/* secure share memory last unsigned are used
	* for wakeup communication between BL30/BL301
	* 0x1000D5FC: store irq number
	* 0x1000D7FC: control wakeup enable
	* after BL301 enable wakeup, bl30 store irq no. in share memory
*/
#define WAKEUP_SRC_IRQ_ADDR_BASE		(CONFIG_RAM_END - 128)
#define SECURE_TASK_SHARE_MEM_BASE       CONFIG_RAM_END
#define SECURE_TASK_RESPONSE_MEM_BASE   (CONFIG_RAM_END + 0x200)
#define SECURE_TASK_RESPONSE_WAKEUP_EN  (CONFIG_RAM_END + 0x400 - 4)
#define HIGH_TASK_SHARE_MEM_BASE        (CONFIG_RAM_END + 0x400)
#define HIGH_TASK_RESPONSE_MEM_BASE     (CONFIG_RAM_END + 0x600)
#define LOW_TASK_SHARE_MEM_BASE         (CONFIG_RAM_END + 0x800)
#define LOW_TASK_RESPONSE_MEM_BASE      (CONFIG_RAM_END + 0xA00)
/*
  * BL30/BL301 share memory command list
*/
#define COMMAND_SUSPEND_ENTER			0x1
#define HIGH_TASK_SET_CLOCK	0x2
#define LOW_TASK_GET_DVFS_INFO 0x3
#define HIGH_TASK_GET_DVFS 0x4
#define HIGH_TASK_SET_DVFS 0x5
#define SEC_TASK_GET_WAKEUP_SRC	0x6

#define LOW_TASK_USR_DATA  0x100

	/*bl301 resume to BL30*/
#define RESPONSE_OK					0x0
#define RESPONSE_SUSPEND_LEAVE			0x1

#endif//_CONFIG_H_
