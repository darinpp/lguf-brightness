# Adjust brightness of LG UltraFine displays

Compiles and works on Ubuntu 16.04 and 18.10

Based on these two projects:
- https://github.com/unknownzerx/lguf-brightness
- https://github.com/csujedihy/LG-Ultrafine-Brightness

## Instructions to compile and run

In order to compile this project, you will need `g++`, `cmake`, and `libusb-1.0-0-dev`.

### Commands to compile

* `cd build`
* `cmake ..`
* `make`

### In order to run this program, you need administrator privileges

`sudo ./lguf_brightness 0 100`

Compared to the original version, this version ditches curses and takes two
command line arguments. The first is the index of the display you want to change
the brightness for (0 would be the first, 1 would be the second, etc.) followed
by a number between 0 and 100 representing the percent of max brightness to
apply. So the above example describes setting the first display to maximum
brightness.

Beware, setting brightness to 0 will turn backlighting off.

You can also apply the same brightness settings to all displays by specifying
`all` instead of the display index. So setting all displays to 50% brightness
would look like this:

`sudo ./lguf_brightness all 50`
