// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for IOT2050
 * Copyright (c) Siemens AG, 2018-2022
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
#include <malloc.h>
#include <net.h>
#include <phy.h>
#include <spl.h>
#include <version.h>
#include <linux/delay.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/hardware.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <fdt_support.h>

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

#define M2_CFG_PIN_NUM    4
#define PCIE_MUX_CTRL_NUM 3

static char *fdt_fix_string;

struct gpio_config {
	char *label;
	char *gpio_name;
};

enum serdes0_supported_interface {
	USB30 = 0,
	PCIE0_LANE0,
	SGMII_LANE0,
};

enum serdes1_supported_interface {
	PCIE1_LANE0 = 0,
	PCIE0_LANE1,
	SGMII_LANE1,
};

enum m2_connector_combination_type {
	BKEY_PCIEX2_EKEY_NONE = 0,
	BKEY_PCIE_EKEY_PCIE,
	BKEY_USB30_EKEY_PCIE,
};

struct serdes_combination {
	enum m2_connector_combination_type m2_connector_type;
	enum serdes0_supported_interface  serdes0_interface;
	enum serdes1_supported_interface  serdes1_interface;
};

/* definition is from M.2 Spec */
struct m2_device_config_decodes {
	char config_0;
	char config_1;
	char config_2;
	char config_3;
};

struct serdes_mux_control_pin {
	char ctrl_usb30_pcie0_lane0;
	char ctrl_pcie1_pcie0;
	char ctrl_usb30_pcie0_lane1;
};

struct m2_config_table {
	struct m2_device_config_decodes	m2_config_decodes;
	struct serdes_mux_control_pin serdes_mux_ctrl_pin;
	struct serdes_combination serdes_select;
};

static struct gpio_config serdes_mux_ctl_pin_info[PCIE_MUX_CTRL_NUM] = {
	{"CTRL_USB30_PCIE0_LANE0", "gpio@600000_88"},
	{"CTRL_PCIE1_PCIE0",       "gpio@600000_82"},
	{"CTRL_USB30_PCIE0_LANE1", "gpio@600000_89"},
};

static struct gpio_config m2_bkey_cfg_pin_info[M2_CFG_PIN_NUM] = {
	{"KEY_CONFIG_0", "gpio@601000_18"},
	{"KEY_CONFIG_1", "gpio@601000_19"},
	{"KEY_CONFIG_2", "gpio@601000_88"},
	{"KEY_CONFIG_3", "gpio@601000_89"},
};

static struct m2_config_table m2_config_table[] = {
	{{0, 1, 0, 0}, {0, 0, 1}, {BKEY_PCIEX2_EKEY_NONE, PCIE0_LANE0, PCIE0_LANE1}},
	{{0, 0, 1, 0}, {0, 1, 0}, {BKEY_PCIE_EKEY_PCIE, PCIE0_LANE0, PCIE1_LANE0}},
	{{0, 1, 1, 0}, {0, 1, 0}, {BKEY_PCIE_EKEY_PCIE, PCIE0_LANE0, PCIE1_LANE0}},
	{{1, 0, 0, 1}, {0, 1, 0}, {BKEY_PCIE_EKEY_PCIE, PCIE0_LANE0, PCIE1_LANE0}},
	{{1, 1, 0, 1}, {0, 1, 0}, {BKEY_PCIE_EKEY_PCIE, PCIE0_LANE0, PCIE1_LANE0}},
	{{0, 0, 0, 1}, {1, 1, 0}, {BKEY_USB30_EKEY_PCIE, USB30, PCIE1_LANE0}},
	{{0, 1, 0, 1}, {1, 1, 0}, {BKEY_USB30_EKEY_PCIE, USB30, PCIE1_LANE0}},
	{{0, 0, 1, 1}, {1, 1, 0}, {BKEY_USB30_EKEY_PCIE, USB30, PCIE1_LANE0}},
	{{0, 1, 1, 1}, {1, 1, 0}, {BKEY_USB30_EKEY_PCIE, USB30, PCIE1_LANE0}},
	{{1, 0, 1, 1}, {1, 1, 0}, {BKEY_USB30_EKEY_PCIE, USB30, PCIE1_LANE0}},
};

static struct gpio_config m2_enable_power = {"P3V3_M2_EN", "gpio@601000_17"};

