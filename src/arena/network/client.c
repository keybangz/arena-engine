#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Client Creation
// ============================================================================

NetClient* net_client_create(World* world) {
    NetClient* client = calloc(1, sizeof(NetClient));
    if (!client) return NULL;

    client->world = world;
    client->state = NET_CLIENT_DISCONNECTED;

    client->socket = net_socket_create();
    if (!client->socket.is_valid) {
        free(client);
        return NULL;
    }

    net_socket_set_nonblocking(&client->socket, true);

    printf("NetClient: Created\n");
    return client;
}

void net_client_destroy(NetClient* client) {
    if (!client) return;

    if (client->state != NET_CLIENT_DISCONNECTED) {
        net_client_disconnect(client);
    }

    net_socket_destroy(&client->socket);
    free(client);
    printf("NetClient: Destroyed\n");
}

// ============================================================================
// Connection
// ============================================================================

bool net_client_connect(NetClient* client, const char* host, uint16_t port, const char* name) {
    if (!client || client->state != NET_CLIENT_DISCONNECTED) return false;

    client->server_address = net_address_create(host, port);
    client->state = NET_CLIENT_CONNECTING;
    client->input_sequence = 0;

    // Send connect request
    PacketConnectRequest req = {0};
    packet_write_header(&req.header, PACKET_CONNECT_REQUEST, 0, 0);
    req.magic = PROTOCOL_MAGIC;
    req.version = PROTOCOL_VERSION;
    strncpy(req.player_name, name, sizeof(req.player_name) - 1);

    net_socket_send(&client->socket, client->server_address, &req, sizeof(req));

    char addr_str[32];
    net_address_to_string(client->server_address, addr_str, sizeof(addr_str));
    printf("NetClient: Connecting to %s...\n", addr_str);

    return true;
}

void net_client_disconnect(NetClient* client) {
    if (!client || client->state == NET_CLIENT_DISCONNECTED) return;

    // Send disconnect packet
    PacketHeader header;
    packet_write_header(&header, PACKET_DISCONNECT, 0, 0);
    net_socket_send(&client->socket, client->server_address, &header, sizeof(header));

    client->state = NET_CLIENT_DISCONNECTED;
    printf("NetClient: Disconnected\n");
}

bool net_client_is_connected(NetClient* client) {
    return client && client->state == NET_CLIENT_CONNECTED;
}

// ============================================================================
// Packet Handling
// ============================================================================

static void handle_connect_accept(NetClient* client, PacketConnectAccept* accept) {
    client->state = NET_CLIENT_CONNECTED;
    client->client_id = accept->client_id;
    client->server_tick = accept->server_tick;
    client->tick_rate = accept->tick_rate;

    printf("NetClient: Connected as client %d (server tick: %u)\n",
           client->client_id, client->server_tick);
}

static void handle_connect_reject(NetClient* client, PacketConnectReject* reject) {
    const char* reasons[] = {"Server full", "Version mismatch", "Banned"};
    const char* reason = (reject->reason < 3) ? reasons[reject->reason] : "Unknown";

    printf("NetClient: Connection rejected: %s\n", reason);
    client->state = NET_CLIENT_DISCONNECTED;
}

static void handle_state_snapshot(NetClient* client, PacketStateSnapshot* snapshot, int packet_size) {
    (void)packet_size;

    client->server_tick = snapshot->header.tick;

    EntityState* states = (EntityState*)((uint8_t*)snapshot + sizeof(PacketStateSnapshot));

    // Update entities from server state
    for (uint16_t i = 0; i < snapshot->entity_count; i++) {
        EntityState* state = &states[i];

        // Find or create entity
        Entity entity = entity_make(state->entity_id, 0);

        if (!world_is_alive(client->world, entity)) {
            // Entity doesn't exist, spawn it
            // Note: In a real implementation, we'd track server entities properly
            continue;
        }

        // Update transform
        Transform* t = world_get_transform(client->world, entity);
        if (t) {
            t->x = unpack_position(state->x);
            t->y = unpack_position(state->y);
        }
    }
}

static void receive_packets(NetClient* client) {
    NetAddress from;
    int received;

    while ((received = net_socket_receive(&client->socket, &from,
            client->packet_buffer, NET_MAX_PACKET_SIZE)) > 0) {

        if (received < (int)PACKET_HEADER_SIZE) continue;
        if (!net_address_equals(from, client->server_address)) continue;

        PacketHeader header = packet_read_header(client->packet_buffer);

        switch (header.type) {
            case PACKET_CONNECT_ACCEPT:
                if (received >= (int)sizeof(PacketConnectAccept)) {
                    handle_connect_accept(client, (PacketConnectAccept*)client->packet_buffer);
                }
                break;

            case PACKET_CONNECT_REJECT:
                if (received >= (int)sizeof(PacketConnectReject)) {
                    handle_connect_reject(client, (PacketConnectReject*)client->packet_buffer);
                }
                break;

            case PACKET_STATE_SNAPSHOT:
                handle_state_snapshot(client, (PacketStateSnapshot*)client->packet_buffer, received);
                break;

            default:
                break;
        }
    }
}

// ============================================================================
// Client Update
// ============================================================================

void net_client_update(NetClient* client, double dt) {
    (void)dt;

    if (!client) return;

    receive_packets(client);
}

void net_client_send_input(NetClient* client, InputFlags flags, int16_t mouse_x, int16_t mouse_y) {
    if (!client || client->state != NET_CLIENT_CONNECTED) return;

    PacketInput input = {0};
    packet_write_header(&input.header, PACKET_INPUT, ++client->input_sequence, 0);
    input.client_tick = client->server_tick;
    input.flags = flags;
    input.mouse_x = mouse_x;
    input.mouse_y = mouse_y;

    net_socket_send(&client->socket, client->server_address, &input, sizeof(input));
}

Entity net_client_get_local_player(NetClient* client) {
    if (!client) return ENTITY_NULL;
    return client->local_player;
}

InputFlags input_flags_from_keys(bool up, bool down, bool left, bool right,
                                  bool fire, bool ab1, bool ab2, bool ab3) {
    InputFlags flags = {0};
    flags.move_up = up;
    flags.move_down = down;
    flags.move_left = left;
    flags.move_right = right;
    flags.fire = fire;
    flags.ability1 = ab1;
    flags.ability2 = ab2;
    flags.ability3 = ab3;
    return flags;
}
