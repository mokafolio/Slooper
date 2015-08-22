#ifndef SLOOPER_VIDEOPLAYER_VIDEOPLAYER_HPP
#define SLOOPER_VIDEOPLAYER_VIDEOPLAYER_HPP

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

namespace slooper
{
	namespace videoPlayer
	{
		
		static inline OMX_TICKS toOMXTime(int64_t pts)
		{
			OMX_TICKS ticks;
			ticks.nLowPart = pts;
			ticks.nHighPart = pts >> 32;
			return ticks;
		}

			static inline double fromOMXTime(OMX_TICKS ticks)
			{
				uint64_t pts = ticks.nLowPart | ((uint64_t)ticks.nHighPart << 32);
				return (double)pts / (double)OMX_TICKS_PER_SECOND;
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
				m_videoFile(NULL),
				m_bPause(false),
				m_bReachedEnd(false)
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
      			//TODO
				}

				void pause()
				{
					{
						//ScopedLock lock(m_mutex);
						//std::cout<<"PAUSE START"<<std::endl;
						OMX_ERRORTYPE omx_err = OMX_ErrorNone;
						OMX_TIME_CONFIG_SCALETYPE scaleType;
						OMX_INIT_STRUCTURE(scaleType);

			           	scaleType.xScale = 0.0; // pause

			           	omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeScale, &scaleType);
			           	if(omx_err != OMX_ErrorNone)
			           	{
			           		ScopedLock lock(m_mutex);
			           		m_errorString = "Could not pause omx video player";
			           		return;
			           	}
			           }
			           m_bPause = true;
	           		//std::cout<<"PAUSE END"<<std::endl;
			       }

