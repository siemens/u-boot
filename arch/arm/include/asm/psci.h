/*
 * Copyright (C) 2013 - ARM Ltd
 * Author: Marc Zyngier <marc.zyngier@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ARM_PSCI_H__
#define __ARM_PSCI_H__

#define ARM_PSCI_FN32_BASE		0x84000000
#define ARM_PSCI_FN(n)			(ARM_PSCI_FN32_BASE + (n))

#define ARM_PSCI_FN_PSCI_VERSION	ARM_PSCI_FN(0)
#define ARM_PSCI_FN_CPU_SUSPEND		ARM_PSCI_FN(1)
#define ARM_PSCI_FN_CPU_OFF		ARM_PSCI_FN(2)
#define ARM_PSCI_FN_CPU_ON		ARM_PSCI_FN(3)
#define ARM_PSCI_FN_AFFINITY_INFO	ARM_PSCI_FN(4)
#define ARM_PSCI_FN_MIGRATE		ARM_PSCI_FN(5)
#define ARM_PSCI_FN_MIGRATE_INFO_TYPE	ARM_PSCI_FN(6)
#define ARM_PSCI_FN_MIGRATE_INFO_UP_CPU	ARM_PSCI_FN(7)
#define ARM_PSCI_FN_SYSTEM_OFF		ARM_PSCI_FN(8)
#define ARM_PSCI_FN_SYSTEM_RESET	ARM_PSCI_FN(9)

#define ARM_PSCI_RET_SUCCESS		0
#define ARM_PSCI_RET_NI			(-1)
#define ARM_PSCI_RET_INVAL		(-2)
#define ARM_PSCI_RET_DENIED		(-3)
#define ARM_PSCI_RET_ALREADY_ON		(-4)
#define ARM_PSCI_RET_ON_PENDING		(-5)
#define ARM_PSCI_RET_INTERNAL_FAILURE	(-6)
#define ARM_PSCI_RET_NOT_PRESENT	(-7)
#define ARM_PSCI_RET_DISABLED		(-8)

#endif /* __ARM_PSCI_H__ */
