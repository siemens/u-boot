/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration header file for IOT2050
 * Copyright (c) Siemens AG, 2018-2020
 *
 * Authors:
 *   Le Jin <le.jin@siemens.com>
 *   Jan Kiszka <jan.kiszk@siemens.com>
 */

#ifndef __CONFIG_IOT2050_H
#define __CONFIG_IOT2050_H

#include <linux/sizes.h>

/* SPL Loader Configuration */
#define CONFIG_SYS_INIT_SP_ADDR		(CONFIG_SPL_TEXT_BASE + \
					 CONFIG_SYS_K3_NON_SECURE_MSRAM_SIZE)

#define CONFIG_SKIP_LOWLEVEL_INIT

#define CONFIG_SPL_MAX_SIZE		CONFIG_SYS_K3_MAX_DOWNLODABLE_IMAGE_SIZE

#define CONFIG_SYS_BOOTM_LEN		SZ_64M

/* U-Boot general configuration */
#define EXTRA_ENV_IOT2050_BOARD_SETTINGS				\
	"loadaddr=0x80080000\0"						\
	"pxefile_addr_r=0x80080000\0"					\
	"scriptaddr=0x83000000\0"					\
	"kernel_addr_r=0x80080000\0"					\
	"ramdisk_addr_r=0x81000000\0"					\
	"fdt_addr_r=0x82000000\0"					\
	"overlay_addr_r=0x83000000\0"					\
	"load_icssg0_pru1_fw=sf read $loadaddr 0x7d0000 0x8000; rproc load 4 $loadaddr 0x8000\0" \
	"load_icssg0_rtu1_fw=sf read $loadaddr 0x7f0000 0x8000; rproc load 5 $loadaddr 0x8000\0" \
	"init_icssg0=rproc init; sf probe; run load_icssg0_pru1_fw; run load_icssg0_rtu1_fw\0" \
	"usb_pgood_delay=900\0"

#ifndef CONFIG_SPL_BUILD

#define BOOTENV_RUN_NET_PLATFORM_START	"run init_icssg0; "

/*
 * This defines all MMC devices, even if the basic variant has no mmc1.
 * The non-supported device will be removed from the boot targets during
 * runtime, when that board was detected.
 */
#define BOOT_TARGET_DEVICES(func) \
	func(MMC, mmc, 1) \
	func(MMC, mmc, 0) \
	func(USB, usb, 0) \
	func(USB, usb, 1) \
	func(USB, usb, 2) \
	func(PXE, pxe, na) \
	func(DHCP, dhcp, na)

#include <config_distro_bootcmd.h>

#endif

#define CONFIG_EXTRA_ENV_SETTINGS					\
	BOOTENV								\
	EXTRA_ENV_IOT2050_BOARD_SETTINGS

#include <configs/ti_armv7_common.h>

#endif /* __CONFIG_IOT2050_H */
