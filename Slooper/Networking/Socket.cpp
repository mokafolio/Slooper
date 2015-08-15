#include <Slooper/Networking/Socket.hpp>
#include <iostream>

namespace slooper
{
	namespace networking
	{
		ByteArray byteArrayFromString(const std::string & _str)
		{
			return ByteArray(_str.c_str(), _str.c_str() + _str.size());
		}

		std::string byteArrayToString(const ByteArray & _arr)
		{
			return std::string(_arr.begin(), _arr.begin() + _arr.size());
		}

		Socket::Socket(SocketType _type) noexcept :
		m_type(_type),
		m_socketfd(-1)
		{

		}

		Socket::~Socket() noexcept
		{
			if(isOpen())
				close();
		}

		void Socket::open(AddressFamily _family, std::error_condition & _error) noexcept
		{
			int addrType = 0;
			int sockType = 0;
			int protType = 0;

			if(_family == AddressFamily::IP4)
				addrType = PF_INET;
			else if(_family == AddressFamily::IP6)
				addrType = PF_INET6;

			if(m_type == SocketType::TCP)
			{
				sockType = SOCK_STREAM;
				protType = IPPROTO_TCP;
			}
			else if(m_type == SocketType::UDP)
			{
				sockType = SOCK_DGRAM;
				protType = IPPROTO_UDP;
			}

			m_socketfd = socket(addrType, sockType, protType);
			if(m_socketfd < 0)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
		}

		SocketAddress Socket::localhostWithPort(unsigned short _port)
		{
			SocketAddress ret;
			ret.ip4Address.sin_family = AF_INET;
			ret.ip4Address.sin_port = htons(_port);
			ret.ip4Address.sin_addr.s_addr = INADDR_ANY;
			return ret;
		}

		SocketAddress Socket::socketAddressFromString(const std::string & _address, std::error_condition & _error)
		{
			//retrieve port
			SocketAddress ret;
            unsigned short port = 0;
            std::string addr;
            std::size_t pos = _address.rfind(":");
            if(pos != std::string::npos)
            {
                std::string theId = _address.substr(pos+1);
                addr = _address.substr(0, pos);
                //Ipv6 needs to be in brackets, so we remove them
                std::string::iterator it = addr.begin();
                for(; it < addr.end(); )
                {
                    if((*it) == '[' || (*it) == ']')
                        it = addr.erase(it, it + 1);
                        else
                            ++it;
                }
                port = atoi(theId.c_str());
            }
            else
            {
            	//TODO: Error
            }

            std::cout<<"ADDR: "<<addr<<" PORT: "<<port<<std::endl;

            ret.ip4Address.sin_family = AF_INET;
			ret.ip4Address.sin_port = htons(port);

            //we only support ip4 for now
            int result = inet_pton(AF_INET, addr.c_str(), &(ret.ip4Address.sin_addr));
            if(result < 0)
            {
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
			return ret;
		}

		std::string Socket::socketAddressToString(const SocketAddress & _addr)
		{
			//only ip4 for now
			char str[INET_ADDRSTRLEN];
            const char * ret = ::inet_ntop(AF_INET, &(_addr.baseAddress), str, INET_ADDRSTRLEN);
            if(!ret)
                return "";
            return ret;
		}

		SocketAddress Socket::socketAddress(std::error_condition & _err) const noexcept
		{
			SocketAddress ret;
			socklen_t len = sizeof(ret);
			int res = ::getsockname(m_socketfd, &ret.baseAddress, &len);
			if(res < 0)
			{
				_err = std::error_code(errno, std::system_category()).default_error_condition();
			}
			return ret;
		}

		void Socket::close() noexcept
		{
			::close(m_socketfd);
		}

		bool Socket::isOpen() const noexcept
		{
			return m_socketfd != -1;
		}

		void Socket::setBlocking(bool _flag, std::error_condition & _error) noexcept
		{
			int flags = fcntl(m_socketfd, F_GETFL, 0);

			if(flags == -1)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}

			int res;
			if(!_flag)
				res = fcntl(m_socketfd, F_SETFL, flags | O_NONBLOCK);
			else
				res = fcntl(m_socketfd, F_SETFL, flags & ~O_NONBLOCK);

			if(res < 0)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
		}

		void Socket::setNativeSocket(int _socketfd) noexcept
		{
			if(isOpen())
				close();
			m_socketfd = _socketfd;
		}

		bool Socket::isListening() const noexcept
		{
			//TODO
			return false;
		}

		void Socket::setOption(int _level, int _option, int _value, std::error_condition & _error) noexcept
		{
			int res = ::setsockopt(m_socketfd, _level, _option, &_value, sizeof(_value));
			
			if(res < 0)
			{
				_error = std::error_code(errno, std::system_category()).default_error_condition();
			}
		}

		bool Socket::isBlocking() const noexcept
		{
			int flags = fcntl(m_socketfd, F_GETFL, 0);

			if(flags == -1)
				return false;

			return (flags & O_NONBLOCK) == 0;
		}
	}
}