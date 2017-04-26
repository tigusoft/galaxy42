#include "c_tuntap_linux_obj.hpp"
#ifdef ANTINET_linux

#include "../../../depends/cjdns-code/NetPlatform.h"
#include "../../cpputils.hpp"
#include "../../utils/check.hpp"
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../haship.hpp"

c_tuntap_linux_obj::c_tuntap_linux_obj() :
	m_tun_fd(open("/dev/net/tun", O_RDWR)),
	m_io_service(),
	m_tun_stream(m_io_service, m_tun_fd)
{
	_fact("tuntap opened with m_tun_fd=" << m_tun_fd);
	_try_sys(m_tun_fd != -1);
	_check_sys(m_tun_stream.is_open());
	_goal("tuntap is opened correctly");
}

size_t c_tuntap_linux_obj::send_to_tun(const unsigned char *data, size_t size) {
	return m_tun_stream.write_some(boost::asio::buffer(data, size));
}

size_t c_tuntap_linux_obj::read_from_tun(unsigned char *const data, size_t size) {
	return m_tun_stream.read_some(boost::asio::buffer(data, size));
}

size_t c_tuntap_linux_obj::read_from_tun_separated_addresses(unsigned char *const data, size_t size,
	std::array<unsigned char, IPV6_LEN> &src_binary_address,
	std::array<unsigned char, IPV6_LEN> &dst_binary_address) {
	// field sizes based on rfc2460
	// https://tools.ietf.org/html/rfc2460
	std::array<boost::asio::mutable_buffer, 4> buffers;
	buffers.at(0) = boost::asio::buffer(data, 8); // version, traffic, flow label, payload length, next header, hop limit
	buffers.at(1) = boost::asio::buffer(src_binary_address.data(), src_binary_address.size());
	buffers.at(2) = boost::asio::buffer(dst_binary_address.data(), dst_binary_address.size());
	buffers.at(3) = boost::asio::buffer(data + 8, size - 8); // 8 bytes are filled in buffers.at(0)
	return m_tun_stream.read_some(buffers) - src_binary_address.size() - dst_binary_address.size();
}

void c_tuntap_linux_obj::async_receive_from_tun(unsigned char *const data,
                                                size_t size,
                                                const c_tuntap_base_obj::read_handler & handler) {

	auto asio_handler = [data, handler](const boost::system::error_code& error, std::size_t bytes_transferred) {
		handler(data, bytes_transferred, error);
	};
	return m_tun_stream.async_read_some(boost::asio::buffer(data, size), asio_handler);
}

void c_tuntap_linux_obj::set_tun_parameters(const std::array<unsigned char, IPV6_LEN> &binary_address,
                                            int prefix_len,
                                            uint32_t mtu) {

	c_haship_addr address(c_haship_addr::tag_constr_by_array_uchar(), binary_address);
	_goal("Configuring tuntap options: IP address: " << address << "/" << prefix_len << " MTU=" << mtu);
	as_zerofill< ifreq > ifr; // the if request
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
	strncpy(ifr.ifr_name, "galaxy%d", IFNAMSIZ);
	int errcode_ioctl =  ioctl(m_tun_fd, TUNSETIFF, static_cast<void *>(&ifr));
	_check_sys(errcode_ioctl != -1);
	_check_extern(binary_address[0] == 0xFD);
	_check_extern(binary_address[1] == 0x42);
	NetPlatform_addAddress(ifr.ifr_name, binary_address.data(), prefix_len, Sockaddr_AF_INET6);
	NetPlatform_setMTU(ifr.ifr_name, mtu);
	m_tun_stream.release();
	m_tun_stream.assign(m_tun_fd);
	_goal("Configuring tuntap options - done");
}

#endif // ANTINET_linux