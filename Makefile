all: podman-ubus

CXX?=g++
CXXFLAGS?=--std=c++17
#-ggdb
# For debugging

OBJS:= \
	objs/loop.o \
	objs/app.o \
	objs/signal.o \
	objs/mutex.o \
	objs/main.o

SHARED_OBJS:=objs/common.o objs/log.o

PODMAN_OBJS:= \
	objs/podman_query.o objs/podman_socket.o objs/podman_validate.o \
	objs/podman_network.o objs/podman_pod.o objs/podman_container.o \
	objs/podman_cmds.o objs/podman_scheduler.o objs/podman_busystat.o \
	objs/podman_t.o

UBUS_OBJS:= \
	objs/ubus.o objs/podman.o objs/ubus_podman.o

PODMAN_EXAMPLE_OBJS:= \
	objs/mutex.o objs/podman_signal.o objs/podman_dump.o objs/podman_main.o

include Makefile.curl

LIBS:=

# for cross-compiler:
UBUS_LIBS:=-lubox -lblobmsg_json -luci -lubus
# for native:
#UBUS_LIBS:=/usr/lib/libubox.a /usr/lib/libblobmsg_json.a /usr/lib/libuci.a /usr/lib/libubus.a

JSON_LIBS:=-ljsoncpp

INCLUDES:=-I./include -I./podman/include -I. $(CURL_INCLUDES)
EXTRA_CXXFLAGS:=-Wno-builtin-declaration-mismatch

DEPS:=$(CURL_LIBS)

objs/common.o: shared/common.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/app.o: shared/app.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/mutex.o: shared/mutex.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/signal.o: signal.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/log.o: shared/log.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_query.o: podman/podman_query.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_socket.o: podman/podman_socket.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_validate.o: podman/podman_validate.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_network.o: podman/podman_network.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_pod.o: podman/podman_pod.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_container.o: podman/podman_container.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_cmds.o: podman/podman_cmds.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_t.o: podman/podman_t.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_scheduler.o: podman/podman_scheduler.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_busystat.o: podman/podman_busystat.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ubus.o: ubus/ubus.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/loop.o: loop.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman.o: podman/podman.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/ubus_podman.o: ubus/ubus_podman.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/main.o: main.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_dump.o: podman/example/podman_dump.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -c -o $@ $<;

objs/podman_signal.o: podman/example/podman_signal.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -I./podman/example -c -o $@ $<;

objs/podman_main.o: podman/example/main.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(INCLUDES) -I./podman/example -c -o $@ $<;

podman-ubus: $(DEPS) $(SHARED_OBJS) $(OBJS) $(PODMAN_OBJS) $(UBUS_OBJS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(LDFLAGS) $(SHARED_OBJS) $(OBJS) $(PODMAN_OBJS) $(UBUS_OBJS) $(CURL_LIBS) $(LIBS) $(UBUS_LIBS) $(JSON_LIBS) -o $@;

podman_example: $(DEPS) $(SHARED_OBJS) $(PODMAN_OBJS) $(PODMAN_EXAMPLE_OBJS)
	$(CXX) $(CXXFLAGS) $(EXTRA_CXXFLAGS) $(LDFLAGS) $(SHARED_OBJS) $(PODMAN_OBJS) $(PODMAN_EXAMPLE_OBJS) $(CURL_LIBS) $(JSON_LIBS) -o $@;

clean: curl-clean
	rm -f objs/** podman-ubus podman_example
