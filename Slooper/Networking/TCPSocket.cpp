#include <Slooper/Networking/TCPSocket.hpp>

namespace slooper
{
	namespace networking
	{
		TCPSocket::TCPSocket() noexcept :
		Socket(SocketType::TCP)
		{

		}

		TCPSocket::~TCPSocket() noexcept
		{

		}

		void TCPSocket::connect(const std::string & _address, std::error_condition & _error) noexcept
		{
			int result;
			SocketAddress addr = Socket::socketAddressFromString(_address, _error);
			if(_error) return;
			do
			{
            	//only ip4 for now
				result = ::connect(m_socketfd, &addr.baseAddress, sizeof(sockaddr_in));
			}
			while(result != 0 && errno == EINTR);

			if(result < 0 && result != EINTR)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
		}

		std::size_t TCPSocket::send(const ByteArray & _bytes, std::error_condition & _error) noexcept
		{	ssize_t result;
			do
			{
				result = ::send(m_socketfd, &_bytes[0], _bytes.size(), 0);
			}
			while(result != 0 && errno == EINTR);
			if(result < 0 && result != EINTR)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
			return result;
		}

		std::size_t TCPSocket::receive(ByteArray & _buffer, std::error_condition & _error) noexcept
		{
			ssize_t result;
			do
			{
				result = ::recv(m_socketfd, &_buffer[0], _buffer.size(), 0);
			}
			while(result != 0 && errno == EINTR);

			if(result < 0 && result != EINTR)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
			return result;
		}
	}
}