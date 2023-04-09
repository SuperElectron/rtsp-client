#include "RtspStreamSink.h"

#include <gst/gst.h>
#include <json-glib/json-glib.h>

#include <string>
#include <fstream>
#include <streambuf>
#include <vector>
#include <utility>
#include <mutex>
#include <condition_variable>
#include "signal.h"

typedef std::vector<StreamInfo> StreamNames;

// to handle exiting cleanly from main 
std::mutex mtx;
std::condition_variable cv;
bool running = true;

void handle_sigint(int signo)
{
  if (signo == SIGINT)
  {
    
    std::unique_lock<std::mutex> lck(mtx);
    running = false;
    cv.notify_one();
  }
}

// parse config file for services
bool parseConfigFile(std::string filename,
                     StreamNames &list,
                     std::string &archiveDir,
                     std::string &hlsDir,
                     std::string &url,
                     std::string &portrange)
{
  
  JsonParser *parser;
  JsonNode *root;
  JsonNode *child;
  GError *error;
  
  parser = json_parser_new();
  error = NULL;
  json_parser_load_from_file(parser, filename.c_str(), &error);
  
  if (error) return false;
  
  JsonReader *reader = json_reader_new(json_parser_get_root(parser));
  
  if (json_reader_read_member(reader, "archivedir"))
    archiveDir = json_reader_get_string_value(reader);
  json_reader_end_member(reader);
  
  if (json_reader_read_member(reader, "hlsdir"))
    hlsDir = json_reader_get_string_value(reader);
  json_reader_end_member(reader);
  
  if (json_reader_read_member(reader, "portrange"))
    portrange = json_reader_get_string_value(reader);
  json_reader_end_member(reader);
  
  if (json_reader_read_member(reader, "url"))
    url = json_reader_get_string_value(reader);
  json_reader_end_member(reader);
  
  if (json_reader_read_member(reader, "rtspstreams"))
  {
    
    if (json_reader_is_array(reader))
    {
      
      // get number of elements
      gint num_streams = json_reader_count_elements(reader);
      for (int i = 0; i < num_streams; ++i)
      {
        
        json_reader_read_element(reader, i);
        
        gint count = json_reader_count_members(reader);
        gchar **rtspelements = json_reader_list_members(reader);
        
        // make sure value exists
        if (count > 0)
        {
          
          std::string name(rtspelements[0]);
          json_reader_read_member(reader, name.c_str());
          std::string value(json_reader_get_string_value(reader));
          json_reader_end_member(reader);
          
          StreamInfo info;
          info.type = RTSP;
          info.name = name;
          info.url = value;
          info.port = 9000;
          list.push_back(info);
        }
        json_reader_end_element(reader);
      }
    }
  }
  json_reader_end_member(reader);
  
  if (json_reader_read_member(reader, "udpstreams"))
  {
    
    if (json_reader_is_array(reader))
    {
      
      // get number of elements
      gint num_streams = json_reader_count_elements(reader);
      for (int i = 0; i < num_streams; ++i)
      {
        
        json_reader_read_element(reader, i);
        
        gint count = json_reader_count_members(reader);
        gchar **rtspelements = json_reader_list_members(reader);
        
        // make sure value exists
        if (count > 0)
        {
          
          std::string name(rtspelements[0]);
          json_reader_read_member(reader, name.c_str());
          std::string value(json_reader_get_string_value(reader));
          json_reader_end_member(reader);
          
          // TODO read port
          
          StreamInfo info;
          info.type = UDP;
          info.name = name;
          info.url = value;
          info.port = 15400;
          list.push_back(info);
        }
        json_reader_end_element(reader);
      }
    }
  }
  json_reader_end_member(reader);
  
  g_object_unref(reader);
  g_object_unref(parser);
  
  return true;
}

// main loop for running the rtsp services
int main(int argc, char **argv)
{
  
  // signal for exit
  signal(SIGINT, handle_sigint);
  
  // read args
  if (argc < 2)
  {
    g_printerr(
        "Usage: %s config.json \n", argv[0]);
    g_printerr(
        "Arguments: config.json (rtsp stream and configuration information)\n");
    return -1;
  }
  
  // options/variables from config file
  StreamNames snames;
  std::vector < RtspStreamSink * > streams;
  std::string archdir, hlsdir, uploadurl, portrange;
  
  // parsse the config file
  if (!parseConfigFile(argv[1], snames, archdir, hlsdir, uploadurl, portrange))
  {
    std::cout << "Error: failed to parse json config\n";
    return -1;
  }
  
  // verify correct data exists in file else exit
  if (archdir.empty() || snames.size() == 0)
  { //|| uploadurl.empty()
    std::cout << "Error: json config file missing required values\n";
    return -1;
  }
  
  // warning about missing hls dir
  if (hlsdir.empty()) std::cout << "Warning: No hlsdir defined, so no live hls streams will be created\n";
  
  // create a thread per argument
  // try and connect if exit occurs do a sleep and then try and re-connect
  for (StreamInfo &value : snames)
  {
    streams.push_back(new RtspStreamSink(value, archdir, hlsdir, uploadurl, portrange));
  }
  
  std::unique_lock<std::mutex> lck(mtx);
  while (running) cv.wait(lck);
  
  std::cerr << "Exiting MediaServer\n";
  return 0;
};
