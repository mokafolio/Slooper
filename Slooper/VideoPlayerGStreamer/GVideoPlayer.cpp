#include <gst/gst.h>

gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
    GstElement *play = GST_ELEMENT(data);
    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            /* restart playback if at end */
            if (!gst_element_seek(play, 
                        1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                        GST_SEEK_TYPE_SET, 0,
                        GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
                g_print("Seek failed!\n");
            }
            break;
        default:
            break;
    }
    return TRUE;
}

gint
main (gint   argc,
      gchar *argv[])
{
  GMainLoop *loop;
  GstElement *play;
  GstBus *bus;

  /* init GStreamer */
  gst_init (&argc, &argv);
  loop = g_main_loop_new (NULL, FALSE);

  /* make sure we have a URI */
  if (argc != 2) {
    g_print ("Usage: %s <URI>\n", argv[0]);
    return -1;
  }

  /* set up */
  play = gst_element_factory_make ("playbin", "play");
  //g_object_set (G_OBJECT (play), "uri", argv[1], NULL);

  bus = gst_pipeline_get_bus (GST_PIPELINE (play));
  gst_bus_add_watch (bus, bus_callback, play);
  gst_object_unref (bus);

  gst_element_set_state (play, GST_STATE_PLAYING);

  /* now run */
  g_main_loop_run (loop);

  /* also clean up */
  gst_element_set_state (play, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (play));

  return 0;
}