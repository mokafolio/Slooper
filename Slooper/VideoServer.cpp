#include <iostream>
#include <Slooper/Networking/TCPAcceptor.hpp>
#include <thread>
#include <atomic>
#include <sstream>
#include <cassert>
#include <signal.h>

using namespace slooper;

class VideoConnection
{
public:

	enum class Command
	{
		WaitForReadyFromRemote,
		WaitForOthers,
		StartVideo
	};

	VideoConnection(std::size_t _id) :
	m_id(_id),
	m_bReadyToLoop(false),
	m_currentCommand(Command::WaitForReadyFromRemote)
	{

	}

	~VideoConnection()
	{
		std::cout<<"~VideoConnection"<<std::endl;
		if(m_thread)
			m_thread->join();
	}

	void start()
	{
		m_bIsRunning = true;
		m_thread = std::unique_ptr<std::thread>(new std::thread(std::bind(&VideoConnection::threadFunc, this)));
	}

	void threadFunc()
	{
		std::cout<<"THREAD ID: "<<std::this_thread::get_id()<<std::endl;
		int num = 0;
		while(m_bIsRunning)
		{
			auto sleepTime = std::chrono::milliseconds(10);
			if(m_socket.isOpen())
			{
				if(m_currentCommand == Command::WaitForReadyFromRemote)
				{
					std::cout<<"Waiting for remote client"<<std::endl;
					std::error_condition err;
					networking::ByteArray bytes(5);
					std::size_t numBytes = m_socket.receive(bytes, err);
					bytes.resize(numBytes);

					if(err)
					{
						std::cout<<"An error occured when waiting for ready signal: "<<err.message()<<std::endl;
						m_bIsRunning = false;
						break;
					}

					if(networking::byteArrayToString(bytes) == "ready")
					{
						m_bReadyToLoop = true;
						m_currentCommand = Command::WaitForOthers;
					}
					else
					{
						std::cout<<"The video client could not get ready!"<<std::endl;
						break;
					}
				}
				else if(m_currentCommand == Command::StartVideo)
				{
					std::error_condition err;
					networking::ByteArray data = networking::byteArrayFromString("start");
					std::size_t bs = m_socket.send(data, err);

					if(err)
					{
						std::cout<<"Could not send start playing video message to client: "<<err.message()<<std::endl;
						break;
					}
					else
					{
						m_currentCommand = Command::WaitForReadyFromRemote;
					}
				}
				/*else if(m_currentCommand == Command)
				{

				}*/
			}
			else
			{
				std::cout<<"NOT OPEN"<<std::endl;
				break;
			}

			std::this_thread::sleep_for(sleepTime);
		}

		std::cout<<"Leaving thread"<<std::endl;
	}

	std::size_t m_id;

	networking::TCPSocket m_socket;

	std::unique_ptr<std::thread> m_thread;

	std::atomic<bool> m_bIsRunning;

	std::atomic<bool> m_bReadyToLoop;
	
	Command m_currentCommand;
};

int main(int _argc, const char * _args[])
{	
	int clientCount = 0;
	if(_argc == 2)
	{
		clientCount = atoi(_args[1]);
	}
	std::vector<std::unique_ptr<VideoConnection>> videoConnections(clientCount);

	if(clientCount)
	{
		//start server
	}

	bool bIsPlaying = false;
	double timeElapsed = 0;
	while(true)
	{
		if(!bIsPlaying)
		{
			bool bAllReady = false;
			if(videoConnections.size())
			{
				auto it = videoConnections.begin();
				bool bAll = true;
				for(; it != videoConnections.end(); ++it)
				{
					if(!(*it)->m_bReadyToLoop)
					{
						bAll = false;
						break;
					}
				}
				bAllReady = bAll;
			}
			else
			{
				bAllReady = true;
			}

			if(bAllReady)
			{
				std::cout<<"Starting Video"<<std::endl;
				bIsPlaying = true;
			}
		}
		else
		{
			if(timeElapsed >= 5.0)
			{
				std::cout<<"Finished playing"<<std::endl;
				bIsPlaying = false;
				timeElapsed = 0;
			}
			timeElapsed += (1.0/1000.0) * 10.0;
			//std::cout<<timeElapsed<<std::endl;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}