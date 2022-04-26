// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include "key_manage_i.h"
#include <fdt.h>
#include <linux/libfdt.h>

#define UNIFYKEY_DATAFORMAT_HEXDATA	    "hexdata"
#define UNIFYKEY_DATAFORMAT_HEXASCII	"hexascii"
#define UNIFYKEY_DATAFORMAT_ALLASCII	"allascii"

#define UNIFYKEY_DEVICE_EFUSEKEY	    "efuse"
#define UNIFYKEY_DEVICE_NORMAL		    "normal"
#define UNIFYKEY_DEVICE_SECURESKEY	    "secure"
#define UNIFYKEY_DEVICE_PROVISIONKEY    "provision"

#define UNIFYKEY_PERMIT_READ		"read"
#define UNIFYKEY_PERMIT_WRITE		"write"
#define UNIFYKEY_PERMIT_DEL			"del"

static struct key_info_t unify_key_info={.key_num =0, .key_flag = 0, .efuse_version = -1, .encrypt_type = 0};
static struct key_item_t *unifykey_item=NULL;
static struct key_item_t* _defProvisonItem =NULL;//keyname start with "KEY_PROVISION_" and device is "provison"
static struct key_item_t *_defEfuseItem;//keyname start with "KEY_EFUSE_" and device is "efuse"
#define _PROVSION_DEFAULT_KEY_NAME  "KEY_PROVISION_XXX"
#define _EFUSE_DEFAULT_KEY_NAME		"KEY_EFUSE_XXX"

static int unifykey_item_verify_check(struct key_item_t *key_item)
{
	if (!key_item) {
		KM_ERR("unify key item is invalid\n");
		return -1;
	}

	//if (!key_item->name || (key_item->dev == KEY_M_UNKNOW_DEV) ||(key_item->datFmt == KEY_M_MAX_DF)) {
	if ((key_item->dev == KEY_M_UNKNOW_DEV) ||(key_item->datFmt == KEY_M_MAX_DF)) {
		KM_ERR("unify key item is invalid\n");
		return -1;
	}

	return 0;
}

static struct key_item_t *unifykey_find_item_by_name(const char *name)
{
	struct key_item_t *pre_item;
	int i = 0;
	const unsigned cnt = unify_key_info.key_num;

	for (pre_item = unifykey_item; i < cnt; ++pre_item, ++i) {
		if (!strcmp(pre_item->name, name))
			return pre_item;
	}

	if (!strncmp(_PROVSION_DEFAULT_KEY_NAME, name, strlen(_PROVSION_DEFAULT_KEY_NAME) - 3))
		return _defProvisonItem;
	if (!strncmp(_EFUSE_DEFAULT_KEY_NAME, name, strlen(_EFUSE_DEFAULT_KEY_NAME) - 3))
		return _defEfuseItem;
	KM_ERR("cannot find dev for keyname %s\n", name);
	return NULL;
}

enum key_manager_df_e keymanage_dts_get_key_fmt(const char *keyname)
{
	struct key_item_t *key_manage;
    enum key_manager_df_e keyValFmt = KEY_M_MAX_DF;

    if (!unify_key_info.key_flag) {
        KM_ERR("/unify not parsed yet!\n");
        return KEY_M_MAX_DF;
    }

	key_manage = unifykey_find_item_by_name(keyname);
	if (key_manage == NULL) {
		KM_ERR ("%s key name is not exist\n", keyname) ;
		return keyValFmt;
	}
	if (unifykey_item_verify_check(key_manage)) {
		KM_ERR ("%s key name is invalid\n", keyname) ;
		return keyValFmt;
	}

    keyValFmt = key_manage->datFmt;
	return keyValFmt;
}

//which device does the key stored in
enum key_manager_dev_e keymanage_dts_get_key_device(const char *keyname)
{
	struct key_item_t *key_manage;

    if (!unify_key_info.key_flag) {
        KM_ERR("/unify not parsed yet!\n");
        return KEY_M_MAX_DEV;
    }
	key_manage = unifykey_find_item_by_name(keyname);
	if (key_manage == NULL) {
		KM_ERR("%s key name is not exist\n",keyname);
		return KEY_M_MAX_DEV;
	}
	if (unifykey_item_verify_check(key_manage)) {
		KM_ERR("%s key name is invalid\n",keyname);
		return KEY_M_MAX_DEV;
	}

	return key_manage->dev;
}

const char* keymanage_dts_get_enc_type(const char* keyname)
{
	struct key_item_t *key_manage;

    if (!unify_key_info.key_flag) {
		KM_ERR("/unify not parsed yet!\n");
		return NULL;
	}
	key_manage = unifykey_find_item_by_name(keyname);
	if (key_manage == NULL) {
		KM_ERR("%s key name is not exist\n",keyname);
		return NULL;
	}

	return key_manage->encType;
}

const char* keymanage_dts_get_key_type(const char* keyname)
{
	struct key_item_t *key_manage;

    if (!unify_key_info.key_flag) {
        KM_ERR("/unify not parsed yet!\n");
        return NULL;
    }
	key_manage = unifykey_find_item_by_name(keyname);
	if (key_manage == NULL) {
		KM_ERR("%s key name is not exist\n",keyname);
		return NULL;
	}

	return key_manage->keyType;
}

