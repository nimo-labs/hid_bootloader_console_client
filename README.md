![Build status](https://github.com/nimo-labs/hid_bootloader_console_client/actions/workflows/build_lin_stable.yml/badge.svg)
![Build status](https://github.com/nimo-labs/hid_bootloader_console_client/actions/workflows/build_win_stable.yml/badge.svg)

# Command line client for the NIMO bootloader

[Bootloader repository](https://github.com/nimo-labs/m032HidBootloader)

## Usage

Usage examples:\n
Copy external flash to internal:
	hidBoot c
Device bootloader version:
	hidBoot d
Erase internal flash:
	hidBoot e
List supported controllers:
	hidBoot l
Write internal flash:
	hidBoot w 0x3000 blinky.bin
Write external flash:
	hidBoot x blinky.bin

