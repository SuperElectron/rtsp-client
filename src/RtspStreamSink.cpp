#include "RtspStreamSink.h"

#include <sstream>
#include <cstring>

// set up to support multiple klv streams
void RtspStreamSink::on_klv_pad_added(GstElement *element, GstPad *srcpad, gpointer data)
{
  GstPad *sinkpad;
  GstPadLinkReturn ret;
  
  GstCaps *caps = gst_pad_get_current_caps(srcpad);
  const GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *name = gst_structure_get_name(str);
  
  RtspStreamSink *streamsink = (RtspStreamSink *) data;
  
  // only attach if media_client is a video stream
  if (!strncmp(name, "meta/x-klv", 10))
  {
    
    std::string srcpadname = (GST_OBJECT_NAME(srcpad));
    
    gst_element_set_state(streamsink->getPipelinePtr(), GST_STATE_PAUSED);
    
    // initialize and set up klv pad (if reusing one unlink it first
    GstElement *filesink = streamsink->initializeOrSetKlvSink(srcpadname);
    
    sinkpad = gst_element_get_static_pad(filesink, "sink");
    // If our converter is already linked, we have nothing to do here
    if (gst_pad_is_linked(sinkpad))
    {
      g_print("*** We are already linked ***\n");
      gst_object_unref(sinkpad);
      return;
    } else
    {
      g_print("proceeding to linking ...\n");
    }
    ret = gst_pad_link(srcpad, sinkpad);
    
    if (GST_PAD_LINK_FAILED(ret))
    {
      // failed
      g_print("KLV: failed to link to %s dynamically\n", srcpadname.c_str());
    } else
    {
      
      // pass
      g_print("KLV: dynamically link on pad %s successful\n", srcpadname.c_str());
    }
    gst_object_unref(sinkpad);
    
    // restart pipeline
    gst_element_set_state(streamsink->getPipelinePtr(), GST_STATE_PLAYING);
  }
}

void RtspStreamSink::initializeOrSetHlsSink()
{
  
  // if doesnt exist create
  if (_hlssink == nullptr)
  {
    
    //gst_element_set_state( _pipeline, GST_STATE_PAUSED);
    _parserqueue = gst_element_factory_make("queue", NULL);
    
    // create a tee and add the filesink and attach
    _hlsqueue = gst_element_factory_make("queue", NULL);
    _hlssink = gst_element_factory_make("hlssink2", NULL);
    g_object_set(G_OBJECT(_hlssink), "max-files", 10, NULL);
    g_object_set(G_OBJECT(_hlssink), "target-duration", 10, NULL);
    g_object_set(G_OBJECT(_hlssink), "location", getHlsFilename().c_str(), NULL);
    g_object_set(G_OBJECT(_hlssink), "playlist-location", getHlsPlaylist().c_str(), NULL);
    
    // add elements to pipeline
    gst_bin_add_many(GST_BIN(_pipeline), _parserqueue, _hlsqueue, _hlssink, NULL);
    if (!gst_element_link_many(_hlsqueue, _hlssink, NULL))
    {
      g_printerr("Failed to link hlsqueue to hlssink. Exiting.\n");
      return;
    }
    gst_element_sync_state_with_parent(_parserqueue);
    gst_element_sync_state_with_parent(_hlsqueue);
    gst_element_sync_state_with_parent(_hlssink);
  } else
  {
    gst_element_unlink(_tsdemux, _parserqueue);
    gst_element_set_state(_hlssink, GST_STATE_NULL);
    g_object_set(G_OBJECT(_hlssink), "location", getHlsFilename().c_str(), NULL);
    g_object_set(G_OBJECT(_hlssink), "playlist-location", getHlsPlaylist().c_str(), NULL);
    gst_element_sync_state_with_parent(_hlssink); // TODO not sure if needed
  }
}

