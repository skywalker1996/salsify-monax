/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

/* Copyright 2013-2018 the Alfalfa authors
                       and the Massachusetts Institute of Technology

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

      1. Redistributions of source code must retain the above copyright
         notice, this list of conditions and the following disclaimer.

      2. Redistributions in binary form must reproduce the above copyright
         notice, this list of conditions and the following disclaimer in the
         documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include <getopt.h>

#include <cstdlib>
#include <random>
#include <unordered_map>
#include <utility>
#include <tuple>
#include <queue>
#include <deque>
#include <thread>
#include <condition_variable>
#include <future>

#include <typeinfo>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <unistd.h>

#include "socket.hh"
#include "packet.hh"
#include "poller.hh"
#include "optional.hh"
#include "player.hh"
#include "display.hh"
#include "paranoid.hh"
#include "procinfo.hh"
#include "../uWebsockets/App.h"
#include "json.hpp"
#include "jsoncpp.cpp"

using namespace std;
using namespace std::chrono;
using namespace PollerShortNames;

int img_id = 1;

double frame_one_way_delay;
double AvgRTT;
double sending_throughput;


struct PerSocketData {
        /* Fill with user data */
    };

std::string_view ReadImage(const std::string& imagePath)
{
    std::ifstream imgStream;
    imgStream.open((char*)imagePath.c_str(), ios::binary|ios::in);
    imgStream.seekg(0, imgStream.end);
    int length=imgStream.tellg();
    imgStream.seekg(0,imgStream.beg);
    char* blob = new char[length];
    if(imgStream.is_open()){
      imgStream.read(blob,length);
    }
    imgStream.close();
    std::string_view image(blob,length);
    return image;
}


class AverageInterPacketDelay
{
private:
  static constexpr double ALPHA = 0.1;

  double value_ { -1.0 };
  uint64_t last_update_{ 0 };

public:
  void add( const uint64_t timestamp_us, const int32_t grace )
  {
    assert( timestamp_us >= last_update_ );

    if ( value_ < 0 ) {
      value_ = 0;
    }
    // else if ( timestamp_us - last_update_ > 0.2 * 1000 * 1000 /* 0.2 seconds */ ) {
    //   value_ /= 4;
    // }
    else {
      double new_value = max( 0l, static_cast<int64_t>( timestamp_us - last_update_ - grace ) );
      value_ = ALPHA * new_value + ( 1 - ALPHA ) * value_;
    }

    last_update_ = timestamp_us;
  }

  uint32_t int_value() const { return static_cast<uint32_t>( value_ ); }
};



void SendPicture(){
  uWS::App().ws<PerSocketData>("/*", {
      /* Settings */
      .compression = uWS::SHARED_COMPRESSOR,
      .maxPayloadLength = 128 * 1024,
      .idleTimeout = 0,
      .maxBackpressure = 1 * 1024 * 1024,
      /* Handlers */
      .upgrade = nullptr,
      .open = [](auto *ws) {
          /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */
          std::cout << "Connection Established!" << std::endl;
      },

      .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {

            std::string imageDir = "./images/";
            std::string imageName;
            std::string imagePath;
            std::string_view image;
            std::string FILENAME=imageDir + std::to_string(img_id)+ ".jpg";
            std::string FILENAME_next=imageDir + std::to_string(img_id+1)+ ".jpg";
            cout<<"read image file: "<<FILENAME<<endl;
            ifstream inFile(FILENAME_next, ifstream::in | ios::binary);
            // inFile.open(FILENAME, ios::in);

            // while(!inFile){
            //   usleep(10000);
            //   inFile.open(FILENAME,ios::in);
            // }
            
            // // inFile.open(FILENAME,ios::in);
            if(!inFile){
              cout<<"image read error (null)!!!"<<endl;
            }else{
              // imageName = std::string(message) + ".jpg";
              // imagePath = imageDir + imageName;
              image = ReadImage(FILENAME);

              jsonxx::json monitor_info;
              monitor_info["frame_one_way_delay"] = frame_one_way_delay;
              monitor_info["AvgRTT"] = AvgRTT;
              monitor_info["sending_throughput"] = sending_throughput;

              ws->send(image);
              ws->send(monitor_info.dump());

              // ws->send(image);
              img_id+=1;
              // remove((char*)imagePath.c_str());
              remove(FILENAME.c_str());
            }

            
      },
      .drain = [](auto */*ws*/) {
          /* Check ws->getBufferedAmount() here */
      },
      .ping = [](auto */*ws*/) {
          /* Not implemented yet */
      },
      .pong = [](auto */*ws*/) {
          /* Not implemented yet */
      },
      .close = [](auto */*ws*/, int /*code*/, std::string_view /*message*/) {
          /* You may access ws->getUserData() here */
          std::cout << "Connection Closed!" << std::endl;
      }

  }).listen(9001, [](auto *listen_socket) {
      if (listen_socket) {
          std::cout << "Listening on port " << 9001 << std::endl;
      }
  }).run();
}


