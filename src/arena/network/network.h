#ifndef ARENA_NETWORK_H
#define ARENA_NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// Platform Detection
// ============================================================================

#ifdef _WIN32
    #define NET_PLATFORM_WINDOWS
#else
    #define NET_PLATFORM_UNIX
#endif

// ============================================================================
// Constants
// ============================================================================

#define NET_MAX_PACKET_SIZE     1400    // Safe MTU for UDP
#define NET_DEFAULT_PORT        7777
#define NET_MAX_CLIENTS         10
#define NET_TIMEOUT_MS          5000    // Connection timeout
#define NET_TICK_RATE           30      // Server tick rate

// ============================================================================
// Address
// ============================================================================

typedef struct NetAddress {
    uint32_t host;      // IPv4 address in network byte order
    uint16_t port;      // Port in host byte order
} NetAddress;

// Address utilities
NetAddress net_address_create(const char* host, uint16_t port);
NetAddress net_address_any(uint16_t port);
NetAddress net_address_loopback(uint16_t port);
bool net_address_equals(NetAddress a, NetAddress b);
void net_address_to_string(NetAddress addr, char* out, size_t out_size);

// ============================================================================
// Socket
// ============================================================================

typedef struct NetSocket {
    int handle;
    bool is_valid;
    bool is_blocking;
} NetSocket;

// Socket lifecycle
bool net_init(void);                // Initialize networking (call once)
void net_shutdown(void);            // Cleanup networking

NetSocket net_socket_create(void);
void net_socket_destroy(NetSocket* socket);
bool net_socket_bind(NetSocket* socket, uint16_t port);
bool net_socket_set_nonblocking(NetSocket* socket, bool nonblocking);

// Socket I/O
int net_socket_send(NetSocket* socket, NetAddress dest,
                    const void* data, size_t size);
int net_socket_receive(NetSocket* socket, NetAddress* from,
                       void* buffer, size_t buffer_size);

// ============================================================================
// Packet Types
// ============================================================================

typedef enum PacketType {
    PACKET_NONE = 0,

    // Connection
    PACKET_CONNECT_REQUEST,     // Client -> Server: Request to join
    PACKET_CONNECT_ACCEPT,      // Server -> Client: Connection accepted
    PACKET_CONNECT_REJECT,      // Server -> Client: Connection rejected
    PACKET_DISCONNECT,          // Both: Graceful disconnect
    PACKET_HEARTBEAT,           // Both: Keep-alive

    // Game data
    PACKET_INPUT,               // Client -> Server: Player inputs
    PACKET_STATE_SNAPSHOT,      // Server -> Client: Full world state
    PACKET_STATE_DELTA,         // Server -> Client: Incremental update
    PACKET_EVENT,               // Server -> Client: Game events

    PACKET_TYPE_COUNT
} PacketType;

// ============================================================================
// Packet Header
// ============================================================================

#pragma pack(push, 1)

typedef struct PacketHeader {
    uint8_t type;           // PacketType
    uint8_t flags;          // Reserved
    uint16_t sequence;      // Packet sequence number
    uint32_t tick;          // Server tick number
} PacketHeader;

#define PACKET_HEADER_SIZE sizeof(PacketHeader)

#pragma pack(pop)

// ============================================================================
// Packet Utilities
// ============================================================================

void packet_write_header(void* buffer, PacketType type, uint16_t seq, uint32_t tick);
PacketHeader packet_read_header(const void* buffer);

#endif