			       void resume()
			       {
			       	{
		           		//ScopedLock lock(m_mutex);
			       		OMX_ERRORTYPE omx_err = OMX_ErrorNone;
			       		OMX_TIME_CONFIG_SCALETYPE scaleType;
			       		OMX_INIT_STRUCTURE(scaleType);

		           		scaleType.xScale = 1 << 16; // unpause

		           		omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeScale, &scaleType);

		           		if(omx_err != OMX_ErrorNone)
		           		{
		           			ScopedLock lock(m_mutex);
		           			m_errorString = "Could not resume omx video player";
		           			return;
		           		}
		           	}
		           	m_bPause = false;
		           }

		           void setSpeed(float _speed)
		           {
	           		//ScopedLock lock(m_mutex);
		           	OMX_ERRORTYPE omx_err = OMX_ErrorNone;
		           	OMX_TIME_CONFIG_SCALETYPE scaleType;
		           	OMX_INIT_STRUCTURE(scaleType);
		           	scaleType.xScale = floor((_speed * pow(2,16)));
		           	omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeScale, &scaleType);

		           	if(omx_err != OMX_ErrorNone)
		           	{
		           		ScopedLock lock(m_mutex);
		           		m_errorString = "Could not pause omx video player";
		           	}
		           	std::cout<<"SET DA SPEED"<<std::endl;
		           }

		           void seek(double _seconds)
		           {
		           	ScopedLock lock(m_mutex);
		           	ilclient_change_component_state(m_omxVideoRenderer, OMX_StatePause);
		           	ilclient_change_component_state(m_omxVideoDecode, OMX_StatePause);
		           	ilclient_change_component_state(m_omxVideoScheduler, OMX_StatePause);
		           	ilclient_change_component_state(m_omxClock, OMX_StatePause);
		           	OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  					OMX_TIME_CONFIG_CLOCKSTATETYPE cs;
  					OMX_INIT_STRUCTURE(cs);

  					cs.eState = OMX_TIME_ClockStateStopped;

  					omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeClockState, &cs);
					if(omx_err != OMX_ErrorNone)
					{
						m_errorString = "Could not stop omx clock.";
						return;
					}

					/*OMX_TIME_CONFIG_TIMESTAMPTYPE ts;
					OMX_INIT_STRUCTURE(ts);
					ts.nPortIndex = OMX_ALL;
					ts.nTimestamp = toOMXTime((uint64_t)_seconds * OMX_TICKS_PER_SECOND);
					omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimePosition, &ts);
					if(omx_err != OMX_ErrorNone)
		           	{
		           		m_errorString = "Could not set media time.";
		           	}*/

		           	//flush all the tunnels
					ilclient_flush_tunnels(m_omxTunnels, 3);

					cs.eState = OMX_TIME_ClockStateWaitingForStartTime;
					cs.nStartTime = toOMXTime(0);
					cs.nWaitMask |= OMX_CLOCKPORT1;
      				cs.nWaitMask |= OMX_CLOCKPORT2;
  					omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeClockState, &cs);
					if(omx_err != OMX_ErrorNone)
					{
						m_errorString = "Could not restart omx clock.";
						return;
					}
					ilclient_change_component_state(m_omxClock, OMX_StateExecuting);
					ilclient_change_component_state(m_omxVideoScheduler, OMX_StateExecuting);
					ilclient_change_component_state(m_omxVideoDecode, OMX_StateExecuting);
					ilclient_change_component_state(m_omxVideoRenderer, OMX_StateExecuting);
		           }

		           double currentTime() const
		           {
		           	OMX_ERRORTYPE omx_err = OMX_ErrorNone;
		           	double pts = 0;

		           	OMX_TIME_CONFIG_TIMESTAMPTYPE timeStamp;
		           	OMX_INIT_STRUCTURE(timeStamp);
		           	timeStamp.nPortIndex = OMX_ALL;

		           	omx_err = OMX_GetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeCurrentMediaTime, &timeStamp);
		           	if(omx_err != OMX_ErrorNone)
		           	{
		           		m_errorString = "Could not get media time.";
		           		return 0;
		           	}

		           	pts = fromOMXTime(timeStamp.nTimestamp);
		           	return pts;
		           }  

		           double currentPosition() const
		           {
		           }

		           void setClockTime(double _seconds)
		           {
		           	
		           	/*OMX_ERRORTYPE omx_err = OMX_ErrorNone;
					OMX_TIME_CONFIG_TIMESTAMPTYPE ts;
					OMX_INIT_STRUCTURE(ts);

					ts.nPortIndex = 80;
					ts.nTimestamp = toOMXTime((uint64_t)_seconds * OMX_TICKS_PER_SECOND);
					omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeClientStartTime, &ts);
					//omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeCurrentVideoReference, &ts);
					if(omx_err != OMX_ErrorNone)
		           	{
		           		m_errorString = "Could not set media time.";
		           	}*/

		           	/*OMX_ERRORTYPE omx_err = OMX_ErrorNone;
  					OMX_TIME_CONFIG_CLOCKSTATETYPE clock;
  					OMX_INIT_STRUCTURE(clock);

  					clock.eState = OMX_TIME_ClockStateStopped;

  					omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeClockState, &clock);
					if(omx_err != OMX_ErrorNone)
					{
						m_errorString = "Could not stop omx clock.";
						return;
					}*/
					/*
		           	OMX_TIME_CONFIG_CLOCKSTATETYPE cstate;
					memset(&cstate, 0, sizeof(cstate));
					cstate.nSize = sizeof(cstate);
					cstate.nVersion.nVersion = OMX_VERSION;
					cstate.eState = OMX_TIME_ClockStateRunning;
					cstate.nWaitMask = 1;
					if(OMX_SetParameter(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeClockState, &cstate) != OMX_ErrorNone)
					{
						m_errorString = "Could not restart omx clock.";
						return;
					}*/

					//set_tunnel(m_omxTunnels+2, m_omxClock, 80, m_omxVideoScheduler, 12);
					//ilclient_change_component_state(m_omxClock, OMX_StateIdle);

						/*OMX_TIME_CONFIG_MEDIATIMEREQUESTTYPE request;
						OMX_INIT_STRUCTURE(request);
						request.nPortIndex = OMX_ALL;
						request.nMediaTimestamp = toOMXTime((uint64_t)_seconds * OMX_TICKS_PER_SECOND);
						omx_err = OMX_SetConfig(ILC_GET_HANDLE(m_omxClock), OMX_IndexConfigTimeMediaTimeRequest, &request);
						if(omx_err != OMX_ErrorNone)
					{
						m_errorString = "Could not seek omx clock.";
						return;
					}*/


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

		           bool reachedEnd() const
		           {
		           	return m_bReachedEnd;
		           }

		           void resetFileReadPosition()
		           {
		           	ScopedLock lock(m_mutex);
		           	std::fseek(m_videoFile, 0, SEEK_SET);
		           	m_bReachedEnd = false;
		           }

		       private:

		       	void playbackThreadFunc()
		       	{
		       		unsigned int data_len = 0;
		       		OMX_BUFFERHEADERTYPE *buf;
		       		int port_settings_changed = 0;
		       		int first_packet = 1;

		       		{
	       			//ScopedLock lock(m_mutex);
		       			ilclient_change_component_state(m_omxVideoDecode, OMX_StateExecuting);
		       		}

		       		do
		       		{
		       			if(!m_bPause && !m_bReachedEnd)
		       			{
	       					//ScopedLock lock(m_mutex);
							std::cout<<"LOOP START"<<std::endl;
							auto start = std::chrono::system_clock::now();
		       				buf = ilclient_get_input_buffer(m_omxVideoDecode, 130, 1);
					//std::cout<<"GET INPUT BUFFER: "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count()<<std::endl;

         			// feed data and wait until we get port settings changed
		       				unsigned char * dest = buf->pBuffer;
		       				data_len += std::fread(dest, 1, buf->nAllocLen-data_len, m_videoFile);
		       				std::cout<<"LOOP END: "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start).count()<<std::endl;
		       				
		       				if(port_settings_changed == 0 &&
		       					((data_len > 0 && ilclient_remove_event(m_omxVideoDecode, OMX_EventPortSettingsChanged, 131, 0, 0, 1) == 0) ||
		       						(data_len == 0 && ilclient_wait_for_event(m_omxVideoDecode, OMX_EventPortSettingsChanged, 131, 0, 0, 1,
		       							ILCLIENT_EVENT_ERROR | ILCLIENT_PARAMETER_CHANGED, 10000) == 0)))
		       				{
		       					std::cout<<"SET UP TUNNELS"<<std::endl;
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
	       					//std::fseek(m_videoFile, 0, SEEK_SET);
	       					//first_packet = true;
		       					m_bReachedEnd = true;
		       					port_settings_changed = 0;
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
		       			}
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

		       	mutable std::string m_errorString;

		       	COMPONENT_T * m_omxClock;
		       	COMPONENT_T * m_omxVideoDecode;
		       	COMPONENT_T * m_omxVideoScheduler;
		       	COMPONENT_T * m_omxVideoRenderer;
		       	COMPONENT_T * m_omxComponentList[5];
		       	TUNNEL_T m_omxTunnels[4];
		       	ILCLIENT_T * m_ilClient;
		       	std::FILE * m_videoFile;
		       	std::size_t m_fileSize;
		       	std::atomic<bool> m_bPause;
		       	std::atomic<bool> m_bReachedEnd;
		       }; 
		   }
		}

#endif //SLOOPER_VIDEOPLAYER_VIDEOPLAYER_HPP