static int get_pinvalue(const char *gpio_name, const char *label)
{
	struct gpio_desc gpio;

	if (dm_gpio_lookup_name(gpio_name, &gpio) < 0 ||
	    dm_gpio_request(&gpio, label) < 0 ||
	    dm_gpio_set_dir_flags(&gpio, GPIOD_IS_IN) < 0)
		return false;

	return dm_gpio_get_value(&gpio);
}

static void set_pinvalue(const char *gpio_name, const char *label, int value)
{
	struct gpio_desc gpio;

	if (dm_gpio_lookup_name(gpio_name, &gpio) < 0 ||
	    dm_gpio_request(&gpio, label) < 0 ||
	    dm_gpio_set_dir_flags(&gpio, GPIOD_IS_OUT) < 0) {
		pr_err("IOT2050: Cannot set pin for M.2 configuration\n");
		return;
	}
	dm_gpio_set_value(&gpio, value);
}

static void get_m2_config_decodes(struct m2_device_config_decodes *decodes)
{
	decodes->config_0 = get_pinvalue(m2_bkey_cfg_pin_info[0].gpio_name,
								    m2_bkey_cfg_pin_info[0].label);
	decodes->config_1 = get_pinvalue(m2_bkey_cfg_pin_info[1].gpio_name,
								    m2_bkey_cfg_pin_info[1].label);
	decodes->config_2 = get_pinvalue(m2_bkey_cfg_pin_info[2].gpio_name,
								    m2_bkey_cfg_pin_info[2].label);
	decodes->config_3 = get_pinvalue(m2_bkey_cfg_pin_info[3].gpio_name,
								    m2_bkey_cfg_pin_info[3].label);
}

static void get_serdes_combination(struct serdes_combination *serdes_combination,
									int m2_connector_type)
{
	switch (m2_connector_type) {
	case BKEY_PCIEX2_EKEY_NONE:
		*serdes_combination = m2_config_table[0].serdes_select;
		break;
	case BKEY_PCIE_EKEY_PCIE:
		*serdes_combination = m2_config_table[1].serdes_select;
		break;
	case BKEY_USB30_EKEY_PCIE:
		*serdes_combination = m2_config_table[5].serdes_select;
		break;
	default:
		*serdes_combination = m2_config_table[5].serdes_select;
		break;
	}
}

static void update_m2_load_info(struct serdes_combination serdes_combination, const char **fdt_name)
{
	if (serdes_combination.serdes0_interface == PCIE0_LANE0 &&
		serdes_combination.serdes1_interface == PCIE0_LANE1) {
		fdt_fix_string = "siemens,iot2050-advanced-m2-pciex2";
		*fdt_name = "ti/k3-am6548-iot2050-advanced-m2-bkey-pciex2.dtb";
	} else if (serdes_combination.serdes0_interface == USB30 &&
			serdes_combination.serdes1_interface == PCIE0_LANE1) {
		fdt_fix_string = "siemens,iot2050-advanced-m2-usb3-pcie";
		*fdt_name = "ti/k3-am6548-iot2050-advanced-m2-bkey-usb3-ekey-pcie.dtb";
	} else if (serdes_combination.serdes0_interface == PCIE0_LANE0 &&
			serdes_combination.serdes1_interface == PCIE1_LANE0) {
		fdt_fix_string = "siemens,iot2050-advanced-m2-pcie-pcie";
		*fdt_name = "ti/k3-am6548-iot2050-advanced-m2-bkey-pcie-ekey-pcie.dtb";
	} else {
		fdt_fix_string = "siemens,iot2050-advanced-m2-usb3-pcie";
		*fdt_name = "ti/k3-am6548-iot2050-advanced-m2-bkey-usb3-ekey-pcie.dtb";
	}
}

static void get_serdes_mux_ctrl(struct serdes_mux_control_pin *serdes_mux_ctrl_info,
				int m2_connector_type)
{
	switch (m2_connector_type) {
	case BKEY_PCIEX2_EKEY_NONE:
		*serdes_mux_ctrl_info = m2_config_table[0].serdes_mux_ctrl_pin;
		break;
	case BKEY_PCIE_EKEY_PCIE:
		*serdes_mux_ctrl_info = m2_config_table[1].serdes_mux_ctrl_pin;
		break;
	case BKEY_USB30_EKEY_PCIE:
		*serdes_mux_ctrl_info = m2_config_table[5].serdes_mux_ctrl_pin;
		break;
	default:
		*serdes_mux_ctrl_info = m2_config_table[5].serdes_mux_ctrl_pin;
		break;
	}
}