void RtspStreamSink::initializeOrSetSourceSink()
{
  
  // if doesnt exist create
  if (_sourcesink == nullptr)
  {
    
    gst_element_set_state(_pipeline, GST_STATE_PAUSED);
    
    g_print("Initializing source sink for: %s with path %s\n", _info.name.c_str(), getFilenamePathTs().c_str());
    
    // create a tee and add the filesink and attach
    _sourcesink = gst_element_factory_make("filesink", NULL);
    g_object_set(G_OBJECT(_sourcesink), "location", getFilenamePathTs().c_str(), NULL);
    
    // add elements to pipeline
    gst_bin_add(GST_BIN(_pipeline), _sourcesink);
    
    if (!gst_element_link_many(_sourcequeue, _sourcesink, NULL))
    {
      g_printerr("Failed to link: %s ,sourcequeue to sourcesink. Exiting.\n", _info.name.c_str());
      return;
    }
    
    //gst_element_sync_state_with_parent( _sourcesink );
    //gst_element_set_state( _pipeline, GST_STATE_PLAYING);
  } else
  {
    g_print("Updating source sink: %s with path %s\n", _info.name.c_str(), getFilenamePathTs().c_str());
    // set rawts filename
    gst_element_set_state(_sourcesink, GST_STATE_NULL);
    g_object_set(G_OBJECT(_sourcesink), "location", getFilenamePathTs().c_str(), NULL);
    //gst_element_sync_state_with_parent( _sourcesink );
  }
  
  gst_element_sync_state_with_parent(_sourcesink);
  gst_element_set_state(_pipeline, GST_STATE_PLAYING);
}

GstElement *RtspStreamSink::initializeOrSetKlvSink(std::string key)
{
  
  std::stringstream klvname;
  klvname << getCurrentArchDir() << "/data_" << key << ".klv";
  
  std::map<std::string, GstElement *>::iterator it;
  if ((it = _klvsinks.find(key)) == _klvsinks.end())
  {
    
    //gst_element_set_state( _pipeline, GST_STATE_PAUSED);
    g_print("Creating klvsink: %s for %s\n", key.c_str(), _info.name.c_str());
    
    // create klvsink
    GstElement *filesink = gst_element_factory_make("filesink", NULL);
    g_object_set(G_OBJECT(filesink), "location", klvname.str().c_str(), NULL);
    
    // add file seink to map
    _klvsinks[key] = filesink;
    
    gst_bin_add(GST_BIN(_pipeline), filesink);
    
    gst_element_sync_state_with_parent(filesink);
    
    return filesink;
  } else
  { // update existing klvsink
    
    //g_print("Updating klvsink: %s for %s\n", key, _info.name.c_str());
    gst_element_unlink(_tsdemux, it->second);
    gst_element_set_state(it->second, GST_STATE_NULL);
    g_object_set(G_OBJECT(it->second), "location", klvname.str().c_str(), NULL);
    return it->second;
  }
}

