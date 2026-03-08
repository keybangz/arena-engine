#ifndef ARENA_NET_CLIENT_H
#define ARENA_NET_CLIENT_H

#include "network.h"
#include "protocol.h"
#include "../ecs/ecs.h"

// ============================================================================
// Client Connection State
// ============================================================================

typedef enum NetClientState {
    NET_CLIENT_DISCONNECTED,
    NET_CLIENT_CONNECTING,
    NET_CLIENT_CONNECTED,
    NET_CLIENT_DISCONNECTING
} NetClientState;

// ============================================================================
// Network Client
// ============================================================================

typedef struct NetClient {
    NetSocket socket;
    NetAddress server_address;
    NetClientState state;
    
    // Connection info
    uint8_t client_id;
    uint32_t server_tick;
    uint16_t tick_rate;
    
    // Packet tracking
    uint16_t input_sequence;
    uint16_t last_received_sequence;
    
    // World state (client-side)
    World* world;
    Entity local_player;
    
    // Timing
    double connect_time;
    double last_packet_time;
    
    // Packet buffer
    uint8_t packet_buffer[NET_MAX_PACKET_SIZE];
} NetClient;

// ============================================================================
// Client API
// ============================================================================

// Lifecycle
NetClient* net_client_create(World* world);
void net_client_destroy(NetClient* client);

// Connection
bool net_client_connect(NetClient* client, const char* host, uint16_t port, const char* name);
void net_client_disconnect(NetClient* client);
bool net_client_is_connected(NetClient* client);

// Update (call each frame)
void net_client_update(NetClient* client, double dt);

// Send input to server
void net_client_send_input(NetClient* client, InputFlags flags, int16_t mouse_x, int16_t mouse_y);

// Get local player entity
Entity net_client_get_local_player(NetClient* client);

#endif

