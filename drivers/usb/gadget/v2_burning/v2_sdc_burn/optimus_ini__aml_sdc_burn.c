// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "optimus_sdc_burn_i.h"

#define dbg(fmt ...)  //printf("[INI_SDC]"fmt)
#define msg           DWN_MSG
#define err           DWN_ERR

#define  SET_BURN_PARTS     "burn_parts"
#define  SET_CUSTOM_PARA    "common"
#define  SET_BURN_PARA_EX    "burn_ex"
#define  SET_BURN_DISPLAY    "display"

static const char* _iniSets[] = {
    SET_BURN_PARTS      ,
    SET_CUSTOM_PARA     ,
    SET_BURN_PARA_EX    ,
    SET_BURN_DISPLAY    ,
};

#define TOTAL_SET_NUM   ( sizeof(_iniSets)/sizeof(const char*) )

ConfigPara_t g_sdcBurnPara = {
    .setsBitMap.burnParts   = 0,
    .setsBitMap.custom      = 1,
    .setsBitMap.burnEx      = 1,

    .burnParts      = {
        .burn_num           = 0,
        .bitsMap4BurnParts  = 0,
    },

    .custom         = {
        .eraseBootloader    = 1,//default to erase bootloader! no effect for usb_upgrade
        .eraseFlash         = 1,//default erase flash for all cases
        .bitsMap.eraseBootloader    = 1,
        .bitsMap.eraseFlash         = 1,
    },

    .burnEx         = {
        .bitsMap.pkgPath    = 1,
        .bitsMap.mediaPath  = 0,
    },
};

static int init_config_para(ConfigPara_t* pCfgPara)
{
    memset(pCfgPara, 0, sizeof(ConfigPara_t));

    pCfgPara->setsBitMap.burnParts   = 0;
    pCfgPara->setsBitMap.custom      = 0;
    pCfgPara->setsBitMap.burnEx      = 0;

    pCfgPara->burnParts.burn_num           = 0;
    pCfgPara->burnParts.bitsMap4BurnParts  = 0;

    pCfgPara->custom.eraseBootloader    = 1;//default to erase bootloader!
    pCfgPara->custom.eraseFlash         = 0;
    pCfgPara->custom.bitsMap.eraseBootloader    = 0;
    pCfgPara->custom.bitsMap.eraseFlash         = 0;

    pCfgPara->burnEx.bitsMap.pkgPath    = 0;
    pCfgPara->burnEx.bitsMap.mediaPath  = 0;

    return 0;
}

int print_burn_parts_para(const BurnParts_t* pBurnParts)
{
    int partIndex = 0;

    printf("[%s]\n", SET_BURN_PARTS);
    printf("burn_num         = %d\n", pBurnParts->burn_num);

    for (; partIndex < pBurnParts->burn_num; ++partIndex)
    {
        printf("burn_part%d       = %s\n", partIndex, pBurnParts->burnParts[partIndex]);
    }
    printf("\n");

    return 0;
}

int print_sdc_burn_para(const ConfigPara_t* pCfgPara)
{
    printf("\n=========sdc_burn_paras=====>>>\n");

    {
        const CustomPara_t* pCustom = &pCfgPara->custom;

        printf("[%s]\n", SET_CUSTOM_PARA);
        printf("erase_bootloader = %d\n", pCustom->eraseBootloader);
        printf("erase_flash      = %d\n", pCustom->eraseFlash);
        printf("reboot           = 0x%x\n", pCustom->rebootAfterBurn);
        printf("key_overwrite    = 0x%x\n", pCustom->keyOverwrite);
        printf("\n");
    }

    {
        const BurnEx_t*     pBurnEx = &pCfgPara->burnEx;

        printf("[%s]\n", SET_BURN_PARA_EX);
        printf("package          = %s\n", pBurnEx->pkgPath);
        printf("media            = %s\n", pBurnEx->mediaPath);
        printf("\n");
    }

    print_burn_parts_para(&pCfgPara->burnParts);

    printf("<<<<=====sdc_burn_paras======\n\n");

    return 0;
}

static int parse_set_burnEx(const char* key, const char* strVal)
{
    BurnEx_t* pBurnEx = &g_sdcBurnPara.burnEx;

    if (!strcmp("package", key))
    {
        if (pBurnEx->bitsMap.pkgPath) {
            err("key package in burn_ex is duplicated!\n");
            return __LINE__;
        }
        if (!strVal) {
            err("value for package in set burn_ex can't be empty!\n");
            return __LINE__;
        }

        strncpy(pBurnEx->pkgPath, strVal, sizeof pBurnEx->pkgPath - 1);
        pBurnEx->bitsMap.pkgPath = 1;

        return 0;
    }

    if (!strcmp("media", key))
    {
        if (pBurnEx->bitsMap.mediaPath) {
            err("key media in burn_ex is duplicated!\n");
            return __LINE__;
        }
        if (strVal)
        {
            strncpy(pBurnEx->mediaPath, strVal, sizeof pBurnEx->mediaPath - 1);
            pBurnEx->bitsMap.mediaPath = 1;
        }

        return 0;
    }

    return 0;
}

