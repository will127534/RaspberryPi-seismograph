# RaspberryPi-seismograph
A simple seismograph using Raspberry Pi

## Usage

### /Software/RaspberryCode

C code for reading data from ADS1256 and logging to files, approx 750Hz sampling rate for each of the three channel, and switching file per 5 minutes.

The code is slightily modified from the example code of https://www.waveshare.com/wiki/High-Precision_AD/DA_Board

Besure to open SPI interface before usage.

### Software/plotgen.py
Generate one day plot from the data, with CWB earthquake event overlay.
It will automatically download the files using rclone and process it then upload back to the cloud using rclone, it is designed for server to process the data automatically.
And rclone needs to setup before usage.
it will generate somthing like this:
![](https://i.imgur.com/LQMdwf7.png)

### Software/rclone-backup.sh
Shell script for automatically upload the data to cloud, since the data from sensor is pretty large, it is better to upload it to cloud.
The script is using rclone to copy the data, and remove the old data(>12Hr) from local 

### Hardware
Just a dxf file for baseplate to put the sensor together.
