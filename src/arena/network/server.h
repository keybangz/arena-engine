#ifndef ARENA_NET_SERVER_H
#define ARENA_NET_SERVER_H

#include "network.h"
#include "protocol.h"
#include "../ecs/ecs.h"

// ============================================================================
// Client Connection
// ============================================================================

typedef enum ClientState {
    CLIENT_STATE_DISCONNECTED,
    CLIENT_STATE_CONNECTING,
    CLIENT_STATE_CONNECTED,
    CLIENT_STATE_DISCONNECTING
} ClientState;

typedef struct ClientConnection {
    NetAddress address;
    ClientState state;
    uint32_t last_packet_time;      // Time of last received packet
    uint16_t last_input_sequence;   // Last processed input sequence
    InputFlags last_input;          // Most recent input
    Entity player_entity;           // Player's entity in the world
    char name[32];
} ClientConnection;

// ============================================================================
// Game Server
// ============================================================================

typedef struct GameServer {
    NetSocket socket;
    uint16_t port;
    
    // Clients
    ClientConnection clients[NET_MAX_CLIENTS];
    uint8_t client_count;
    
    // Game state
    World* world;
    uint32_t tick;
    
    // Timing
    double tick_accumulator;
    double last_time;
    
    // Packet buffer
    uint8_t packet_buffer[NET_MAX_PACKET_SIZE];
    
    // Running state
    bool running;
} GameServer;

// ============================================================================
// Server API
// ============================================================================

// Lifecycle
GameServer* server_create(uint16_t port, World* world);
void server_destroy(GameServer* server);

// Main loop
void server_update(GameServer* server, double dt);
void server_tick(GameServer* server);

// Network
void server_receive_packets(GameServer* server);
void server_broadcast_state(GameServer* server);

// Client management
int server_find_client(GameServer* server, NetAddress addr);
int server_add_client(GameServer* server, NetAddress addr, const char* name);
void server_remove_client(GameServer* server, int client_id);

#endif

