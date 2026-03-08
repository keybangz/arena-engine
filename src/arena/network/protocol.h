#ifndef ARENA_PROTOCOL_H
#define ARENA_PROTOCOL_H

#include "network.h"
#include "../ecs/ecs.h"
#include <stdint.h>

// ============================================================================
// Protocol Version
// ============================================================================

#define PROTOCOL_VERSION    1
#define PROTOCOL_MAGIC      0x41524E41  // "ARNA"

// ============================================================================
// Input Packet (Client -> Server)
// ============================================================================

#pragma pack(push, 1)

typedef struct InputFlags {
    uint8_t move_up     : 1;
    uint8_t move_down   : 1;
    uint8_t move_left   : 1;
    uint8_t move_right  : 1;
    uint8_t fire        : 1;
    uint8_t ability1    : 1;
    uint8_t ability2    : 1;
    uint8_t ability3    : 1;
} InputFlags;

typedef struct PacketInput {
    PacketHeader header;
    uint32_t client_tick;       // Client's local tick when input was generated
    InputFlags flags;
    int16_t mouse_x;            // Mouse position
    int16_t mouse_y;
} PacketInput;

// ============================================================================
// Connect Packets
// ============================================================================

typedef struct PacketConnectRequest {
    PacketHeader header;
    uint32_t magic;
    uint8_t version;
    char player_name[32];
} PacketConnectRequest;

typedef struct PacketConnectAccept {
    PacketHeader header;
    uint8_t client_id;          // Assigned client ID (0-9)
    uint32_t server_tick;       // Current server tick
    uint16_t tick_rate;         // Server tick rate
} PacketConnectAccept;

typedef struct PacketConnectReject {
    PacketHeader header;
    uint8_t reason;             // 0=full, 1=version, 2=banned
} PacketConnectReject;

// ============================================================================
// Entity State (for snapshots)
// ============================================================================

typedef struct EntityState {
    uint32_t entity_id;         // Entity index
    uint8_t components;         // Bitmask of components present
    
    // Transform (always present for visible entities)
    int16_t x, y;               // Position (fixed point, /100 for float)
    int16_t rotation;           // Rotation (/100)
    
    // Velocity (if moving)
    int16_t vel_x, vel_y;
    
    // Health (if applicable)
    int16_t health;
    int16_t max_health;
    
    // Visual
    uint8_t sprite_id;
    uint32_t color;
} EntityState;

#define ENTITY_STATE_HAS_VELOCITY   0x01
#define ENTITY_STATE_HAS_HEALTH     0x02
#define ENTITY_STATE_HAS_PLAYER     0x04
#define ENTITY_STATE_HAS_AI         0x08

// ============================================================================
// State Snapshot (Server -> Client)
// ============================================================================

typedef struct PacketStateSnapshot {
    PacketHeader header;
    uint16_t entity_count;
    // Followed by entity_count * EntityState
} PacketStateSnapshot;

// ============================================================================
// Game Event (Server -> Client)
// ============================================================================

typedef enum GameEventType {
    EVENT_NONE = 0,
    EVENT_ENTITY_SPAWN,
    EVENT_ENTITY_DEATH,
    EVENT_DAMAGE,
    EVENT_ABILITY_CAST,
    EVENT_SOUND
} GameEventType;

typedef struct PacketEvent {
    PacketHeader header;
    uint8_t event_type;
    uint32_t entity_id;
    uint32_t data1;
    uint32_t data2;
} PacketEvent;

#pragma pack(pop)

// ============================================================================
// Serialization Helpers
// ============================================================================

// Convert float position to int16 (multiply by 100)
static inline int16_t pack_position(float v) {
    return (int16_t)(v * 10.0f);
}

static inline float unpack_position(int16_t v) {
    return (float)v / 10.0f;
}

// Build InputFlags from input state
InputFlags input_flags_from_keys(bool up, bool down, bool left, bool right,
                                  bool fire, bool ab1, bool ab2, bool ab3);

#endif

