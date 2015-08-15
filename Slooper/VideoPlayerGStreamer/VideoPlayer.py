#!/usr/bin/python3

import os
import time
import gi
gi.require_version('Gst', '1.0')
from gi.repository import GObject, Gst

loop = GObject.MainLoop()
GObject.threads_init()
Gst.init(None)

class Player(object):
    def __init__(self):
        """self.src = Gst.ElementFactory.make("filesrc", None)
        self.src.set_property('location', 'file:///home/pi/USBVideoMount/orbit_build9_tiffs_shorter.m4v')
        self.parse = Gst.ElementFactory.make("h264parse", None)
        self.decode = Gst.ElementFactory.make("omxh264dec", None)
        self.convert = Gst.ElementFactory.make("videoconvert", None)
        self.sink = Gst.ElementFactory.make("eglglessink", None)
        self.pipeline = Gst.Pipeline()
        self.pipeline.add(self.src)
        self.pipeline.add(self.parse)
        self.pipeline.add(self.decode)
        self.pipeline.add(self.convert)
        self.pipeline.add(self.sink)

        self.src.link(self.parse)
        self.parse.link(self.decode)
        self.decode.link(self.convert)
        self.convert.link(self.sink)

        self.pipeline.set_state(Gst.State.PLAYING)"""


        """
         gst-launch-1.0 filesrc location=/home/pi/USBVideoMount/bouquet1_1920x1080_bitrate20_long_1.mp4 ! qtdemux ! queuemax-size-bytes=10000000 ! h264parse ! omxh264dec ! queue max-size-buffers=4 ! eglglessink
        self.player = Gst.ElementFactory.make("playbin", "player")
        fakesink = Gst.ElementFactory.make("eglglessink", "sink")
        self.player.set_property("video-sink", fakesink)
        self.player.set_property("uri", 'file:///home/pi/USBVideoMount/orbit_build9_tiffs_shorter.m4v')
        self.player.set_state(Gst.State.PLAYING)"""

        self.pipeline = Gst.parse_launch("playbin uri=file:///home/pi/bouquet1_1920x1080_bitrate20_new.mp4")
        self.pipeline.set_state(Gst.State.PLAYING)
        self.bus = self.pipeline.get_bus()
        self.bus.add_signal_watch()

        self.bus.connect("message::eos", self.on_finish)

        print("START PLAYING")

    def on_finish(self, bus, message):
        self.pipeline.seek_simple(Gst.Format.TIME, Gst.SeekFlags.FLUSH | Gst.SeekFlags.KEY_UNIT, 0)
        print "END"

p = Player()
loop.run()