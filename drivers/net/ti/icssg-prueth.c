// SPDX-License-Identifier: GPL-2.0+
/*
 * Texas Instruments K3 AM65 PRU Ethernet Driver
 *
 * Copyright (C) 2019, Texas Instruments, Incorporated
 *
 */

#include <common.h>
#include <asm/io.h>
#include <asm/processor.h>
#include <clk.h>
#include <dm.h>
#include <dm/lists.h>
#include <dm/device.h>
#include <dma-uclass.h>
#include <dm/of_access.h>
#include <fs_loader.h>
#include <miiphy.h>
#include <misc.h>
#include <net.h>
#include <phy.h>
#include <power-domain.h>
#include <linux/soc/ti/ti-udma.h>
#include <regmap.h>
#include <remoteproc.h>
#include <syscon.h>
#include <ti-pruss.h>

#include "cpsw_mdio.h"
#include "icssg.h"

#define ICSS_SLICE0     0
#define ICSS_SLICE1     1

#ifdef PKTSIZE_ALIGN
#define UDMA_RX_BUF_SIZE PKTSIZE_ALIGN
#else
#define UDMA_RX_BUF_SIZE ALIGN(1522, ARCH_DMA_MINALIGN)
#endif

#ifdef PKTBUFSRX
#define UDMA_RX_DESC_NUM PKTBUFSRX
#else
#define UDMA_RX_DESC_NUM 4
#endif

enum prueth_mac {
	PRUETH_MAC0 = 0,
	PRUETH_MAC1,
	PRUETH_NUM_MACS,
};

enum prueth_port {
	PRUETH_PORT_HOST = 0,	/* host side port */
	PRUETH_PORT_MII0,	/* physical port MII 0 */
	PRUETH_PORT_MII1,	/* physical port MII 1 */
};

/* Config region lies in shared RAM */
#define ICSS_CONFIG_OFFSET_SLICE0	0
#define ICSS_CONFIG_OFFSET_SLICE1	0x8000

/* Firmware flags */
#define ICSS_SET_RUN_FLAG_VLAN_ENABLE		BIT(0)	/* switch only */
#define ICSS_SET_RUN_FLAG_FLOOD_UNICAST		BIT(1)	/* switch only */
#define ICSS_SET_RUN_FLAG_PROMISC		BIT(2)	/* MAC only */
#define ICSS_SET_RUN_FLAG_MULTICAST_PROMISC	BIT(3)	/* MAC only */

/* CTRLMMR_ICSSG_RGMII_CTRL register bits */
#define ICSSG_CTRL_RGMII_ID_MODE		BIT(24)

/**
 * enum pruss_pru_id - PRU core identifiers
 */
enum pruss_pru_id {
	PRUSS_PRU0 = 0,
	PRUSS_PRU1,
	PRUSS_NUM_PRUS,
};

struct prueth {
	struct udevice		*dev;
	struct regmap		*miig_rt;
	struct regmap		*mii_rt;
	fdt_addr_t		mdio_base;
	phys_addr_t		pruss_shrdram2;
	phys_addr_t		tmaddr;
	struct mii_dev		*bus;
	u32			port_id;
	u32			sram_pa;
	struct phy_device	*phydev;
	bool			has_phy;
	ofnode			phy_node;
	u32			phy_addr;
	ofnode			eth_node[PRUETH_NUM_MACS];
	struct icssg_config	config[PRUSS_NUM_PRUS];
	u32			mdio_freq;
	int			phy_interface;
	struct			clk mdiofck;
	struct dma		dma_tx;
	struct dma		dma_rx;
	u32			rx_next;
	u32			rx_pend;
	int			slice;
};

/**
 * TX IPG Values to be set for 100M and 1G link speeds.  These values are
 * in ocp_clk cycles. So need change if ocp_clk is changed for a specific
 * h/w design.
 */
#define MII_RT_TX_IPG_100M	0x166
#define MII_RT_TX_IPG_1G	0x18

#define RGMII_CFG_OFFSET	4

/* Constant to choose between MII0 and MII1 */
#define ICSS_MII0	0
#define ICSS_MII1	1

/* RGMII CFG Register bits */
#define RGMII_CFG_GIG_EN_MII0	BIT(17)
#define RGMII_CFG_GIG_EN_MII1	BIT(21)
#define RGMII_CFG_FULL_DUPLEX_MII0	BIT(18)
#define RGMII_CFG_FULL_DUPLEX_MII1	BIT(22)

