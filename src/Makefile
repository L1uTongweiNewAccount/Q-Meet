ARDUINO := arduino/cores/arduino/CDC.cpp arduino/cores/arduino/HardwareSerial.cpp \
	arduino/cores/arduino/HardwareSerial0.cpp arduino/cores/arduino/HardwareSerial1.cpp \
	arduino/cores/arduino/HardwareSerial2.cpp arduino/cores/arduino/HardwareSerial3.cpp \
	arduino/cores/arduino/IPAddress.cpp arduino/cores/arduino/PluggableUSB.cpp \
	arduino/cores/arduino/Print.cpp arduino/cores/arduino/Stream.cpp \
	arduino/cores/arduino/Tone.cpp arduino/cores/arduino/USBCore.cpp \
	arduino/cores/arduino/WInterrupts.c arduino/cores/arduino/WMath.cpp \
	arduino/cores/arduino/WString.cpp arduino/cores/arduino/abi.cpp \
	arduino/cores/arduino/hooks.c arduino/libraries/SPI/src/SPI.cpp \
	arduino/cores/arduino/new.cpp arduino/cores/arduino/wiring.c \
	arduino/cores/arduino/wiring_analog.c arduino/cores/arduino/wiring_digital.c \
	arduino/cores/arduino/wiring_pulse.c arduino/cores/arduino/wiring_shift.c
build:
	avr-gcc -Ofast -fomit-frame-pointer -ffunction-sections -fdata-sections -Imicro-ecc micro-ecc/uECC.c -DF_CPU=16000000UL -mmcu=atmega328p -c -o uECC.o
	avr-g++ -Os -ffunction-sections -fdata-sections -Wl,--gc-section,-flto,-Map=qmeet.map -o qmeet.elf -DF_CPU=16000000UL -mmcu=atmega328p \
		main.cpp aes256/aes256.c uECC.o sha-2/sha-256.c $(ARDUINO) -Imicro-ecc -Iarduino/variants/standard \
		-Iaes256 -Iarduino/cores/arduino -Iarduino/libraries/SPI/src -Imt19937ar -Isha-2 -std=c++14
	avr-objcopy -R .eeprom -S -O ihex qmeet.elf qmeet.hex
	avr-size qmeet.elf
clean:
	-rm qmeet.map qmeet.elf qmeet.hex *.o