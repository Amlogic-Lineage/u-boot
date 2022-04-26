#!/bin/bash

set -e
# set -x

#
# Variables
#

EXEC_BASEDIR=$(dirname $(readlink -f $0))
ACPU_IMAGETOOL=${EXEC_BASEDIR}/../binary-tool/acpu-imagetool

BASEDIR_TOP=$(readlink -f ${EXEC_BASEDIR}/..)

#
# Settings
#

BASEDIR_PAYLOAD=$1

BASEDIR_NONCE="./nonce"

CHIPSET_NAME=$3
KEY_TYPE=$4
SOC=$5
CHIPSET_VARIANT_SUFFIX=$6

BASEDIR_AESKEY_PROT_BL2="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl2/aes/${CHIPSET_NAME}"
BASEDIR_RSAKEY_LVLX_BL2="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl2/rsa/${CHIPSET_NAME}"

BASEDIR_AESKEY_PROT_BL31="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl31/aes/${CHIPSET_NAME}"
BASEDIR_RSAKEY_LVLX_BL31="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl31/rsa/${CHIPSET_NAME}"

BASEDIR_AESKEY_PROT_BL32="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl32/aes/${CHIPSET_NAME}"
BASEDIR_RSAKEY_LVLX_BL32="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl32/rsa/${CHIPSET_NAME}"

BASEDIR_AESKEY_PROT_BL40="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl40/aes/${CHIPSET_NAME}"
BASEDIR_RSAKEY_LVLX_BL40="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/bl40/rsa/${CHIPSET_NAME}"

BASEDIR_TEMPLATE="${BASEDIR_TOP}/keys/${KEY_TYPE}/${SOC}/chipset/cert-template/${CHIPSET_NAME}"

BASEDIR_OUTPUT_BLOB=$2
postfix=.signed
#
# Arguments
#
#stage 2
BB1ST_ARGS="${BB1ST_ARGS}"

### Input: template ###

BB1ST_ARGS="${BB1ST_ARGS} --infile-template-bb1st=${BASEDIR_PAYLOAD}/bb1st${FEAT_BL2_TEMPLATE_TYPE}${CHIPSET_VARIANT_SUFFIX}.bin.bl2-only"

### Input: payloads ###
BB1ST_ARGS="${BB1ST_ARGS} --infile-csinit-params=${BASEDIR_PAYLOAD}/csinit-params.bin"
BB1ST_ARGS="${BB1ST_ARGS} --infile-ddr-fwdata=${BASEDIR_PAYLOAD}/ddr-fwdata.bin"

### Input: Chipset Level-1/2 Private RSA keys

BB1ST_ARGS="${BB1ST_ARGS} --infile-signkey-chipset-lvl1=${BASEDIR_RSAKEY_LVLX_BL2}/level-1-rsa-priv.pem"
BB1ST_ARGS="${BB1ST_ARGS} --infile-signkey-chipset-lvl2=${BASEDIR_RSAKEY_LVLX_BL2}/level-2-rsa-priv.pem"

### Input: pre-generated ProtKey for payloads
BB1ST_ARGS="${BB1ST_ARGS} --infile-aes256-csinit-params=${BASEDIR_AESKEY_PROT_BL2}/genkey-prot-csinit-params.bin"
BB1ST_ARGS="${BB1ST_ARGS} --infile-aes256-ddr-fwdata=${BASEDIR_AESKEY_PROT_BL2}/genkey-prot-ddr-fwdata.bin"

### Features, flags and switches ###
BB1ST_ARGS="${BB1ST_ARGS} --switch-chipset-sign-bl2=0"

### Output: blobs ###
BB1ST_ARGS="${BB1ST_ARGS} --outfile-bb1st=${BASEDIR_OUTPUT_BLOB}/bb1st${FEAT_BL2_TEMPLATE_TYPE}${CHIPSET_VARIANT_SUFFIX}.bin${postfix}"

#
# Main
#

BB1ST_ARGS="${BB1ST_ARGS} --flag-enable-ddrfw-fip"
set -x

${ACPU_IMAGETOOL} \
        create-boot-blobs \
        ${BB1ST_ARGS}

# vim: set tabstop=2 expandtab shiftwidth=2:
