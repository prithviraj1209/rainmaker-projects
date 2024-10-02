# Switch Project

Clone this repository into the esp-rainmaker/examples folder

## Before building and flashing 

Note whether your relays are active-low or active-high and update that in the app_driver.c file accordingly.
Change the Relay and Button GPIOs set in the Kconfig.projbuild file according to your need.

## Build and Flash firmware

Follow the ESP RainMaker Documentation [Get Started](https://rainmaker.espressif.com/docs/get-started.html) section to build and flash this firmware. Just note the path of this example.

## What to expect from this program?

Once the board is wifi provisioned, you can control the devices via
+ RainMaker mobile application when connected to the internet.
+ Physical buttons.
+ Additionally you can also control the devices from the mobile application even if the wifi has no internet access.
  (Note: The mobile device and the esp device must be connected to the same wifi.)

You will receive various updates about the devices on your serial monitor.

### What is different in this project?

+ Not made using Arduino IDE.
+ Device state is recovered after a power cut(NVS).
+ Can control using ESP Rainmaker mobile application even without internet access.

### Reset wifi

Press and hold the BOOT button for more than 3 seconds to reset the wifi credentials. You will have to follow the wifi provisioning process again to connect to wifi.

### Reset to factory 

Press amd hold the BOOT button for more than 10 seconds to reset the board to factory defaults. You will have to provision the board again to use it.
