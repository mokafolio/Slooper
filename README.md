#Slooper

##Overview
- **Slooper** turns your raspberry pi into a seamless HD video looper. (no pause, hickup between loop iterations)
- **Slooper** will play a video from a usb drive. Changing the video means simply replacing the video file on the usb drive.
- **Slooper** will cleanly boot into the video to turn your pi into a video playback kiosk.
- **Slooper** will allow you to turn the pi on and off with a remote.
- **Slooper** does **not** support audio right now.

##What do you need
- A Raspberry Pi and a fitting SD card, power supply etc.
- A remote board: http://www.msldigital.com/collections/all-products
- Any IR remote.
- A USB thumb drive

##Installation
- Install the remote board on your raspberry pi as described in the *Installation* section here: http://www.msldigital.com/pages/support-for-remotepi-board-plus-2015/
- Download the most recent *Raspbian* image from the raspberry pi website: http://www.raspberrypi.org/downloads/
- Install the image on the SD card of your choice as described here: http://www.raspberrypi.org/documentation/installation/installing-images/mac.md
- Connect the Pi to the internet (with an ethernet cable).
- Start up your raspberry pi by plugging the power into its power slot.
- Boot up your pi and perform all the *raspi-config* steps that you want to do (look here for more info: http://www.raspberrypi.org/documentation/configuration/raspi-config.md)
- Run `sudo apt-get install git` to make sure git is up to date.
- Enter the directory that you want to clone the **Slooper** repository to, i.e. `cd /home/pi`
- Clone this git repository `git clone https://github.com/mokafolio/Slooper.git`
- Enter the repository directory, i.e. `cd Slooper`
- Run the installation script: `sudo python InstallSlooperWithRemote.py`, this will take a while.
- Shut down the pi: `sudo poweroff`
- Replug the power to the remote pi board.
- To install the powerbutton of your remote, do the following: *Choose the button on the remote you want to use to switch your RPi on and off in the future, then press the hardware button on top of the RemotePi Board for about 10 seconds until you see the LED blinking green and red. Now you have about 20 seconds time to aim the remote towards the RemotePi Board’s infrared receiver and press the button you want to use for powering on your RPi. You will see the green LED flashing once when the button was learned. If no infrared command is learned within 20 seconds you see the red LED flash and the learning mode is exited without changing the current configuration.*
