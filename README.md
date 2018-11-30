# chipwhisperer_XTEA

This project implements side channel analysis attack for the XTEA algorithm.
It is an entry for the NewAE 2018 contest.

https://en.wikipedia.org/wiki/XTEA
Because is a fairly small symmetric block cipher implementation it is a good candidate for boot-loaders and other embedded projects. With a 128bit key offers a good level of security. Question is how secure is when running on a 32bit microcontroller? Side channel attacks are like magic. First you don't believe your eyes when you recover something from an 8 bit processor. Then when you can convince people that is possible, arises the question of the 32bit architecture. Should be harder, if not impossible.
Harder?, yes, impossible No.
So lets build everything from ground up, and see how secure we are if we use XTEA in something.

The hardware: 
For the tests i used a Mikroelektronika MIN-32 
https://www.mikroe.com/mini-pic32mx
The board features a PIC32MX534F064H Microchip microcontroller. The board was programmed whit PICKIT3, erasing the Mikroelektronika bootloader.
Target software was developed in MPLAB X IDE , compiled with XC32 1.40 and legacy pic32 plib libraries (on Microchip`s website it is present in the archives). 
Target is clocked from the chipwhisperer. Clock is routed  on the target trough the PLL to further complicate things.

The target implements a simple XTEA decryption, using the Simpleserial protocol, baudrate 19200. Trigger is wired from an IO toggle(RE6). 
UART is connected to RD2 RD3 (UART1)
Power consumption is measured on the VCORE pin. Capacitor E3 is removed and connected trough a  1 OHM resistor. The voltage drop is amplified with the differential probe. To the capacitor is applied a 1.96v external power supply to overpower the internal regulator. The exact voltage
depends on the chip. 
