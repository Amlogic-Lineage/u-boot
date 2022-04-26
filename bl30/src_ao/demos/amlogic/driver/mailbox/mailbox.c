
/*
 *  Copyright (C) 2014-2018 Amlogic, Inc. All rights reserved.
 *
 *  All information contained herein is Amlogic confidential.
 *
 */

/*Mailbox driver*/
#include <stdint.h>
#include <stdlib.h>
#include <util.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include "myprintf.h"
#include <unistd.h>
#include "n200_eclic.h"
#include "n200_func.h"
#include "uart.h"
#include "common.h"
#include "riscv_encoding.h"

#include "mailbox.h"
#include "mailbox-irq.h"
#include "mailbox-in.h"
#include "mailbox-htbl.h"
#include "mailbox-api.h"

#define MBTAG "AOCPU"
#define PRINT_DBG(...)  //printf(__VA_ARGS__)
#define PRINT_ERR(...)  printf(__VA_ARGS__)
#define PRINT(...)	printf(__VA_ARGS__)

#define MHU_MB_STK_SIZE		2048
#define MB_DATA_SHR_SIZE	240

#undef TASK_PRIORITY
#define TASK_PRIORITY		0x2

#define AO_MBOX_ONLY_SYNC	1

void *g_tbl_ao;

TaskHandle_t ReembHandler;
TaskHandle_t TeembHandler;
static uint32_t ulReeSyncTaskWake;
static uint32_t ulTeeSyncTaskWake;
mbPackInfo syncReeMbInfo;
mbPackInfo syncTeeMbInfo;

extern xHandlerTableEntry xMbHandlerTable[IRQ_MAX];
extern void vRpcUserCmdInit(void);

static void vEnterCritical(void)
{
        taskENTER_CRITICAL();
}

static void vExitCritical(void)
{
        taskEXIT_CRITICAL();
}

static void vMbHandleIsr(void)
{
	uint64_t val = 0;
	uint64_t ulPreVal = 0;
	uint64_t ulIrqMask = IRQ_MASK;
	int i = 0;

	val = xGetMbIrqStats();
	PRINT_DBG("[%s]: mb isr: 0x%x\n", MBTAG, val);
	val &= ulIrqMask;
	while (val) {
		for (i = 0; i <= IRQ_MAX; i++) {
			if (val & (1 << i)) {
				if (xMbHandlerTable[i].vHandler != NULL) {
					xMbHandlerTable[i].vHandler(xMbHandlerTable[i].vArg);
				}
			}
		}
		ulPreVal = val;
		val = xGetMbIrqStats();
		val &= ulIrqMask;
		val = (val | ulPreVal) ^ ulPreVal;
		PRINT_DBG("[%s]: mb isr: 0x%x\n", MBTAG, val);
	}
}
//DECLARE_IRQ(IRQ_NUM_MB_4, vMbHandleIsr)

