include .env

help:
	@echo "--Project commands --"
	@echo "[start]\t\t\t make build run"
	@echo "[stop]\t\t\t make stop clean"
	@echo "[certs]\t\t\t make copy_certs"
	@echo ""
	@echo "--VARIABLES (in .env) --"
	@echo "[timezone]\t\t $(shell date)"
	@echo "[CONFIG_FILE]\t\t $(CONFIG_FILE)"
	@echo "[KEY_FIELDS]\t\t $(KEY_FIELDS)"

# Docker project commands
build:
	docker build -t rtsp_client --build-arg KEY_FIELDS=$(KEY_FIELDS) .
run:
	docker run -d --name rtsp_client -v $(TZ_BIND) -v $(CONFIG_FILE):/rtsp_client/config/config.json rtsp_client
stop:
	docker kill rtsp_client && docker container rm rtsp_client
clean:
	docker image prune && docker image rm rtsp_client

# View application logs
logs:
	docker logs -f rtsp_client

# copy keys to share with another application service
copy_certs:
	mkdir -p keys
	docker cp rtsp_client:/etc/ssl/certs/dhparam.pem keys/
	docker cp rtsp_client:/etc/ssl/private/nginx-selfsigned.key keys/
	docker cp rtsp_client:/etc/ssl/certs/nginx-selfsigned.crt keys/

docs:
	doxygen
	@echo "Put this into your browser: $(shell pwd)/docs/doxygen/html/index.html"
