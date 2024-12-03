# SFP-Master
SFP-module data programmer for CH341a devices

![SFP-Master](img/screenshot.png) 

**SFP-Master** is a free software programmer of optical `SFP modules` for CH341a devices. It can be used to read, write and save SFP module data to the computer. It requires an SFP to I2C adapter. This adapter is used to read and program SFP-module data. It must be inserted into the slot labelled `24xxx` of the CH341a programmer.

![Adapter schematic](img/my_sfp_adapter_sch.png)

![Adapter schematic](img/my_sfp_adapter_3d.png)

- See more details [here](https://github.com/bigbigmdm/Tools_for_CH341A_programmer?tab=readme-ov-file#Homemade-Chip-adapters).

Jumpers J1 to J3 must be installed initially. They are used to supply power to the SFP module. If you want to programm a module with hardware write protection, remove one of the jumpers and try to programm the module. If it fails, remove the other jumper and repeat the operation.

### Linux

For build are needed:
- g++ or clang
- CMake
- libusb 1.0
- Qt5
- Qt5 Qt5LinguistTools
- pkgconf or pkg-config
- udev

On Debian and derivatives:

`sudo apt-get install cmake g++ libusb-1.0-0-dev qtbase5-dev qttools5-dev pkgconf`

On Debian >=13 and Ubuntu >=23.10:

`sudo apt-get install systemd-dev`

On older:

`sudo apt-get install udev`

For compilling and installing the SFP-Master type:

`sudo ./builb_all.sh`

For uninstalling type:

`sudo ./uninstall.sh`
