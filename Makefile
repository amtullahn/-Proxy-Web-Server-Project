# CSE 422 Project 2 makefile
# Modified from CSE422 FS13

# Options to set when compiling/linking the project.
CXXFLAGS=-g -std=c++11 -DBUFFER_SIZE=2048
LDFLAGS=

# The name of the executable to generate.
TARGET=proj2_client proj2_proxy


# The objects that should be compiled from the project source files (expected
# to correspond to actual source files, e.g. URL.o will come from URL.cc).
#
# You will want to add the name of your driver object to this list.
common_OBJS=HTTP_Message.o \
	HTTP_Request.o \
	HTTP_Response.o \
	TCP_Socket.o\
	URL.o

client_OBJS=proj2_client.o

proxy_OBJS=Proxy_Worker.o \
	proj2_proxy.o


# Have everything built automatically based on the above settings.
all: $(TARGET)

%.o: %.cc
	g++ -o $@ $(CXXFLAGS) -c $<

proj2_client: $(client_OBJS) $(common_OBJS)
	g++ -o $@ $^ $(LDFLAGS)

proj2_proxy: $(proxy_OBJS) $(common_OBJS)
	g++ -o $@ $^ $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(proxy_OBJS) $(client_OBJS) $(common_OBJS)
	$(RM) -rf Download


# Dependencies follow (i.e. which source files and headers a given object is
# built from).
TCP_Socket.o: TCP_Socket.h TCP_Socket.cc
URL.o: URL.cc URL.h
HTTP_Message.o: HTTP_Message.cc HTTP_Message.h
HTTP_Request.o: HTTP_Request.cc HTTP_Request.h HTTP_Message.h URL.o TCP_Socket.o
HTTP_Response.o: HTTP_Response.cc HTTP_Response.h HTTP_Message.h URL.o TCP_Socket.o
proj2_client.o: proj2_client.cc proj2_client.h HTTP_Request.o HTTP_Response.o
Proxy_Worker.o: Proxy_Worker.cc Proxy_Worker.h HTTP_Request.o HTTP_Response.o URL.o
proj2_proxy.o: proj2_proxy.cc Proxy_Worker.o
