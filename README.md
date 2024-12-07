# SFP-Master
SFP-module data programmer for CH341a devices

![SFP-Master](img/screenshot.png) 

**SFP-Master** is a free software programmer of optical `SFP modules` for CH341a devices. It can be 
used to read, write and save SFP module data to the computer. It requires an SFP to I2C adapter. 
This adapter is used to read and program SFP-module data. It must be inserted into the slot labelled 
`24xxx` of the CH341a programmer.

![Adapter schematic](img/my_sfp_adapter_sch.png)

![Adapter schematic](img/my_sfp_adapter_3d.png)

- See more details [here](https://github.com/bigbigmdm/Tools_for_CH341A_programmer?tab=readme-ov-file#Homemade-Chip-adapters).

Jumpers J1 to J3 (TxPWR, RxPWR, TxEN) must be installed initially. They are used to supply power to the SFP module. If you want 
to programm a module with hardware write protection, remove one of the jumpers and try to programm the module. 
If it fails, remove the other jumper and repeat the operation.

# Installing in Linux

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

To build and install the SFP-Master enter:

`sudo ./build_all.sh`

To uninstall, enter:

`sudo ./uninstall.sh`

# Connection

To work with the programmer, connect the SFP module to the connector in the SFP adapter, connect 
the SFP adapter to the CH341A programmer device to the slot marked `24xx`. Connect the CH341A 
Programmer Unit to the USB connector of the computer and start the `SFP-Master` programm.

# How to use
The hexadecimal chip editor (right side of the screen) is used to display and 
modify buffer data.

It contains the following controls: `Hex-Editor / Undo` or ![Undo](img/undo64.png) or `<Ctrl+Z>` undo and 
`Hex-Editor / Redo` or ![Redo](img/redo64.png) or `<Ctrl+Y>` redo.

- Pressing `SFP-module / Read from SFP` or ![Read](img/read64.png) or `<Ctrl+R>` to read data from 
the SFP-module into the computer buffer.
- Pressing `SFP-module / Write to SFP` or ![Write](img/write64.png) or `<Ctrl+W>` to write data from the 
computer buffer into the SFP-module.
- Press `SFP Module / Set Module Password` or ![Password](img/password64.png) or `<Ctrl+P>` to bring up
the password setting menu for modules that are password protected.
- The checkboxes are used to select the address area for read, write or save procedures. The yellow
checkbox is used for addresses 0x180 - 0x1FF, red for 0x100 - 0x17F, blue for 0x080 - 0x0FF, and
green for 0x000 - 0x07F (always checked).

The `Parse` button is used to re-parse hexadecimal data in the hex editor, if they have been changed manually.

The `Checksum` button is used to calculate two checksums (addresses 0x03F and 0x05F according to SFF-8472 Rev 12.3), 
if the module data have been changed manually in the hex editor.

Changing the data on the left side of the screen automatically causes the data to be changed in the hex editor.

- The `File / Save` or ![Save](img/save64.png) or `<Ctrl+S>` button is used to save the 
computer buffer to a file.
- The `File / Open`  or ![Open](img/open64.png) or `<Ctrl+O>` button is used to save the file in 
the computer buffer.
- The `File / Exit`  or ![Open](img/exit64.png) or `<Ctrl+X>` button is used to close the program. 

