#include "server.h"
#include "../game/ability.h"
#include "../game/combat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Server Creation
// ============================================================================

GameServer* server_create(uint16_t port, World* world) {
    GameServer* server = calloc(1, sizeof(GameServer));
    if (!server) return NULL;

    server->world = world;
    server->port = port;

    // Create and bind socket
    server->socket = net_socket_create();
    if (!server->socket.is_valid) {
        free(server);
        return NULL;
    }

    if (!net_socket_bind(&server->socket, port)) {
        net_socket_destroy(&server->socket);
        free(server);
        return NULL;
    }

    net_socket_set_nonblocking(&server->socket, true);

    server->running = true;
    printf("Server: Created on port %u\n", port);

    return server;
}

void server_destroy(GameServer* server) {
    if (!server) return;

    // Notify all clients
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        if (server->clients[i].state == CLIENT_STATE_CONNECTED) {
            server_remove_client(server, i);
        }
    }

    net_socket_destroy(&server->socket);
    free(server);
    printf("Server: Destroyed\n");
}

// ============================================================================
// Client Management
// ============================================================================

int server_find_client(GameServer* server, NetAddress addr) {
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        if (server->clients[i].state != CLIENT_STATE_DISCONNECTED &&
            net_address_equals(server->clients[i].address, addr)) {
            return i;
        }
    }
    return -1;
}

int server_add_client(GameServer* server, NetAddress addr, const char* name) {
    if (server->client_count >= NET_MAX_CLIENTS) {
        return -1;
    }

    // Find free slot
    int slot = -1;
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        if (server->clients[i].state == CLIENT_STATE_DISCONNECTED) {
            slot = i;
            break;
        }
    }

    if (slot < 0) return -1;

    ClientConnection* client = &server->clients[slot];
    memset(client, 0, sizeof(ClientConnection));
    client->address = addr;
    client->state = CLIENT_STATE_CONNECTED;
    strncpy(client->name, name, sizeof(client->name) - 1);

    // Create player entity
    client->player_entity = world_spawn(server->world);

    Transform* t = world_add_transform(server->world, client->player_entity);
    t->x = 640.0f;
    t->y = 360.0f + slot * 50.0f;
    t->scale_x = 1.0f;
    t->scale_y = 1.0f;

    world_add_velocity(server->world, client->player_entity);

    Sprite* s = world_add_sprite(server->world, client->player_entity);
    s->width = 32.0f;
    s->height = 32.0f;
    s->color = 0xFF00FFFF;  // Cyan

    Health* h = world_add_health(server->world, client->player_entity);
    h->current = 100.0f;
    h->max = 100.0f;

    Player* p = world_add_player(server->world, client->player_entity);
    p->player_id = (uint8_t)slot;
    p->team = 0;

    Collider* c = world_add_collider(server->world, client->player_entity);
    c->radius = 16.0f;

    server->client_count++;

    char addr_str[32];
    net_address_to_string(addr, addr_str, sizeof(addr_str));
    printf("Server: Client %d (%s) connected from %s\n", slot, name, addr_str);

    return slot;
}

void server_remove_client(GameServer* server, int client_id) {
    if (client_id < 0 || client_id >= NET_MAX_CLIENTS) return;

    ClientConnection* client = &server->clients[client_id];
    if (client->state == CLIENT_STATE_DISCONNECTED) return;

    // Remove player entity
    if (world_is_alive(server->world, client->player_entity)) {
        world_despawn(server->world, client->player_entity);
    }

    printf("Server: Client %d (%s) disconnected\n", client_id, client->name);

    client->state = CLIENT_STATE_DISCONNECTED;
    server->client_count--;
}

// ============================================================================
// Packet Handling
// ============================================================================

static void handle_connect_request(GameServer* server, NetAddress from,
                                   PacketConnectRequest* req) {
    // Validate magic and version
    if (req->magic != PROTOCOL_MAGIC || req->version != PROTOCOL_VERSION) {
        PacketConnectReject reject = {0};
        packet_write_header(&reject.header, PACKET_CONNECT_REJECT, 0, server->tick);
        reject.reason = 1;  // Version mismatch
        net_socket_send(&server->socket, from, &reject, sizeof(reject));
        return;
    }

    // Check if already connected
    if (server_find_client(server, from) >= 0) {
        return;
    }

    // Try to add client
    int client_id = server_add_client(server, from, req->player_name);

    if (client_id < 0) {
        PacketConnectReject reject = {0};
        packet_write_header(&reject.header, PACKET_CONNECT_REJECT, 0, server->tick);
        reject.reason = 0;  // Server full
        net_socket_send(&server->socket, from, &reject, sizeof(reject));
        return;
    }

    // Send accept
    PacketConnectAccept accept = {0};
    packet_write_header(&accept.header, PACKET_CONNECT_ACCEPT, 0, server->tick);
    accept.client_id = (uint8_t)client_id;
    accept.server_tick = server->tick;
    accept.tick_rate = NET_TICK_RATE;
    net_socket_send(&server->socket, from, &accept, sizeof(accept));
}

