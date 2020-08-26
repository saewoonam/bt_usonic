#  Open source code to implement ultrasonic ranging with a piezo speaker for exposure notification using a silabs Thunderboard

##  Hardware Requirements
1.  Silabs Thunderboard - EFR32BG22, SLBT010A
2.  Piezo speaker connected to expansion port 15, GPIO PD03


## Firmware Programming
1.  Used Simplicity Studio



## Design information

### Basic Scheme
- Use bluetooth connection between two devices
- Use connection because advertising and scan response was not reliable
- Master / Client always records
- Slave / Server always sends audio burst
- Bluetooth packet sent on the connection is assumed to be instantaneous
- Measure time delay of received audio tone
- Audio tone is simple 1ms duration audio tone at a fixed square wave frequency
- Things considered:
    -- sine wave frequency chirps, not enough memory
    -- sq. wave frequency chirps, x-correlation not good enough
    -- sigma-delta generated tones



