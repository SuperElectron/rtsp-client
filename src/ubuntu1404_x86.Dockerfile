FROM ubuntu:14.04

RUN apt-get -y update
RUN apt-get -y install libgtk2.0-dev libglib2.0-dev g++ make
RUN apt-get -y install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly
RUN apt-get -y install gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa libgstreamer-plugins-base1.0-dev
RUN apt-get -y install gstreamer1.0* libjson-glib-dev

COPY . /src
WORKDIR /src

RUN make

CMD ["./arcticmist-media-server", "config/config.json"]