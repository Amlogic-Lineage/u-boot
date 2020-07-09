
declare UBOOT_FOLDER="bl33"
declare MAIN_FOLDER=""
export UBOOT_FOLDER MAIN_FOLDER

declare UBOOT_VERSION1=""
declare UBOOT_VERSION2="v2019"
export UBOOT_VERSION1 UBOOT_VERSION2

# build environment
BUILD_FOLDER="build/"
FIP_FOLDER="fip/"
MANIFEST=".repo/manifest.xml"
FIP_BUILD_FOLDER="fip/_tmp/"
declare -a BLX_NAME_GLB=("bl2" "bl30" "bl31" "bl32")

# include uboot pre-build defines
#BL33_BUILD_FOLDER=("bl33/build/")
#SOURCE_FILE=("${BL33_BUILD_FOLDER}.config")
#CONFIG_FILE=("${BL33_BUILD_FOLDER}include/autoconf.mk")

# variables
declare BOARD_DIR=""
declare CUR_SOC=""

declare DEFCFG_FOLDER="board/amlogic/defconfigs"

# for make clean
declare BL33_PATH1=""
declare BL33_PATH2="bl33/v2019"

declare BL33_TOOLCHAIN1="/opt/toolchains/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin/aarch64-none-elf-"
declare BL33_TOOLCHAIN2="${BL33_TOOLCHAIN1}"

declare BL33_DEFCFG1="board/khadas/defconfigs"
declare BL33_DEFCFG2="board/amlogic/defconfigs"

declare FIP_DDR_PARSE="${FIP_FOLDER}/tools/ddr_parse/"
# current branch/path/rev/name/remote in xml
declare -a GIT_INFO=("branch", "path", "rev", "name", "remote")

function export_variables() {
	export BUILD_FOLDER
	export FIP_FOLDER
	export MANIFEST
	export CUR_SOC
	export BOARD_DIR
}