char unifykey_get_efuse_version(void)
{
	char ver=0;

    if (!unify_key_info.key_flag) {
        KM_ERR("/unify not parsed yet!\n");
        return 0;
    }

	if (unify_key_info.efuse_version != -1) {
		ver = (char)unify_key_info.efuse_version;
	}
	return ver;
}

int unifykey_get_encrypt_type(void)
{
	return unify_key_info.encrypt_type;
}

static int unifykey_item_dt_parse(const void* dt_addr,int nodeoffset,int id,char *item_path)
{
	struct key_item_t *temp_item=NULL;
	char *propdata;
	struct fdt_property *prop;
	int count;

	temp_item = unifykey_item + id;

	propdata = (char*)fdt_getprop(dt_addr, nodeoffset, "key-encrypt", NULL);
	if (propdata) {
		count = strlen(propdata);
        if ( count > KEY_UNIFY_TYPE_LEN_MAX ) {
			KM_ERR("key-encrypt [%s] too long\n", propdata);
			return __LINE__;
		}
		memcpy(temp_item->encType, propdata, count);
	}

	propdata = (char*)fdt_getprop(dt_addr, nodeoffset, "key-name",NULL);
	if (!propdata) {
		printf("%s get key-name fail,%s:%d\n",item_path,__func__,__LINE__);
        return __LINE__;
	}

	count = strlen(propdata);
	if (count >= KEY_UNIFY_NAME_LEN) {
        KM_ERR("key-name strlen (%d) > max(%d) at key_%d\n", count, KEY_UNIFY_NAME_LEN - 1, id);
        return __LINE__;
	}
    memcpy(temp_item->name, propdata, count);
    temp_item->name[count] = 0;

	propdata = (char*)fdt_getprop(dt_addr, nodeoffset, "key-device",NULL);
	if (!propdata) {
		KM_ERR("%s get key-device fail at key_%d\n",item_path, id);
        return __LINE__;
	}

    if (strcmp(propdata,UNIFYKEY_DEVICE_EFUSEKEY) == 0) {
        temp_item->dev = KEY_M_EFUSE_NORMAL;
		if (!strcmp("secure_boot_set", temp_item->name)) {
			_defEfuseItem = temp_item;
		}
    }
    else if(strcmp(propdata,UNIFYKEY_DEVICE_SECURESKEY) == 0){
        temp_item->dev = KEY_M_SECURE_KEY;
    }
    else if(strcmp(propdata,UNIFYKEY_DEVICE_NORMAL) == 0){
        temp_item->dev = KEY_M_NORAML_KEY;
    }
    else{
        KM_ERR("key-device %s is unknown at key_%d\n", propdata, id);
        return __LINE__;
    }

	propdata = (char*)fdt_getprop((const void *)dt_addr, nodeoffset, "key-type",NULL);
	if (!propdata) //prop 'key-type' not configured, default to raw except special names
    {
        strcpy(temp_item->keyType, "raw");
	}
    else
    {
        const int keyTypeLen = strlen(propdata);
        if (keyTypeLen > KEY_UNIFY_TYPE_LEN_MAX) {
            KM_ERR("key[%s]cfg key-type[%s] sz %d > max %d\n", temp_item->name, propdata, keyTypeLen, KEY_UNIFY_TYPE_LEN_MAX);
            return __LINE__;
        }
        strcpy(temp_item->keyType, propdata);
    }

	prop = (struct fdt_property*)fdt_get_property((const void *)dt_addr,nodeoffset,"key-permit",NULL) ;
	if (!prop) {
		KM_ERR("%s get key-permit fail at  key_%d\n",item_path, id);
        return __LINE__;
	}

    temp_item->permit = 0;
    const int propLen = prop->len > 512 ? strnlen(prop->data, 512) : prop->len;
    if (fdt_stringlist_contains(prop->data, propLen, UNIFYKEY_PERMIT_READ)) {
        temp_item->permit |= KEY_M_PERMIT_READ;
    }
    if (fdt_stringlist_contains(prop->data, propLen, UNIFYKEY_PERMIT_WRITE)) {
        temp_item->permit |= KEY_M_PERMIT_WRITE;
    }
    if (fdt_stringlist_contains(prop->data, propLen, UNIFYKEY_PERMIT_DEL)) {
        temp_item->permit |= KEY_M_PERMIT_DEL;
    }

	temp_item->id = id;

    KM_DBG("key[%02d] keyname %s, %d\n", id, temp_item->name, temp_item->dev);

	return 0;
}

static int unifykey_item_create(const void* dt_addr,int num)
{
    int ret = 0;
    int i,nodeoffset;
    char item_path[100];

    for (i=0;i<num;i++)
    {
        sprintf(item_path, "/unifykey/key_%d", i);

        nodeoffset = fdt_path_offset (dt_addr, item_path) ;
        if (nodeoffset < 0) {
            KM_ERR(" dts: not find  node %s.\n",fdt_strerror(nodeoffset));
            return __LINE__;
        }

        ret = unifykey_item_dt_parse(dt_addr,nodeoffset, i, item_path);
        if (ret) {
            KM_ERR("Fail at parse %s\n", item_path);
            return __LINE__;
        }
    }

    //	printf("unifykey-num fact is %x\n",count);
    return 0;
}

