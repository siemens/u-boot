/*
 * Common board functions for siemens AM335X based boards
 * (C) Copyright 2013 Siemens Schweiz AG
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __FACTORYSET_H
#define __FACTORYSET_H

struct factorysetcontainer {
	uchar mac[6];
	int usb_vendor_id;
	int usb_product_id;
};

int factoryset_read_eeprom(int i2c_addr);
int factoryset_setenv(void);
extern struct factorysetcontainer factory_dat;

#endif /* __FACTORYSET_H */
