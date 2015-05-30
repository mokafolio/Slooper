#include <Slooper/Networking/TCPAcceptor.hpp>

namespace slooper
{
	namespace networking
	{
		TCPAcceptor::TCPAcceptor() noexcept :
		Socket(SocketType::TCP)
		{

		}

		TCPAcceptor::~TCPAcceptor() noexcept
		{

		}

		void TCPAcceptor::bind(const SocketAddress & _address, std::error_condition & _error) noexcept
		{
			//only ip4 for now
			int res = ::bind(m_socketfd, &_address.baseAddress, sizeof(sockaddr_in));
			if(res < 0)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
		}

		void TCPAcceptor::bind(const std::string & _address, std::error_condition & _error) noexcept
		{
			SocketAddress addr = Socket::socketAddressFromString(_address, _error);
			if(_error) return;

			bind(addr, _error);
		}

		void TCPAcceptor::listen(unsigned int _queueSize, std::error_condition & _error) noexcept
		{
			int res = ::listen(m_socketfd, _queueSize);
			if(res < 0)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
		}

		void TCPAcceptor::accept(TCPSocket & _sock, SocketAddress & _newAddress, std::error_condition & _error) noexcept
		{
			int result;
			socklen_t len;
			do
			{
				result = ::accept(m_socketfd, &_newAddress.baseAddress, &len);
			}
			while(result != 0 && errno == EINTR);
			if(result < 0 && result != EINTR)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
			_sock.setNativeSocket(result);
		}
	}
}