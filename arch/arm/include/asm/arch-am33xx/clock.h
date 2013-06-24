/*
 * clock.h
 *
 * clock header
 *
 * Copyright (C) 2011, Texas Instruments Incorporated - http://www.ti.com/
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

#ifndef _CLOCKS_H_
#define _CLOCKS_H_

#include <asm/arch/clocks_am33xx.h>

#define LDELAY 1000000

/* CM_CLKMODE_DPLL */
#define CM_CLKMODE_DPLL_REGM4XEN_SHIFT		11
#define CM_CLKMODE_DPLL_REGM4XEN_MASK		(1 << 11)
#define CM_CLKMODE_DPLL_LPMODE_EN_SHIFT		10
#define CM_CLKMODE_DPLL_LPMODE_EN_MASK		(1 << 10)
#define CM_CLKMODE_DPLL_RELOCK_RAMP_EN_SHIFT	9
#define CM_CLKMODE_DPLL_RELOCK_RAMP_EN_MASK	(1 << 9)
#define CM_CLKMODE_DPLL_DRIFTGUARD_EN_SHIFT	8
#define CM_CLKMODE_DPLL_DRIFTGUARD_EN_MASK	(1 << 8)
#define CM_CLKMODE_DPLL_RAMP_RATE_SHIFT		5
#define CM_CLKMODE_DPLL_RAMP_RATE_MASK		(0x7 << 5)
#define CM_CLKMODE_DPLL_EN_SHIFT		0
#define CM_CLKMODE_DPLL_EN_MASK			(0x7 << 0)

#define CM_CLKMODE_DPLL_DPLL_EN_SHIFT		0
#define CM_CLKMODE_DPLL_DPLL_EN_MASK		7

#define DPLL_EN_STOP			1
#define DPLL_EN_MN_BYPASS		4
#define DPLL_EN_LOW_POWER_BYPASS	5
#define DPLL_EN_LOCK			7

/* CM_IDLEST_DPLL fields */
#define ST_DPLL_CLK_MASK		1

/* CM_CLKSEL_DPLL */
#define CM_CLKSEL_DPLL_M_SHIFT			8
#define CM_CLKSEL_DPLL_M_MASK			(0x7FF << 8)
#define CM_CLKSEL_DPLL_N_SHIFT			0
#define CM_CLKSEL_DPLL_N_MASK			0x7F

struct dpll_params {
	u32 m;
	u32 n;
	s8 m2;
	s8 m3;
	s8 m4;
	s8 m5;
	s8 m6;
};

struct dpll_regs {
	u32 cm_clkmode_dpll;
	u32 cm_idlest_dpll;
	u32 cm_autoidle_dpll;
	u32 cm_clksel_dpll;
	u32 cm_div_m2_dpll;
	u32 cm_div_m3_dpll;
	u32 cm_div_m4_dpll;
	u32 cm_div_m5_dpll;
	u32 cm_div_m6_dpll;
};

extern const struct dpll_regs dpll_mpu_regs;
extern const struct dpll_regs dpll_core_regs;
extern const struct dpll_regs dpll_per_regs;
extern const struct dpll_regs dpll_ddr_regs;
extern const struct dpll_params dpll_mpu;
extern const struct dpll_params dpll_core;
extern const struct dpll_params dpll_per;
extern const struct dpll_params dpll_ddr;

extern const struct cm_wkuppll *cmwkup;

void setup_dplls(void);

#endif
