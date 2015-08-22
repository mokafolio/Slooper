#include <iostream>
#include <Slooper/Networking/TCPSocket.hpp>

using namespace slooper;

int main(int _argc, const char * _args[])
{
	std::error_condition err;

	networking::TCPSocket connection;
	connection.open(networking::AddressFamily::IP4, err);
	if(err)
	{
		std::cout<<"Could not open socket"<<std::endl;
		return EXIT_FAILURE;
	}

	connection.connect("192.168.1.58:11000", err);
	if(err)
	{
		std::cout<<"Could not connect to server: "<<err.message()<<std::endl;
		return EXIT_FAILURE;
	}

	while(true)
	{
		networking::ByteArray bytes(100);
		std::size_t numBytes = connection.receive(bytes, err);
		bytes.resize(numBytes);

		if(err)
		{
			std::cout<<"Could not receive: "<<err.message()<<std::endl;
			return EXIT_FAILURE;
		}
		
		if(numBytes == 0)
		{
			std::cout<<"Disconnected from server"<<std::endl;
			break;
		}

		std::string message = networking::byteArrayToString(bytes);
		if(message == "server: 5")
		{
			std::cout<<"Closing connection"<<std::endl;
			connection.close();
			break;
		}
		else
		{
			std::cout<<message<<std::endl;
		}
	}

	return EXIT_SUCCESS;
}