/* PRUSS_MII_RT Registers */
#define PRUSS_MII_RT_RXCFG0		0x0
#define PRUSS_MII_RT_RXCFG1		0x4
#define PRUSS_MII_RT_TXCFG0		0x10
#define PRUSS_MII_RT_TXCFG1		0x14
#define PRUSS_MII_RT_TX_CRC0		0x20
#define PRUSS_MII_RT_TX_CRC1		0x24
#define PRUSS_MII_RT_TX_IPG0		0x30
#define PRUSS_MII_RT_TX_IPG1		0x34
#define PRUSS_MII_RT_PRS0		0x38
#define PRUSS_MII_RT_PRS1		0x3c
#define PRUSS_MII_RT_RX_FRMS0		0x40
#define PRUSS_MII_RT_RX_FRMS1		0x44
#define PRUSS_MII_RT_RX_PCNT0		0x48
#define PRUSS_MII_RT_RX_PCNT1		0x4c
#define PRUSS_MII_RT_RX_ERR0		0x50
#define PRUSS_MII_RT_RX_ERR1		0x54

static inline void icssg_update_rgmii_cfg(struct regmap *miig_rt, bool gig_en,
					  bool full_duplex, int mii)
{
	u32 gig_en_mask, gig_val = 0, full_duplex_mask, full_duplex_val = 0;

	gig_en_mask = (mii == ICSS_MII0) ? RGMII_CFG_GIG_EN_MII0 :
					RGMII_CFG_GIG_EN_MII1;
	if (gig_en)
		gig_val = gig_en_mask;
	regmap_update_bits(miig_rt, RGMII_CFG_OFFSET, gig_en_mask, gig_val);

	full_duplex_mask = (mii == ICSS_MII0) ? RGMII_CFG_FULL_DUPLEX_MII0 :
					   RGMII_CFG_FULL_DUPLEX_MII1;
	if (full_duplex)
		full_duplex_val = full_duplex_mask;
	regmap_update_bits(miig_rt, RGMII_CFG_OFFSET, full_duplex_mask,
			   full_duplex_val);
}

static inline void icssg_update_mii_rt_cfg(struct regmap *mii_rt, int speed,
					   int mii)
{
	u32 ipg_reg, val;

	ipg_reg = (mii == ICSS_MII0) ? PRUSS_MII_RT_TX_IPG0 :
				       PRUSS_MII_RT_TX_IPG1;
	switch (speed) {
	case SPEED_1000:
		val = MII_RT_TX_IPG_1G;
		break;
	case SPEED_100:
		val = MII_RT_TX_IPG_100M;
		break;
	default:
		/* Other links speeds not supported */
		pr_err("Unsupported link speed\n");
		return;
	}
	regmap_write(mii_rt, ipg_reg, val);
}

static int icssg_phy_init(struct udevice *dev)
{
	struct prueth *priv = dev_get_priv(dev);
	struct phy_device *phydev;
	u32 supported = PHY_GBIT_FEATURES;
	int ret;

	phydev = phy_connect(priv->bus,
			     priv->phy_addr,
			     priv->dev,
			     priv->phy_interface);

	if (!phydev) {
		dev_err(dev, "phy_connect() failed\n");
		return -ENODEV;
	}

	/* disable unsupported features */
	supported &= ~(PHY_10BT_FEATURES |
			SUPPORTED_100baseT_Half |
			SUPPORTED_1000baseT_Half |
			SUPPORTED_Pause |
			SUPPORTED_Asym_Pause);

	phydev->supported &= supported;
	phydev->advertising = phydev->supported;

#ifdef CONFIG_DM_ETH
	if (ofnode_valid(priv->phy_node))
		phydev->node = priv->phy_node;
#endif

	priv->phydev = phydev;
	ret = phy_config(phydev);
	if (ret < 0)
		pr_err("phy_config() failed: %d", ret);

	return ret;
}

static int icssg_mdio_init(struct udevice *dev)
{
	struct prueth *prueth = dev_get_priv(dev);

	prueth->bus = cpsw_mdio_init(dev->name, prueth->mdio_base,
				     prueth->mdio_freq,
				     clk_get_rate(&prueth->mdiofck));
	if (!prueth->bus)
		return -EFAULT;

	return 0;
}

static void icssg_config_set(struct prueth *prueth)
{
	void __iomem *va;

	va = (void __iomem *)prueth->pruss_shrdram2 + prueth->slice *
			ICSSG_CONFIG_OFFSET_SLICE1;

	memcpy_toio(va, &prueth->config[0], sizeof(prueth->config[0]));
}