// determine parser to add to scene
void RtspStreamSink::on_h265_pad_added(GstElement *element, GstPad *srcpad, gpointer data)
{
  GstPad *sinkpad;
  GstPadLinkReturn ret;
  
  GstCaps *caps = gst_pad_get_current_caps(srcpad);
  const GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *name = gst_structure_get_name(str);
  RtspStreamSink *streamsink = (RtspStreamSink *) data;
  
  // check parser required
  if (!strncmp(name, "video/x-h265", 12))
  {
    
    // make sure pipeline is paused during connection
    gst_element_set_state(streamsink->getPipelinePtr(), GST_STATE_PAUSED);
    
    // make sure hls sink is set up
    streamsink->initializeOrSetHlsSink();
    
    // initalize only once 
    if (streamsink->getParserPtr() == NULL)
    {
      
      // create in this callback  
      streamsink->setParserPtr(gst_element_factory_make("h265parse", NULL));
      g_object_set(G_OBJECT(streamsink->getParserPtr()), "config-interval", -1, NULL);
      
      GstElement *dec265 = gst_element_factory_make("libde265dec", NULL);
      GstElement *enc264 = gst_element_factory_make("x264enc", NULL);
      gst_bin_add_many(GST_BIN(streamsink->getPipelinePtr()), streamsink->getParserPtr(), dec265, enc264, NULL);
      
      if (!gst_element_link_many(streamsink->getParserQueuePtr(),
                                 streamsink->getParserPtr(),
                                 dec265,
                                 enc264,
                                 streamsink->getHlsQueuePtr(),
                                 NULL))
      {
        g_printerr("Failed to link 265 parser to hlssink. Exiting.\n");
        return;
      }
    }
    
    // try and sink pad
    sinkpad = gst_element_get_static_pad(streamsink->getParserQueuePtr(), "sink");
    // If our converter is already linked, we have nothing to do here
    if (gst_pad_is_linked(sinkpad))
    {
      g_print("*** We are already linked ***\n");
      gst_object_unref(sinkpad);
      return;
    } else
    {
      g_print("proceeding to linking ...\n");
    }
    ret = gst_pad_link(srcpad, sinkpad);
    
    if (GST_PAD_LINK_FAILED(ret))
    {
      // failed
      g_print("Parser: failed to link h265 dynamically\n");
    } else
    {
      // pass
      g_print("Parser: dynamically linked h265 successful\n");
    }
    gst_object_unref(sinkpad);
    
    // restart pipeline
    gst_element_set_state(streamsink->getPipelinePtr(), GST_STATE_PLAYING);
  }
}

void RtspStreamSink::on_h264_pad_added(GstElement *element, GstPad *srcpad, gpointer data)
{
  GstPad *sinkpad;
  GstPadLinkReturn ret;
  
  GstCaps *caps = gst_pad_get_current_caps(srcpad);
  const GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *name = gst_structure_get_name(str);
  RtspStreamSink *streamsink = (RtspStreamSink *) data;
  
  // check parser required
  if (!strncmp(name, "video/x-h264", 12))
  {
    
    // make sure pipeline is paused during connection
    gst_element_set_state(streamsink->getPipelinePtr(), GST_STATE_PAUSED);
    
    // make sure hls sink is set up
    streamsink->initializeOrSetHlsSink();
    
    // initalize only once 
    if (streamsink->getParserPtr() == NULL)
    {
      
      // create in this callback  
      streamsink->setParserPtr(gst_element_factory_make("h264parse", NULL));
      g_object_set(G_OBJECT(streamsink->getParserPtr()), "config-interval", -1, NULL);
      
      gst_bin_add(GST_BIN(streamsink->getPipelinePtr()), streamsink->getParserPtr());
      
      if (!gst_element_link_many(streamsink->getParserQueuePtr(),
                                 streamsink->getParserPtr(),
                                 streamsink->getHlsQueuePtr(),
                                 NULL))
      {
        g_printerr("Failed to link 264 parser to hlssink. Exiting.\n");
        return;
      }
    }
    
    // try and sink pad
    sinkpad = gst_element_get_static_pad(streamsink->getParserQueuePtr(), "sink");
    // If our converter is already linked, we have nothing to do here
    if (gst_pad_is_linked(sinkpad))
    {
      g_print("*** We are already linked ***\n");
      gst_object_unref(sinkpad);
      return;
    } else
    {
      g_print("proceeding to linking ...\n");
    }
    ret = gst_pad_link(srcpad, sinkpad);
    
    if (GST_PAD_LINK_FAILED(ret))
    {
      // failed
      g_print("Parser: failed to link h264 dynamically\n");
    } else
    {
      // pass
      g_print("Parser: dynamically linking h264 successful\n");
    }
    gst_object_unref(sinkpad);
    
    // restart pipeline
    gst_element_set_state(streamsink->getPipelinePtr(), GST_STATE_PLAYING);
  }
}