/*Ree 2 AOCPU mailbox*/
static void vAoRevMbHandler(void *vArg)
{
	//BaseType_t xYieldRequired = pdFALSE;
	uint32_t mbox = (uint32_t)vArg;
	mbPackInfo mbInfo;
	MbStat_t st;
	uint32_t addr, ulMbCmd, ulSize, ulSync;

	vDisableMbInterrupt(IRQ_REV_BIT(mbox));
	st = xGetMboxStats(MAILBOX_STAT(mbox));
	addr = xRevAddrMbox(mbox);
	ulMbCmd = st.cmd;
	ulSize = st.size;
	ulSync = st.sync;

	PRINT_DBG("[%s]: prvRevMbHandler 0x%x, 0x%x, 0x%x\n", MBTAG, ulMbCmd, ulSize, ulSync);

	if (ulMbCmd == 0) {
		PRINT_DBG("[%s] mbox cmd is 0, cannot match\n");
		vClrMboxStats(MAILBOX_CLR(mbox));
		vClrMbInterrupt(IRQ_REV_BIT(mbox));
		vEnableMbInterrupt(IRQ_REV_BIT(mbox));
		return;
	}

	if (ulSize != 0)
		vGetPayload(addr, &mbInfo.mbdata, ulSize);

	PRINT_DBG("%s taskid: 0x%llx\n", MBTAG, mbInfo.mbdata.taskid);
	PRINT_DBG("%s complete: 0x%llx\n", MBTAG, mbInfo.mbdata.complete);
	PRINT_DBG("%s ullclt: 0x%llx\n", MBTAG, mbInfo.mbdata.ullclt);

	switch (ulSync) {
	case MB_SYNC:
		if (ulReeSyncTaskWake && (MAILBOX_ARMREE2AO == xGetChan(mbox))) {
			PRINT("ulReeSyncTaskWake Busy\n");
			break;
		}
		if (ulTeeSyncTaskWake && (MAILBOX_ARMTEE2AO == xGetChan(mbox))) {
			PRINT("ulTeeSyncTaskWake Busy\n");
			break;
		}
		PRINT_DBG("[%s]: SYNC\n", MBTAG);
		mbInfo.ulCmd = ulMbCmd;
		mbInfo.ulSize = ulSize;
		mbInfo.ulChan = xGetChan(mbox);
		if (MAILBOX_ARMREE2AO == xGetChan(mbox)) {
			syncReeMbInfo = mbInfo;
			ulReeSyncTaskWake = 1;
			vTaskNotifyGiveFromISR(ReembHandler, NULL);
		}
		if (MAILBOX_ARMTEE2AO == xGetChan(mbox)) {
			syncTeeMbInfo = mbInfo;
			ulTeeSyncTaskWake = 1;
			vTaskNotifyGiveFromISR(TeembHandler, NULL);
		}
		//portYIELD_FROM_ISR(xYieldRequired);
		break;
	case MB_ASYNC:
#ifdef AO_MBOX_ONLY_SYNC
		PRINT_DBG("[%s]: ASYNC no support\n", MBTAG);
		vClrMboxStats(MAILBOX_CLR(mbox));
		vClrMbInterrupt(IRQ_REV_BIT(mbox));
		vEnableMbInterrupt(IRQ_REV_BIT(mbox));
#endif
		break;
	default:
		PRINT_ERR("[%s]: Not SYNC or ASYNC, Fail\n", MBTAG);
		vClrMboxStats(MAILBOX_CLR(mbox));
		vClrMbInterrupt(IRQ_REV_BIT(mbox));
		vEnableMbInterrupt(IRQ_REV_BIT(mbox));
		break;
	}
}

static void vReeSyncTask(void *pvParameters)
{
	uint32_t addr = 0;
	uint32_t mbox = 0;
	int index = 0;

	pvParameters = pvParameters;
	while (1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		PRINT_DBG("[%s]:ReeSyncTask\n", MBTAG);

		index = mailbox_htbl_invokeCmd(g_tbl_ao, syncReeMbInfo.ulCmd,
					       syncReeMbInfo.mbdata.data);
		mbox = xGetRevMbox(syncReeMbInfo.ulChan);
		PRINT_DBG("[%s]:ReeSyncTask mbox:%d\n", MBTAG, mbox);
		addr = xSendAddrMbox(mbox);
		if (index != 0) {
			if (index == MAX_ENTRY_NUM) {
				memset(&syncReeMbInfo.mbdata.data, 0, sizeof(syncReeMbInfo.mbdata.data));
				syncReeMbInfo.mbdata.status = ACK_FAIL;
				vBuildPayload(addr, &syncReeMbInfo.mbdata, sizeof(syncReeMbInfo.mbdata));
				PRINT_DBG("[%s]: undefine cmd or no callback\n", MBTAG);
			} else {
				PRINT_DBG("[%s]:SyncTask re len:%d\n", MBTAG, sizeof(syncReeMbInfo.mbdata));
				syncReeMbInfo.mbdata.status = ACK_OK;
				vBuildPayload(addr, &syncReeMbInfo.mbdata, sizeof(syncReeMbInfo.mbdata));
			}
		}

		vEnterCritical();
		PRINT_DBG("[%s]:Ree Sync clear mbox:%d\n", MBTAG, mbox);
		ulReeSyncTaskWake = 0;
		vClrMboxStats(MAILBOX_CLR(mbox));
		vClrMbInterrupt(IRQ_REV_BIT(mbox));
		vEnableMbInterrupt(IRQ_REV_BIT(mbox));
		vExitCritical();
	}
}

