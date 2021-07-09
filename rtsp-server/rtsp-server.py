#!/usr/bin/env python
# Simple RTSP server. Run as-is or with a command-line to replace the default pipeline
import sys
import gi
gi.require_version("Gst", "1.0")
gi.require_version("GstRtspServer", "1.0")
import logging
from gi.repository import Gst, GstRtspServer, GObject  # noqa E402
import gstreamer.utils as gst_utils  # noqa E402

from pathlib import Path
from os import system
from datetime import datetime


def setup_logging(folder="/logs/python/", filename="python.log", level=logging.DEBUG):
    try:
        system(f"mkdir -p {folder}")
        filepath = folder + filename
        Path(filepath).touch(mode=0o777, exist_ok=True)
        system(f"ln -sf /dev/stdout {filepath}")
        logging.basicConfig(
            filename=filepath,
            filemode="w",
            level=level,
            format='%(asctime)s,%(msecs)d %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s'
        )
        logging.debug(f"\t\tLOG FILE LOCATION: {filepath}")
        print(f"Python logger started at {datetime.now()}\n", flush=True)
        pass
    except Exception as err:
        print(f"ERROR (setup_logging): {err}")


class MyFactory(GstRtspServer.RTSPMediaFactory):
    """ view below each `self.handler` for the gst-launch command to test that streams are working."""
    def __init__(self, pipeline_str=None, video_file=None):
        self.pipeline_str = pipeline_str
        self.video = video_file
        GstRtspServer.RTSPMediaFactory.__init__(self)

    def do_create_element(self, url):
        """
        Create a pipeline once the RTSP url has been called

        Parameters
        ----------
        url: passed in background from the application, has no use in this function but must be here!

        Returns
        -------
        Gst.parse_launch(pipeline_str) => gst-launch-1.0 `pipeline_str`
        """

        if self.pipeline_str == "video_h264rtp":
            # gst-launch-1.0 rtspsrc location=rtsp://127.0.0.1:8554/test latency=100 ! application/x-rtp,clock-rate=90000,payload=96 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! video/x-raw,width=640,height=480 ! autovideosink # noqa 501
            pipeline_str = gst_utils.to_gst_string(
                [
                    f"filesrc location={self.video}",
                    "queue max-size-buffers=10000",
                    "tsparse set-timestamps=true",
                    "tsdemux parse-private-sections=false name=demux",
                    "h264parse",
                    "rtph264pay config-interval=1 name=pay0 pt=96",
                ]
            )
        else:
            pipeline_str = gst_utils.to_gst_string(
                [
                    "videotestsrc is-live=true",
                    "video/x-raw,rate=2997/100,width=960,height=540",
                    "queue max-size-buffers=100 name=q_enc",
                    "textoverlay name=overlay shaded-background=true",
                    "x264enc tune=zerolatency",
                    "rtph264pay name=pay0 pt=96",
                ]
            )

        logging.debug("<starting pipeline> gst-launch-1.0 " + pipeline_str)
        return Gst.parse_launch(pipeline_str)


class GstServer:
    def __init__(self, args):
        mapping = args['mapping']
        if mapping[0] != '/':
            mapping = '/' + mapping
        # ip_address = "192.168.80.55"
        ip_address = "172.22.0.4"
        port = "8554"

        if args['pipeline'] == 'video_mpegts':
            logging.warning("I couldnt get the code below to work with a video file... ")
            raise RuntimeError

            # udp_port = '15000'
            # udp_host = '224.1.1.1'
            # logging.info("Setting up RTSP for mpegts")
            # self.server = GstRtspServer.RTSPServer.new()
            # self.server.props.service = port
            # self.server.attach(None)
            #
            # factory = GstRtspServer.RTSPMediaFactory.new()
            # factory.set_launch(
            #     f"( udpsrc name=pay0 port={udp_port} buffer-size=524288 caps=\"application/x-rtp, media=(string)video, encoding-name=(string)MP2T, clock-rate=(int)90000, pt=96 \" )")
            # factory.set_shared(True)
            # print("Mounting server")
            # self.server.get_mount_points().add_factory(mapping, factory)
            # user_message = f"/usr/bin/gst-launch-1.0 filesrc location=/videos/MX10_Combined.ts ! tee name=t ! tsparse set-timestamps=true ! tsdemux parse-private-sections=false program-number=1 ! mpegtsmux ! capsfilter caps='video/mpegts' ! rtpmp2tpay pt=96 ! rndbuffersize min=1500 max=3500 ! udpsink port={udp_port} host={udp_host} t. ! queue ! fakesink dump=true"
            # logging.info(f"RTSP server: start your stream with  ***\n\n {user_message}")

        else:
            logging.info("Setting up RTSP factory")
            self.server = GstRtspServer.RTSPServer()
            self.server.set_address(ip_address)
            self.server.set_service(port)
            f = MyFactory(pipeline_str=args['pipeline'], video_file=args['video'])
            f.set_shared(True)
            m = self.server.get_mount_points()
            m.add_factory(mapping, f)

        logging.debug(f"Gstreamer pipeline being launched: {args['pipeline']}")
        logging.debug(f"View stream: gst-play-1.0 rtsp://{ip_address}:{port}{mapping}")

        try:
            self.server.attach(None)
            loop.run()
        except Exception as err:
            logging.error(f"loop.run(): {err}")
        finally:
            print('Clean up and shut down rtsp server ...')
            Gst.Object.unref(loop)
            print('rtsp server closed')


if __name__ == "__main__":
    setup_logging()

    GObject.threads_init()
    Gst.init(None)
    loop = GObject.MainLoop()

    """
    You can change the following to invoke different pipeline strings for the RTSP server.
    Pipeline strings: 
        "video_mpegts": mpegtsmux ==> rtpmp2tpay ==> RTSP   (not working)
        "video_h264rtp": rtph264pay ==> RTSP (works)
        None: videotestsrc ==> h264enc ==> rtph264pay ==> RTSP (works)
    """

    parameters = {
        "mapping": '/test',
        "pipeline": "video_h264rtp",
        'loop': loop,
        'video': '/videos/MX10_combined.ts'
    }

    GstServer(parameters)  # noqa 501
    print('GstServer Finished.')
