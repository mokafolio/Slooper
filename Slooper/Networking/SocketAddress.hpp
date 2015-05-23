#ifndef SLOOPER_NETWORKING_SOCKETADDRESS_HPP
#define SLOOPER_NETWORKING_SOCKETADDRESS_HPP

namespace slooper
{
	namespace networking
	{
		/** 
         * @brief Holds an address and a port ready to be used by a Socket.
         */
        class SocketAddress
        {
            
            friend core::String toString(const SocketAddress &) noexcept;
            
            friend Address toAddress(const SocketAddress &, core::ErrorReport &) noexcept;

        public:
            
            /** 
             * @brief Default constructor.
             */
            SocketAddress() noexcept;
            
            /** 
             * @brief Constructs a SocketAddress from a native socket address.
             * @param _addr Pointer to the native address.
             * @param _len Size of the native address in bytes.
             */
            SocketAddress(detail::NativeSocketAddressType * _addr, detail::NativeSocketLengthType _len) noexcept;
            
            /** 
             * @brief Constructs a local SocketAddress bound to a port.
             * @param _family The address family of the address.
             * @param _port The local port.
             */
            SocketAddress(AddressFamily _family, core::UInt16 _port) noexcept;
            
            /** 
             * @brief Constructs a SocketAddress from a String and a port.
             * @param _address The String form of the address.
             * @param _port The port.
             */
            SocketAddress(const core::String & _address, core::UInt16 _port) noexcept;
            
            /** 
             * @brief Constructs a SocketAddress from just a String
             *
             * The String should contain a valid address that is seperated from the port
             * IP4 example: 127.0.0.1:6666, IP6 example: [::1]:6666
             * @param _address The String containing address and port.
             */
            SocketAddress(const core::String & _addressAndPort);
            
            /** 
             * @brief Constructs a SocketAddress from and Address object and a port.
             * @param _address The Address object.
             * @param _port The port.
             */
            SocketAddress(const Address & _address, core::UInt16 _port) noexcept;
            
            /** 
             * @brief Copy constructs the SocketAddress.
             * @param _other The SocketAddress to copy.
             */
            SocketAddress(const SocketAddress & _other) noexcept;
            
            /**
             * @brief Creates an SocketAddress from a string
             * @param _other The SocketAddress as a String.
             */
            void setFromString(const core::String & _addressAndPort);
            
            /**
             * @brief Creates a SocketAddress from a string
             * @param _other The SocketAddress as a String.
             * @param _error Set if an error occurs.
             */
            void setFromString(const core::String & _addressAndPort, core::ErrorReport & _error) noexcept;
            
            /**
             * @brief Copies and assigns the internal SocketAddress representation from another SocketAddress object.
             * @param _other The SocketAddress to assign.
             * @return A reference to *this.
             */
            SocketAddress & operator = (const SocketAddress & _other) noexcept;
            
            /** 
             * @brief Compares the internal representations for equality.
             * @param _other The SocketAddress to compare to.
             * @return true if they are equal, false otherwise.
             */
            bool operator == (const SocketAddress & _other) const noexcept;
            
            /** 
             * @brief Compares the internal representations for equality.
             * @param _other The SocketAddress to compare to.
             * @return true if they are not equal, false otherwise.
             */
            bool operator != (const SocketAddress & _other) const noexcept;
            
            /** 
             * @brief Gets a pointer to the native representation.
             * @return The pointer to the internal representation.
             */
            const void * ptr() const noexcept;
            
            /** 
             * @brief Gets the size in bytes of the internal representation.
             * @return The size in bytes.
             */
            core::Size byteCount() const noexcept;
            
            /** 
             * @brief Gets the port of the SocketAddress.
             * @return The port of the SocketAddress.
             */
            core::UInt16 port() const noexcept;
            
            /** 
             * @brief Gets the IP of the SocketAddress as a String.
             * @return The IP as a String.
             */
            core::String ip() const noexcept;
            
            /** 
             * @brief Checks if the SocketAddress is an IP4.
             * @return true if the SocketAddress is an IP4, false otherwise.
             */
            bool isIP4() const noexcept;
            
            /** 
             * @brief Gets the native address family identifier.
             * 
             * The return value depends on the platform.
             */
            core::Int32 nativeFamilyFlag() const noexcept;
            
            /** 
             * @brief Gets the AddressFamily of the SocketAddress.
             * @return The AddressFamily of the SocketAddress.
             * @see motor::networking::AddressFamily
             */
            AddressFamily family() const noexcept;
            
            /** 
             * @brief Creates an IP4 SocketAddress of "127.0.0.1" and port 0.
             *
             * A port of 0 means, that the system will chose a free port.
             * @return The newly created SocketAddress.
             */
            static SocketAddress localhost() noexcept;
            
        private:
            
            union DataUnion
            {
                detail::NativeSocketAddressType m_base;
                detail::NativeSocketAddressV4Type m_v4;
                detail::NativeSocketAddressV6Type m_v6;
            } m_data;
            
            /** 
             * @brief private constructor helper
             */
            void fromAddressAndPort(const Address & _address, core::UInt16 _port) noexcept;
        };
	}
}

#endif //SLOOPER_NETWORKING_SOCKETADDRESS_HPP