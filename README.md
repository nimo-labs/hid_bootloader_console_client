![Build status](https://github.com/nimo-labs/hid_bootloader_console_client/actions/workflows/build_lin_stable.yml/badge.svg)
![Build status](https://github.com/nimo-labs/hid_bootloader_console_client/actions/workflows/build_win_stable.yml/badge.svg?branch=dev)

# Command line client for the NIMO bootloader

[Bootloader repository](https://github.com/nimo-labs/m032HidBootloader)

## Usage

Usage examples:\n
Copy external flash to internal:
	hidBoot.py c
Device bootloader version:
	hidBoot.py d
Erase internal flash:
	hidBoot.py e
List supported controllers:
	hidBoot.py l
Write internal flash:
	hidBoot.py w 0x3000 blinky.bin
Write external flash:
	hidBoot.py x blinky.bin

