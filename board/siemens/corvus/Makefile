#
# Makefile for siemens CORVUS (AT91SAM9G45) based board
# (C) Copyright 2013 Siemens AG
#
# Based on:
# U-Boot file: board/atmel/at91sam9m10g45ek/Makefile
#
# (C) Copyright 2003-2008
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# (C) Copyright 2008
# Stelian Pop <stelian@popies.net>
# Lead Tech Design <www.leadtechdesign.com>
#
# SPDX-License-Identifier:	GPL-2.0+
#


include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).o

COBJS-y += board.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS-y:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS-y))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(obj).depend $(OBJS) $(SOBJS)
	$(call cmd_link_o_target, $(OBJS) $(SOBJS))

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