static int icssg_update_link(struct prueth *priv)
{
	struct phy_device *phy = priv->phydev;
	bool gig_en = false, full_duplex = false;

	if (phy->link) { /* link up */
		if (phy->speed == SPEED_1000)
			gig_en = true;
		if (phy->duplex == DUPLEX_FULL)
			full_duplex = true;
		/* Set the RGMII cfg for gig en and full duplex */
		icssg_update_rgmii_cfg(priv->miig_rt, gig_en, full_duplex,
				       priv->slice);
		/* update the Tx IPG based on 100M/1G speed */
		icssg_update_mii_rt_cfg(priv->mii_rt, phy->speed, priv->slice);

		printf("link up on port %d, speed %d, %s duplex\n",
		       priv->port_id, phy->speed,
		       (phy->duplex == DUPLEX_FULL) ? "full" : "half");
	} else {
		printf("link down on port %d\n", priv->port_id);
	}

	return phy->link;
}

static int prueth_start(struct udevice *dev)
{
	struct prueth *priv = dev_get_priv(dev);
	struct eth_pdata *pdata = dev->platdata;
	int ret, i;
	char tx_chn_name[16];
	char rx_chn_name[16];

	icssg_class_set_mac_addr(priv->miig_rt, priv->slice,
				 (u8 *)pdata->enetaddr);
	icssg_class_default(priv->miig_rt, priv->slice);

	/* To differentiate channels for SLICE0 vs SLICE1 */
	snprintf(tx_chn_name, sizeof(tx_chn_name), "tx%d-0", priv->slice);
	snprintf(rx_chn_name, sizeof(rx_chn_name), "rx%d", priv->slice);

	ret = dma_get_by_name(dev, tx_chn_name, &priv->dma_tx);
	if (ret)
		dev_err(dev, "TX dma get failed %d\n", ret);

	ret = dma_get_by_name(dev, rx_chn_name, &priv->dma_rx);
	if (ret)
		dev_err(dev, "RX dma get failed %d\n", ret);

	for (i = 0; i < UDMA_RX_DESC_NUM; i++) {
		ret = dma_prepare_rcv_buf(&priv->dma_rx,
					  net_rx_packets[i],
					  UDMA_RX_BUF_SIZE);
		if (ret)
			dev_err(dev, "RX dma add buf failed %d\n", ret);
	}

	ret = dma_enable(&priv->dma_tx);
	if (ret) {
		dev_err(dev, "TX dma_enable failed %d\n", ret);
		goto tx_fail;
	}

	ret = dma_enable(&priv->dma_rx);
	if (ret) {
		dev_err(dev, "RX dma_enable failed %d\n", ret);
		goto rx_fail;
	}

	ret = phy_startup(priv->phydev);
	if (ret) {
		dev_err(dev, "phy_startup failed\n");
		goto phy_fail;
	}

	ret = icssg_update_link(priv);
	if (!ret) {
		ret = -ENODEV;
		goto phy_shut;
	}

	return 0;

phy_shut:
	phy_shutdown(priv->phydev);
phy_fail:
	dma_disable(&priv->dma_rx);
	dma_free(&priv->dma_rx);
rx_fail:
	dma_disable(&priv->dma_tx);
	dma_free(&priv->dma_tx);

tx_fail:
	icssg_class_disable(priv->miig_rt, priv->slice);

	return ret;
}

void prueth_print_buf(ulong addr, const void *data, uint width,
		      uint count, uint linelen)
{
	print_buffer(addr, data, width, count, linelen);
}

static int prueth_send(struct udevice *dev, void *packet, int length)
{
	struct prueth *priv = dev_get_priv(dev);
	int ret;

	ret = dma_send(&priv->dma_tx, packet, length, NULL);

	return ret;
}

static int prueth_recv(struct udevice *dev, int flags, uchar **packetp)
{
	struct prueth *priv = dev_get_priv(dev);
	int ret;

	/* try to receive a new packet */
	ret = dma_receive(&priv->dma_rx, (void **)packetp, NULL);

	return ret;
}

static int prueth_free_pkt(struct udevice *dev, uchar *packet, int length)
{
	struct prueth *priv = dev_get_priv(dev);
	int ret = 0;

	if (length > 0) {
		u32 pkt = priv->rx_next % UDMA_RX_DESC_NUM;

		dev_dbg(dev, "%s length:%d pkt:%u\n", __func__, length, pkt);

		ret = dma_prepare_rcv_buf(&priv->dma_rx,
					  net_rx_packets[pkt],
					  UDMA_RX_BUF_SIZE);
		priv->rx_next++;
	}

	return ret;
}

