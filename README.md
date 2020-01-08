
## focaltech-dkms

FocalTech PS/2 (FTE0001) Touchpad DKMS Driver, Tested on Ubuntu 19.10 running kernel version 5.30

This DKMS Module is for Haier Y11C, ACPI Device name `FTE0001`

Horizontal Edge Scrolling will not work as libinput require height>=40mm

### ChangeLog
* Fixed Device Resolution
* Improved Cursor Movement
* Fixed a bug that rejected input at specific locations on Touchpad

### Installation
```
sudo ./dkms-install.sh
```
### Uninstallation
```
sudo ./dkms-remove.sh
```

### Credits

* Seth Forshee – [Touchpad Protocol Reverse Engineering](http://www.forshee.me/2011/11/18/touchpad-protocol-reverse-engineering.html)
* Mathias Gottschlag – [focaltech-tools](https://github.com/mgottschlag/focaltech-tools)
* Peter Hutterer – [libinput](https://gitlab.freedesktop.org/libinput/libinput)
