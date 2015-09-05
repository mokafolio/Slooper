#!/usr/bin/env python

import RPi.GPIO as GPIO
import time
from datetime import datetime, timedelta
import subprocess
import signal
import os
import json
import sys
import struct
import curses

damn = curses.initscr()
damn.nodelay(1) # doesn't keep waiting for a key press
damn.keypad(1) # i.e. arrow keys
curses.noecho() # stop keys echoing

def isKeyDown(_key):
	infile_path = "/dev/input/event" + (sys.argv[1] if len(sys.argv) > 1 else "0")

	#long int, long int, unsigned short, unsigned short, unsigned int
	FORMAT = 'llHHI'
	EVENT_SIZE = struct.calcsize(FORMAT)

	#open file in binary mode
	in_file = open(infile_path, "rb")

	event = in_file.read(EVENT_SIZE)

	while event:
	    (tv_sec, tv_usec, type, code, value) = struct.unpack(FORMAT, event)

	    if type != 0 or code != 0 or value != 0:
	        print("Event type %u, code %u, value: %u at %d, %d" % \
	            (type, code, value, tv_sec, tv_usec))
	    else:
	        # Events with code, type and value == 0 are "separator" events
	        print("===========================================")

    	event = in_file.read(EVENT_SIZE)

	in_file.close()

def log(_str, _bTimeStamp = True):
	logFilePath = "/home/pi/USBVideoMount/SlooperLog.txt"

	if os.path.exists(logFilePath):
		with open(logFilePath, "r") as fin:
			data = fin.read().splitlines(True);
		if len(data) > 100:
			diff = len(data) - 99
			with open(logFilePath, "w") as fout:
				fout.writelines(data[diff:])

	logFile = open(logFilePath, "a")
	message = ""
	if _bTimeStamp:
		message += str(datetime.now()) + ": "
	message += _str
	print message
	logFile.write(message + "\n")

with open('/home/pi/USBVideoMount/Videos.json') as jsonFile:    
    jsonData = json.load(jsonFile)

# discribes a display configuration
class DisplaySettings:
	# some reasonable defaults
	displayPixelWidth = 1920
	displayPixelHeight = 1080
	displayScan = "progressive"
	displayRate = 60

# describes a collection of video files and that share settings and keyboard key
class VideoGroup:
	files = [] 
	currentIndex = 0 #current videoIndex
	settings = ""
	myIndex = 0 #index into the videos array

# describes a playing video
class VideoSession:
	processID = -1
	videoGroup = None

# dict mapping a settings name to its DisplaySettings obj
displaySettings = {}
# array holding all the Video objects
videos = []
# dict mapping a key press to a Video object
keyVideoMap = {}
#variable holding the maximum play time, after which th pi will shut down
maxPlayMinutes = int(jsonData["maxPlayMinutes"])

# parse the settings from the Videos.json
for k,v in jsonData["settings"].items():
	settings = DisplaySettings()
	if v.get('displayPixelWidth'):
		settings.displayPixelWidth = v["displayPixelWidth"]
	if v.get('displayPixelHeight'):
		settings.displayPixelHeight = v["displayPixelHeight"]
	if v.get('displayScan'):
		settings.displayScan = v["displayScan"]
	if v.get('displayRate'):
		settings.displayRate = v["displayRate"]
	displaySettings[k] = settings

# parse the videos from Videos.json
for video in jsonData["videoGroups"]:
	vid = VideoGroup()
	vid.files = video["files"]
	vid.settings = video["settings"]
	videos.append(vid)
	vid.myIndex = len(videos) - 1
	keyVideoMap[video["key"]] = vid

# holds the currently playing video and some extra info
currentVideoSession = None

# helper to stop the currently playing video process
def stopCurrentVideo():
	os.killpg(currentVideoSession.processID, signal.SIGTERM)