static void prueth_stop(struct udevice *dev)
{
	struct prueth *priv = dev_get_priv(dev);

	icssg_class_disable(priv->miig_rt, priv->slice);

	phy_shutdown(priv->phydev);

	dma_disable(&priv->dma_tx);
	dma_free(&priv->dma_tx);

	dma_disable(&priv->dma_rx);
	dma_free(&priv->dma_rx);

	/* Workaround for shutdown command */
	writel(0x0, priv->tmaddr + priv->slice * 0x200);
}

static const struct eth_ops prueth_ops = {
	.start		= prueth_start,
	.send		= prueth_send,
	.recv		= prueth_recv,
	.free_pkt	= prueth_free_pkt,
	.stop		= prueth_stop,
};

static int icssg_ofdata_parse_phy(struct udevice *dev, ofnode port_np)
{
	struct prueth *priv = dev_get_priv(dev);
	struct ofnode_phandle_args out_args;
	const char *phy_mode;
	int ret = 0;

	phy_mode = ofnode_read_string(port_np, "phy-mode");
	if (phy_mode) {
		priv->phy_interface =
				phy_get_interface_by_name(phy_mode);
		if (priv->phy_interface == -1) {
			dev_err(dev, "Invalid PHY mode '%s'\n",
				phy_mode);
			ret = -EINVAL;
			goto out;
		}
	}

	ret = ofnode_parse_phandle_with_args(port_np, "phy-handle",
					     NULL, 0, 0, &out_args);
	if (ret) {
		dev_err(dev, "can't parse phy-handle port (%d)\n", ret);
		ret = 0;
	}

	priv->phy_node = out_args.node;
	ret = ofnode_read_u32(priv->phy_node, "reg", &priv->phy_addr);
	if (ret)
		dev_err(dev, "failed to get phy_addr port (%d)\n", ret);

out:
	return ret;
}

static int prueth_config_rgmiidelay(struct prueth *prueth,
				    ofnode eth_np)
{
	struct regmap *ctrl_mmr;
	int ret = 0;
	ofnode node;
	u32 tmp[2];

	ret = ofnode_read_u32_array(eth_np, "syscon-rgmii-delay", tmp, 2);
	if (ret) {
		dev_err(dev, "no syscon-rgmii-delay\n");
		return ret;
	}

	node = ofnode_get_by_phandle(tmp[0]);
	if (!ofnode_valid(node)) {
		dev_err(dev, "can't get syscon-rgmii-delay node\n");
		return -EINVAL;
	}

	ctrl_mmr = syscon_node_to_regmap(node);
	if (!ctrl_mmr) {
		dev_err(dev, "can't get ctrl_mmr regmap\n");
		return -EINVAL;
	}

	regmap_update_bits(ctrl_mmr, tmp[1], ICSSG_CTRL_RGMII_ID_MODE, 0);

	return 0;
}

