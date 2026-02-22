#include "network.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <map>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <unistd.h>
#include <chrono>
#include <cerrno>

namespace py = pybind11;

// ---------------------------------------------------------------------------
// ping — Send ICMP echo requests (requires raw socket or setcap)
// ---------------------------------------------------------------------------

static py::dict ping_impl(const std::string& host, int count, double timeout) {
    // Resolve hostname
    struct addrinfo hints = {}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int err = getaddrinfo(host.c_str(), nullptr, &hints, &res);
    if (err != 0)
        throw py::value_error("ping: unknown host " + host + ": " + gai_strerror(err));

    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in* addr = (struct sockaddr_in*)res->ai_addr;
    inet_ntop(AF_INET, &addr->sin_addr, ip_str, sizeof(ip_str));
    freeaddrinfo(res);

    // Try to create raw socket for proper ICMP ping
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    bool can_ping = (sock >= 0);

    py::dict result;
    result["host"] = host;
    result["ip"]   = std::string(ip_str);

    if (!can_ping) {
        // Fallback: just resolve and report
        result["reachable"] = true;  // resolved but can't ICMP
        result["note"] = "ICMP raw sockets require root or CAP_NET_RAW. Host resolved successfully.";
        result["packets_sent"]     = 0;
        result["packets_received"] = 0;
        return result;
    }

    // Set timeout
    struct timeval tv;
    tv.tv_sec = static_cast<int>(timeout);
    tv.tv_usec = static_cast<int>((timeout - tv.tv_sec) * 1000000);
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in dest = {};
    dest.sin_family = AF_INET;
    inet_pton(AF_INET, ip_str, &dest.sin_addr);

    int sent = 0, received = 0;
    double total_rtt = 0, min_rtt = 1e9, max_rtt = 0;

    for (int i = 0; i < count; i++) {
        // ICMP echo request via DGRAM ICMP socket
        struct icmphdr icmp_hdr = {};
        icmp_hdr.type = ICMP_ECHO;
        icmp_hdr.code = 0;
        icmp_hdr.un.echo.id = getpid() & 0xFFFF;
        icmp_hdr.un.echo.sequence = i + 1;

        // Checksum
        uint32_t sum = 0;
        uint16_t* ptr = (uint16_t*)&icmp_hdr;
        for (size_t j = 0; j < sizeof(icmp_hdr) / 2; j++)
            sum += ptr[j];
        sum = (sum >> 16) + (sum & 0xFFFF);
        sum += (sum >> 16);
        icmp_hdr.checksum = ~sum;

        auto t_start = std::chrono::high_resolution_clock::now();

        ssize_t n = sendto(sock, &icmp_hdr, sizeof(icmp_hdr), 0,
                           (struct sockaddr*)&dest, sizeof(dest));
        if (n <= 0) { sent++; continue; }
        sent++;

        char recv_buf[1024];
        struct sockaddr_in from = {};
        socklen_t fromlen = sizeof(from);
        n = recvfrom(sock, recv_buf, sizeof(recv_buf), 0,
                     (struct sockaddr*)&from, &fromlen);

        auto t_end = std::chrono::high_resolution_clock::now();
        double rtt = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        if (n > 0) {
            received++;
            total_rtt += rtt;
            min_rtt = std::min(min_rtt, rtt);
            max_rtt = std::max(max_rtt, rtt);
        }
    }

    close(sock);

    result["packets_sent"]     = sent;
    result["packets_received"] = received;
    result["packet_loss"]      = sent > 0 ? (1.0 - static_cast<double>(received) / sent) * 100.0 : 100.0;
    result["reachable"]        = received > 0;

    if (received > 0) {
        result["rtt_min_ms"] = min_rtt;
        result["rtt_avg_ms"] = total_rtt / received;
        result["rtt_max_ms"] = max_rtt;
    }

    return result;
}

// ---------------------------------------------------------------------------
// nslookup — DNS resolution
// ---------------------------------------------------------------------------

static py::dict nslookup_impl(const std::string& hostname, bool ipv6) {
    struct addrinfo hints = {}, *res = nullptr;
    hints.ai_family = ipv6 ? AF_INET6 : AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    if (err != 0)
        throw py::value_error("nslookup: can't resolve '" + hostname + "': " +
                              gai_strerror(err));

    py::dict result;
    result["hostname"] = hostname;

    py::list addresses;
    for (struct addrinfo* rp = res; rp != nullptr; rp = rp->ai_next) {
        char ip_str[INET6_ADDRSTRLEN];
        if (rp->ai_family == AF_INET) {
            struct sockaddr_in* addr4 = (struct sockaddr_in*)rp->ai_addr;
            inet_ntop(AF_INET, &addr4->sin_addr, ip_str, sizeof(ip_str));
        } else if (rp->ai_family == AF_INET6) {
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)rp->ai_addr;
            inet_ntop(AF_INET6, &addr6->sin6_addr, ip_str, sizeof(ip_str));
        } else {
            continue;
        }

        py::dict addr;
        addr["address"] = std::string(ip_str);
        addr["family"]  = rp->ai_family == AF_INET ? "IPv4" : "IPv6";
        addresses.append(addr);
    }

    result["addresses"] = addresses;

    // Reverse lookup for each address
    if (res) {
        char host_buf[NI_MAXHOST];
        if (getnameinfo(res->ai_addr, res->ai_addrlen,
                        host_buf, sizeof(host_buf),
                        nullptr, 0, NI_NAMEREQD) == 0) {
            result["canonical_name"] = std::string(host_buf);
        }
    }

    freeaddrinfo(res);
    return result;
}