static void configure_serdes_mux(struct serdes_combination serdes_selected_combination)
{
	struct serdes_mux_control_pin serdes_mux_ctrl_info;

	get_serdes_mux_ctrl(&serdes_mux_ctrl_info, serdes_selected_combination.m2_connector_type);

	set_pinvalue(serdes_mux_ctl_pin_info[0].gpio_name, serdes_mux_ctl_pin_info[0].label,
				serdes_mux_ctrl_info.ctrl_usb30_pcie0_lane0);
	set_pinvalue(serdes_mux_ctl_pin_info[1].gpio_name, serdes_mux_ctl_pin_info[1].label,
				serdes_mux_ctrl_info.ctrl_pcie1_pcie0);
	set_pinvalue(serdes_mux_ctl_pin_info[2].gpio_name, serdes_mux_ctl_pin_info[2].label,
				serdes_mux_ctrl_info.ctrl_usb30_pcie0_lane1);
}


static void set_m2_load_info(const char **fdt_name)
{
	int i;
	int config_table_size = ARRAY_SIZE(m2_config_table);
	struct m2_device_config_decodes	config;
	struct serdes_combination current_serdes_combination = {BKEY_USB30_EKEY_PCIE,
								USB30, PCIE1_LANE0};
	enum m2_connector_combination_type current_m2_connector_type;

	if (env_get("m2_manual_config")) {
		printf("Maunal Select M.2 Configuration\n");
		current_m2_connector_type = simple_strtoul(env_get("m2_manual_config"), NULL, 10);
		get_serdes_combination(&current_serdes_combination, current_m2_connector_type);
	} else { /*auto detect*/
		get_m2_config_decodes(&config);
		for (i = 0; i < config_table_size; i++) {
			if (!memcmp(&config, &m2_config_table[i].m2_config_decodes,
						sizeof(struct m2_device_config_decodes))) {
				current_serdes_combination =  m2_config_table[i].serdes_select;
				break;
			}
		}
	}

	update_m2_load_info(current_serdes_combination, fdt_name);
	configure_serdes_mux(current_serdes_combination);
}

/*
 * M.2 Board is a Variant Based On PG2 board
 */
static bool board_is_m2(void)
{
	struct iot2050_info *info = IOT2050_INFO_DATA;

	return info->magic == IOT2050_INFO_MAGIC &&
		strcmp((char *)info->name, "IOT2050-ADVANCED-M2") == 0;
}

static void m2_power_enable(void)
{
	/* enable m.2 connector power */
	set_pinvalue(m2_enable_power.gpio_name, m2_enable_power.label, 1);
	udelay(4 * 100);
}

static bool board_is_advanced(void)
{
	struct iot2050_info *info = IOT2050_INFO_DATA;

	return info->magic == IOT2050_INFO_MAGIC &&
		strstr((char *)info->name, "IOT2050-ADVANCED") != NULL;
}

static void remove_mmc1_target(void)
{
	char *boot_targets = strdup(env_get("boot_targets"));
	char *mmc1 = strstr(boot_targets, "mmc1");

	if (mmc1) {
		memmove(mmc1, mmc1 + 4, strlen(mmc1 + 4) + 1);
		env_set("boot_targets", boot_targets);
	}

	free(boot_targets);
}

