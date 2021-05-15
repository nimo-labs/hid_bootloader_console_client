# This file is part of the hid_bootloader_console_client distribution
# (https://github.com/nimo-labs/hid_bootloader_console_client).
# Copyright (c) 2021 Nimolabs Ltd. www.nimo.uk
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.


UNAME := $(shell uname)

SRCS = \
  main.c \
  hidBlProtocol.c \
  support.c

HDRS = \
  usb_hid.h \

ifeq ($(UNAME), Linux)
  BIN = hidBoot
  SRCS += usb_hid_lin.c
  LIBS += -ludev
else
  BIN = hidBoot.exe
  SRCS += usb_hid_win.c
  LIBS += -lhid -lsetupapi
endif

CFLAGS += -W -Wall -Wextra -O2 -std=gnu99
CFLAGS += -Wno-deprecated-declarations

all: $(BIN)

$(BIN): $(SRCS) $(HDRS)
	gcc $(CFLAGS) $(SRCS) $(LIBS) -o $(BIN)

clean:
	rm -rvf $(BIN) *~ *.*~

