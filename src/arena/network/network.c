#include "network.h"

#include <stdio.h>
#include <string.h>

#ifdef NET_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

// ============================================================================
// Platform Initialization
// ============================================================================

static bool g_net_initialized = false;

bool net_init(void) {
    if (g_net_initialized) return true;

#ifdef NET_PLATFORM_WINDOWS
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "Network: WSAStartup failed\n");
        return false;
    }
#endif

    g_net_initialized = true;
    printf("Network: Initialized\n");
    return true;
}

void net_shutdown(void) {
    if (!g_net_initialized) return;

#ifdef NET_PLATFORM_WINDOWS
    WSACleanup();
#endif

    g_net_initialized = false;
    printf("Network: Shutdown\n");
}

// ============================================================================
// Address Functions
// ============================================================================

NetAddress net_address_create(const char* host, uint16_t port) {
    NetAddress addr = {0};
    addr.port = port;

    if (host == NULL || strcmp(host, "0.0.0.0") == 0) {
        addr.host = INADDR_ANY;
    } else if (strcmp(host, "127.0.0.1") == 0 || strcmp(host, "localhost") == 0) {
        addr.host = htonl(INADDR_LOOPBACK);
    } else {
        struct in_addr in;
        if (inet_pton(AF_INET, host, &in) == 1) {
            addr.host = in.s_addr;
        }
    }

    return addr;
}

NetAddress net_address_any(uint16_t port) {
    return (NetAddress){INADDR_ANY, port};
}

NetAddress net_address_loopback(uint16_t port) {
    return (NetAddress){htonl(INADDR_LOOPBACK), port};
}

bool net_address_equals(NetAddress a, NetAddress b) {
    return a.host == b.host && a.port == b.port;
}

void net_address_to_string(NetAddress addr, char* out, size_t out_size) {
    struct in_addr in;
    in.s_addr = addr.host;
    snprintf(out, out_size, "%s:%u", inet_ntoa(in), addr.port);
}

// ============================================================================
// Socket Functions
// ============================================================================

NetSocket net_socket_create(void) {
    NetSocket sock = {0};

    sock.handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock.handle == INVALID_SOCKET) {
        fprintf(stderr, "Network: Failed to create socket\n");
        return sock;
    }

    sock.is_valid = true;
    sock.is_blocking = true;
    return sock;
}

void net_socket_destroy(NetSocket* socket) {
    if (!socket || !socket->is_valid) return;

    closesocket(socket->handle);
    socket->is_valid = false;
    socket->handle = INVALID_SOCKET;
}

bool net_socket_bind(NetSocket* socket, uint16_t port) {
    if (!socket || !socket->is_valid) return false;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(socket->handle, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Network: Failed to bind socket to port %u\n", port);
        return false;
    }

    printf("Network: Socket bound to port %u\n", port);
    return true;
}

bool net_socket_set_nonblocking(NetSocket* socket, bool nonblocking) {
    if (!socket || !socket->is_valid) return false;

#ifdef NET_PLATFORM_WINDOWS
    u_long mode = nonblocking ? 1 : 0;
    if (ioctlsocket(socket->handle, FIONBIO, &mode) != 0) {
        return false;
    }
#else
    int flags = fcntl(socket->handle, F_GETFL, 0);
    if (flags == -1) return false;

    flags = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    if (fcntl(socket->handle, F_SETFL, flags) == -1) {
        return false;
    }
#endif

    socket->is_blocking = !nonblocking;
    return true;
}

// ============================================================================
// Socket I/O
// ============================================================================

int net_socket_send(NetSocket* socket, NetAddress dest,
                    const void* data, size_t size) {
    if (!socket || !socket->is_valid || !data || size == 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = dest.host;
    addr.sin_port = htons(dest.port);

    int sent = sendto(socket->handle, data, (int)size, 0,
                      (struct sockaddr*)&addr, sizeof(addr));

    return sent;
}

int net_socket_receive(NetSocket* socket, NetAddress* from,
                       void* buffer, size_t buffer_size) {
    if (!socket || !socket->is_valid || !buffer || buffer_size == 0) return -1;

    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);

    int received = recvfrom(socket->handle, buffer, (int)buffer_size, 0,
                            (struct sockaddr*)&addr, &addr_len);

    if (received > 0 && from) {
        from->host = addr.sin_addr.s_addr;
        from->port = ntohs(addr.sin_port);
    }

    return received;
}

// ============================================================================
// Packet Utilities
// ============================================================================

void packet_write_header(void* buffer, PacketType type, uint16_t seq, uint32_t tick) {
    PacketHeader* header = (PacketHeader*)buffer;
    header->type = (uint8_t)type;
    header->flags = 0;
    header->sequence = seq;
    header->tick = tick;
}

PacketHeader packet_read_header(const void* buffer) {
    return *(const PacketHeader*)buffer;
}