// generic dynamic pad
void RtspStreamSink::on_pad_added(GstElement *element, GstPad *pad, gpointer data)
{
  
  // connect up pads
  GstPad *sinkpad;
  GstPadLinkReturn ret;
  
  GstCaps *caps = gst_pad_get_current_caps(pad);
  const GstStructure *str = gst_caps_get_structure(caps, 0);
  const gchar *name = gst_structure_get_name(str);
  
  RtspStreamSink *streamsink = (RtspStreamSink *) data;
  
  if (streamsink->getStreamType() == RTSP)
    sinkpad = gst_element_get_static_pad(streamsink->getRtpDepayPtr(), "sink");
  else
    sinkpad = gst_element_get_static_pad(streamsink->getTeePtr(), "sink");
  
  g_print("Accepting %s connection: %s\n", streamsink->getName().c_str(), name);
  
  // connecting to a stream generate file names
  streamsink->generateFilenames();
  
  // initialize source and/or adjust directory
  streamsink->initializeOrSetSourceSink();
  
  if (!g_str_has_prefix(GST_PAD_NAME(pad), "recv_rtp_src_"))
  {
    g_print("It is not the right pad.  Need recv_rtp_src_. Ignoring.\n");
    return;
  }
  
  // If our converter is already linked, we have nothing to do here
  if (gst_pad_is_linked(sinkpad))
  {
    g_print("*** We are already linked ***\n");
    gst_object_unref(sinkpad);
    return;
  } else
  {
    g_print("proceeding to linking ...\n");
  }
  ret = gst_pad_link(pad, sinkpad);
  
  if (GST_PAD_LINK_FAILED(ret))
  {
    // failed
    g_print("Pad: %s failed to link dynamically\n", streamsink->getName().c_str());
  } else
  {
    // pass
    g_print("Pad: %s dynamically link successful\n", streamsink->getName().c_str());
  }
  
  gst_object_unref(sinkpad);
}

void RtspStreamSink::linkTeeElements()
{
  GstPad *sinkpad;
  GstPadLinkReturn ret;
  
  // get the src_0 from the tee to bind to the filesink 
  GstPad *teesrcpad0 = gst_element_get_request_pad(_tee, "src_%u");
  if (!teesrcpad0)
  {
    g_printerr("tee request src_0 pad failed. Exiting.\n");
    return;
  }
  
  GstPad *teesrcpad1 = gst_element_get_request_pad(_tee, "src_%u");
  if (!teesrcpad1)
  {
    g_printerr("tee request src_1 pad failed. Exiting.\n");
    return;
  }
  
  // get the fink pad from the new file sink
  GstPad *qsinkpad = gst_element_get_static_pad(_sourcequeue, "sink");
  if (!qsinkpad)
  {
    g_printerr("filequeue request sink pad failed. Exiting.\n");
    return;
  }
  
  // get sink for tsdemux 
  GstPad *dsinkpad = gst_element_get_static_pad(_tsdemux, "sink");
  if (!dsinkpad)
  {
    g_printerr("tsdemux request sink pad failed. Exiting.\n");
    return;
  }
  
  // link tee src_0 to the filesinkpad
  ret = gst_pad_link(teesrcpad0, qsinkpad);
  
  if (GST_PAD_LINK_FAILED(ret))
  {
    // failed
    g_print("tee: %s failed to link queue/file sink\n", _info.name.c_str());
  } else
  {
    // pass
    g_print("tee: %s link successful queue/file sink\n", _info.name.c_str());
  }
  gst_object_unref(qsinkpad);
  
  // link tee src_1 to the tsdemuxsinkpad
  ret = gst_pad_link(teesrcpad1, dsinkpad);
  
  if (GST_PAD_LINK_FAILED(ret))
  {
    // failed
    g_print("tee: %s failed to link tsdemux sink\n", _info.name.c_str());
  } else
  {
    // pass
    g_print("tee: %s link successful tsdemux sink\n", _info.name.c_str());
  }
  gst_object_unref(dsinkpad);
}

