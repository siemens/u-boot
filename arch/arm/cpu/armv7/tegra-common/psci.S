/*
 * Copyright (C) 2014, NVIDIA
 * Copyright (C) 2015, Siemens AG
 *
 * Authors:
 *  Thierry Reding <treding@nvidia.com>
 *  Jan Kiszka <jan.kiszka@siemens.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/linkage.h>
#include <asm/psci.h>

	.pushsection ._secure.text, "ax"
	.arch_extension sec

#define TEGRA_SB_CSR_0			0x6000c200
#define NS_RST_VEC_WR_DIS		(1 << 1)

#define TEGRA_RESET_EXCEPTION_VECTOR	0x6000f100

#define TEGRA_FLOW_CTRL_BASE		0x60007000
#define FLOW_CTRL_CPU_CSR		0x08
#define CSR_ENABLE			(1 << 0)
#define CSR_IMMEDIATE_WAKE		(1 << 3)
#define CSR_WAIT_WFI_SHIFT		8
#define FLOW_CTRL_CPU1_CSR		0x18

@ converts CPU ID into FLOW_CTRL_CPUn_CSR offset
.macro get_csr_reg cpu, ofs, tmp
	cmp	\cpu, #0		@ CPU0?
	lsl	\tmp, \cpu, #3	@ multiple by 8 (register offset CPU1-3)
	moveq	\ofs, #FLOW_CTRL_CPU_CSR
	addne	\ofs, \tmp, #FLOW_CTRL_CPU1_CSR - 8
.endm

ENTRY(psci_arch_init)
	mrc	p15, 0, r5, c1, c1, 0	@ Read SCR
	bic	r5, r5, #1		@ Secure mode
	mcr	p15, 0, r5, c1, c1, 0	@ Write SCR
	isb

	@ lock reset vector
	ldr	r6, =TEGRA_SB_CSR_0
	ldr	r5, [r6]
	orr	r5, r5, #NS_RST_VEC_WR_DIS
	str	r5, [r6]

	mrc	p15, 0, r4, c0, c0, 5	@ MPIDR
	and	r4, r4, #7		@ number of CPUs in cluster
	mov	r5, #400		@ 1 KiB of stack per CPU
	mul	r4, r4, r5

	adr	r5, text_end		@ end of text
	add	r5, r5, #0x2000		@ Skip two pages
	lsr	r5, r5, #12		@ Align to start of page
	lsl	r5, r5, #12
	sub	sp, r5, r4		@ here's our stack!

	bx	lr
ENDPROC(psci_arch_init)

ENTRY(psci_cpu_off)
	bl psci_cpu_off_common

	mrc	p15, 0, r1, c0, c0, 5		@ MPIDR
	and	r1, r1, #7			@ number of CPUs in cluster

	get_csr_reg r1, r2, r3

	ldr	r6, =TEGRA_FLOW_CTRL_BASE
	mov	r5, #(CSR_ENABLE)
	add	r5, r1, lsl #CSR_WAIT_WFI_SHIFT
	str	r5, [r6, r2]

_loop:	wfi
	b	_loop
ENDPROC(psci_cpu_off)

ENTRY(psci_cpu_on)
	ldr	r0, =_psci_target_pc
	str	r2, [r0]
	dsb

	ldr	r6, =TEGRA_RESET_EXCEPTION_VECTOR
	ldr	r5, =psci_cpu_entry
	str	r5, [r6]

	get_csr_reg r1, r2, r3

	ldr	r6, =TEGRA_FLOW_CTRL_BASE
	mov	r5, #(CSR_IMMEDIATE_WAKE | CSR_ENABLE)
	str	r5, [r6, r2]

	mov	r0, #ARM_PSCI_RET_SUCCESS	@ Return PSCI_RET_SUCCESS
	mov	pc, lr
ENDPROC(psci_cpu_on)

text_end:
	.popsection
