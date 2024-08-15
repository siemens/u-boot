// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Siemens AG, 2024
 */

#include <dm.h>
#include <sysinfo.h>
#include <net.h>
#include <asm/arch/hardware.h>

#include "iot2050.h"

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
	u8 padding[3];
	u32 ddr_size_mb;
} __packed;

/**
 * struct sysinfo_iot2050_priv - sysinfo private data
 * @info: iot2050 board info
 */
struct sysinfo_iot2050_priv {
	struct iot2050_info *info;
};

static int sysinfo_iot2050_detect(struct udevice *dev)
{
	struct sysinfo_iot2050_priv *priv = dev_get_priv(dev);

	if (priv->info == NULL || priv->info->magic != IOT2050_INFO_MAGIC)
		return -EFAULT;

	return 0;
}

static int sysinfo_iot2050_get_str(struct udevice *dev, int id, size_t size,
				   char *val)
{
	struct sysinfo_iot2050_priv *priv = dev_get_priv(dev);
	char byte_str[3] = {0};
	unsigned int n;

	switch (id) {
	case BOARD_NAME:
		strncpy(val, priv->info->name, size);
		break;
	case BOARD_SERIAL:
		strncpy(val, priv->info->serial, size);
		break;
	case BOARD_MLFB:
		strncpy(val, priv->info->mlfb, size);
		break;
	case BOARD_UUID:
		for (n = 0; n < min(size, (size_t)16); n++) {
			memcpy(byte_str, priv->info->uuid + n * 2, 2);
			val[n] = (char)hextoul(byte_str, NULL);
		}
		break;
	case BOARD_A5E:
		strncpy(val, priv->info->a5e, size);
		break;
	case BOARD_SEBOOT_VER:
		strncpy(val, priv->info->seboot_version, size);
		break;
	case BOARD_MAC_ADDR_1:
	case BOARD_MAC_ADDR_2:
	case BOARD_MAC_ADDR_3:
	case BOARD_MAC_ADDR_4:
	case BOARD_MAC_ADDR_5:
	case BOARD_MAC_ADDR_6:
	case BOARD_MAC_ADDR_7:
	case BOARD_MAC_ADDR_8:
		memcpy(val, priv->info->mac_addr[id - BOARD_MAC_ADDR_START],
		       ARP_HLEN);
		return 0;
	case BOARD_DDR_SIZE:
		memcpy(val, &priv->info->ddr_size_mb,
		       sizeof(priv->info->ddr_size_mb));
		return 0;
	default:
		return -EINVAL;
	};

	val[size - 1] = '\0';
	return 0;
}

static int sysinfo_iot2050_get_int(struct udevice *dev, int id, int *val)
{
	struct sysinfo_iot2050_priv *priv = dev_get_priv(dev);

	switch (id) {
	case BOARD_MAC_ADDR_CNT:
		*val = priv->info->mac_addr_cnt;
		return 0;
	default:
		return -EINVAL;
	};
}

static const struct sysinfo_ops sysinfo_iot2050_ops = {
	.detect = sysinfo_iot2050_detect,
	.get_str = sysinfo_iot2050_get_str,
	.get_int = sysinfo_iot2050_get_int,
};

static int sysinfo_iot2050_probe(struct udevice *dev)
{
	struct sysinfo_iot2050_priv *priv = dev_get_priv(dev);
	unsigned long offset;

	offset = dev_read_u32_default(dev, "offset",
				      TI_SRAM_SCRATCH_BOARD_EEPROM_START);
	priv->info = (struct iot2050_info *)offset;

	return 0;
}

static const struct udevice_id sysinfo_iot2050_ids[] = {
	{ .compatible = "siemens,sysinfo-iot2050" },
	{ /* sentinel */ }
};

U_BOOT_DRIVER(sysinfo_iot2050) = {
	.name           = "sysinfo_iot2050",
	.id             = UCLASS_SYSINFO,
	.of_match       = sysinfo_iot2050_ids,
	.ops		= &sysinfo_iot2050_ops,
	.priv_auto	= sizeof(struct sysinfo_iot2050_priv),
	.probe          = sysinfo_iot2050_probe,
};