# this function stops the current video, if any, and starts playing the requested one
# Before it starts playing, it will try to apply the new videos requested DisplaySettings
def playVideoInGroup(videoGroup, videoIndex):
	global currentVideoSession
	lastGroup = None
	videoGroup.currentIndex = videoIndex

	# quit the previos video process if any
	if currentVideoSession:
		stopCurrentVideo()
		lastGroup = currentVideoSession.videoGroup

	# make the new session
	currentVideoSession = VideoSession()
	currentVideoSession.videoGroup = videoGroup
	# make sure the settings exist
	currentSettings = displaySettings[currentVideoSession.videoGroup.settings]
	assert currentSettings != None

	log("The current video group index is: " + str(currentVideoSession.videoGroup.myIndex))
	log("The current video index is: " + str(currentVideoSession.videoGroup.currentIndex))

	videoPath = "/home/pi/USBVideoMount/" + currentVideoSession.videoGroup.files[currentVideoSession.videoGroup.currentIndex]
	if not os.path.exists(videoPath):
		log("The video files does not exist")
		sys.exit()

	if lastGroup == None or lastGroup != videoGroup:
		requestedWidth = currentSettings.displayPixelWidth
		requestedHeight = currentSettings.displayPixelHeight
		requestedScan = currentSettings.displayScan
		requestedRate = currentSettings.displayRate

		log("The requested display pixel width is " + str(requestedWidth) + ".")
		log("The requested display pixel height is " + str(requestedHeight) + ".")
		log("The requested display scan is " + str(requestedScan) + ".")
		log("The requested display rate is " + str(requestedRate) + ".")

		# get all the available display modes and find the best one for the requested settings
		availableModes = subprocess.check_output("tvservice -j -m CEA", shell=True)
		availableModes2 = subprocess.check_output("tvservice -j -m DMT", shell=True)
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
		
		# apply the best mode
		subprocess.call("tvservice -e \"" + bestMode["type"] + " " + str(bestMode["code"]) + "\"" , shell=True)

	# start the video process
	log("Starting Video: " + videoPath)
	videoProcess = subprocess.Popen("/opt/vc/src/hello_pi/hello_video/hello_video.bin " + videoPath, stdout=subprocess.PIPE, shell=True, preexec_fn=os.setsid)
	currentVideoSession.processID = videoProcess.pid

def playNextVideoInGroup(_videoGroup):

	# generate the full video path and make sure the video file exists
	_videoGroup.currentIndex += 1
	if _videoGroup.currentIndex >= len(_videoGroup.files):
		_videoGroup.currentIndex = 0
	playVideoInGroup(_videoGroup, _videoGroup.currentIndex)

def gpioShutdownSequence():
	#this is the shutdown sequence of the remote board, see the site linked below
	offPin = 15
	GPIO.setup(offPin, GPIO.OUT)
	GPIO.output(offPin, 1)
	time.sleep(0.125)
	GPIO.output(offPin, 0)
	time.sleep(0.2)
	GPIO.output(offPin, 1)
	time.sleep(0.4)
	GPIO.output(offPin, 0)


#check if we cached where to play from
cachePath = "/home/pi/USBVideoMount/SlooperCache.json"
if os.path.exists(cachePath):
	with open(cachePath) as jsonFile:  
		jsonData = json.load(jsonFile)
		playVideoInGroup(videos[jsonData["lastVideoGroupIndex"]], jsonData["lastVideoIndex"])
else:
	#play the first video in the array as the default one
	playNextVideoInGroup(videos[0])

#GPIO pin stuff based on the instructions from the remote board people here:
#http://www.msldigital.com/pages/support-for-remotepi-board-plus-2015/
pin=14
GPIO.setmode(GPIO.BCM)
GPIO.setup(pin, GPIO.IN)

bShutDown = False
startTime = time.time()
# the main loop
while True:
	bQuit = False
	minutesElapsed = (time.time() - startTime) / 60.0

	if minutesElapsed > maxPlayMinutes:
		log("Max time elapsed, shutting down now")
		gpioShutdownSequence()
		bQuit = True
	else:
		# query if a key is pressed
		c = damn.getch()
	  	if c == ord('q'):
	  		# if q is pressed, we quit and shutdown, replicating the remote behavior
	  		log("Keyboard q pressed, we quit and shut down now")
	  		gpioShutdownSequence()
	  		bQuit = True
	  	if c == ord('w'):
	  		# if w is pressed, we simply quit the video and this app, without shutting down
	  		log("Keyboard w pressed, we quit now")
	  		stopCurrentVideo()
	  		break
	  	elif c in range(256):
	  		# otherwise we check if the key is mapped to a video. If so, we start playing that video
	  		if chr(c) in keyVideoMap:
	  			playNextVideoInGroup(keyVideoMap[chr(c)])

	  	# check if the remote is pressed
		if GPIO.input(pin) != 0:
			log("Remote pressed, we quit and shut down now")
			bQuit = True

	# do shutdown procedure
	if bQuit:
		log("Remote shut down procedure")
		stopCurrentVideo()
		GPIO.setup(pin, GPIO.OUT)
		GPIO.output(pin, 1)
		time.sleep(3)
		bShutDown = True
		break
	time.sleep(0.1)

# close curses
curses.endwin()

# show the console again
subprocess.call("fbset -depth 8 && fbset -depth 16", shell=True)

# cleanup GPIO
GPIO.cleanup()

#save which video we were at
with open(cachePath, "w") as cache:
	dc = {"lastVideoGroupIndex": currentVideoSession.videoGroup.myIndex, "lastVideoIndex": currentVideoSession.videoGroup.currentIndex}
	json.dump(dc, cache)

# and shutdown if requested
if bShutDown:
	log("Shutting down")
	subprocess.call("sudo poweroff", shell=True)