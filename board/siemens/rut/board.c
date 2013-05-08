/*
 *
 * Board functions for TI AM335X based rut board
 * (C) Copyright 2013 Siemens Schweiz AG
 * Heiko Schocher, DENX Software Engineering, hs@denx.de.
 *
 * Based on:
 *
 * Copyright (C) 2011, Texas Instruments, Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR /PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <errno.h>
#include <spl.h>
#include <asm/arch/cpu.h>
#include <asm/arch/hardware.h>
#include <asm/arch/omap.h>
#include <asm/arch/ddr_defs.h>
#include <asm/arch/clock.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mmc_host_def.h>
#include <asm/arch/sys_proto.h>
#include <asm/io.h>
#include <asm/emif.h>
#include <asm/gpio.h>
#include <i2c.h>
#include <miiphy.h>
#include <cpsw.h>
#include <watchdog.h>
#include "board.h"
#include "../common/factoryset.h"

DECLARE_GLOBAL_DATA_PTR;

/*
 * Read header information from EEPROM into global structure.
 */
static int read_eeprom(void)
{
	return 0;
}

#ifdef CONFIG_SPL_BUILD
static void board_init_ddr(void)
{
struct emif_regs rut_ddr3_emif_reg_data = {
	.sdram_config = 0x61C04AB2,
	.sdram_tim1 = 0x0888A39B,
	.sdram_tim2 = 0x26337FDA,
	.sdram_tim3 = 0x501F830F,
	.emif_ddr_phy_ctlr_1 = 0x6,
	.zq_config = 0x50074BE4,
	.ref_ctrl = 0x93B,
};

struct ddr_data rut_ddr3_data = {
	.datardsratio0 = 0x3b,
	.datawdsratio0 = 0x85,
	.datafwsratio0 = 0x100,
	.datawrsratio0 = 0xc1,
	.datauserank0delay = 1,
	.datadldiff0 = PHY_DLL_LOCK_DIFF,
};

struct cmd_control rut_ddr3_cmd_ctrl_data = {
	.cmd0csratio = 0x40,
	.cmd0dldiff = 0,
	.cmd0iclkout = 1,
	.cmd1csratio = 0x40,
	.cmd1dldiff = 0,
	.cmd1iclkout = 1,
	.cmd2csratio = 0x40,
	.cmd2dldiff = 0,
	.cmd2iclkout = 1,
};

	config_ddr(RUT_PLL_FREQ, RUT_IOCTRL_VAL, &rut_ddr3_data,
		   &rut_ddr3_cmd_ctrl_data, &rut_ddr3_emif_reg_data, 0);
}

static void spl_siemens_board_init(void)
{
	return;
}
#endif /* if def CONFIG_SPL_BUILD */

#if defined(CONFIG_DRIVER_TI_CPSW)
static void cpsw_control(int enabled)
{
	/* VTP can be added here */

	return;
}

static struct cpsw_slave_data cpsw_slaves[] = {
	{
		.slave_reg_ofs	= 0x208,
		.sliver_reg_ofs	= 0xd80,
		.phy_id		= 1,
		.phy_if		= PHY_INTERFACE_MODE_RMII,
	},
	{
		.slave_reg_ofs	= 0x308,
		.sliver_reg_ofs	= 0xdc0,
		.phy_id		= 0,
		.phy_if		= PHY_INTERFACE_MODE_RMII,
	},
};

static struct cpsw_platform_data cpsw_data = {
	.mdio_base		= CPSW_MDIO_BASE,
	.cpsw_base		= CPSW_BASE,
	.mdio_div		= 0xff,
	.channels		= 8,
	.cpdma_reg_ofs		= 0x800,
	.slaves			= 1,
	.slave_data		= cpsw_slaves,
	.ale_reg_ofs		= 0xd00,
	.ale_entries		= 1024,
	.host_port_reg_ofs	= 0x108,
	.hw_stats_reg_ofs	= 0x900,
	.mac_control		= (1 << 5),
	.control		= cpsw_control,
	.host_port_num		= 0,
	.version		= CPSW_CTRL_VERSION_2,
};

#if defined(CONFIG_DRIVER_TI_CPSW) || \
	(defined(CONFIG_USB_ETHER) && defined(CONFIG_MUSB_GADGET))
int board_eth_init(bd_t *bis)
{
	struct ctrl_dev *cdev = (struct ctrl_dev *)CTRL_DEVICE_BASE;
	int n = 0;
	int rv;

#ifndef CONFIG_SPL_BUILD
	factoryset_setenv();
#endif

	/* Set rgmii mode and enable rmii clock to be sourced from chip */
	writel((RMII_MODE_ENABLE | RMII_CHIPCKL_ENABLE), &cdev->miisel);

	rv = cpsw_register(&cpsw_data);
	if (rv < 0)
		printf("Error %d registering CPSW switch\n", rv);
	else
		n += rv;
	return n;
}
#endif /* #if defined(CONFIG_DRIVER_TI_CPSW) */
#endif /* #if (defined(CONFIG_DRIVER_TI_CPSW) && !defined(CONFIG_SPL_BUILD)) */

#if defined(CONFIG_HW_WATCHDOG)
static bool hw_watchdog_init_done;
static int  hw_watchdog_trigger_level;

void hw_watchdog_reset(void)
{
	if (!hw_watchdog_init_done)
		return;

	hw_watchdog_trigger_level = hw_watchdog_trigger_level ? 0 : 1;
	gpio_set_value(WATCHDOG_TRIGGER_GPIO, hw_watchdog_trigger_level);
}

void hw_watchdog_init(void)
{
	gpio_request(WATCHDOG_TRIGGER_GPIO, "watchdog_trigger");
	gpio_direction_output(WATCHDOG_TRIGGER_GPIO, hw_watchdog_trigger_level);

	hw_watchdog_reset();

	hw_watchdog_init_done = 1;
}
#endif /* defined(CONFIG_HW_WATCHDOG) */

#include "../common/board.c"
