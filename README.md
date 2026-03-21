# OTA - Over The Air update system

A reliable way to update firmware for deployed products without needing human intervention in the field.

## Description

We plan to implement an Over-The-Air (OTA) update system using an A/B swap. The pipeline of connectivity is as follows: connect the STM32 microcontroller to a Raspberry Pi Pico via UART, then to the Wi-Fi chip via the established protocol on the Pico, and finally to a server over Wi-Fi, which will serve an application. The basic idea is that there is a bootloader in the microcontroller that will be in charge of downloading the program to run, storing it in flash, and then booting from the new location in flash. This can be done by writing to the memory space directly from data sent by the remote server, or by writing to a second flash space which is not live, and then once the transmission is complete, rebooting and choosing the newly written flash memory space to boot into.

## Authors

Nick Baret  
Anthony Lesik  
Sameeran Chandorkar 
Jeffrey Hansen 

## Version History

0.0 Initial repository setup