static int parse_set_display(const char* key, const char* strVal)
{
    BurnDisplay_t* pburnDisplay = &g_sdcBurnPara.display;

    if (!strcmp("outputmode", key))
    {
        if (pburnDisplay->bitsMap4Display & (1U<<0)) {
            err("key outputmode in burn_ex is duplicated!\n");
            return __LINE__;
        }
        if (!strVal) {
            err("value for package in set burn_ex can't be empty!\n");
            return __LINE__;
        }
        setenv(key, strVal);
        DWN_MSG("^^Set %s to (%s) for upgrade^^\n", key, strVal);
        pburnDisplay->bitsMap4Display |= 1U<<0;

        return 0;
    }

    return 0;
}

static int parse_set_custom_para(const char* key, const char* strVal)
{
    CustomPara_t* pCustome = &g_sdcBurnPara.custom;
    const unsigned cfgVal  = strVal ? simple_strtoul(strVal, NULL, 0) : 0;

    if (!strcmp(key, "erase_bootloader"))
    {
        if (pCustome->bitsMap.eraseBootloader) {
            goto _key_dup;
        }

        if (strVal)
        {
            pCustome->eraseBootloader = cfgVal;
            pCustome->bitsMap.eraseBootloader = 1;
        }

    }

    if (!strcmp(key, "erase_flash"))
    {
        if (pCustome->bitsMap.eraseFlash) {
            goto _key_dup;
        }

        if (strVal)
        {
            pCustome->eraseFlash = cfgVal;
            pCustome->bitsMap.eraseFlash = 1;
        }

    }

    if (!strcmp(key, "reboot"))
    {
        if (pCustome->bitsMap.rebootAfterBurn) {
            goto _key_dup;
        }

        if (strVal)
        {
            pCustome->rebootAfterBurn = cfgVal;
            pCustome->bitsMap.rebootAfterBurn = 1;
        }

    }

    if (!strcmp(key, "key_overwrite"))
    {
        if (pCustome->bitsMap.keyOverwrite) {
            goto _key_dup;
        }

        if (strVal)
        {
            pCustome->keyOverwrite = cfgVal;
            pCustome->bitsMap.keyOverwrite = 1;
        }

    }

    if (!strcmp(key, "erase_ddr_para"))
    {
        if (pCustome->bitsMap.eraseDdrPara) {
            goto _key_dup;
        }

        if (strVal)
        {
            pCustome->eraseDdrPara = cfgVal;
            pCustome->bitsMap.eraseDdrPara = 1;
        }
    }

    return 0;

_key_dup:
    err("key %s is duplicated!\n", key);
    return -1;
}

#if 0
int check_custom_para(const CustomPara_t* pCustome)
{
    //TODO: not completed!!
    return 0;
}
#endif

static int parse_burn_parts(const char* key, const char* strVal)
{
    BurnParts_t* pBurnParts = &g_sdcBurnPara.burnParts;

    if ( !strcmp("burn_num", key) )
    {
        if (!strVal) {
            err("burn_num in burn_parts can't be empty!!");
            return __LINE__;
        }

        pBurnParts->burn_num = simple_strtoul(strVal, NULL, 0);
        if (pBurnParts->burn_num < 1) {
            err("value for burn_num in burn_parts in invalid\n");
            return __LINE__;
        }

        return 0;
    }

    if (pBurnParts->burn_num < 1) {
        err("burn_num is not config or 0 ??\n");
        return __LINE__;
    }

    {
        const char burn_partx[] = "burn_partx";
        const int  validKeyLen = sizeof(burn_partx) - 2;
        const int totalBurnNum = pBurnParts->burn_num;
        int burnIndex = 0;
        char* partName = NULL;

        if (strncmp(burn_partx, key, validKeyLen))
        {
            err("error burn part name [%s]\n", key);
            return __LINE__;
        }

        burnIndex = key[validKeyLen] - '0';
        if (!(burnIndex >= 0 && burnIndex < totalBurnNum))
        {
            err("Error \"%s\", only burn_part[0~%d] is valid as burn_num is %d\n",
                key, totalBurnNum - 1, totalBurnNum);
            return __LINE__;
        }

        if (pBurnParts->bitsMap4BurnParts & (1U<<burnIndex)) {
            err("key %s is duplicated in burn_parts\n", key);
            return __LINE__;
        }
        pBurnParts->bitsMap4BurnParts |= 1U<<burnIndex;

        partName = (char*)pBurnParts->burnParts[burnIndex];
        if (!strVal) {
            err("value of %s can't empty\n", key);
            return __LINE__;
        }

        if (!strcmp("bootloader", strVal)) {
            err("bootloader not need to configure at burn_parts\n");
            return __LINE__;
        }

        strncpy(partName, strVal, PART_NAME_LEN_MAX - 1);
    }

    return 0;
}

