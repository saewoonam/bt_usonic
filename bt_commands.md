# bluetooth commads for rw characteristic

- 'f': put device in a mode to transfer data from flash over bluetooth, normally entire memory starting from a memory address sest by 'O'.   This offset needs to be set prior to using 'f'
- 'F': take device out of the mode to transfer data from flash over bluetoooth
- 'g': get flash memory address with oldest data NOT uploaded to the server.
- 'G': set device so that the 'f' transfers data not uploaded to the server.
- 'Z': set the device so that the 'f' transfers all the data (start from flash memory address 0)
- 'Y': mark in the flash memory that all the data was uploaded to the server
- 't': put device in mode to transfer data faster... does not work
- 'T': take device out of mode 't' above.  
- 'u': retrieve mtu packet size, read result from SPP_data characteristic
- 'e': start transmitting recent encounters from the last 10 minutes or last 31 which ever is shorter
- 'E': put device in ENCOUNTER mode... not relevant for this firmware
- 'R': put device in RAW mode... not relevant for this firmware

- 'w': start storing encounters to flash
- 's': stop storing encounters to flash

- 'W': play alarm over speaker

- 'A': send how long device has been up (since last reboot) in milliseconds, comes over as two 32bit littlen endian numbers.   First number is least significant byte.   Second 32bit number is the number of times the first number has overflowed.  The counter on the device is only 32bits... It overflows every ~36 days or so... **The overflow value is not the full value of the 32bit number.**

- 'a': send over time synchronization stored on the device... Three 32bit little endian numbers:  number 1:  epochtime in seconds, numbers 2 and 3 are boot time corresponding to the epoch time given by number 1

- 'C': erase flash

- 'S': play 3 beeps on the speaker

- 'B': set bluetooth scan parameters:  interval and window: two uint16  not implemented

- 'P': set bluetooth connection paramters, three uint16, not implemented

- 'b': set bluetooth advertising parameters: interval and window: two uint16 (probably works)

- 'O': set time sync for bluetooth device (see 'a'... this reads back the numbers set here).  Send three number in SPP prior to sending this command... Three numbers are 32bit unsigned integers, little endian.  First number is the Unix epoch time in seconds, the second and third numbers are the uptime of the device corresponding to the epoch time.

- 'N': set name for the device, 8 characters, use NISTXXXX where XXXX is a 4 digit number

- 'M': Put a mark (special event) in the data stored in the flash memory.   Can be used to indicate close to another device
- 'U': Put an "unmark" special event in the data stored in the flash memory.  Can be used to indicate no longer close to another device
- 'h': tell device to print version string to usb serial port
- 'v': tell device to send ota_version string over bluetooth
- '0': Send ths command to the device to tell it the master making requests is another bluetooth device... Used in ultrasonic protocol

- '1': Send this command to tell the device the master making requests is a computer, disables ultrasonic pings

