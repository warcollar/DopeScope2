## Setting up your environment
- Install the arduino development environment:
https://www.arduino.cc/en/main/software
- Follow the guide for installing the Arduino core for the ESP32:
https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md
- Using the Arduino Library Manager (Tools->Manage Libraries), install the following extra libraries:
Adafruit GFX Library (>= 1.5.6)
ArduinoJSON (= 5.13.5)
- You may also want to add the following utilities:
    - ESP32 Exception Decoder - This will help you debug code that throws exceptions.  https://github.com/me-no-dev/EspExceptionDecoder
    - Arduino ESP32 Filesystem Uploader - You can use this to upload files to the extra space on the ESP.  https://github.com/me-no-dev/arduino-esp32fs-plugin
- Configure the Arduino IDE Board configuration.  Under the Tools menu, set the following:
    - Board: ESP32 Dev Module
    - Partition Scheme: No OTA (2MB APP/2MB SPIFFS)
    - Port: Make sure this is the USB/Serial port that the Dopescope showed up as.

