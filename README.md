
## focaltech-dkms

FocalTech PS/2 (FTE0001) TouchPad DKMS Driver, Tested on Ubuntu 19.10 running kernel version 5.30

This DKMS Module only works with `FTE0001` TouchPad found in `Haier Y11C` and **doesn't work** with older FocalTech Devices (FLT0101, FLT0102, FLT0103)

### ChangeLog

* Fixed a bug that rejected input at specific locations on Touchpad

* Improved Cursor Movement

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