static void vTeeSyncTask(void *pvParameters)
{
	uint32_t addr = 0;
	uint32_t mbox = 0;
	int index = 0;

	pvParameters = pvParameters;
	while (1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		PRINT_DBG("[%s]:TeeSyncTask\n", MBTAG);

		index = mailbox_htbl_invokeCmd(g_tbl_ao, syncTeeMbInfo.ulCmd,
					       syncTeeMbInfo.mbdata.data);
		mbox = xGetRevMbox(syncTeeMbInfo.ulChan);
		PRINT_DBG("[%s]:TeeSyncTask mbox:%d\n", MBTAG, mbox);
		addr = xSendAddrMbox(mbox);
		if (index != 0) {
			if (index == MAX_ENTRY_NUM) {
				memset(&syncTeeMbInfo.mbdata.data, 0, sizeof(syncTeeMbInfo.mbdata.data));
				syncTeeMbInfo.mbdata.status = ACK_FAIL;
				vBuildPayload(addr, &syncTeeMbInfo.mbdata, sizeof(syncTeeMbInfo.mbdata));
				PRINT_DBG("[%s]: undefine cmd or no callback\n", MBTAG);
			} else {
				PRINT_DBG("[%s]:SyncTask re len:%d\n", MBTAG, sizeof(syncTeeMbInfo.mbdata));
				syncTeeMbInfo.mbdata.status = ACK_OK;
				vBuildPayload(addr, &syncTeeMbInfo.mbdata, sizeof(syncTeeMbInfo.mbdata));
			}
		}

		vEnterCritical();
		PRINT_DBG("[%s]:Tee Sync clear mbox:%d\n", MBTAG, mbox);
		ulTeeSyncTaskWake = 0;
		vClrMboxStats(MAILBOX_CLR(mbox));
		vClrMbInterrupt(IRQ_REV_BIT(mbox));
		vEnableMbInterrupt(IRQ_REV_BIT(mbox));
		vExitCritical();
	}
}

void vMbInit(void)
{
	PRINT("[%s]: mailbox init start\n", MBTAG);
	mailbox_htbl_init(&g_tbl_ao);

	/* Set MBOX IRQ Handler and Priority */
	vSetMbIrqHandler(IRQ_REV_NUM(MAILBOX_ARMREE2AO), vAoRevMbHandler, (void *)MAILBOX_ARMREE2AO, 10);

	vSetMbIrqHandler(IRQ_REV_NUM(MAILBOX_ARMTEE2AO), vAoRevMbHandler, (void *)MAILBOX_ARMTEE2AO, 10);

	//vEnableIrq(IRQ_NUM_MB_4, MAILBOX_AOCPU_IRQ);
	RegisterIrq(MAILBOX_AOCPU_IRQ, 1, vMbHandleIsr);
	//printf("%s: TODO: please use new vEnableIiq function.\n", __func__);
	EnableIrq(MAILBOX_AOCPU_IRQ);

	xTaskCreate(vReeSyncTask,
		    "AOReeSyncTask",
		    configMINIMAL_STACK_SIZE,
		    0,
		    TASK_PRIORITY,
		    (TaskHandle_t *)&ReembHandler);
	xTaskCreate(vTeeSyncTask,
		    "AOTeeSyncTask",
		    configMINIMAL_STACK_SIZE,
		    0,
		    TASK_PRIORITY,
		    (TaskHandle_t *)&TeembHandler);

	vRpcUserCmdInit();
	PRINT("[%s]: mailbox init end\n", MBTAG);
}

BaseType_t xInstallRemoteMessageCallbackFeedBack(uint32_t ulChan, uint32_t cmd,
						 void *(*handler) (void *),
						 uint8_t needFdBak)
{
	VALID_CHANNEL(ulChan);
	UNUSED(ulChan);
	return mailbox_htbl_reg_feedback(g_tbl_ao, cmd, handler, needFdBak);
}

BaseType_t xUninstallRemoteMessageCallback(uint32_t ulChan, int32_t cmd)
{
	UNUSED(ulChan);
	return mailbox_htbl_unreg(g_tbl_ao, cmd);
}