// ---------------------------------------------------------------------------
// ifconfig — Network interface information
// ---------------------------------------------------------------------------

static py::list ifconfig_impl(const std::string& interface_name) {
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1)
        throw py::value_error("ifconfig: cannot get interfaces: " +
                              std::string(std::strerror(errno)));

    // Collect all interface info
    std::map<std::string, py::dict> interfaces;

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        std::string name = ifa->ifa_name;
        if (!interface_name.empty() && name != interface_name) continue;

        if (interfaces.find(name) == interfaces.end()) {
            py::dict iface;
            iface["name"] = name;
            iface["flags"] = static_cast<int>(ifa->ifa_flags);
            iface["is_up"] = (ifa->ifa_flags & IFF_UP) != 0;
            iface["is_loopback"] = (ifa->ifa_flags & IFF_LOOPBACK) != 0;
            iface["is_running"] = (ifa->ifa_flags & IFF_RUNNING) != 0;
            interfaces[name] = iface;
        }

        char ip_str[INET6_ADDRSTRLEN];

        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
            inet_ntop(AF_INET, &addr->sin_addr, ip_str, sizeof(ip_str));
            interfaces[name]["ipv4_address"] = std::string(ip_str);

            if (ifa->ifa_netmask) {
                struct sockaddr_in* mask = (struct sockaddr_in*)ifa->ifa_netmask;
                inet_ntop(AF_INET, &mask->sin_addr, ip_str, sizeof(ip_str));
                interfaces[name]["ipv4_netmask"] = std::string(ip_str);
            }
            if (ifa->ifa_broadaddr && !(ifa->ifa_flags & IFF_LOOPBACK)) {
                struct sockaddr_in* bcast = (struct sockaddr_in*)ifa->ifa_broadaddr;
                inet_ntop(AF_INET, &bcast->sin_addr, ip_str, sizeof(ip_str));
                interfaces[name]["ipv4_broadcast"] = std::string(ip_str);
            }
        } else if (ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)ifa->ifa_addr;
            inet_ntop(AF_INET6, &addr6->sin6_addr, ip_str, sizeof(ip_str));
            interfaces[name]["ipv6_address"] = std::string(ip_str);
        } else if (ifa->ifa_addr->sa_family == AF_PACKET) {
            // MAC address — read from /sys
            std::ifstream mac_ifs("/sys/class/net/" + name + "/address");
            std::string mac;
            if (mac_ifs.is_open() && std::getline(mac_ifs, mac))
                interfaces[name]["mac_address"] = mac;

            // MTU
            std::ifstream mtu_ifs("/sys/class/net/" + name + "/mtu");
            int mtu;
            if (mtu_ifs.is_open() && (mtu_ifs >> mtu))
                interfaces[name]["mtu"] = mtu;
        }
    }

    freeifaddrs(ifaddr);

    py::list result;
    for (auto& [name, iface] : interfaces)
        result.append(iface);

    return result;
}

// ===========================================================================
// pybind11 registration
// ===========================================================================

void init_network(py::module_ &m) {

    // -- ping ---------------------------------------------------------------
    m.def("ping", &ping_impl,
        R"doc(
        Send ICMP echo requests to a host.

        Equivalent to the ``ping`` shell command. Resolves the hostname and
        sends ICMP echo requests. Requires raw socket capability
        (root or CAP_NET_RAW). Falls back to DNS resolution only if
        raw sockets are unavailable.

        Args:
            host (str): Hostname or IP address to ping.
            count (int): Number of echo requests to send. Default is 4.
                         Equivalent to ``ping -c``.
            timeout (float): Timeout in seconds for each request. Default is 2.0.
                             Equivalent to ``ping -W``.

        Returns:
            dict: Keys "host", "ip", "reachable", "packets_sent",
                  "packets_received", "packet_loss", "rtt_min_ms",
                  "rtt_avg_ms", "rtt_max_ms".

        Raises:
            ValueError: If host cannot be resolved.
        )doc",
        py::arg("host"),
        py::arg("count") = 4,
        py::arg("timeout") = 2.0);

    // -- nslookup -----------------------------------------------------------
    m.def("nslookup", &nslookup_impl,
        R"doc(
        Query DNS to resolve a hostname.

        Equivalent to the ``nslookup`` shell command. Resolves a hostname
        to its IP addresses using the system's DNS resolver.

        Args:
            hostname (str): The hostname to resolve.
            ipv6 (bool): If True, prefer IPv6 addresses.
                         Equivalent to ``nslookup -type=AAAA``.

        Returns:
            dict: Keys "hostname", "addresses" (list of dicts with "address"
                  and "family"), and optionally "canonical_name".

        Raises:
            ValueError: If the hostname cannot be resolved.
        )doc",
        py::arg("hostname"),
        py::arg("ipv6") = false);

    // -- ifconfig -----------------------------------------------------------
    m.def("ifconfig", &ifconfig_impl,
        R"doc(
        Display network interface information.

        Equivalent to the ``ifconfig`` shell command. Returns information
        about all network interfaces or a specific one, including
        IP addresses, netmask, MAC address, MTU, and status flags.

        Args:
            interface_name (str): Specific interface to query (e.g. "eth0").
                                  Empty string for all interfaces.

        Returns:
            list[dict]: Each dict contains "name", "ipv4_address",
                        "ipv4_netmask", "ipv6_address", "mac_address",
                        "mtu", "is_up", "is_loopback", "is_running".

        Raises:
            ValueError: If interface info cannot be retrieved.
        )doc",
        py::arg("interface_name") = "");
}
