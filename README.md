## Setting up your environment
- Install the arduino development environment:
https://www.arduino.cc/en/main/software
- Follow the guide for installing the Arduino core for the ESP32:
https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md
- Using the Arduino Library Manager (Tools->Manage Libraries), install the following extra libraries:
Adafruit GFX Library (>= 1.5.6)
ArduinoJSON (v6.12)
Install the Arduino ESP32 Filesystem Uploader - https://github.com/me-no-dev/arduino-esp32fs-plugin
- You may also want to add the following utility:
    - ESP32 Exception Decoder - This will help you debug code that throws exceptions.  https://github.com/me-no-dev/EspExceptionDecoder
- Configure the Arduino IDE Board configuration.  Under the Tools menu, set the following:
    - Board: ESP32 Dev Module
    - Partition Scheme: No OTA (2MB APP/2MB SPIFFS)
    - Port: Make sure this is the USB/Serial port that the Dopescope showed up as.

## Flashing ##
1.  Power on your DopeScope2 and connect it to your computer with the provided USB cable.

2. Double-click on the dopescope2.ino to bring up the Arduino IDE

3. Select Tools --> Port and select your DopeScope from the list of connected devices

4. Use the built-in arduino "Upload" command to compile and upload the software (the Arrow at the top left-hand side of the IDE) to the DopeScope.  
  -- Wait for the device to reboot.

5. Upload the BLE Manufacturer DB by selecting Tools --> ESP32 Sketch Data Upload.

6. Enjoy

## BLE Manufacturer Database ##
BLE devices use a vendor code to specify the manufacturer of a given device.  This is very similar to a MAC address OUI.  The database is maintained by the Bluetooth Special Interest Group (SIG) and is available here: (https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/).
This list is too large to put in program memory and would take too long for a microcontroller to do a normal lookup, so a utility is included to generate a ready-to-go solution.  When complete, a flat DB file is created where the name of each company is truncated to a fixed length and stored in the file at the offset corresponding to it's ID.

To create or update the database:

- Go to the [linked page](https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers/):
- Select the option to view all records (Be sure to leave the default sort option of highest ID to lowest)
- Select and copy the records
- Paste them into a file, eg. sourcefile.txt

Run the utiltiy, `manpacker.py`, with the following arguments:

`manpacker.py SOURCEFILE MAXLENGTH`

**SOURCEFILE** is the source tx file we created earlier: sourcefile.txt

**MAXLENGTH** is the size of each name.  Any names shorter that this are padded with spaces and any names longer are truncated to this length.  Keep in mind that the base display can't show more than ~23 chars, so there isn't much value in exceeding that.

When done, be sure to move the resulting manufacturers.db to the /data folder in the source tree so it can be uploaded to the DopeScope using the "ESP32 Sketch Data Upload" function
