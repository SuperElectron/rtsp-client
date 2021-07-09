APP:= arcticmist-media-server

TARGET_DEVICE = $(shell gcc -w -Wno-xxxx -Wno-error -Wdeprecated-declarations -std=c++11 | cut -f1 -d -)

SRCS:= $(wildcard *.cpp) $(wildcard *.c)

INCS:= $(wildcard *.h)

PKGS:= glib-2.0 gstreamer-1.0 gstreamer-video-1.0 x11 json-glib-1.0 gstreamer-app-1.0

#OBJS:= $(SRCS:.c=.o)
OBJS:= $(SRCS:.cpp=.o)

CFLAGS+= -I./ -I/usr/include/gstreamer-1.0 #-I/usr/lib/x86_64-linux-gnu/glib-2.0/include

LIBS+= -L$(LIB_INSTALL_DIR) -lgstapp-1.0 -lpthread -lm \
       -ldl -Wl,-rpath,$(LIB_INSTALL_DIR)

CFLAGS+= `pkg-config --cflags $(PKGS)`
LIBS+= `pkg-config --libs $(PKGS)`

all: $(APP)

%.o: %.cpp $(INCS) Makefile
	g++ -std=c++11 -c -o $@ $(CFLAGS) $(EXTRA_FLAGS) $<
#	$(CC) -std=c++11 -c -o $@ $(CFLAGS) $(EXTRA_FLAGS) $<

$(APP): $(OBJS) Makefile
	g++ -std=c++11 -o $(APP) $(OBJS) $(LIBS)
#       $(CC) -std=c++11 -o $(APP) $(OBJS) $(LIBS)

install: $(APP)
	cp -rv $(APP) $(APP_INSTALL_DIR)

clean:
	rm -rf $(OBJS) $(APP)
