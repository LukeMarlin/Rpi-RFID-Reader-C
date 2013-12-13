C Raspberry Pi Wiegand RFID controller
======================================


C program to connect multiple Wiegand RFID Cards Readers on _Raspberry Pi_ using the _wiringPi_ library.

This program was written for Rev1 Raspberry Pi GPIO. For Rev2 raspi, replace the references of GPIO numbers.

##Prerequisites

* gcc
* wiringPi library

##How to

1. Clone the repo
2. For each reader you connect, add a call to `createCardReader(...)` in `initReaders(...)` (in main.c) like in this template : `createCardReader("Name of the reader", GPIO_0, GPIO_1, &callback0, &callback1)` where GPIO\_0 and GPIO\_1 is respectively the number of GPIO connected to D0 and D1 of the RFID wiegand reader. Don't forget to adapt the number of the callback with the corresponding pin number.
3. Compile with the flowing command :
`gcc -lwiringPi -lrt -o <name> main.c`
4. Run the program (in root) with :
`./<name>`

##Connection diagram

![Connection diagram](https://github.com/LukeMarlin/Rpi-RFID-Reader-C/blob/master/Diagram.png?raw=true "Connection diagram")


##Links

* [wiringPi Library](http://wiringpi.com "Link to the wiringPi Library home page")
* [Raspberry Pi](http://www.raspberrypi.org "Link to the Raspberry Pi projet home page")
