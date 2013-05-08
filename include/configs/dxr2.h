/*
 * siemens dxr2
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

#ifndef __CONFIG_DXR2_H
#define __CONFIG_DXR2_H

#define CONFIG_SIEMENS_DXR2
#define MACH_TYPE_DXR2			4315
#define CONFIG_SIEMENS_MACH_TYPE	MACH_TYPE_DXR2

#include "siemens-am33x-common.h"

#define CONFIG_SYS_MPUCLK	275
#define DXR2_IOCTRL_VAL	0x18b
#define DXR2_PLL_FREQ	303
#define CONFIG_SPL_AM33XX_DO_NOT_ENABLE_RTC32K

#define BOARD_DFU_BUTTON_GPIO	27
#define BOARD_DFU_BUTTON_LED	64

#undef CONFIG_DOS_PARTITION
#undef CONFIG_CMD_FAT


 /* Physical Memory Map */
#define CONFIG_MAX_RAM_BANK_SIZE	(1024 << 20)	/* 1GB */

/* I2C Configuration */
#define CONFIG_SYS_I2C_SPEED		100000

#define CONFIG_SYS_I2C_EEPROM_ADDR              0x50
#define EEPROM_ADDR_DDR3 0x90
#define EEPROM_ADDR_CHIP 0x120

#define CONFIG_SYS_U_BOOT_MAX_SIZE_SECTORS	0x300

#undef CONFIG_SPL_NET_SUPPORT
#undef CONFIG_SPL_NET_VCI_STRING
#undef CONFIG_SPL_ETH_SUPPORT

#undef CONFIG_MII
#undef CONFIG_PHY_GIGE
#define CONFIG_PHY_ADDR			1
#define CONFIG_PHY_SMSC

#define CONFIG_FACTORYSET

/* Watchdog */
#define CONFIG_OMAP_WATCHDOG

#ifndef CONFIG_SPL_BUILD

/* Default env settings */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"hostname=dxr2\0" \
	"nand_img_size=0x300000\0" \
	"optargs=\0" \
	CONFIG_COMMON_ENV_SETTINGS

#ifndef CONFIG_RESTORE_FLASH
/* set to negative value for no autoboot */
#define CONFIG_BOOTDELAY		3

#define CONFIG_BOOTCOMMAND \
"if dfubutton; then " \
	"run dfu_start; " \
	"reset; " \
"fi;" \
"if ping ${serverip}; then " \
	"run net_nfs; " \
"fi;" \
"run nand_boot;"

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
#endif	/* ! __CONFIG_DXR2_H */
