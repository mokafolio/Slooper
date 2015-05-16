/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Video deocode demo using OpenMAX IL though the ilcient helper library

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <cmath>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdio>

extern "C"
{
   #include "bcm_host.h"
   #include "ilclient.h"
}

#define OMX_INIT_STRUCTURE(a) \
  memset(&(a), 0, sizeof(a)); \
  (a).nSize = sizeof(a); \
  (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
  (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
  (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
  (a).nVersion.s.nStep = OMX_VERSION_STEP

static inline uint64_t fromOMXTime(OMX_TICKS ticks)
{
   uint64_t pts = ticks.nLowPart | ((uint64_t)ticks.nHighPart << 32);
   return pts;
}

class VideoPlayer
{
public:

   typedef std::unique_lock<std::mutex> ScopedLock;

   VideoPlayer(const std::string & _videoPath) :
   m_omxClock(NULL),
   m_omxVideoDecode(NULL),
   m_omxVideoScheduler(NULL),
   m_omxVideoRenderer(NULL),
   m_ilClient(NULL),
   m_videoFile(NULL)
   {
      m_ilClient = ilclient_init();
      if(!m_ilClient)
      {
         m_errorString = "Could not initialize ILClient";
         return;
      }

      if(OMX_Init() != OMX_ErrorNone)
      {
         m_errorString = "Could not initialize OMX";
         return;
      }

      m_videoFile = std::fopen(_videoPath.c_str(), "rb");
      if(!m_videoFile)
      {
         m_errorString = "Could not open video file at: " + _videoPath;
         return;
      }

      std::fseek(m_videoFile, 0, SEEK_END);
      m_fileSize = std::ftell(m_videoFile);
      std::fseek(m_videoFile, 0, SEEK_SET);
      std::cout<<"FILE SIZE: "<<m_fileSize<<std::endl;

      memset(m_omxComponentList, 0, sizeof(m_omxComponentList));
      memset(m_omxTunnels, 0, sizeof(m_omxTunnels));

      // create video_decode
      if(ilclient_create_component(m_ilClient, &m_omxVideoDecode, "video_decode", (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0)
      {
         m_errorString = "Could not create omx video decorder.";
         return;
      }
      m_omxComponentList[0] = m_omxVideoDecode;

      // create video_render
      if(ilclient_create_component(m_ilClient, &m_omxVideoRenderer, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      {
         m_errorString = "Could not create omx video renderer.";
         return;
      }
      m_omxComponentList[1] = m_omxVideoRenderer;

      // create clock
      if(ilclient_create_component(m_ilClient, &m_omxClock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      {
         m_errorString = "Could not create omx clock.";
         return;
      }
      m_omxComponentList[2] = m_omxClock;

      // configure clock
      OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
      memset(&cstate, 0, sizeof(cstate));
      cstate.nSize = sizeof(cstate);
      cstate.nVersion.nVersion = OMX_VERSION;
      cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
      cstate.nWaitMask = 1;
      if(OMX_SetParameter(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
      {
         m_errorString = "Could not configure omx clock.";
         return;
      }

      // create video_scheduler
      if(ilclient_create_component(m_ilClient, &m_omxVideoScheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      {
         m_errorString = "Could not create omx video scheduler.";
         return;
      }
      m_omxComponentList[3] = m_omxVideoScheduler;

      set_tunnel(m_omxTunnels, m_omxVideoDecode, 131, m_omxVideoScheduler, 10);
      set_tunnel(m_omxTunnels+1, m_omxVideoScheduler, 11, m_omxVideoRenderer, 90);
      set_tunnel(m_omxTunnels+2, m_omxClock, 80, m_omxVideoScheduler, 12);

      // setup clock tunnel first
      if(ilclient_setup_tunnel(m_omxTunnels+2, 0, 0) != 0)
      {
         m_errorString = "Could not create omx clock tunnel.";
         return;
      }

      ilclient_change_component_state(m_omxClock, OMX_StateExecuting);
      ilclient_change_component_state(m_omxVideoDecode, OMX_StateIdle);

      OMX_VIDEO_PARAM_PORTFORMATTYPE format;
      memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
      format.nVersion.nVersion = OMX_VERSION;
      format.nPortIndex = 130;
      format.eCompressionFormat = OMX_VIDEO_CodingAVC;

      if(OMX_SetParameter(ILC_GET_HANDLE(m_omxVideoDecode), OMX_IndexParamVideoPortFormat, &format) != OMX_ErrorNone)
      {
         m_errorString = "Could not set video settings on omx decoder.";
         return;
      }

      if(ilclient_enable_port_buffers(m_omxVideoDecode, 130, NULL, NULL, NULL) != 0)
      {
         m_errorString = "Could not enable port buffer on omx video decoder.";
         return;
      }

      m_playbackThread = std::unique_ptr<std::thread>(new std::thread(std::bind(&VideoPlayer::playbackThreadFunc, this)));
   }

   ~VideoPlayer()
   {
      //join thread
      if(m_playbackThread)
      {
         m_playbackThread->join();
      }

      //clean up
   }

   void pause()
   {

   }

   void unpause()
   {

   }

   void setSpeed(float _speed)
   {

   }

   void seek(double _seconds)
   {

   }

   double currentTime() const
   {
      ScopedLock lock(m_mutex);

      OMX_TIME_CONFIG_TIMESTAMPTYPE tt;
      //OMX_INIT_STRUCTURE(tt);
      tt.nPortIndex = OMX_ALL;
      if (OMX_GetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeCurrentWallTime, &tt) != OMX_ErrorNone)
      {
         printf("timestamp: %d \n", tt.nTimestamp);
         return fromOMXTime(tt.nTimestamp);
      }
      else
      {
         std::cout<<"could not get timestamp"<<std::endl;
         return 0;
      }
   }  

   double currentPosition() const
   {

   }

   bool error() const
   {
      ScopedLock lock(m_mutex);
      return m_errorString.length() > 0;
   }

   const std::string & errorMessage() const
   {
      ScopedLock lock(m_mutex);
      return m_errorString;
   }

private:

   void playbackThreadFunc()
   {
      /*
      OMX_VIDEO_PARAM_PORTFORMATTYPE format;
      OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
      COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL, *clock = NULL;
      COMPONENT_T *list[5];
      TUNNEL_T tunnel[4];
      ILCLIENT_T *client;


      FILE *in;
      int status = 0;*/

      unsigned int data_len = 0;
      OMX_BUFFERHEADERTYPE *buf;
      int port_settings_changed = 0;
      int first_packet = 1;

      {
         ScopedLock lock(m_mutex);
         ilclient_change_component_state(m_omxVideoDecode, OMX_StateExecuting);
      }

      do
      {
         ScopedLock lock(m_mutex);
         
         buf = ilclient_get_input_buffer(m_omxVideoDecode, 130, 1);

         // feed data and wait until we get port settings changed
         unsigned char * dest = buf->pBuffer;
         data_len += std::fread(dest, 1, buf->nAllocLen-data_len, m_videoFile);

         if(port_settings_changed == 0 &&
            ((data_len > 0 && ilclient_remove_event(m_omxVideoDecode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
             (data_len == 0 && ilclient_wait_for_event(m_omxVideoDecode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                       ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
         {
            port_settings_changed = 1;

            if(ilclient_setup_tunnel(m_omxTunnels, 0, 0) != 0)
            {
               m_errorString = "could not set up omx tunnel.";
               break;
            }

            ilclient_change_component_state(m_omxVideoScheduler, OMX_StateExecuting);

            // now setup tunnel to video_render
            if(ilclient_setup_tunnel(m_omxTunnels + 1, 0, 1000) != 0)
            {
               m_errorString = "could not set up omx render tunnel.";
               break;
            }

            ilclient_change_component_state(m_omxVideoRenderer, OMX_StateExecuting);
         }
         
         if(!data_len) 
         {
            std::fseek(m_videoFile, m_fileSize / 10, SEEK_SET);
            first_packet = true;
         }


         buf->nFilledLen = data_len;
         data_len = 0;

         buf->nOffset = 0;
         if(first_packet)
         {
            buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
            first_packet = 0;
         }
         else
            buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

         if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(m_omxVideoDecode), buf) != OMX_ErrorNone)
         {
            m_errorString = "Could not empty omx video decode buffer.";
            break;
         }

         //std::cout<<"LOOP"<<std::endl;
      } while(buf);

      buf->nFilledLen = 0;
      buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

      if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(m_omxVideoDecode), buf) != OMX_ErrorNone)
      {
         m_errorString = "Could not empty omx video decode buffer.";
      }

      // wait for EOS from render
      ilclient_wait_for_event(m_omxVideoRenderer, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0,
                              ILCLIENT_BUFFER_FLAG_EOS, 10000);

      // need to flush the renderer to allow video_decode to disable its input port
      ilclient_flush_tunnels(m_omxTunnels, 0);

      ilclient_disable_port_buffers(m_omxVideoDecode, 130, NULL, NULL, NULL);
   };

   std::unique_ptr<std::thread> m_playbackThread;

   mutable std::mutex m_mutex;

   std::string m_errorString;

   COMPONENT_T * m_omxClock;
   COMPONENT_T * m_omxVideoDecode;
   COMPONENT_T * m_omxVideoScheduler;
   COMPONENT_T * m_omxVideoRenderer;
   COMPONENT_T * m_omxComponentList[5];
   TUNNEL_T m_omxTunnels[4];
   ILCLIENT_T * m_ilClient;
   std::FILE * m_videoFile;
   std::size_t m_fileSize;
};

static int video_decode_test(char *filename)
{
   OMX_VIDEO_PARAM_PORTFORMATTYPE format;
   OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
   COMPONENT_T *video_decode = NULL, *video_scheduler = NULL, *video_render = NULL, *clock = NULL;
   COMPONENT_T *list[5];
   TUNNEL_T tunnel[4];
   ILCLIENT_T *client;
   FILE *in;
   int status = 0;
   unsigned int data_len = 0;

   memset(list, 0, sizeof(list));
   memset(tunnel, 0, sizeof(tunnel));

   if((in = fopen(filename, "rb")) == NULL)
      return -2;

   if((client = ilclient_init()) == NULL)
   {
      fclose(in);
      return -3;
   }

   if(OMX_Init() != OMX_ErrorNone)
   {
      ilclient_destroy(client);
      fclose(in);
      return -4;
   }

   // create video_decode
   if(ilclient_create_component(client, &video_decode, "video_decode", (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_DISABLE_ALL_PORTS | ILCLIENT_ENABLE_INPUT_BUFFERS)) != 0)
      status = -14;
   list[0] = video_decode;

   // create video_render
   if(status == 0 && ilclient_create_component(client, &video_render, "video_render", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[1] = video_render;

   // create clock
   if(status == 0 && ilclient_create_component(client, &clock, "clock", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[2] = clock;

   OMX_ERRORTYPE omx_err = OMX_ErrorNone;
   OMX_TIME_CONFIG_SCALETYPE scaleType;
   OMX_INIT_STRUCTURE(scaleType);

   float speed = 1.0;
   scaleType.xScale = std::floor((speed * std::pow(2,16)));

   printf("set speed to : %.2f, %d\n", speed, scaleType.xScale);
    
   omx_err = OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeScale, &scaleType);
   if(omx_err != OMX_ErrorNone)
   {
        printf("Speed error setting OMX_IndexConfigTimeClockState : %d\n", omx_err);
        return -1;
   }

   memset(&cstate, 0, sizeof(cstate));
   cstate.nSize = sizeof(cstate);
   cstate.nVersion.nVersion = OMX_VERSION;
   cstate.eState = OMX_TIME_ClockStateWaitingForStartTime;
   cstate.nWaitMask = 1;
   if(clock != NULL && OMX_SetParameter(ILC_GET_HANDLE(clock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
      status = -13;

   // create video_scheduler
   if(status == 0 && ilclient_create_component(client, &video_scheduler, "video_scheduler", ILCLIENT_DISABLE_ALL_PORTS) != 0)
      status = -14;
   list[3] = video_scheduler;

   set_tunnel(tunnel, video_decode, 131, video_scheduler, 10);
   set_tunnel(tunnel+1, video_scheduler, 11, video_render, 90);
   set_tunnel(tunnel+2, clock, 80, video_scheduler, 12);

   // setup clock tunnel first
   if(status == 0 && ilclient_setup_tunnel(tunnel+2, 0, 0) != 0)
      status = -15;
   else
      ilclient_change_component_state(clock, OMX_StateExecuting);

   if(status == 0)
      ilclient_change_component_state(video_decode, OMX_StateIdle);

   memset(&format, 0, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
   format.nSize = sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE);
   format.nVersion.nVersion = OMX_VERSION;
   format.nPortIndex = 130;
   format.eCompressionFormat = OMX_VIDEO_CodingAVC;

   if(status == 0 &&
      OMX_SetParameter(ILC_GET_HANDLE(video_decode), OMX_IndexParamVideoPortFormat, &format) == OMX_ErrorNone &&
      ilclient_enable_port_buffers(video_decode, 130, NULL, NULL, NULL) == 0)
   {
      OMX_BUFFERHEADERTYPE *buf;
      int port_settings_changed = 0;
      int first_packet = 1;

      ilclient_change_component_state(video_decode, OMX_StateExecuting);

      while((buf = ilclient_get_input_buffer(video_decode, 130, 1)) != NULL)
      {
         // feed data and wait until we get port settings changed
         unsigned char *dest = buf->pBuffer;

         data_len += fread(dest, 1, buf->nAllocLen-data_len, in);

         if(port_settings_changed == 0 &&
            ((data_len > 0 && ilclient_remove_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
             (data_len == 0 && ilclient_wait_for_event(video_decode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
                                                       ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
         {
            port_settings_changed = 1;

            if(ilclient_setup_tunnel(tunnel, 0, 0) != 0)
            {
               status = -7;
               break;
            }

            ilclient_change_component_state(video_scheduler, OMX_StateExecuting);

            // now setup tunnel to video_render
            if(ilclient_setup_tunnel(tunnel+1, 0, 1000) != 0)
            {
               status = -12;
               break;
            }

            ilclient_change_component_state(video_render, OMX_StateExecuting);
         }
         if(!data_len) std::fseek(in, 0, SEEK_SET);

         buf->nFilledLen = data_len;
         data_len = 0;

         buf->nOffset = 0;
         if(first_packet)
         {
            buf->nFlags = OMX_BUFFERFLAG_STARTTIME;
            first_packet = 0;
         }
         else
            buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN;

         if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
         {
            status = -6;
            break;
         }
      }

      buf->nFilledLen = 0;
      buf->nFlags = OMX_BUFFERFLAG_TIME_UNKNOWN | OMX_BUFFERFLAG_EOS;

      if(OMX_EmptyThisBuffer(ILC_GET_HANDLE(video_decode), buf) != OMX_ErrorNone)
         status = -20;

      // wait for EOS from render
      ilclient_wait_for_event(video_render, OMX_EventBufferFlag, 90, 0, OMX_BUFFERFLAG_EOS, 0,
                              ILCLIENT_BUFFER_FLAG_EOS, 10000);

      // need to flush the renderer to allow video_decode to disable its input port
      ilclient_flush_tunnels(tunnel, 0);

      ilclient_disable_port_buffers(video_decode, 130, NULL, NULL, NULL);
   }

   fclose(in);

   ilclient_disable_tunnel(tunnel);
   ilclient_disable_tunnel(tunnel+1);
   ilclient_disable_tunnel(tunnel+2);
   ilclient_teardown_tunnels(tunnel);

   ilclient_state_transition(list, OMX_StateIdle);
   ilclient_state_transition(list, OMX_StateLoaded);

   ilclient_cleanup_components(list);

   OMX_Deinit();

   ilclient_destroy(client);
   return status;
}

int main (int argc, char **argv)
{
   if (argc < 2) {
      printf("Usage: %s <filename>\n", argv[0]);
      exit(1);
   }
   bcm_host_init();
   //return video_decode_test(argv[1]);
   VideoPlayer player(argv[1]);
   if(player.error())
   {
      std::cout<<"Could not initialize video player: "<<player.errorMessage()<<std::endl;
      return EXIT_FAILURE;
   }
   while(true)
   {
      std::cout<<"CURRENT TIME: "<<player.currentTime()<<std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   }
   return EXIT_SUCCESS;
}


