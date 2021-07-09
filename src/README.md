Description
-----------

A service that will connect to multiple rtsp mpegts video streams. 
It will demux content (video and multiple klv), create a directory with the klv and original 
mpegts source compress it and generate a fingerprint file that will be upload to remote blockchain. 
If a hlsdir is specificied in the config.json then a h265/h264 video is encoded into separate hls streams per rtsp source.

The live hls video streams are made available via an nginx proxy ( indexed directory ).

gstreamer environment
---------------------

apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio

Sample nginx addition 
---------------------

Note: end point will be https://someurl.com:443/media/

server {

    # enable ssl conenction only
    listen 443 ssl http2 default_server;
    listen [::]:443 ssl http2 default_server   

    # create certificates to use
    ssl_certificate /etc/ssl/certs/cert.pem;
    ssl_certificate_key /etc/ssl/private/privkey.pem;

    # directory where the m3u8 files are stored
    root /www/data;

    # general url for recorded media
    location /media/ {
        autoindex on;
        keepalive_timeout 65;
    }

    # deny access to .htaccess files, if Apache's document root
    # concurs with nginx's one
    #
    location ~ /\.ht {
        deny  all;
    }
}

Config file example
-------------------

// config.json example
{
  "archivedir": "/tmp/archive",                      // archivedir directory where all the files are create under (directory nginx will refer too)
  "hlsdir": "/tmp/hls",                              // hls media directory ( if not specified no media files are generated )
  "portrange": "3000-3010",                          // specify port range ( if not set will allow all ports to be used )
  "url": "https://127.0.0.1/blah",                   // the block chain end point
  "streams": [                                       // an array of streams to connect too
    {"eo": "rtsp://127.0.0.1:8554/test"},
    {"ir": "rtsp://127.0.0.1:9000/test"}
  ]
}

To Run
------

./arcticmist-media-server config.json

Docker
------

The sample config will constaintly keep trying to connect to whatever end points you specify in the config, which do not exist. ( polls every 10 seconds )

This is an example of test I ran:
I have set a single endpoint called ir... and I ran a rtsp server on rtsp://192.168.1.18:9000/test

{
  "archivedir": "/tmp/example/archive",
  "hlsdir": "/tmp/example/media",
  "url": "https://24.71.226.107/blockchain/data/write",
  "rtspstreams": [
    {"ir": "rtsp://192.168.1.18:9000/test"}
  ]
}

docker-compose
--------------

I added these volume mounts so I could specify the config /tmp/config.json and /tmp on my local machine is where all the data got written too.

volumes:
  - /tmp:/tmp
  - /tmp:/src/config

