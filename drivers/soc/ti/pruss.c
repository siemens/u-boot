// SPDX-License-Identifier: GPL-2.0
/*
 * PRU-ICSS platform driver for various TI SoCs
 *
 * Copyright (C) 2020 Texas Instruments Incorporated - http://www.ti.com/
 *	Keerthy <j-keerthy@ti.com>
 */

#include <common.h>
#include <dm.h>
#include <dm/of_access.h>
#include <errno.h>
#include <clk.h>
#include <reset.h>
#include <regmap.h>
#include <syscon.h>
#include <asm/io.h>
#include <power-domain.h>
#include <ti-pruss.h>

#define PRUSS_CFG_IEPCLK	0x30
#define ICSSG_CFG_CORE_SYNC	0x3c

#define ICSSG_TASK_MGR_OFFSET	0x2a000

/* PRUSS_IEPCLK register bits */
#define PRUSS_IEPCLK_IEP_OCP_CLK_EN		BIT(0)

/* ICSSG CORE_SYNC register bits */
#define ICSSG_CORE_VBUSP_SYNC_EN		BIT(0)

/**
 * enum pruss_mem - PRUSS memory range identifiers
 */
enum pruss_mem {
	PRUSS_MEM_DRAM0 = 0,
	PRUSS_MEM_DRAM1,
	PRUSS_MEM_SHRD_RAM2,
	PRUSS_MEM_MAX,
};

/*
 * pruss_request_tm_region() - Request pruss for task manager region
 * @dev:	corresponding k3 device
 * @loc:	the task manager physical address
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
int pruss_request_tm_region(struct udevice *dev, phys_addr_t *loc)
{
	struct pruss *priv;

	priv = dev_get_priv(dev);
	if (!priv || !priv->pruss_dram0)
		return -EINVAL;

	*loc = priv->pruss_dram0 + ICSSG_TASK_MGR_OFFSET;

	return 0;
}

/*
 * pruss_request_shrmem_region() - Request pruss for shared memory region
 * @dev:	corresponding k3 device
 * @loc:	the shared memory physical address
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
int pruss_request_shrmem_region(struct udevice *dev, phys_addr_t *loc)
{
	struct pruss *priv;

	priv = dev_get_priv(dev);
	if (!priv || !priv->pruss_shrdram2)
		return -EINVAL;

	*loc = priv->pruss_shrdram2;

	return 0;
}

/**
 * pruss_probe() - Basic probe
 * @dev:	corresponding k3 device
 *
 * Return: 0 if all goes good, else appropriate error message.
 */
static int pruss_probe(struct udevice *dev)
{
	struct pruss *priv;
	int ret, idx;
	ofnode sub_node, node, memories;
	struct regmap *regmap_cfg;
	struct udevice *syscon;

	priv = dev_get_priv(dev);
	node = dev_ofnode(dev);
	sub_node = ofnode_find_subnode(node, "cfg");
	memories = ofnode_find_subnode(node, "memories");

	idx = ofnode_stringlist_search(memories, "reg-names", "dram0");
	priv->pruss_dram0 = ofnode_get_addr_size_index(memories, idx,
						       (u64 *)&priv->pruss_dram0sz);
	idx = ofnode_stringlist_search(memories, "reg-names", "dram1");
	priv->pruss_dram1 = ofnode_get_addr_size_index(memories, idx,
						       (u64 *)&priv->pruss_dram1sz);
	idx = ofnode_stringlist_search(memories, "reg-names", "shrdram2");
	priv->pruss_shrdram2 = ofnode_get_addr_size_index(memories, idx,
							  (u64 *)&priv->pruss_shrdram2sz);

	ret = uclass_get_device_by_ofnode(UCLASS_SYSCON, sub_node,
					  &syscon);

	regmap_cfg = syscon_get_regmap(syscon);

	/*
	 * The CORE block uses two multiplexers to allow software to
	 * select one of three source clocks (ICSSGn_CORE_CLK, ICSSGn_ICLK or
	 * ICSSGn_IEP_CLK) for the final clock source of the CORE block.
	 * The user needs to configure ICSSG_CORE_SYNC_REG[0] CORE_VBUSP_SYNC_EN
	 * bit & ICSSG_IEPCLK_REG[0] IEP_OCP_CLK_EN bit in order to select the
	 * clock source to the CORE block.
	 */
	ret = regmap_update_bits(regmap_cfg, ICSSG_CFG_CORE_SYNC,
				 ICSSG_CORE_VBUSP_SYNC_EN,
				 ICSSG_CORE_VBUSP_SYNC_EN);
	if (ret)
		return ret;
	ret = regmap_update_bits(regmap_cfg, PRUSS_CFG_IEPCLK,
				 PRUSS_IEPCLK_IEP_OCP_CLK_EN,
				 PRUSS_IEPCLK_IEP_OCP_CLK_EN);
	if (ret)
		return ret;

	dev_dbg(dev, "pruss successfully probed %s\n", dev->name);

	return 0;
}

static const struct udevice_id pruss_ids[] = {
	{ .compatible = "ti,am654-icssg"},
	{}
};

U_BOOT_DRIVER(pruss) = {
	.name = "pruss",
	.of_match = pruss_ids,
	.id = UCLASS_MISC,
	.probe = pruss_probe,
	.priv_auto_alloc_size = sizeof(struct pruss),
};