//parse and cache the dts cfg
//TODO: check keys names has no duplicated
int keymanage_dts_parse(const void* dt_addr)
{
    int ret = 0;
	int child;
	int nodeoffset, provisionOffset;
	int unifykeyNum = 0, provisionNum = 0;
	char *punifykey_num, *encrypt_type;

	if (fdt_check_header(dt_addr)!= 0) {
        KM_ERR("not a fdt at 0x%p\n", dt_addr);
        return __LINE__;
    }

	nodeoffset = fdt_path_offset(dt_addr, "/unifykey");
	if (nodeoffset < 0) {
		KM_ERR("dts: err(%s) in find /unifykey.\n",fdt_strerror(nodeoffset));
		return __LINE__;
	}

	unify_key_info.efuse_version = -1;
	punifykey_num = (char*)fdt_getprop((const void *)dt_addr, nodeoffset, "efuse-version",NULL);
	if (punifykey_num) {
		unify_key_info.efuse_version = be32_to_cpup((unsigned int*)punifykey_num);
		KM_MSG("efuse-version config is %x\n",unify_key_info.efuse_version);
	}

	unify_key_info.key_num = 0;
	fdt_for_each_subnode(child, dt_addr, nodeoffset) {
		unifykeyNum++;
	}

	provisionOffset = fdt_path_offset(dt_addr, "/provisionkey");
	if (provisionOffset >= 0) {
		fdt_for_each_subnode(child, dt_addr, provisionOffset) {
			provisionNum++;
		}
	}
	unify_key_info.key_num = unifykeyNum + provisionNum;
	KM_MSG("key_num: %d\n", unify_key_info.key_num);

	unify_key_info.encrypt_type = -1;
	encrypt_type = (char*)fdt_getprop((const void *)dt_addr, nodeoffset, "unifykey-encrypt",NULL);
	if (encrypt_type) {
		unify_key_info.encrypt_type = be32_to_cpup((unsigned int*)encrypt_type);
	}

	if (unify_key_info.key_num <= 0) {
		KM_ERR("unifykey-num is not configured\n");
        return __LINE__;
	}
    if (unify_key_info.key_num > 256) {
        KM_ERR("Cfg key_num is %d > 32,pls check!\n", unify_key_info.key_num);
        return __LINE__;
    }

    if (unifykey_item) {
        free(unifykey_item);
    }
    const unsigned keyInfBufLen = unify_key_info.key_num * sizeof(struct key_item_t);
    unifykey_item = (struct key_item_t*)malloc(keyInfBufLen);
    memset(unifykey_item, 0 , keyInfBufLen);

    ret = unifykey_item_create(dt_addr,unifykeyNum);
    unify_key_info.key_flag = ret ? 0 : 1;

    if (provisionOffset >= 0)
    {
        KM_DBG("dts: in find /provisionkey.\n");

        int defPermits = 0;
        const struct fdt_property *prop = fdt_get_property(dt_addr, provisionOffset,"key-permit-default",NULL) ;
        if (prop) {
            const int propLen = prop->len > 512 ? strnlen(prop->data, 512) : prop->len;
            if (fdt_stringlist_contains(prop->data, propLen, UNIFYKEY_PERMIT_READ)) {
                defPermits |= KEY_M_PERMIT_READ;
            }
            if (fdt_stringlist_contains(prop->data, propLen, UNIFYKEY_PERMIT_WRITE)) {
                defPermits |= KEY_M_PERMIT_WRITE;
            }
            if (fdt_stringlist_contains(prop->data, propLen, UNIFYKEY_PERMIT_DEL)) {
                defPermits |= KEY_M_PERMIT_DEL;
            }
        }

        int node = 0;
        int id = unifykeyNum;
        int szlen = 0;
        fdt_for_each_subnode(node, dt_addr, provisionOffset) {
            int len = 0;
            const char* keyName = fdt_get_name(dt_addr, node, &len);
            KM_DBG("provisionkey[%s] len %d\n", keyName, len);

            struct key_item_t *pItem= unifykey_item + id;

            szlen = strnlen(keyName, KEY_UNIFY_NAME_LEN - 1);
            memcpy(pItem->name, keyName, szlen);
            if (szlen < KEY_UNIFY_NAME_LEN) pItem->name[szlen] = '\0';

            strcpy(pItem->keyType, "raw");
            pItem->dev = KEY_M_PROVISION_KEY;
            pItem->permit = defPermits;
            pItem->id      = id++;
            if (!strcmp(_PROVSION_DEFAULT_KEY_NAME, keyName)) _defProvisonItem = pItem;
        }

        if ((node < 0) && (node != -FDT_ERR_NOTFOUND)) {
            KM_ERR("in parse /provisionkey, err(%s)\n", fdt_strerror(node));
            return __LINE__;
        }
    }

	return ret;
}

