#ifndef __STREAMSINK_H__
#define __STREAMSINK_H__

#include <ctype.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <map>
#include <iostream>
#include <signal.h>
#include <stdio.h>

// gstreamer libraries
#include <glib-2.0/glib.h>
#include <glib/gprintf.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "stdlib.h"

enum StreamType
{
  RTSP = 0,
  UDP
};

struct StreamInfo
{
  StreamType type;
  std::string name;
  std::string url;
  int port;
};

class RtspStreamSink
{
 
 public:
  RtspStreamSink(StreamInfo info, std::string archivedir, std::string hlsdir, std::string url, std::string portrange);
  virtual ~RtspStreamSink();
  
  GstElement *getTSDemuxPtr()
  { return _tsdemux; };
  GstElement *getPipelinePtr()
  { return _pipeline; };
  GstElement *getTeePtr()
  { return _tee; };
  GstElement *getRtpDepayPtr()
  { return _rtpdepay; };
  GstElement *getSourcePtr()
  { return _src; };
  GstElement *getHlsSinkPtr()
  { return _hlssink; };
  GstElement *getHlsQueuePtr()
  { return _hlsqueue; };
  GstElement *getParserPtr()
  { return _parser; };
  GstElement *getSourceSinkPtr()
  { return _sourcesink; };
  GstElement *getParserQueuePtr()
  { return _parserqueue; };
  void setPipelinePtr(GstElement *pipeline)
  { _pipeline = pipeline; };
  void setParserPtr(GstElement *parser)
  { _parser = parser; };
  
  // check bus calls ( eos )
  static void on_pad_added(GstElement *element, GstPad *srcpad, gpointer data); //data-> element to attach too
  static void on_klv_pad_added(GstElement *element, GstPad *srcpad, gpointer data); //data -> this
  static void on_h265_pad_added(GstElement *element, GstPad *pad, gpointer data); // data -> this
  static void on_h264_pad_added(GstElement *element, GstPad *pad, gpointer data); // data -> this
 
 protected:
  RtspStreamSink()
  {};
  bool hasExited()
  { return _exit_service; };
  
  // wen called generates and returns the current time stamp
  std::string getName()
  { return _info.name; };
  std::string getUploadUrl()
  { return _uploadUrl; };
  std::string getHlsFilename()
  { return _hlsFilename; };
  std::string getHlsPlaylist()
  { return _hlsPlaylist; };
  std::string getCurrentHlsDir()
  { return _currentHlsDir; };
  std::string getArchDir()
  { return _archivedir; };
  std::string getCurrentArchDir()
  { return _currentArchDir; };
  std::string getTimeStampStr()
  { return _datetimeStr; };
  std::string getFilenamePathTs()
  { return _filenamePathTs; };
  StreamType getStreamType()
  { return _info.type; };
  void setCurrentTimeStamp();
  
  // creates a pipeline
  void create();
  void reconnectAndUpload(bool upload);
  void linkTeeElements();
  
  // retry rtsp server
  void retry();
  
  // end of stream package and forward
  void upload();
  
  // generate filenames ( used each restart )
  void generateFilenames();
  void resetFilenames();
  void initializeOrSetSourceSink();
  void initializeOrSetHlsSink();
  GstElement *initializeOrSetKlvSink(std::string key);
  
  // thread loop
  static void run(RtspStreamSink &);
  
  // elements
  GstElement *_pipeline;
  GstElement *_tsdemux;
  GstElement *_tee;
  GstElement *_rtpdepay;
  GstElement *_sourcesink;
  GstElement *_sourcequeue;
  GstElement *_parserqueue;
  
  // elements that need to be removed and recreated during reconnects
  std::map<std::string, GstElement *> _klvsinks;
  GstElement *_src;    // can be rtsp or udp
  GstElement *_hlsqueue;
  GstElement *_hlssink;
  GstElement *_parser; // can be eith h265 or h264 parser
  
  // time
  time_t _rawtime;
  struct tm *_timeinfo;
  
  // thread
  std::thread *_thread;
  
  // flag for failed connection
  bool _exit_service;
  
  // options
  StreamInfo _info;
  std::string _archivedir;
  std::string _hlsdir;
  std::string _portrange;
  std::string _filenamePathTs;
  std::string _archiveFilename;
  std::string _hlsFilename;
  std::string _hlsPlaylist;
  std::string _currentArchDir;
  std::string _currentHlsDir;
  std::string _uploadUrl; // end point to push sha256 too
  std::string _datetimeStr; // readable datetime representation
  
  // epoch times
  unsigned long _epochStart;
  unsigned long _epochEnd;
};

#endif
