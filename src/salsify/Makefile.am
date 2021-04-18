AM_CPPFLAGS = -I$(srcdir)/../util -I$(srcdir)/../decoder -I$(srcdir)/../display -I$(srcdir)/../input -I$(srcdir)/../encoder -I$(srcdir)/../net $(X264_CFLAGS) $(CXX11_FLAGS) `pkg-config opencv4 --cflags` 
AM_CXXFLAGS = $(PICKY_CXXFLAGS) $(NODEBUG_CXXFLAGS)
USOCKET_CPPFLAGS = -I$(srcdir)/../uWebsockets/uSockets/src
JSONXX_CPPFLAGS = -I$(srcdir)/../util/jsonxx
JSONCPP_CPPFLAGS = -I$(srcdir)/../util/jsoncpp

GRPC_CPPFLAGS = -I/home/librah/.local/include

AM_LDFLAGS = $(STATIC_BUILD_FLAG)
BASE_LDADD = ../input/libalfalfainput.a ../decoder/libalfalfadecoder.a ../util/libalfalfautil.a $(X264_LIBS) $(JPEG_LIBS)

LIBS = `pkg-config opencv4 --libs` -ljpeg -lz -pthread

VP8PLAY_BUILD :=
if BUILDVP8PLAY
VP8PLAY_BUILD += salsify-receiver real-webcam display-jpeg
endif

bin_PROGRAMS = salsify-sender fake-webcam $(VP8PLAY_BUILD)

salsify_sender_SOURCES = salsify-sender.cc
salsify_sender_CPPFLAGS = $(AM_CPPFLAGS) $(JSONXX_CPPFLAGS) $(JSONCPP_CPPFLAGS) $(GRPC_CPPFLAGS) $(GLU_CFLAGS) $(GLEW_CFLAGS) $(GLFW3_CFLAGS)  
salsify_sender_LDADD = ../net/libnet.a ../encoder/libalfalfaencoder.a $(BASE_LDADD) 

salsify_sender_LDFLAGS = -L/usr/local/lib -L/home/librah/.local/lib \
                            -pthread \
                            -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
                            -ldl

salsify_receiver_SOURCES = salsify-receiver.cc
salsify_receiver_CPPFLAGS = $(AM_CPPFLAGS) $(JSONXX_CPPFLAGS) $(JSONCPP_CPPFLAGS) $(USOCKET_CPPFLAGS) $(OPENCV_CPPFLAGS) $(GLU_CFLAGS) $(GLEW_CFLAGS) $(GLFW3_CFLAGS) 
salsify_receiver_LDADD = ../display/libalfalfadisplay.a ../net/libnet.a ../uWebsockets/uSockets/uSockets.a $(BASE_LDADD) $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS)
salsify_receiver_LDFLAGS = -pthread 

fake_webcam_SOURCES = fake-webcam.cc 
fake_webcam_LDADD = ../encoder/libalfalfaencoder.a $(BASE_LDADD)

real_webcam_SOURCES = real-webcam.cc
real_webcam_CPPFLAGS = $(AM_CPPFLAGS) $(GLU_CFLAGS) $(GLEW_CFLAGS) $(GLFW3_CFLAGS)
real_webcam_LDADD = ../display/libalfalfadisplay.a $(BASE_LDADD) $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS)
real_webcam_LDFLAGS =

display_jpeg_SOURCES = display-jpeg.cc
display_jpeg_CPPFLAGS = $(AM_CPPFLAGS) $(GLU_CFLAGS) $(GLEW_CFLAGS) $(GLFW3_CFLAGS)
display_jpeg_LDADD = ../display/libalfalfadisplay.a ../net/libnet.a $(BASE_LDADD) $(GLU_LIBS) $(GLEW_LIBS) $(GLFW3_LIBS)
display_jpeg_LDFLAGS = 