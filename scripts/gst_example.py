#!/usr/bin/env python3

import gi
gi.require_version('Gst', '1.0')
gi.require_version('GstBase', '1.0')
gi.require_version('Gtk', '3.0')
from gi.repository import GObject, Gst, GstBase, Gtk, GObject

class Main:
	def __init__(self):
		Gst.init(None)

		#self.pipeline = Gst.Pipeline("mypipeline")
		self.pipeline = Gst.Pipeline()

		self.audiotestsrc = Gst.ElementFactory.make("audiotestsrc", "audio")
		self.pipeline.add(self.audiotestsrc)

		self.sink = Gst.ElementFactory.make("autoaudiosink", "sink")
		self.pipeline.add(self.sink)

		self.audiotestsrc.link(self.sink)

		self.pipeline.set_state(Gst.State.PLAYING)

start=Main()
Gtk.main()