void set_board_info_env(void)
{
	struct iot2050_info *info = IOT2050_INFO_DATA;
	u8 __maybe_unused mac_cnt;
	const char *fdtfile;

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
	if (IS_ENABLED(CONFIG_NET)) {
		/* set MAC addresses to ensure forwarding to the OS */
		for (mac_cnt = 0; mac_cnt < info->mac_addr_cnt; mac_cnt++) {
			if (is_valid_ethaddr(info->mac_addr[mac_cnt]))
				eth_env_set_enetaddr_by_index("eth",
							      mac_cnt + 1,
							      info->mac_addr[mac_cnt]);
		}
	}

	if (board_is_advanced()) {
		if (IS_ENABLED(CONFIG_TARGET_IOT2050_A53_PG1))
			fdtfile = "ti/k3-am6548-iot2050-advanced.dtb";
		else if(board_is_m2())
			fdtfile = "ti/k3-am6548-iot2050-advanced-m2-bkey-usb3-ekey-pcie.dtb";
		else
			fdtfile = "ti/k3-am6548-iot2050-advanced-pg2.dtb";
	} else {
		if (IS_ENABLED(CONFIG_TARGET_IOT2050_A53_PG1))
			fdtfile = "ti/k3-am6528-iot2050-basic.dtb";
		else
			fdtfile = "ti/k3-am6528-iot2050-basic-pg2.dtb";
		/* remove the unavailable eMMC (mmc1) from the list */
		remove_mmc1_target();
	}
	env_set("fdtfile", fdtfile);

	env_save();
}

void update_m2_board_info_env(void)
{
	const char *fdtfile;

	m2_power_enable();
	set_m2_load_info(&fdtfile);
	env_set("fdtfile", fdtfile);
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

	/* skip the prefix "k3-am65x8-" */
	name += 10;

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

#ifdef CONFIG_IOT2050_BOOT_SWITCH
static bool user_button_pressed(void)
{
	struct udevice *red_led = NULL;
	unsigned long count = 0;
	struct gpio_desc gpio;

	memset(&gpio, 0, sizeof(gpio));

	if (dm_gpio_lookup_name("gpio@42110000_25", &gpio) < 0 ||
	    dm_gpio_request(&gpio, "USER button") < 0 ||
	    dm_gpio_set_dir_flags(&gpio, GPIOD_IS_IN) < 0)
		return false;

	if (dm_gpio_get_value(&gpio) == 1)
		return false;

	printf("USER button pressed - booting from external media only\n");

	led_get_by_label("status-led-red", &red_led);

	if (red_led)
		led_set_state(red_led, LEDST_ON);

	while (dm_gpio_get_value(&gpio) == 0 && count++ < 10000)
		mdelay(1);

	if (red_led)
		led_set_state(red_led, LEDST_OFF);

	return true;
}
#endif

#define SERDES0_LANE_SELECT	0x00104080

int board_late_init(void)
{
	/* change CTRL_MMR register to let serdes0 not output USB3.0 signals. */
	writel(0x3, SERDES0_LANE_SELECT);

	set_board_info_env();
	if (board_is_m2())
		update_m2_board_info_env();

	/* remove the eMMC if requested via button */
	if (IS_ENABLED(CONFIG_IOT2050_BOOT_SWITCH) && board_is_advanced() &&
	    user_button_pressed())
		remove_mmc1_target();

	return 0;
}

static void m2_fdt_fixup(void *blob)
{
	int ret;

	ret = fdt_find_and_setprop(blob, "/", "compatible", fdt_fix_string,
								strlen(fdt_fix_string) + 1, 0);
	if (ret)
		pr_err("%s: m.2 fdt fixup failed:%d\n", __func__, ret);
}

#if defined(CONFIG_OF_LIBFDT) && defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
	int ret;

	ret = fdt_fixup_msmc_ram(blob, "/bus@100000", "sram@70000000");
	if (ret < 0)
		ret = fdt_fixup_msmc_ram(blob, "/interconnect@100000",
					 "sram@70000000");
	if (ret)
		pr_err("%s: fixing up msmc ram failed %d\n", __func__, ret);

	if (board_is_m2())
		m2_fdt_fixup(blob);

	return ret;
}
#endif

void spl_board_init(void)
{
}

#if CONFIG_IS_ENABLED(LED) && CONFIG_IS_ENABLED(SHOW_BOOT_PROGRESS)
/*
 * Indicate any error or (accidental?) entering of CLI via the red status LED.
 */
void show_boot_progress(int progress)
{
	struct udevice *dev;
	int ret;

	if ((progress < 0 && progress != -BOOTSTAGE_ID_NET_ETH_START) ||
	    progress == BOOTSTAGE_ID_ENTER_CLI_LOOP) {
		ret = led_get_by_label("status-led-green", &dev);
		if (ret == 0)
			led_set_state(dev, LEDST_OFF);

		ret = led_get_by_label("status-led-red", &dev);
		if (ret == 0)
			led_set_state(dev, LEDST_ON);
	}
}
#endif
