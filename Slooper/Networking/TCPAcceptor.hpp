#ifndef SLOOPER_NETWORKING_TCPACCEPTOR_HPP
#define SLOOPER_NETWORKING_TCPACCEPTOR_HPP

#include <Slooper/Networking/Socket.hpp>

namespace slooper
{
	namespace networking
	{
		class TCPAcceptor : public Socket
		{
		public:

			TCPAcceptor() noexcept;

			~TCPAcceptor() noexcept;


			void bind(const std::string & _address, std::error_condition & _error) noexcept;

			void listen(unsigned int _queueSize, std::error_condition & _error) noexcept;
		};
	}
}

#endif //SLOOPER_NETWORKING_TCPACCEPTOR_HPP