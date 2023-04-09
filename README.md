
# Media Client

---

# Description


A service that will connect to multiple rtsp MPEG-TS video streams.

It will demux content (video and multiple klv), create a directory with the klv and original MPEG-TS source, compress it, and generate a fingerprint file that will be uploaded to remote blockchain. 

If a hlsdir is specified in the config.json then a h265/h264 video is encoded into separate hls streams per rtsp source.

The live hls video streams are made available via nginx proxy ( indexed directory ).

# Generating docs

- generate docs

```bash
# install packages
$ sudo apt-get install -y graphviz doxygen
# generate docs
$ make docs
  lookup cache used 0/65536 hits=0 misses=0
  finished...
  Put this into your browser: ~/rtsp-client/docs/doxygen/html/index.html

# View the Docs in your web browser (copy/paste output path below)
echo `pwd`/docs/doxygen/html/index.html
```

# Running the project

- the application is run from the config file located in src/config/config.json (read more below to customize)

```bash
# build docker container
make build
# run docker container
make run
# copy certs for an external client who wishes to pull HLS files via RTMP/RTSP
make copy_certs
```

---

# Configuring the application

__KEY_FIELDS: generating SSL keys__

- in the `.env` view the entry `KEY_FIELDS` which is used as the `-subj` switch for openssl when generating keys.
- read more in the [openssl-docs](https://www.digicert.com/kb/ssl-support/openssl-quick-reference-guide.htm#Usingthe-subjSwitch) for how to configure it for your own use

```bash
KEY_FIELDS="/C=CA/ST=BC/L=City/O=TestCompanyName/OU=IT/CN=mydomain.com"
```


__NGINX__

- In `install/server`, there are nginx configs to that the generated HLS sink  (rtsp in --> hlssink) can be used by an external client who wishes to connect and stream from this server
    - Files: `nginx-default.conf`, `nginx.conf`
    - Endpoint: an external client can connect and stream from  https://mydomain.com:443/media/. If you want to change `https://mydomain.com`, then change the argument based on `KEY_FIELDS` above

__CONFIG_FILE__

- for convenience, `make run` mounts `src/config/config.json` so that you may update it for each `make run`
- you may change `CONFIG_FILE` in .env to a path of your own if you wish

- **Definitions**
    - `archivedir`: directory where all the files are create under (directory nginx will refer too)
    - `hlsdir`: is where the HLS m3u8 files are kept (if not specified no media files are generated)
    - `portrange`: specifies ports range you wish to listen to for RTSP input ( if not set will allow all ports to be used )
    - `url`: the end point to send images (set up for blockchain client to receive and fingerprint images)
    - `streams`: an array of RTSP streams to which the application will connect 

```json
{
  "archivedir": "/tmp/archive", 
  "hlsdir": "/tmp/hls",                              
  "portrange": "3000-3010",
  "url": "https://127.0.0.1/blah",
  "streams": [
    {"video1": "rtsp://127.0.0.1:8554/test"},
    {"video2": "rtsp://127.0.0.1:9000/test"}
  ]
}
```

---

# Testing the application 

- clone the RTSP server: https://github.com/GStreamer/gst-rtsp-server
- run the following commands to serve video to this application

```bash
./test-launch --port 8554 "( filesrc location=/path/to/video1.ts ! tsparse time-stamp=true ! rtpmp2tpay name=pay0 pt=96 )"
./test-launch --port 9000 "( filesrc location=/path/to/video2.ts ! tsparse time-stamp=true ! rtpmp2tpay name=pay0 pt=96 )"
```
- then you may connect with this application and stream 

```bash
# install ffmpeg
$ sudo apt-get instally ffmpeg
# play the generated videos
$ ffplay "https://mydomain.com:443/media/"
```

---