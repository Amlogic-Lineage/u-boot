// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

/* System Headers */
#include <common.h>
#include <asm/arch/io.h>
#include <asm/arch/secure_apb.h>


/* Amlogic Headers */
#include <amlogic/canvas.h>

#define CANVAS_DEBUG_ENABLE
#ifdef CANVAS_DEBUG_ENABLE
#define canvas_log(fmt, args...) \
	do { \
		printf("[CANVAS]"fmt"\n", ##args); \
	} while (0)
#else
#define canvas_log(fmt, args...)
#endif

#ifndef DMC_REG_BASE
#define DMC_REG_BASE                    (0xFF638000L)
#endif
#define REG_CANVAS_ADDR(reg)               (reg + 0L)

#define CANVAS_NUM 256
static canvas_t canvasPool[CANVAS_NUM];
static int canvas_inited = 0;

static inline u32 canvas_reg_read(u32 reg)
{
	u32 val;

	if (reg > 0x10000)
		val = *(volatile unsigned int *)REG_CANVAS_ADDR(reg);
	else
		val = readl(DMC_REG_BASE + reg);
	return val;
}

static inline void canvas_reg_write(const u32 val, u32 reg)
{
	if (reg > 0x10000)
		*(volatile unsigned int *)REG_CANVAS_ADDR(reg) = (val);
	else
		writel(val, (DMC_REG_BASE + reg));
}

void canvas_init(void)
{
	int index = 0;

	if (canvas_inited == 1)
		return;

	canvas_log("canvas init");
	canvas_reg_write(0, DC_CAV_LUT_DATAL);
	canvas_reg_write(0, DC_CAV_LUT_DATAH);
	for (index = 0; index < CANVAS_NUM; index++) {
		canvas_reg_write(CANVAS_LUT_WR_EN | index, DC_CAV_LUT_ADDR);
		canvas_reg_read(DC_CAV_LUT_DATAH);
	}

	canvas_inited = 1;
}

void canvas_config(u32 index, ulong addr, u32 width,
		   u32 height, u32 wrap, u32 blkmode)
{
	canvas_t *canvasP = &canvasPool[index];

	if (index >= CANVAS_NUM)
		return;

	canvas_init();

	canvas_log("addr=0x%08lx width=%d, height=%d", addr, width, height);
	canvas_reg_write((((addr + 7) >> 3) & CANVAS_ADDR_LMASK) |
			 ((((width + 7) >> 3) & CANVAS_WIDTH_LMASK) << CANVAS_WIDTH_LBIT),
			 DC_CAV_LUT_DATAL);
	canvas_reg_write(((((width + 7) >> 3) >> CANVAS_WIDTH_LWID) <<
			  CANVAS_WIDTH_HBIT) |
			 ((height & CANVAS_HEIGHT_MASK) << CANVAS_HEIGHT_BIT)	|
			 ((wrap & CANVAS_XWRAP) ? CANVAS_XWRAP : 0)              |
			 ((wrap & CANVAS_YWRAP) ? CANVAS_YWRAP : 0)              |
			 ((blkmode & CANVAS_BLKMODE_MASK) << CANVAS_BLKMODE_BIT),
			 DC_CAV_LUT_DATAH);
	canvas_reg_write(CANVAS_LUT_WR_EN | index, DC_CAV_LUT_ADDR);
	// read a cbus to make sure last write finish.
	canvas_reg_read(DC_CAV_LUT_DATAH);

	canvasP->addr = addr;
	canvasP->width = width;
	canvasP->height = height;
	canvasP->wrap = wrap;
	canvasP->blkmode = blkmode;

}

void canvas_read(u32 index, canvas_t *p)
{
	if (index < CANVAS_NUM)
		*p = canvasPool[index];
}

void canvas_copy(u32 src, u32 dst)
{
	unsigned long addr;
	unsigned width, height, wrap, blkmode;

	if ((src >= CANVAS_NUM) || (dst >= CANVAS_NUM))
		return;

	addr = canvasPool[src].addr;
	width = canvasPool[src].width;
	height = canvasPool[src].height;
	wrap = canvasPool[src].wrap;
	blkmode = canvasPool[src].blkmode;

	canvas_reg_write((((addr + 7) >> 3) & CANVAS_ADDR_LMASK) |
			 ((((width + 7) >> 3) & CANVAS_WIDTH_LMASK) << CANVAS_WIDTH_LBIT),
			 DC_CAV_LUT_DATAL);
	canvas_reg_write(((((width + 7) >> 3) >> CANVAS_WIDTH_LWID) <<
			  CANVAS_WIDTH_HBIT) |
			 ((height & CANVAS_HEIGHT_MASK) << CANVAS_HEIGHT_BIT)    |
			 ((wrap & CANVAS_XWRAP) ? CANVAS_XWRAP : 0)              |
			 ((wrap & CANVAS_YWRAP) ? CANVAS_YWRAP : 0)              |
			 ((blkmode & CANVAS_BLKMODE_MASK) << CANVAS_BLKMODE_BIT),
			 DC_CAV_LUT_DATAH);
	canvas_reg_write(CANVAS_LUT_WR_EN | dst, DC_CAV_LUT_ADDR);
	// read a cbus to make sure last write finish.
	canvas_reg_read(DC_CAV_LUT_DATAH);

	canvasPool[dst].addr = addr;
	canvasPool[dst].width = width;
	canvasPool[dst].height = height;
	canvasPool[dst].wrap = wrap;
	canvasPool[dst].blkmode = blkmode;

	return;
}

void canvas_update_addr(u32 index, u32 addr)
{
	if (index >= CANVAS_NUM)
		return;

	canvasPool[index].addr = addr;

	canvas_reg_write((((canvasPool[index].addr + 7) >> 3) & CANVAS_ADDR_LMASK) |
			 ((((canvasPool[index].width + 7) >> 3) & CANVAS_WIDTH_LMASK) <<
			  CANVAS_WIDTH_LBIT), DC_CAV_LUT_DATAL);
	canvas_reg_write(((((canvasPool[index].width + 7) >> 3) >> CANVAS_WIDTH_LWID) <<
			  CANVAS_WIDTH_HBIT) |
			 ((canvasPool[index].height & CANVAS_HEIGHT_MASK) << CANVAS_HEIGHT_BIT)   |
			 ((canvasPool[index].wrap & CANVAS_XWRAP) ? CANVAS_XWRAP : 0)             |
			 ((canvasPool[index].wrap & CANVAS_YWRAP) ? CANVAS_YWRAP : 0)             |
			 ((canvasPool[index].blkmode & CANVAS_BLKMODE_MASK) << CANVAS_BLKMODE_BIT),
			 DC_CAV_LUT_DATAH);
	canvas_reg_write(CANVAS_LUT_WR_EN | index, DC_CAV_LUT_ADDR);
	// read a cbus to make sure last write finish.
	canvas_reg_read(DC_CAV_LUT_DATAH);

	return;
}

unsigned int canvas_get_addr(u32 index)
{
	return canvasPool[index].addr;
}
