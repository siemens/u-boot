// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for IOT2050
 * Copyright (c) Siemens AG, 2018-2020
 *
 * Authors:
 *   Le Jin <le.jin@siemens.com>
 *   Jan Kiszka <jan.kiszka@siemens.com>
 */

#include <common.h>
#include <bootstage.h>
#include <dm.h>
#include <i2c.h>
#include <led.h>
#include <net.h>
#include <phy.h>
#include <spl.h>
#include <version.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/hardware.h>
#include <asm/gpio.h>
#include <asm/io.h>

#define IOT2050_INFO_MAGIC		0x20502050

struct iot2050_info {
	u32 magic;
	u16 size;
	char name[20 + 1];
	char serial[16 + 1];
	char mlfb[18 + 1];
	char uuid[32 + 1];
	char a5e[18 + 1];
	u8 mac_addr_cnt;
	u8 mac_addr[8][ARP_HLEN];
	char seboot_version[40 + 1];
} __packed;

/*
 * Scratch SRAM (available before DDR RAM) contains extracted EEPROM data.
 */
#define IOT2050_INFO_DATA ((struct iot2050_info *) \
			     TI_SRAM_SCRATCH_BOARD_EEPROM_START)

DECLARE_GLOBAL_DATA_PTR;

static bool board_is_advanced(void)
{
	struct iot2050_info *info = IOT2050_INFO_DATA;

	return info->magic == IOT2050_INFO_MAGIC &&
		!strcmp((char *)info->name, "IOT2050-ADVANCED");
}

void set_board_info_env(void)
{
	struct iot2050_info *info = IOT2050_INFO_DATA;
	u8 __maybe_unused mac_cnt;

	if (info->magic != IOT2050_INFO_MAGIC) {
		pr_err("IOT2050: Board info parsing error!\n");
		return;
	}

	if (env_get("board_uuid"))
		return;

	env_set("board_name", info->name);
	env_set("board_serial", info->serial);
	env_set("mlfb", info->mlfb);
	env_set("board_uuid", info->uuid);
	env_set("board_a5e", info->a5e);
	env_set("fw_version", PLAIN_VERSION);
	env_set("seboot_version", info->seboot_version);

#ifdef CONFIG_NET
	/* MAC address */
	for (mac_cnt = 0; mac_cnt < info->mac_addr_cnt; mac_cnt++) {
		if (is_valid_ethaddr(info->mac_addr[mac_cnt]))
			eth_env_set_enetaddr_by_index("eth", mac_cnt + 1,
						      info->mac_addr[mac_cnt]);
	}

	/*
	 * Set the MAC address environment variable that ICSSG0-PRU eth0 will
	 * use in u-boot
	 */
	env_set("ethaddr", env_get("eth1addr"));
#endif

	if (board_is_advanced()) {
		env_set("fdtfile", "siemens/iot2050-advanced.dtb");
	} else {
		env_set("fdtfile", "siemens/iot2050-basic.dtb");
		/* remove the unavailable eMMC (mmc1) from the list */
		env_set("boot_targets", "mmc0 usb0 usb1 usb2");
	}

	env_save();
}

int board_init(void)
{
	return 0;
}

int dram_init(void)
{
	if (board_is_advanced())
		gd->ram_size = SZ_2G;
	else
		gd->ram_size = SZ_1G;

	return 0;
}

int dram_init_banksize(void)
{
	dram_init();

	/* Bank 0 declares the memory available in the DDR low region */
	gd->bd->bi_dram[0].start = CONFIG_SYS_SDRAM_BASE;
	gd->bd->bi_dram[0].size = gd->ram_size;

	/* Bank 1 declares the memory available in the DDR high region */
	gd->bd->bi_dram[1].start = 0;
	gd->bd->bi_dram[1].size = 0;

	return 0;
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	struct iot2050_info *info = IOT2050_INFO_DATA;
	char upper_name[32];

	if (info->magic != IOT2050_INFO_MAGIC ||
	    strlen(name) >= sizeof(upper_name))
		return -1;

	str_to_upper(name, upper_name, sizeof(upper_name));
	if (!strcmp(upper_name, (char *)info->name))
		return 0;

	return -1;
}
#endif

int do_board_detect(void)
{
	return 0;
}

#define SERDES0_LANE_SELECT	0x00104080

int board_late_init(void)
{
	/* change CTRL_MMR register to let serdes0 not output USB3.0 signals. */
	writel(0x3, SERDES0_LANE_SELECT);

	set_board_info_env();

	return 0;
}

#if defined(CONFIG_OF_LIBFDT) && defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
	int ret;

	ret = fdt_fixup_msmc_ram(blob, "/interconnect@100000", "sram@70000000");
	if (ret)
		printf("%s: fixing up msmc ram failed %d\n", __func__, ret);

	return ret;
}
#endif

void spl_board_init(void)
{
}

/* RJ45 LED configuration */
#define DP83867_DEVADDR			0x1f

#define DP83867_LEDCR_1			0x0018

#define DP83867_LED_1000_BT_LINK	0x5
#define DP83867_LED_100_BTX_LINK	0x6
#define DP83867_LED_LINK_BLINK_ACTIVE	0xb
#define DP83867_LED_SET(n, v)		((v) << ((n) * 4))

#define DP83867_LED_SETTINGS \
	(DP83867_LED_SET(0, DP83867_LED_LINK_BLINK_ACTIVE) | \
	 DP83867_LED_SET(1, DP83867_LED_1000_BT_LINK) | \
	 DP83867_LED_SET(2, DP83867_LED_100_BTX_LINK) | \
	 DP83867_LED_SET(3, DP83867_LED_100_BTX_LINK))

int board_phy_config(struct phy_device *phydev)
{
	return phy_write_mmd(phydev, DP83867_DEVADDR, DP83867_LEDCR_1,
			     DP83867_LED_SETTINGS);
}

/*
 * Indicate any error or (accidental?) entering of CLI via the red status LED.
 */
#if CONFIG_IS_ENABLED(LED)
void show_boot_progress(int progress)
{
	struct udevice *dev;
	int ret;

	if (progress < 0 || progress == BOOTSTAGE_ID_ENTER_CLI_LOOP) {
		ret = led_get_by_label("status-green", &dev);
		if (ret == 0)
			led_set_state(dev, LEDST_OFF);

		ret = led_get_by_label("status-red", &dev);
		if (ret == 0)
			led_set_state(dev, LEDST_ON);
	}
}
#endif
