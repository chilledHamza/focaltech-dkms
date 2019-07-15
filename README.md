## focaltech-dkms
FocalTech PS/2 (FTE0001) TouchPad DKMS Driver, Tested on kernel 4.19

This DKMS Module only works with 'FTE0001' TouchPad found in 'Haier Y11C' and **doesn't work** with older FocalTech Devices (FLT0101, FLT0102, FLT0103)

Installation
```
sudo dkms add ./focaltech-dkms
sudo dkms install -m focaltech -v 2.0
```
Uninstallation
```
sudo dkms remove focaltech/2.0 --all
```