int check_cfg_burn_parts(const ConfigPara_t* burnPara)
{
    const BurnParts_t* pBurnParts = &burnPara->burnParts;
    const int cfgBurnNum    = pBurnParts->burn_num;
    const unsigned bitsMap  = pBurnParts->bitsMap4BurnParts;
    int mediaPathHasCfg = burnPara->burnEx.bitsMap.mediaPath;
    int i = 0;


    for (i = 0; i < cfgBurnNum; i++)
    {
        int b = bitsMap & (1U<<i);

        if (!b) {
            err("Please cfg burn_part%d\n", i);
            return __LINE__;
        }

        if (mediaPathHasCfg)
        {
            if (!strcmp(pBurnParts->burnParts[i], "media")) {
                DWN_ERR("media can't cfg in both media_path and burn_parts\n");
                return __LINE__;
            }
        }
    }

    return 0;
}

static int optimus_aml_sdc_ini_check_set_valid(const char* setName)
{
        const char* pName = NULL;
        int i = 0;
        int isValid = 0;

        for (; i < TOTAL_SET_NUM && !isValid; ++i)
        {
                pName = _iniSets[i];
                isValid = !strcmp(setName, pName);
        }


        return isValid;
}

static int optimus_aml_sdc_burn_ini_parse_usr_cfg(const char* setName, const char* keyName, const char* usrKeyVal)
{
        int ret = 0;

        if (!strcmp(SET_BURN_PARTS, setName))
        {
                return parse_burn_parts(keyName, usrKeyVal);
        }
        if (!strcmp(SET_CUSTOM_PARA, setName))
        {
                return parse_set_custom_para(keyName, usrKeyVal);
        }
        if (!strcmp(SET_BURN_PARA_EX, setName))
        {
                return parse_set_burnEx(keyName, usrKeyVal);
        }
        if (!strcmp(SET_BURN_DISPLAY, setName))
        {
                return parse_set_display(keyName, usrKeyVal);
        }

        return ret;
}

static int _parse_ini_cfg_file(const char* filePath, HIMAGE hImg)
{
    const int MaxFileSz = OPTIMUS_DOWNLOAD_SLOT_SZ;
    char* CfgFileLoadAddr = (char*)OPTIMUS_DOWNLOAD_TRANSFER_BUF_ADDR;
    int rcode = 0;
    const int MaxLines = 1024;//
    char* lines[MaxLines];
    int   validLineNum = 0;

    init_config_para(&g_sdcBurnPara);

    if (hImg) {
        DWN_MSG("try to fetch para from item aml_sdc_burn.ini\n");
        int itemSz = MaxFileSz;
        rcode =  optimus_img_item2buf(hImg, "ini", "aml_sdc_burn", CfgFileLoadAddr, &itemSz);
        if (ITEM_NOT_EXIST == rcode) {
            DWN_MSG("Item ini not existed, so use hard-coded para\n");
            return ITEM_NOT_EXIST;
        } else if(rcode) {
            DWN_ERR("Err when get item ini, rcode %d\n", rcode);
            return __LINE__;
        } else
            validLineNum = parse_ini_buf_2_valid_lines(CfgFileLoadAddr, itemSz, lines);
    } else
        validLineNum = parse_ini_file_2_valid_lines(filePath, CfgFileLoadAddr, MaxFileSz, lines);
    if (!validLineNum) {
        err("error in parse ini file\n");
        return __LINE__;
    }

    rcode = optimus_ini_trans_lines_2_usr_params((const char* *)lines, validLineNum,
                                                    optimus_aml_sdc_ini_check_set_valid,
                                                    optimus_aml_sdc_burn_ini_parse_usr_cfg);
    if (rcode) {
            err("Fail in get cfg from %s\n", filePath);
            return __LINE__;
    }

    rcode = check_cfg_burn_parts(&g_sdcBurnPara);
    if (rcode) {
        err("Fail in check burn parts.\n");
        return __LINE__;
    }

    print_sdc_burn_para(&g_sdcBurnPara);

    return 0;
}

int parse_ini_cfg_file(const char* filePath)
{
    return _parse_ini_cfg_file(filePath, NULL);
}

int parse_ini_cfg_from_item(HIMAGE hImg)
{
    return _parse_ini_cfg_file(NULL, hImg);
}

#define MYDBG 0
#if MYDBG
int do_ini_parser(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    int rcode = 0;
    const char* filePath = "dos_dc_burn.ini";

    //mmc info to ensure sdcard inserted and inited, mmcinfo outer as there U-disk later
    rcode = run_command("mmcinfo", 0);
    if (rcode) {
        err("Fail in init mmc, Does sdcard not plugged in?\n");
        return __LINE__;
    }

    if (2 <= argc) {
        filePath = argv[1];
    }

    rcode = parse_ini_cfg_file(filePath);
    if (rcode) {
        err("error in parse ini file\n");
        return __LINE__;
    }

    return 0;
}

U_BOOT_CMD(
   ini_parser,      //command name
   5,               //maxargs
   0,               //repeatable
   do_ini_parser,   //command function
   "Burning a partition from sdmmc ",           //description
   "Usage: sdc_update partiton image_file_path fileFmt(android sparse, other normal) [,verify_file]\n"   //usage
);
#endif//#if MYDBG

