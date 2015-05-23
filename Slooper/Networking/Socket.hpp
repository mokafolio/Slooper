#ifndef SLOOPER_NETWORKING_SOCKET_HPP
#define SLOOPER_NETWORKING_SOCKET_HPP

#include <system_error>
#include <vector>
#include <string>

#include <stdlib.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

namespace slooper
{
	namespace networking
	{
		enum class AddressFamily
		{
            IP4 = 0, /**< IP4 address family. */
            IP6 = 1, /**< IP6 address family. */
            All = 2, /**< Any address family. */
            Unknown = 3 /**< Unknown address family. */
		};

		enum class SocketType
		{
			TCP,
			UDP
		};

		union SocketAddress
		{
			sockaddr baseAddress;
			sockaddr_in ip4Address;
			sockaddr_in6 ip6Address;
		};

		typedef std::vector<char> ByteArray;

		class Socket
		{
		public:

			Socket(SocketType _type) noexcept;

			virtual ~Socket() noexcept;


			void open(AddressFamily _family, std::error_condition & _error) noexcept;

			void close() noexcept;

			bool isOpen() const noexcept;

			void setBlocking(bool _flag, std::error_condition & _error) noexcept;

			bool isListening() const noexcept;

			bool isBlocking() const noexcept;

			void setNativeSocket(int _socketfd) noexcept;


		protected:


			static SocketAddress socketAddressFromString(const std::string & _address, std::error_condition & _error);


			int m_socketfd;

			SocketType m_type;
		};
	}
}

#endif //SLOOPER_NETWORKING_SOCKET_HPP