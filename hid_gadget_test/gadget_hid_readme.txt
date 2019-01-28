Linux USB HID gadget driver

[Introduction] 
The HID Gadget driver provides emulation of USB Human Interface Devices (HID). The basic HID handling is done in the kernel, and HID reports can be sent/received through I/O on the /dev/hidgX character devices. For more details about HID, see the developer page on http://www.usb.org/developers/hidpage/

[Configuration in hid source code]
g_hid is a platform driver, so to use it you need to add struct platform_device(s) (E.G. my_hid_keyboard and my_hid_mouse) to your platform code defining the HID function descriptors you want to use. We provide 4 Standard HID devices sample and 1 Vendor Device sample here.

[Configuration in menuconfig]
 Device Drivers  ---> 
  [*] USB support  ---> 
      <*>   USB Gadget Support  --->  
  │ │          <M>     HID Gadget                                                                             │ │  
  │ │          <M>       Vendor Device                                                                        │ │  
  │ │          (0x04)      Vendor HID Enpoint Polling interval - High Speed                                   │ │  
  │ │          (0x0A)      Vendor HID Enpoint Polling interval - Full Speed                                   │ │  
  │ │          <M>       Standard Device(s)                                                                   │ │  
  │ │          <*>         Mouse Device                                                                       │ │  
  │ │          <*>         Keyboard Device                                                                    │ │  
  │ │          (0x04)      HID Enpoint Polling interval - High Speed                                          │ │  
  │ │          (0x0A)      HID Enpoint Polling interval - Full Speed                                          │ │  
  │ │          <M>     USB Webcam Gadget  

User can set the Endpoint poll interval (Frequency with which the driver (host) should initiate an interrupt transfer).

[HID module]
Vendor Device:g_hid_vendor.ko
Mouse / Keyboard Device(s):g_hid.ko

[Send and receive HID reports]
*HID reports can be sent/received using read/write on the /dev/hidgX character devices. See below for an example program to do this.
*hid_gadget_test is a small interactive program to test the HID gadget driver. To use, point it at a hidg device and set the device type (keyboard / mouse / joystick) - E.G.:

[Keyboard Demo]
# hid_gadget_test /dev/hidg0 keyboard

	You are now in the prompt of hid_gadget_test. You can type any combination of options and values. 
	Available options and values are listed at program start. In keyboard mode you can send up to six values.

For example type: g i s t r --left-shift

	Hit return and the corresponding report will be sent by the HID gadget.

	Another interesting example is the caps lock test. Type	-–caps-lock and hit return. 
	A report is then sent by the gadget and you should receive the host answer, corresponding to the caps lock LED status.

	--caps-lock

[Mouse Demo]
With this command:

# hid_gadget_test /dev/hidg1 mouse or # hid_gadget_test /dev/hidg0 mouse

	You can test the mouse emulation. Values are two signed numbers.

For example type (Press Mosue left key):
	--b1 

For example type (Press Mosue right key):
	--b2

For example type (Move to left & down):
	--b3 100 100

[Vendor Demo]
With this command and use PC Tool to send command and data:

# hid_gadget_test /dev/hidg0

Press any keyboard to start
A
Erase command - Sector: 0 Sector Cnt: 100
Erase command - Sector: 200 Sector Cnt: 300
HID_CmdReadPages 107008
HID_CmdWritePages 112640
  Compare Data - Size 112640
    Compare Pass!!