void RtspStreamSink::upload()
{
  
  // set epoch end of capture
  _epochEnd = (unsigned long) time(NULL);
  
  // command for sending data to server
  std::stringstream uploadCommand;
  g_print("RtspStreamSink: starting fingerprint.py\n");
  uploadCommand << "python3 /rtsp_server/scripts/fingerprint.py " << _info.name << "_" << _datetimeStr << " "
                << _archivedir << " "
                << _currentArchDir << " " << _uploadUrl << " " << _epochStart << " " << _epochEnd << " &";
  
  // make call to upload signature
  g_print("RtspStreamSink: starting upload command: %s\n", uploadCommand.str().c_str());
  system(uploadCommand.str().c_str());
  
  // reset the file names
  resetFilenames();
}

// check if upload else restart due to connection being broken via EOS Message
void RtspStreamSink::reconnectAndUpload(bool uploadData)
{
  
  //NOTE upload needs to occur before files names are adjusted
  //check if should upload
  g_print("reconnectAndUpload: upload is %d (1=true, 0=false).\n", uploadData);
  if (uploadData)
  {
    upload();
  }
  
  //pause the pipeline while making adjustments
  g_print("reconnectAndUpload: setting state to READY\n");
  gst_element_set_state(_pipeline, GST_STATE_READY);
  
  // enable plat mode
  //g_print("Attempt %s reconnection\n", _info.name.c_str() );
  g_print("reconnectAndUpload: setting state to PLAYING\n");
  gst_element_set_state(_pipeline, GST_STATE_PLAYING);
}

// reset file names between rtpsession connections
void RtspStreamSink::resetFilenames()
{
  
  _filenamePathTs = "";
  _currentArchDir = "";
  _currentHlsDir = "";
  _hlsFilename = "";
  _hlsPlaylist = "";
}

void RtspStreamSink::generateFilenames()
{
  
  //g_print("Generating directory and file names for %s\n, _info.name.c_str());
  
  // generate a timestamp to use
  setCurrentTimeStamp();
  
  // set epoch start time
  _epochStart = (unsigned long) time(NULL);
  
  // create archive path
  std::stringstream filenamePathTs;
  filenamePathTs << _archivedir << "/" << _info.name << "_" << _datetimeStr << "/" << _info.name << ".ts";
  _filenamePathTs = filenamePathTs.str().c_str();
  
  // location for current ts and klv files to be written too
  std::stringstream currentArchDir;
  currentArchDir << _archivedir << "/" << _info.name << "_" << _datetimeStr;
  _currentArchDir = currentArchDir.str();
  
  // check if hls is active
  if (!_hlsdir.empty())
  {
    
    // create hls dir
    std::stringstream currentHlsDir;
    currentHlsDir << _hlsdir << "/" << _info.name << "_" << _datetimeStr;
    _currentHlsDir = currentHlsDir.str();
    
    // hls full filename
    std::stringstream hlsName;
    hlsName << "/" << _currentHlsDir << "/" << _info.name << "%05d.ts";
    _hlsFilename = hlsName.str();
    
    // hls full playlist
    std::stringstream hlsPlaylist;
    hlsPlaylist << "/" << _currentHlsDir << "/" << _info.name << ".m3u8";
    _hlsPlaylist = hlsPlaylist.str();
    
    // create hls dir
    std::stringstream mHlsDir;
    mHlsDir << "mkdir -p " << _currentHlsDir;
    
    // create hls directory
    system(mHlsDir.str().c_str());
  }
  
  // create arch dir
  std::stringstream archDir;
  archDir << "mkdir -p " << _currentArchDir;
  
  // make arch directory 
  system(archDir.str().c_str());
}

