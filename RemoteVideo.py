#!/usr/bin/env python

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