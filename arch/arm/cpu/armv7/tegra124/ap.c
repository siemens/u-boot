/*
 * (C) Copyright 2015, Siemens AG
 * Author: Jan Kiszka <jan.kiszka@siemens.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/flow.h>
#include <asm/arch/powergate.h>
#include <asm/arch-tegra/ap.h>
#include <asm/arch-tegra/pmc.h>

static void park_cpu(void)
{
	while (1)
		asm volatile("wfi");
}

void ap_pm_init(void)
{
	struct flow_ctlr *flow = (struct flow_ctlr *)NV_PA_FLOW_BASE;
	struct pmc_ctlr *pmc = (struct pmc_ctlr *)NV_PA_PMC_BASE;

	writel((u32)park_cpu, EXCEP_VECTOR_CPU_RESET_VECTOR);

	tegra_powergate_power_on(TEGRA_POWERGATE_CPU1);
	tegra_powergate_power_on(TEGRA_POWERGATE_CPU2);
	tegra_powergate_power_on(TEGRA_POWERGATE_CPU3);

	writel((2 << CSR_WAIT_WFI_SHIFT) | CSR_ENABLE, &flow->cpu1_csr);
	writel((4 << CSR_WAIT_WFI_SHIFT) | CSR_ENABLE, &flow->cpu2_csr);
	writel((8 << CSR_WAIT_WFI_SHIFT) | CSR_ENABLE, &flow->cpu3_csr);

	writel(EVENT_MODE_STOP, &flow->halt_cpu1_events);
	writel(EVENT_MODE_STOP, &flow->halt_cpu2_events);
	writel(EVENT_MODE_STOP, &flow->halt_cpu3_events);

	while (readl(&pmc->pmc_pwrgate_status) & ((1 << TEGRA_POWERGATE_CPU1) |
						  (1 << TEGRA_POWERGATE_CPU2) |
						  (1 << TEGRA_POWERGATE_CPU3)))
		/* wait */;
}
