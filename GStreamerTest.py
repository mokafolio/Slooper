import gst
 
# Create the pipeline for our elements.
pipeline = gst.Pipeline('pipeline')

videoSink = gst.element_factory_make('playbin2')