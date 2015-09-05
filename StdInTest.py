#!/usr/bin/python
import struct
import time
import sys

def isKeyDown(_key):
  print("CHECKING FOR EVENT")
  infile_path = "/dev/input/event" + (sys.argv[1] if len(sys.argv) > 1 else "0")

  #long int, long int, unsigned short, unsigned short, unsigned int
  FORMAT = 'llHHI'
  EVENT_SIZE = struct.calcsize(FORMAT)

  #open file in binary mode
  in_file = open(infile_path, "rb")

  event = in_file.read(EVENT_SIZE)

  if event:
      (tv_sec, tv_usec, type, code, value) = struct.unpack(FORMAT, event)

      """if type != 0 or code != 0 or value != 0:
          print("Event type %u, code %u, value: %u at %d, %d" % \
              (type, code, value, tv_sec, tv_usec))
      else:
          # Events with code, type and value == 0 are "separator" events
          print("===========================================")"""

      if type == 1:
        print("key down")

      #event = in_file.read(EVENT_SIZE)

  in_file.close()

while True:
  isKeyDown('a')
  time.sleep(1.0)