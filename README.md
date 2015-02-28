#Slooper

##Overview
- **Slooper** turns your raspberry pi into a seamless HD video looper. (no pause, hickup between loop iterations)
- **Slooper** will play a video from a usb drive. Changing the video means simply replacing the video file on the usb drive.
- **Slooper** will cleanly boot into the video to turn your pi into a video playback kiosk.
- **Slooper** will allow you to turn the pi on and off with a remote.
- **Slooper** comes in the form of a python install script that modifies an existing Raspbian installation which is a lot more maintainable than creating a custom image.
- **Slooper** does **not** support audio right now.

##What do you need
- A Raspberry Pi and a fitting SD card, power supply etc.
- A remote board: http://www.msldigital.com/collections/all-products
- Any IR remote.
- A USB thumb drive

##What is contained in this repository?
- *InstallSlooperWithRemote.py* is the installation script to modify your clean Raspbian image.
- *RemoteVideo.py* is the python script that is installed by the installation script to play the video and handle the remote. It exists so you can test it separately.
**NOTE:** The script is hardcoded in *InstallSlooperWithRemote.py* right now, so changing RemoteVideo.py won't change any installation behavior.

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
- **Done**, you should be able to turn the Pi on and off using your remote now and it should nicely boot into the video on the USB stick.

##What does the installation script do?
- The installation script is losely based on this article: http://curioustechnologist.com/post/90061671996/rpilooper-v2-seamless-video-looper-step-by-step , fully automating the steps that are necessary while adding some extra scripts and things to handle the remote.

### So what is happening in deatail?
- It will update your Raspbian.
- It will build all the pi software examples.
- It will slightly change the *hello_pi/video* example to perform the seamless looping. This will be used to playback the video. (I did a lot of research to see what method provides the best seamless loop, omxplayer, openFrameworks etc.. The hello_video example performed best by far! That's also why there is now audio support right now)
- It will create a mounting point for the USB drive and make sure it mounts on every boot.
- It will install a python script called *RemoteVideo.py* to */home/pi/Scripts*. This script will start and stop the video and handle the remote.
- It will add that script to the startup items (by changing */etc/rc.local*).
- It will adjust */boot/cmdline.txt* to hide the Pi logo and console text while booting.

For implementation details, check out the actual script!