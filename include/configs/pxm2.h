/*
 * siemens pxm2
 * (C) Copyright 2013 Siemens Schweiz AG
 * Heiko Schocher, DENX Software Engineering, hs@denx.de.
 *
 * Based on:
 * am335x_evm.h
 *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __CONFIG_PXM2_H
#define __CONFIG_PXM2_H

#define CONFIG_SIEMENS_PXM2
#define MACH_TYPE_PXM2			4309
#define CONFIG_SIEMENS_MACH_TYPE	MACH_TYPE_PXM2

#include "siemens-am33x-common.h"

#define CONFIG_SYS_MPUCLK	720
#define DXR2_IOCTRL_VAL		0x18b
#define DXR2_PLL_FREQ		266

#define BOARD_DFU_BUTTON_GPIO	59
#define BOARD_DFU_BUTTON_LED	117


 /* Physical Memory Map */
#define CONFIG_MAX_RAM_BANK_SIZE	(1024 << 20)	/* 1GB */

/* I2C Configuration */
#define CONFIG_SYS_I2C_SPEED		400000
#define CONFIG_SYS_I2C_EEPROM_ADDR	0x50


#define CONFIG_SYS_U_BOOT_MAX_SIZE_SECTORS	0x300

#undef CONFIG_SPL_NET_SUPPORT
#undef CONFIG_SPL_NET_VCI_STRING
#undef CONFIG_SPL_ETH_SUPPORT

#define CONFIG_PHY_ADDR			0
#define CONFIG_PHY_ATHEROS

#define CONFIG_FACTORYSET

/* UBI Support */
#ifndef CONFIG_SPL_BUILD
#define CONFIG_CMD_MTDPARTS
#define CONFIG_MTD_PARTITIONS
#define CONFIG_MTD_DEVICE
#define CONFIG_RBTREE
#define CONFIG_LZO
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#endif

/* Watchdog */
#define CONFIG_OMAP_WATCHDOG

#ifndef CONFIG_SPL_BUILD

/* Default env settings */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"hostname=pxm2\0" \
	"nand_img_size=0x500000\0" \
	"optargs=\0" \
	CONFIG_COMMON_ENV_SETTINGS \
	"mmc_dev=0\0" \
	"mmc_root=/dev/mmcblk0p2 rw\0" \
	"mmc_root_fs_type=ext4 rootwait\0" \
	"mmc_load_uimage=" \
		"mmc rescan; " \
		"setenv bootfile uImage;" \
		"fatload mmc ${mmc_dev} ${kloadaddr} ${bootfile}\0" \
	"loadbootenv=fatload mmc ${mmc_dev} ${loadaddr} ${bootenv}\0" \
	"importbootenv=echo Importing environment from mmc ...; " \
		"env import -t $loadaddr $filesize\0" \
	"mmc_args=run bootargs_defaults;" \
		"mtdparts default;" \
		"setenv bootargs ${bootargs} " \
		"root=${mmc_root} ${mtdparts}" \
		"rootfstype=${mmc_root_fs_type} ip=${ip_method} " \
		"eth=${ethaddr} " \
		"\0" \
	"mmc_boot=run mmc_args; " \
		"run mmc_load_uimage; " \
		"bootm ${kloadaddr}\0" \
	""

#ifndef CONFIG_RESTORE_FLASH
/* set to negative value for no autoboot */
#define CONFIG_BOOTDELAY		3

#define CONFIG_BOOTCOMMAND \
	"if dfubutton; then " \
		"run dfu_start; " \
		"reset; " \
	"fi; " \
	"if mmc rescan; then " \
		"echo SD/MMC found on device ${mmc_dev};" \
		"if run loadbootenv; then " \
			"echo Loaded environment from ${bootenv};" \
			"run importbootenv;" \
		"fi;" \
		"if test -n $uenvcmd; then " \
			"echo Running uenvcmd ...;" \
			"run uenvcmd;" \
		"fi;" \
		"if run mmc_load_uimage; then " \
			"run mmc_args;" \
			"bootm ${kloadaddr};" \
		"fi;" \
	"fi;" \
	"run nand_boot;" \
	"if ping ${serverip}; then " \
		"run net_nfs; " \
	"fi; "

#else
#define CONFIG_BOOTDELAY		0

#define CONFIG_BOOTCOMMAND			\
	"setenv autoload no; "			\
	"dhcp; "				\
	"if tftp 80000000 debrick.scr; then "	\
		"source 80000000; "		\
	"fi"
#endif
#endif	/* CONFIG_SPL_BUILD */
#endif	/* ! __CONFIG_PXM2_H */
