#!/usr/bin/env python

import os
import subprocess

#helper function to log in color so we see stuff
def log(_text):
	print("\033[92m\033[1m" + _text + "\033[0m")

#this script os roughly an automated version of this step by step guide:
#http://curioustechnologist.com/post/90061671996/rpilooper-v2-seamless-video-looper-step-by-step

#make sure we are up to date
log("Updating raspbian.")
subprocess.call("sudo apt-get update", shell=True)
subprocess.call("sudo apt-get upgrade", shell=True)

#build the pi examples (we need the dependenvies for the video example)
log("Building the pi examples.")
os.chdir("/opt/vc/src/hello_pi/")
subprocess.call("./rebuild.sh", shell=True)

log("Changing the video example to do the seamless looping.")
#this will adjust the hello_video raspberry pi example to do seamless looping
os.chdir("/opt/vc/src/hello_pi/hello_video/")

 # Read in the file
filedata = None
f = open("video.c", "r")
filedata = f.read()
f.close()

#replace a piece of code to loop instead of end the app
filedata = filedata.replace("""if(!data_len)
            break;""", "if(!data_len) fseek(in, 0, SEEK_SET);")

#print filedata
f = open("video.c", "w")
f.write(filedata)
f.close()

log("Rebuild the video example.")
#build the example
subprocess.call("make", shell=True)

#make an auto mounting usb thingy (this is where we will look for videos)
#if the path already exists, we simply assume, that we allready did this...and don't do it again
log("Creating a mounting point to play the Video from USB.")
if not os.path.exists("/home/pi/USBVideoMount"):
	os.mkdir("/home/pi/USBVideoMount")
	subprocess.call("mount /dev/sda1 /home/pi/USBVideoMount", shell=True)

log("Changing /etc/fstab to auto mount the USB.")
#make sure this mounts on every boot up
f = open("/etc/fstab", "a+")
filedata = f.read()
if not "/dev/sda1 /home/pi/USBVideoMount vfat defaults 0 0" in filedata:
	f.write("/dev/sda1 /home/pi/USBVideoMount vfat defaults 0 0")
f.close()

#now install the remote script
remoteScript = """#!/usr/bin/env python

import RPi.GPIO as GPIO
import time
from datetime import datetime, timedelta
import subprocess
import signal
import os

def log(_str, _bTimeStamp = True):
	logFilePath = "/home/pi/VideoLog.txt"

	if os.path.exists(logFilePath):
		with open(logFilePath, "r") as fin:
			data = fin.read().splitlines(True);
		if len(data) > 100:
			diff = len(data) - 99
			with open(logFilePath, "w") as fout:
				fout.writelines(data[diff:])

	logFile = open("/home/pi/VideoLog.txt", "a")
	message = ""
	if _bTimeStamp:
		message += str(datetime.now()) + ": "
	message += _str
	print message
	logFile.write(message + "\\n")

pin=14

#GPIO pin stuff based on the instructions from the remote board people here:
#http://www.msldigital.com/pages/support-for-remotepi-board-plus-2015/
GPIO.setmode(GPIO.BCM)

#GPIO.setup(pin, GPIO.OUT)
#GPIO.output(pin, 1)
GPIO.setup(pin, GPIO.IN)

log("Starting Video")
videoProcess = subprocess.Popen("/opt/vc/src/hello_pi/hello_video/hello_video.bin /home/pi/USBVideoMount/*.m4v", stdout=subprocess.PIPE, shell=True, preexec_fn=os.setsid)

startTime = datetime.now()
while True:
	bQuit = False
	if datetime.now() - startTime >= timedelta(hours=16):
		log("Sixteen hours have passed, we quit now")
		#this is the shutdown sequence of the remote board, see the site linked above
		offPin = 15
		GPIO.setup(offPin, GPIO.OUT)
		GPIO.output(offPin, 1)
		time.sleep(0.125)
		GPIO.output(offPin, 0)
		time.sleep(0.2)
		GPIO.output(offPin, 1)
		time.sleep(0.4)
		GPIO.output(offPin, 0)
		bQuit = True
	if GPIO.input(pin) != 0:
		log("Remote pressed, we quit now")
		bQuit = True

	if bQuit:
		os.killpg(videoProcess.pid, signal.SIGTERM)
		GPIO.setup(pin, GPIO.OUT)
		GPIO.output(pin, 1)
		time.sleep(3)
		break
	time.sleep(0.1)

GPIO.cleanup()
log("Shutting down")
subprocess.call("sudo poweroff", shell=True)
"""
log("Installing RemoteVideo.py script.")
if not os.path.exists("/home/pi/Scripts"):
	os.mkdir("/home/pi/Scripts")

f = open("/home/pi/Scripts/RemoteVideo.py", "w")
f.write(remoteScript)
f.close()

# make the remote video script executable
subprocess.call("chmod +x /home/pi/Scripts/RemoteVideo.py", shell=True)

# add the remote script to the start up items
log("Add RemoteVideo.py to /etc/rc.local to run after booting.")
f = open("/etc/rc.local", "r")
#replace a piece of code to loop instead of end the app
filedata = f.read()
f.close()
if not "(/home/pi/Scripts/RemoteVideo.py)&\nexit 0" in filedata:
	f = open("/etc/rc.local", "w")
	filedata = filedata.replace("\nexit 0", "(/home/pi/Scripts/RemoteVideo.py)&\nexit 0")
	f.write(filedata)
	f.close()

# change the /boot/cmdline.txt to hide boot up stuff
log("Changing /boot/cmdline.txt to hide boot up logo and text")
f = open("/boot/cmdline.txt", "r")
filedata = f.read()
f.close()
filedata = filedata.replace("console=tty1", "console=tty3")
if not "loglevel=3" in filedata:
	filedata = "loglevel=3 " + filedata
if not "logo.nologo" in filedata:
	filedata = "logo.nologo " + filedata
f = open("/boot/cmdline.txt", "w")
f.write(filedata)
f.close()

log("Done!")
