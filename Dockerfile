# syntax = docker/dockerfile:1.2
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
# build arg to pass in fields to create openssl key
ARG KEY_FIELDS

# apt installations
COPY install/apt /install/apt
RUN rm -f /etc/apt/apt.conf.d/docker-clean
RUN --mount=type=cache,target=/var/cache/apt apt-get update -yqq --fix-missing && \
    apt-get install -yqq --no-install-recommends \
    $(cat /install/apt/general.apt) \
    $(cat /install/apt/gstreamer.apt) \
    $(cat /install/apt/supervisord.apt) \
    $(cat /install/apt/nginx.apt) && \
    rm -rf /var/lib/apt/lists/*

# setup supervisord
COPY install/server/supervisor.conf /etc/supervisor/conf.d/server.conf
RUN mkdir -p /var/lock/apache2 /var/run/apache2 /var/run/sshd /var/log/supervisor

# create SSL keys (to be shared with external client if they want to connect)
RUN mkdir -p /etc/ssl/private
RUN chmod 700 /etc/ssl/private
RUN openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/ssl/private/nginx-selfsigned.key \
    -out /etc/ssl/certs/nginx-selfsigned.crt -subj $KEY_FIELDS
RUN openssl dhparam -out /etc/ssl/certs/dhparam.pem 2048

# setup Nginx server and expose its ports so that others may view it
COPY install/server/nginx-default.conf /etc/nginx/sites-available/default
COPY install/server/nginx.conf /etc/nginx/nginx.conf
EXPOSE 80/tcp
EXPOSE 1935/tcp
WORKDIR /rtsp_client

# build the application
COPY src /rtsp_client
RUN chmod +x /rtsp_client -R
RUN cmake -B build .
RUN cmake --build build
#CMD ["/rtsp_client/build/rtsp_client", "/rtsp_client/config/config.json"]
CMD ["/usr/bin/supervisord", "-c", "/etc/supervisor/conf.d/server.conf"]