// creates a pipeline ( cleans up old pipeline if it exists )
void RtspStreamSink::create()
{
  
  // Create Pipeline element that will form a connection of other elements 
  _pipeline = gst_pipeline_new(_info.name.c_str());
  
  if (_info.type == RTSP)
  {
    g_print("Creating RTSP pipline: %s for %s\n", _info.name.c_str(), _info.url.c_str());
    
    // connection to Deepstream rtsp server(s)
    _src = gst_element_factory_make("rtspsrc", NULL);
    g_object_set(G_OBJECT(_src), "location", _info.url.c_str(), NULL);
    g_object_set(G_OBJECT(_src), "message-forward", TRUE, NULL);
    
    // if a port range is specified
    if (!_portrange.empty())
    {
      printf("Setting port range: %s\n", _portrange.c_str());
      g_object_set(G_OBJECT(_src), "port-range", _portrange.c_str(), NULL);
    }
    
    // to help with possible packloss/delay 
    _rtpdepay = gst_element_factory_make("rtpmp2tdepay", NULL);
  } else
  {
    g_print("Creating UDP pipline: %s for %s\n", _info.name.c_str(), _info.url.c_str());
    
    // connection to Deepstream udp server(s)
    _src = gst_element_factory_make("udpsrc", NULL);
    g_object_set(G_OBJECT(_src), "address", _info.url.c_str(), NULL);
    g_object_set(G_OBJECT(_src), "port", _info.port, NULL);
    
    // expecting mpegts
    g_object_set(G_OBJECT(_src), "caps", "video/mpegts", NULL);
    
    std::cerr << "Created source\n";
  }
  
  // need to reference this 
  _tee = gst_element_factory_make("tee", NULL);
  
  // need to reference this 
  _tsdemux = gst_element_factory_make("tsdemux", NULL);
  
  // create the queue source
  _sourcequeue = gst_element_factory_make("queue", NULL);
  
  // set up rtsp source
  if (_info.type == RTSP)
  {
    
    gst_bin_add_many(GST_BIN(_pipeline), _src, _rtpdepay, _tee, _tsdemux, _sourcequeue, NULL);
    
    // link rtp to tee
    if (!gst_element_link_many(_rtpdepay, _tee, NULL))
    {
      g_printerr("Failed to link rtpdepay to tee.\n");
      return;
    }
  } else
  {
    
    gst_bin_add_many(GST_BIN(_pipeline), _src, _tee, _tsdemux, _sourcequeue, NULL);
  }
  
  // link tee  elements
  linkTeeElements();
  
  // attach sinks for vid streams ( if hlsdir is defined )
  if (!_hlsdir.empty())
  {
    g_signal_connect(_tsdemux, "pad-added", G_CALLBACK(&RtspStreamSink::on_h265_pad_added), this);
    g_signal_connect(_tsdemux, "pad-added", G_CALLBACK(&RtspStreamSink::on_h264_pad_added), this);
  }
  
  // attach sinks for klv streams 
  g_signal_connect(_tsdemux, "pad-added", G_CALLBACK(&RtspStreamSink::on_klv_pad_added), this);
  
  // attach rtsp input stream ( in not called in loop just create a new rtspsrc instead of a new pipeline )
  g_signal_connect(_src, "pad-added", G_CALLBACK(&RtspStreamSink::on_pad_added), this);
  
  // Set the pipeline to "playing" state
  gst_element_set_state(_pipeline, GST_STATE_PLAYING);
}

void RtspStreamSink::setCurrentTimeStamp()
{
  
  // get current time 
  time(&_rawtime);
  _timeinfo = localtime(&_rawtime);
  
  // get time in string format to use
  char sdt[80];
  strftime(sdt, 80, "%F_%H-%M-%S", _timeinfo);
  
  _datetimeStr = sdt;
}

