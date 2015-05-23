#ifndef SLOOPER_NETWORKING_TCPSOCKET_HPP
#define SLOOPER_NETWORKING_TCPSOCKET_HPP

#include <Slooper/Networking/Socket.hpp>

namespace slooper
{
	namespace networking
	{
		class TCPSocket : public Socket
		{
		public:

			TCPSocket() noexcept;

			~TCPSocket() noexcept;


			void connect(const std::string & _address, std::error_condition & _error) noexcept;

			std::size_t send(const ByteArray & _bytes, std::error_condition & _error) noexcept;

			std::size_t receive(ByteArray & _buffer, std::error_condition & _error) noexcept;

		};
	}
}

#endif //SLOOPER_NETWORKING_TCPSOCKET_HPP