void usage( const char *argv0 )
{
  cerr << "Usage: " << argv0 << " [-f, --fullscreen] [--verbose] PORT WIDTH HEIGHT" << endl;
}

uint16_t ezrand()
{
  random_device rd;
  uniform_int_distribution<uint16_t> ud;

  return ud( rd );
}

queue<RasterHandle> display_queue;
mutex mtx;
condition_variable cva;

void display_task( const VP8Raster & example_raster, bool fullscreen )
{
  VideoDisplay display { example_raster, fullscreen };

  while( true ) {
    unique_lock<mutex> lock( mtx );
    cva.wait( lock, []() { return not display_queue.empty(); } );

    while( not display_queue.empty() ) {
      // draw frames
      display.draw( display_queue.front() );
      display_queue.pop();
    }
  }
}

void enqueue_frame( FramePlayer & player, const Chunk & frame )
{
  if ( frame.size() == 0 ) {
    return;
  }

  const Optional<RasterHandle> raster = player.decode( frame );

  async( launch::async,
    [&raster]()
    {
      if ( raster.initialized() ) {
        lock_guard<mutex> lock( mtx );
        display_queue.push( raster.get() );
        cva.notify_all();
      }
    }
  );
}

int main( int argc, char *argv[] )
{
  /* check the command-line arguments */
  if ( argc < 1 ) { /* for sticklers */
    abort();
  }

  /* fullscreen player */
  bool fullscreen = false;
  bool verbose = false;


  const option command_line_options[] = {
    { "fullscreen", no_argument, nullptr, 'f' },
    { "verbose",    no_argument, nullptr, 'v' },
    { 0, 0, 0, 0 }
  };

  while ( true ) {
    const int opt = getopt_long( argc, argv, "f", command_line_options, nullptr );

    if ( opt == -1 ) {
      break;
    }

    switch ( opt ) {
    case 'f':
      fullscreen = true;
      break;

    case 'v':
      verbose = true;
      break;

    default:
      usage( argv[ 0 ] );
      return EXIT_FAILURE;
    }
  }

  if ( optind + 2 >= argc ) {
    usage( argv[ 0 ] );
    return EXIT_FAILURE;
  }

  /* choose a random connection_id */
  const uint16_t connection_id = 1337; // ezrand();
  cerr << "Connection ID: " << connection_id << endl;

  /* construct Socket for incoming  datagrams */
  UDPSocket socket;
  socket.bind( Address( "0", argv[ optind] ) );
  socket.set_timestamps();

  /* construct FramePlayer */
  FramePlayer player( paranoid::stoul( argv[ optind + 1 ] ), paranoid::stoul( argv[ optind + 2 ] ) );
  player.set_error_concealment( true );

  /* construct display thread */
  thread( [&player, fullscreen]() { display_task( player.example_raster(), fullscreen ); } ).detach();
  thread (SendPicture).detach();

  /* frame no => FragmentedFrame; used when receiving packets out of order */
  unordered_map<size_t, FragmentedFrame> fragmented_frames;
  size_t next_frame_no = 0;

  /* EWMA */
  AverageInterPacketDelay avg_delay;



  /*end to end delay */
  uint32_t frame_push_timestamp_last;

  /* decoder states */
  uint32_t current_state = player.current_decoder().get_hash().hash();
  const uint32_t initial_state = current_state;
  deque<uint32_t> complete_states;
  unordered_map<uint32_t, Decoder> decoders { { current_state, player.current_decoder() } };

  /* memory usage logs */
  system_clock::time_point next_mem_usage_report = system_clock::now();

  Poller poller;
  poller.add_action( Poller::Action( socket, Direction::In,
    [&]()
    {
      /* wait for next UDP datagram */
      const auto new_fragment = socket.recv();

      uint16_t frame_finish_state = 0;
      auto ack_start = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
      /* parse into Packet */
      const Packet packet { new_fragment.payload };

      uint32_t now_t = static_cast<uint32_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());

      uint32_t packet_one_way_delay = now_t - packet.packet_send_timestamp();
      // cout << "packet level one way dealy = " << packey_one_way_delay << endl;

      // cout << "size of fragmented_frames = " << fragmented_frames.size() << endl;
      // cout << "size of decoders = " << decoders.size() << endl;
      // cout << "size of complete_states = " << complete_states.size() << endl;

      
      if ( packet.frame_no() < next_frame_no ) {
        /* we're not interested in this anymore */
        return ResultType::Continue;
      }
      else if ( packet.frame_no() > next_frame_no ) {
        /* current frame is not finished yet, but we just received a packet
           for the next frame, so here we just encode the partial frame and
           display it and move on to the next frame */
        cerr << "got a packet for frame #" << packet.frame_no()
             << ", display previous frame(s)." << endl;
        
        frame_one_way_delay = now_t - frame_push_timestamp_last;
        AvgRTT = packet.rtt_average()/double(100);
        sending_throughput = packet.throughput()/double(100);

        // cout << "======================================================" << endl;
        // cout << "frame level one way dealy = " << frame_one_way_delay << endl;
        // cout << "======================================================" << endl;
        frame_finish_state = 1;
        for ( size_t i = next_frame_no; i < packet.frame_no(); i++ ) {
          if ( fragmented_frames.count( i ) == 0 ) continue;

          enqueue_frame( player, fragmented_frames.at( i ).partial_frame() );
          fragmented_frames.erase( i );
        }

        next_frame_no = packet.frame_no();
        current_state = player.current_decoder().minihash();
      }

      /* add to current frame */
      if ( fragmented_frames.count( packet.frame_no() ) ) {
        fragmented_frames.at( packet.frame_no() ).add_packet( packet );
      } else {
        /*
          This was judged "too fancy" by the Code Review Board of Dec. 29, 2016.

          fragmented_frames.emplace( std::piecewise_construct,
                                     forward_as_tuple( packet.frame_no() ),
                                     forward_as_tuple( connection_id, packet ) );
        */

        fragmented_frames.insert( make_pair( packet.frame_no(),
                                             FragmentedFrame( connection_id, packet ) ) );
      }

      /* is the next frame ready to be decoded? */
      if ( fragmented_frames.count( next_frame_no ) > 0 and fragmented_frames.at( next_frame_no ).complete() ) {
        auto & fragment = fragmented_frames.at( next_frame_no );

        uint32_t expected_source_state = fragment.source_state();

        if ( current_state != expected_source_state ) {
          if ( decoders.count( expected_source_state ) ) {
            /* we have this state! let's load it */
            player.set_decoder( decoders.at( expected_source_state ) );
            current_state = expected_source_state;
          }
        }

        if ( current_state == expected_source_state and
             expected_source_state != initial_state ) {
          /* sender won't refer to any decoder older than this, so let's get
             rid of them */

          auto it = complete_states.begin();

          for ( ; it != complete_states.end(); it++ ) {
            if ( *it != expected_source_state ) {
              decoders.erase( *it );
            }
            else {
              break;
            }
          }

          assert( it != complete_states.end() );
          complete_states.erase( complete_states.begin(), it );
        }

        // here we apply the frame
        enqueue_frame( player, fragment.frame() );
        
        frame_one_way_delay = now_t - frame_push_timestamp_last;
        AvgRTT = packet.rtt_average()/double(100);
        sending_throughput = packet.throughput()/double(100);
        // cout << "======================================================" << endl;
        // cout << "frame level one way dealy = " << frame_one_way_delay << endl;
        // cout << "AvgRTT*100 = " << packet.rtt_average() << endl;
        // cout << "======================================================" << endl;

        // state "after" applying the frame
        frame_finish_state = 1;
        current_state = player.current_decoder().minihash();

        if ( current_state == fragment.target_state() and
             current_state != initial_state ) {
          /* this is a full state. let's save it */
          decoders.insert( make_pair( current_state, player.current_decoder() ) );
          complete_states.push_back( current_state );
        }

        fragmented_frames.erase( next_frame_no );
        next_frame_no++;
      }

      avg_delay.add( new_fragment.timestamp_us, packet.time_since_last() );
      uint32_t ack_delay = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count() - ack_start;
      AckPacket( connection_id, packet.frame_no(), packet.fragment_no(),
                 avg_delay.int_value(), current_state, packet.packet_send_timestamp(),
                 ack_delay, frame_one_way_delay, frame_finish_state, complete_states ).sendto( socket, new_fragment.source_address );

      auto now = system_clock::now();

      if ( verbose and next_mem_usage_report < now ) {
        cerr << "["
             << duration_cast<milliseconds>( now.time_since_epoch() ).count()
             << "] "
             << " <mem = " << procinfo::memory_usage() << ">\n";
        next_mem_usage_report = now + 5s;
      }

      frame_push_timestamp_last = packet.frame_push_timestamp();  
      
      return ResultType::Continue;
    },
    [&]() { return not socket.eof(); } )
  );

  /* handle events */
  while ( true ) {
    const auto poll_result = poller.poll( -1 );
    if ( poll_result.result == Poller::Result::Type::Exit ) {
      return poll_result.exit_status;
    }
  }

  return EXIT_SUCCESS;
}
