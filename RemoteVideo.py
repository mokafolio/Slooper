#!/usr/bin/env python

import RPi.GPIO as GPIO
import time
from datetime import datetime, timedelta
import subprocess
import signal
import os
import json

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


with open('/home/pi/USBVideoMount/VideoSettings.json') as jsonFile:    
    jsonData = json.load(jsonFile)

requestedWidth = 1920
requestedHeight = 1080
requestedScan = "progressive"
requestedRate = 60

if jsonData.get('displayPixelWidth'):
	requestedWidth = jsonData["displayPixelWidth"]

if jsonData.get('displayPixelHeight'):
	requestedHeight = jsonData["displayPixelHeight"]

if jsonData.get('displayScan'):
	requestedScan = jsonData["displayScan"]

if jsonData.get('displayRate'):
	requestedRate = jsonData["displayRate"]


log("The requested display pixel width is " + str(requestedWidth) + ".")
log("The requested display pixel height is " + str(requestedHeight) + ".")
log("The requested display scan is " + str(requestedScan) + ".")
log("The requested display rate is " + str(requestedRate) + ".")

availableModes = subprocess.check_output("tvservice -j -m CEA", shell=True)
availableModes2 = subprocess.check_output("tvservice -j -m DMT", shell=True)
#availableModes = availableModes[0:-3] + ", " + availableModes2[1:]
#print availableModes
jsonData = json.loads(availableModes)
jsonData2 = json.loads(availableModes2)

def findBestDisplayMode(_dmtModes, _ceaModes):
	global bestWDiff, bestHDiff, bestRDiff, bestMode
	log("The available display modes are:")
	bestWDiff = bestHDiff = bestRDiff = float("inf")
	bestWidth = bestHeight = 0
	bestMode = {}
	def compare(_modes, _type):
		global bestWDiff, bestHDiff, bestRDiff, bestMode
		for obj in _modes:
			scan = "interlaced" if obj["scan"] == "i" else "progressive"
			log("Width: " + str(obj["width"]) + " Height: " + str(obj["height"]) + " Rate: " + str(obj["rate"]) + " Scan: " + scan)
			#check if we have a perfect match
			if obj["width"] == requestedWidth and obj["height"] == requestedHeight and obj["rate"] == requestedRate and scan == requestedScan:
				bestMode = {"width" : requestedWidth, "height" : requestedHeight, "rate" : requestedRate, "scan" : requestedScan, "code" : obj["code"], "type" : _type}
				break

			wdiff = abs(obj["width"] - requestedWidth)
			hdiff = abs(obj["height"] - requestedHeight)
			rDiff = abs(obj["rate"] - requestedRate)
			if wdiff <= bestWDiff and hdiff <= bestHDiff and rDiff <= bestRDiff:
				bestWDiff = wdiff
				bestHDiff = hdiff
				bestRDiff = rDiff
				bestMode = {"width" : obj["width"], "height" : obj["height"], "rate" : obj["rate"], "scan" : scan, "code" : obj["code"], "type" : _type}
	compare(_dmtModes, "DMT")
	compare(_ceaModes, "CEA")
	return bestMode

bestMode = findBestDisplayMode(jsonData2, jsonData)
log("Best Mode, Width: " + str(bestMode["width"]) + " Height: " + str(bestMode["height"]) + " Rate: " + str(bestMode["rate"]) + " Scan: " + bestMode["scan"])
subprocess.call("tvservice -e \"" + bestMode["type"] + " " + str(bestMode["code"]) + "\"" , shell=True)

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