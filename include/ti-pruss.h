// SPDX-License-Identifier: GPL-2.0

struct pruss {
	phys_addr_t pruss_dram0;
	phys_addr_t pruss_dram1;
	phys_addr_t pruss_shrdram2;
	phys_size_t pruss_dram0sz;
	phys_size_t pruss_dram1sz;
	phys_size_t pruss_shrdram2sz;
};

int pruss_request_shrmem_region(struct udevice *dev, phys_addr_t *loc);
int pruss_request_tm_region(struct udevice *dev, phys_addr_t *loc);