// thread run
void RtspStreamSink::run(RtspStreamSink &self)
{
  
  std::cerr << "Before create\n";
  // create and start a pipeline  
  self.create();
  std::cerr << "After create\n";
  
  // loop until exit
  do
  {
    
    //initilzie termination flag
    bool terminate_instance = false;
    bool upload = false;
    
    GstBus *bus = gst_element_get_bus(self.getPipelinePtr());
    if (bus == NULL) g_printerr("Error: No bus found\n");
    
    // loop to restart stream capture
    do
    {
      
      GstMessage *msg = gst_bus_timed_pop_filtered(bus,
                                                   GST_CLOCK_TIME_NONE,
                                                   (GstMessageType)(
                                                       GST_MESSAGE_ELEMENT | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
      if (msg != NULL)
      {
        GError *err;
        gchar *debug_info;
        
        switch (GST_MESSAGE_TYPE(msg))
        {
          case GST_MESSAGE_ERROR:gst_message_parse_error(msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
            g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);
            terminate_instance = true;
            break;
          case GST_MESSAGE_EOS:g_print("EOS received: %s\n", self.getName().c_str());
            //upload = true;
            if (self.getCurrentArchDir().empty())
            {
              terminate_instance = true;
            }
            break;
          case GST_MESSAGE_STATE_CHANGED:
            /* We are only interested in state-changed messages from the pipeline */
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(self.getPipelinePtr()))
            {
              GstState old_state, new_state, pending_state;
              gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
              g_print("Pipeline state changed from %s to %s:\n",
                      gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
            }
            break;
          case GST_MESSAGE_ELEMENT:
            g_print("GST_MESSAGE_ELEMENT: bus message from element (%s) \n",
                    GST_OBJECT_NAME(msg->src));
            g_print("GST_MESSAGE_ELEMENT: bus message from pipeline (%s) \n", self.getName().c_str());
            g_print("GST_MESSAGE_ELEMENT: Message evaluation (%d) \n",
                    strncmp(GST_OBJECT_NAME(msg->src), "rtpsession", 10));
            
            // catch any message element from rtpsessionX (where x is the session that was created)
            if (!strncmp(GST_OBJECT_NAME(msg->src), "rtpsession", 10))
            {
              g_print("\nGST_MESSAGE_ELEMENT received callback: %s.\n", GST_OBJECT_NAME(msg->src));
              g_print("\nCaught rtpsession element callback.  Terminating %s connection.\n", self.getName().c_str());

//               // use to remove ended HLS element
//               // check if hls media creation is running
//               if( !self.getCurrentHlsDir().empty() ) {
//                 std::stringstream rm_empty_hls_dir;
//                 rm_empty_hls_dir << "rm -r " << self.getCurrentHlsDir();
//                 std::cerr << " trying: " << rm_empty_hls_dir.str() << std::endl;
//                     //system( rm_empty_hls_dir.str().c_str() );
//                 }
              
              // set upload to true (?) and terminate_instance to true to exit while loop
              upload = true;
              terminate_instance = true;
            }
            break;
          default:
            /* We should not reach here */
            g_printerr("Unexpected message received.\n");
            break;
        }
        gst_message_unref(msg);
      }
    } while (!terminate_instance);
    
    g_print("Connection exited: %s\n", self.getName().c_str());
    gst_object_unref(bus);
    
    // reset instance called
    self.reconnectAndUpload(upload);
    
    // sleep for 10 seconds before trying to reconnect
    g_usleep(10000000);
    
  } while (!self.hasExited());
  g_print("Exiting thread: %s\n", self.getName().c_str());
}

RtspStreamSink::~RtspStreamSink()
{
  
  g_print("RtspStreamSink: %s destructor called\n", _info.name.c_str());
  
  // set exit and failed flag and signal
  _exit_service = true;
  
  // send an eos to exit
  gst_element_send_event(_pipeline, gst_event_new_eos());
  
  if (_thread)
  {
    
    if (_thread->joinable())
      _thread->join();
    
    delete _thread;
    _thread = NULL;
  }
}

RtspStreamSink::RtspStreamSink(StreamInfo info,
                               std::string archivedir,
                               std::string hlsdir,
                               std::string uploadurl,
                               std::string portrange) :
    _info(info), _archivedir(archivedir), _hlsdir(hlsdir), _portrange(portrange), _exit_service(false),
    _uploadUrl(uploadurl), _pipeline(nullptr), _parser(nullptr), _hlssink(nullptr), _hlsqueue(nullptr),
    _src(nullptr), _rtpdepay(nullptr), _tee(nullptr), _tsdemux(nullptr),
    _parserqueue(nullptr), _sourcequeue(nullptr), _sourcesink(nullptr), _thread(nullptr)
{
  
  // initilize gst   
  gst_init(NULL, NULL);
  
  // Start a thread ( pass pointer to self )
  _thread = new std::thread(&RtspStreamSink::run, std::ref(*this));
}
