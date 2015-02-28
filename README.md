#Slooper

##Overview
- **Slooper** turns your raspberry pi into a seamless video looper.

##Installation
- Download the most recent *Raspbian* image from the raspberry pi website: http://www.raspberrypi.org/downloads/
- Install the image on the SD card of your choice as described here: http://www.raspberrypi.org/documentation/installation/installing-images/mac.md 
- Start up your raspberry pi and connect it to a network and the internet.
- Boot up your pi and perform all the *raspi-config* steps that you want to do (look here for more info: http://www.raspberrypi.org/documentation/configuration/raspi-config.md)
- Run `sudo apt-get install git` to make sure git is up to date.
- Enter the directory that you want to clone the **Slooper** repository to, i.e. `cd /home/pi`
- Clone this git repository `git clone https://github.com/mokafolio/Slooper.git`
- Enter the repository directory, i.e. `cd Slooper`
- Run the installation script: `sudo python InstallSlooperWithRemote.py`, this will take a while.
- Shut down the pi: `sudo poweroff