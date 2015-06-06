#include <iostream>
#include <Slooper/Networking/TCPAcceptor.hpp>
#include <thread>
#include <atomic>
#include <sstream>
#include <cassert>
#include <signal.h>

using namespace slooper;

class TCPConection
{
public:

	TCPConection(std::size_t _id) :
	m_id(_id)
	{

	}

	~TCPConection()
	{
		std::cout<<"~TCPConection"<<std::endl;
		if(m_thread)
			m_thread->join();
	}

	void start()
	{
		m_bIsRunning = true;
		m_thread = std::unique_ptr<std::thread>(new std::thread(std::bind(&TCPConection::threadFunc, this)));
	}

	void threadFunc()
	{
		std::cout<<"THREAD ID: "<<std::this_thread::get_id()<<std::endl;
		int num = 0;
		while(m_bIsRunning)
		{
			if(m_socket.isOpen())
			{
				std::error_condition err;
				std::stringstream strstr;
				strstr << num;
				networking::ByteArray data = networking::byteArrayFromString("server: " + strstr.str());
				std::size_t bs = m_socket.send(data, err);

				//std::cout<<"Bytes sent: "<<bs<<std::endl;

				//m_bIsRunning = false;
				if(err)
				{
					std::cout<<"An error occured in TCPConnection"<<std::endl;
					m_bIsRunning = false;
					break;
				}
				else
				{
					num++;
					std::this_thread::sleep_for(std::chrono::seconds(1));
				}
			}
			else
			{
				std::cout<<"NOT OPEN"<<std::endl;
				break;
			}
		}

		std::cout<<"Leaving thread"<<std::endl;
	}

	std::size_t m_id;

	networking::TCPSocket m_socket;

	std::unique_ptr<std::thread> m_thread;

	std::atomic<bool> m_bIsRunning;
};

int main(int _argc, const char * _args[])
{
	//we don't want to exit if a remote connection closes
	//due to un unhandled SIGPIPE signal.
	signal(SIGPIPE, SIG_IGN);

	std::error_condition err;
	networking::TCPAcceptor acceptor;
	acceptor.open(networking::AddressFamily::IP4, err);
	if(err)
	{
		std::cout<<"Could not open socket"<<std::endl;
		return EXIT_FAILURE;
	}

	acceptor.setOption(SOL_SOCKET, SO_REUSEADDR, 1, err);

	if(err)
	{
		std::cout<<"Could not set socket option SO_REUSEADDR: "<<err.message()<<std::endl;
		return EXIT_FAILURE;
	}

	acceptor.bind(networking::Socket::localhostWithPort(11000), err);
	if(err)
	{
		std::cout<<"Could not bind socket: "<<err.message()<<std::endl;
		return EXIT_FAILURE;
	}

	acceptor.listen(10, err);
	if(err)
	{
		std::cout<<"Could not listen on TCP socket: "<<err.message()<<std::endl;
		return EXIT_FAILURE;
	}

	networking::SocketAddress addr = acceptor.socketAddress(err);
	if(err)
	{
		std::cout<<"Could not get socket address: "<<err.message()<<std::endl;
		return EXIT_FAILURE;
	}
	std::cout<<"Listening for connections on: "<<networking::Socket::socketAddressToString(addr)<<std::endl;

	std::vector<std::unique_ptr<TCPConection> > connections;
	std::cout<<"MAIN THREAD ID: "<<std::this_thread::get_id()<<std::endl;
	while(true)
	{
		std::cout<<"Waiting for clients to connect"<<std::endl;
		TCPConection * connection = new TCPConection(connections.size());
		networking::SocketAddress newAddress;
		acceptor.accept(connection->m_socket, newAddress, err);
		if(err)
		{
			std::cout<<"Error accepting connection"<<std::endl;
			return EXIT_FAILURE;
		}
		std::cout<<"A client connected"<<std::endl;
		connections.push_back(std::unique_ptr<TCPConection>(connection));
		connection->start();
		std::cout<<"A client started: "<<connections.size()<<std::endl;
		auto it = connections.begin();
		for(; it != connections.end();)
		{
			if(!(*it)->m_bIsRunning)
			{
				std::cout<<"ERAAASE"<<std::endl;
				it = connections.erase(it);
			}
			else
			{
				it++;
			}
		}

		std::cout<<"Num remaining connections: "<<connections.size()<<std::endl;
	}

	return EXIT_SUCCESS;
}