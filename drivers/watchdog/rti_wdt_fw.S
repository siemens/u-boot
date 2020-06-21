// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) Siemens AG, 2020
 *
 * Authors:
 *   Jan Kiszka <jan.kiszka@siemens.com>
 */

.section .rodata

.global rti_wdt_fw
.global rti_wdt_fw_size

rti_wdt_fw:
.align 4
.incbin CONFIG_WDT_K3_RTI_FW_FILE
rti_wdt_fw_end:

rti_wdt_fw_size:
.int rti_wdt_fw_end - rti_wdt_fw