static void handle_input(GameServer* server, int client_id, PacketInput* input) {
    ClientConnection* client = &server->clients[client_id];

    // Check sequence (ignore old inputs)
    if (input->header.sequence <= client->last_input_sequence) {
        return;
    }

    client->last_input_sequence = input->header.sequence;
    client->last_input = input->flags;

    // Apply input to player entity
    if (world_is_alive(server->world, client->player_entity)) {
        Velocity* vel = world_get_velocity(server->world, client->player_entity);
        if (vel) {
            float speed = 200.0f;
            vel->x = 0;
            vel->y = 0;
            if (input->flags.move_up) vel->y -= speed;
            if (input->flags.move_down) vel->y += speed;
            if (input->flags.move_left) vel->x -= speed;
            if (input->flags.move_right) vel->x += speed;
        }

        // Handle fire
        if (input->flags.fire) {
            float mx = (float)input->mouse_x;
            float my = (float)input->mouse_y;
            ability_try_cast(server->world, client->player_entity, 0, mx, my, ENTITY_NULL);
        }
    }
}

void server_receive_packets(GameServer* server) {
    NetAddress from;
    int received;

    while ((received = net_socket_receive(&server->socket, &from,
            server->packet_buffer, NET_MAX_PACKET_SIZE)) > 0) {

        if (received < (int)PACKET_HEADER_SIZE) continue;

        PacketHeader header = packet_read_header(server->packet_buffer);
        int client_id = server_find_client(server, from);

        switch (header.type) {
            case PACKET_CONNECT_REQUEST:
                if (received >= (int)sizeof(PacketConnectRequest)) {
                    handle_connect_request(server, from,
                        (PacketConnectRequest*)server->packet_buffer);
                }
                break;

            case PACKET_INPUT:
                if (client_id >= 0 && received >= (int)sizeof(PacketInput)) {
                    handle_input(server, client_id,
                        (PacketInput*)server->packet_buffer);
                }
                break;

            case PACKET_DISCONNECT:
                if (client_id >= 0) {
                    server_remove_client(server, client_id);
                }
                break;

            case PACKET_HEARTBEAT:
                if (client_id >= 0) {
                    // Update last packet time
                    // server->clients[client_id].last_packet_time = current_time;
                }
                break;

            default:
                break;
        }
    }
}

// ============================================================================
// State Broadcast
// ============================================================================

void server_broadcast_state(GameServer* server) {
    // Build snapshot
    PacketStateSnapshot* snapshot = (PacketStateSnapshot*)server->packet_buffer;
    packet_write_header(&snapshot->header, PACKET_STATE_SNAPSHOT, 0, server->tick);

    EntityState* states = (EntityState*)(server->packet_buffer + sizeof(PacketStateSnapshot));
    uint16_t entity_count = 0;

    // Query all renderable entities
    Query query = world_query(server->world,
        component_mask(COMPONENT_TRANSFORM) | component_mask(COMPONENT_SPRITE));

    Entity entity;
    while (query_next(&query, &entity)) {
        if (entity_count >= 100) break;  // Limit entities per packet

        Transform* t = world_get_transform(server->world, entity);
        Sprite* s = world_get_sprite(server->world, entity);

        EntityState* state = &states[entity_count++];
        state->entity_id = entity_index(entity);
        state->x = pack_position(t->x);
        state->y = pack_position(t->y);
        state->sprite_id = 0;
        state->color = s->color;
        state->components = 0;

        // Add velocity if present
        Velocity* v = world_get_velocity(server->world, entity);
        if (v) {
            state->components |= ENTITY_STATE_HAS_VELOCITY;
            state->vel_x = pack_position(v->x);
            state->vel_y = pack_position(v->y);
        }

        // Add health if present
        Health* h = world_get_health(server->world, entity);
        if (h) {
            state->components |= ENTITY_STATE_HAS_HEALTH;
            state->health = (int16_t)h->current;
            state->max_health = (int16_t)h->max;
        }
    }

    snapshot->entity_count = entity_count;
    size_t packet_size = sizeof(PacketStateSnapshot) + entity_count * sizeof(EntityState);

    // Send to all connected clients
    for (int i = 0; i < NET_MAX_CLIENTS; i++) {
        if (server->clients[i].state == CLIENT_STATE_CONNECTED) {
            net_socket_send(&server->socket, server->clients[i].address,
                           server->packet_buffer, packet_size);
        }
    }
}

// ============================================================================
// Server Update
// ============================================================================

void server_update(GameServer* server, double dt) {
    server->tick_accumulator += dt;

    double tick_interval = 1.0 / NET_TICK_RATE;

    while (server->tick_accumulator >= tick_interval) {
        server_tick(server);
        server->tick_accumulator -= tick_interval;
    }
}

void server_tick(GameServer* server) {
    server->tick++;

    // Receive all pending packets
    server_receive_packets(server);

    // Run game simulation (movement, combat, etc.)
    // This is done by the main server loop

    // Broadcast state to all clients
    server_broadcast_state(server);
}