static int prueth_probe(struct udevice *dev)
{
	struct prueth *prueth;
	int ret = 0, i;
	ofnode eth0_node, eth1_node, node, pruss_node, mdio_node, sram_node;
	u32 phandle, err, sp;
	struct udevice **prussdev = NULL;
	struct icssg_config *config;

	prueth = dev_get_priv(dev);
	prueth->dev = dev;
	err = ofnode_read_u32(dev_ofnode(dev), "prus", &phandle);
	if (err)
		return err;

	node = ofnode_get_by_phandle(phandle);
	if (!ofnode_valid(node))
		return -EINVAL;

	pruss_node = ofnode_get_parent(node);
	err = misc_init_by_ofnode(pruss_node);
	if (err)
		return err;

	ret = device_find_global_by_ofnode(pruss_node, prussdev);
	if (ret)
		dev_err(dev, "error getting the pruss dev\n");

	ret = pruss_request_shrmem_region(*prussdev, &prueth->pruss_shrdram2);
	if (ret)
		return ret;

	ret = pruss_request_tm_region(*prussdev, &prueth->tmaddr);
	if (ret)
		return ret;

	node = dev_ofnode(dev);
	eth0_node = ofnode_find_subnode(node, "ethernet-mii0");
	eth1_node = ofnode_find_subnode(node, "ethernet-mii1");
	/* one node must be present and available else we fail */
	if (!ofnode_valid(eth0_node) && !ofnode_valid(eth1_node)) {
		dev_err(dev, "neither ethernet-mii0 nor ethernet-mii1 node available\n");
		return -ENODEV;
	}

	/*
	 * Exactly one node must be present as uboot ethernet framework does
	 * not support two interfaces in a single probe. So Device Tree should
	 * have exactly one of mii0 or mii1 interface.
	 */
	if (ofnode_valid(eth0_node) && ofnode_valid(eth1_node)) {
		dev_err(dev, "Both slices cannot be supported\n");
		return -EINVAL;
	}

	if (ofnode_valid(eth0_node)) {
		prueth->slice = 0;
		icssg_ofdata_parse_phy(dev, eth0_node);
		prueth->eth_node[PRUETH_MAC0] = eth0_node;
	}

	if (ofnode_valid(eth1_node)) {
		prueth->slice = 1;
		icssg_ofdata_parse_phy(dev, eth1_node);
		prueth->eth_node[PRUETH_MAC0] = eth1_node;
	}

	prueth->miig_rt = syscon_regmap_lookup_by_phandle(dev, "mii-g-rt");
	if (!prueth->miig_rt) {
		dev_err(dev, "couldn't get mii-g-rt syscon regmap\n");
		return -ENODEV;
	}

	prueth->mii_rt = syscon_regmap_lookup_by_phandle(dev, "mii-rt");
	if (!prueth->mii_rt) {
		dev_err(dev, "couldn't get mii-rt syscon regmap\n");
		return -ENODEV;
	}

	ret = clk_get_by_name(dev, "mdio_fck", &prueth->mdiofck);
	if (ret) {
		dev_err(dev, "failed to get clock %d\n", ret);
		return ret;
	}
	ret = clk_enable(&prueth->mdiofck);
	if (ret) {
		dev_err(dev, "clk_enable failed %d\n", ret);
		return ret;
	}

	ret = ofnode_read_u32(dev_ofnode(dev), "sram", &sp);
	if (ret) {
		dev_err(dev, "sram node fetch failed %d\n", ret);
		return ret;
	}

	sram_node = ofnode_get_by_phandle(sp);
	if (!ofnode_valid(node))
		return -EINVAL;

	prueth->sram_pa = ofnode_get_addr(sram_node);

	if (!prueth->slice) {
		ret = prueth_config_rgmiidelay(prueth, eth0_node);
		if (ret) {
			dev_err(dev, "prueth_config_rgmiidelay failed\n");
			return ret;
		}
	} else {
		ret = prueth_config_rgmiidelay(prueth, eth1_node);
		if (ret) {
			dev_err(dev, "prueth_config_rgmiidelay failed\n");
			return ret;
		}
	}

	mdio_node = ofnode_find_subnode(pruss_node, "mdio");
	prueth->mdio_base = ofnode_get_addr(mdio_node);
	ofnode_read_u32(mdio_node, "bus_freq", &prueth->mdio_freq);

	ret = icssg_mdio_init(dev);
	if (ret)
		return ret;

	ret = icssg_phy_init(dev);
	if (ret) {
		dev_err(dev, "phy_init failed\n");
		goto out;
	}

	/* Set Load time configuration */
	config = &prueth->config[0];
	memset(config, 0, sizeof(*config));
	config->addr_lo = cpu_to_le32(lower_32_bits(prueth->sram_pa));
	config->addr_hi = cpu_to_le32(upper_32_bits(prueth->sram_pa));
	config->num_tx_threads = 0;
	config->rx_flow_id = 0; /* flow id for host port */

	for (i = 8; i < 16; i++)
		config->tx_buf_sz[i] = cpu_to_le32(0x1800);

	icssg_config_set(prueth);

	return 0;
out:
	cpsw_mdio_free(prueth->bus);
	clk_disable(&prueth->mdiofck);

	return ret;
}

static const struct udevice_id prueth_ids[] = {
	{ .compatible = "ti,am654-icssg-prueth" },
	{ }
};

U_BOOT_DRIVER(prueth) = {
	.name	= "prueth",
	.id	= UCLASS_ETH,
	.of_match = prueth_ids,
	.probe	= prueth_probe,
	.ops	= &prueth_ops,
	.priv_auto_alloc_size = sizeof(struct prueth),
	.platdata_auto_alloc_size = sizeof(struct eth_pdata),
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};
