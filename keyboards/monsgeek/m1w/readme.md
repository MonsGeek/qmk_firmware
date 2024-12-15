# M1W VIA

* Keyboard Maintainer: [Priyadie](https://github.com/priyadie)
* Hardware Availability: [M1W VIA](https://www.monsgeek.com/product/m1w-via)

Make example for this keyboard (after setting up your build environment):

    make monsgeek/m1w:default

Flashing example for this keyboard:

    make monsgeek/m1w:default:flash

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

Enter the bootloader in 3 ways:

* **Bootmagic reset**: While holding down the key at (0,0) in the matrix (Escape), plug in the keyboard
* **Keycode in layout**: Press `Fn` + `Esc` (only possible after this firmware is flashed or if you mapped this previously)
* **Short `BOOT` and `VCC` pads**:
  * Disassemble the keyboard to gain access to the front of the keyboard PCB. This should reveal `BOOT` and `VCC` pads below the `N` and `M` keys
  * Connect the USB-C daughterboard to the keyboard PCB
  * While using a wire or pair of tweezers to short the `BOOT` and `VCC` pads, plug in the keyboard
