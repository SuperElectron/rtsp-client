FROM ubuntu:18.04
LABEL maintainer="terrasense <matthew.mccann@terrasense.ca>"

RUN apt-get -y update
RUN apt-get -y install libgtk2.0-dev libglib2.0-dev g++ make
RUN apt-get -y install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly
RUN apt-get -y install gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa libgstreamer-plugins-base1.0-dev
RUN apt-get -y install libjson-glib-dev
RUN apt-get -y install gir1.2-gst-rtsp-server-1.0 libgstreamer1.0-0 libgirepository1.0-dev librdkafka-dev
RUN apt-get install -y bash
COPY . /src
WORKDIR /src

#RUN make

#CMD ["./arcticmist-media-server", "config/config.json"]
