#ifndef SLOOPER_NETWORKING_TCPACCEPTOR_HPP
#define SLOOPER_NETWORKING_TCPACCEPTOR_HPP

#include <Slooper/Networking/TCPSocket.hpp>

namespace slooper
{
	namespace networking
	{
		class TCPAcceptor : public Socket
		{
		public:

			TCPAcceptor() noexcept;

			~TCPAcceptor() noexcept;

			void bind(const SocketAddress & _address, std::error_condition & _error) noexcept;
			
			void bind(const std::string & _address, std::error_condition & _error) noexcept;

			void listen(unsigned int _queueSize, std::error_condition & _error) noexcept;

			void accept(TCPSocket & _sock, SocketAddress & _newAddress, std::error_condition & _error) noexcept;
		};
	}
}

#endif //SLOOPER_NETWORKING_TCPACCEPTOR_HPP