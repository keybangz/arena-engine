#ifndef ARENA_ITEM_H
#define ARENA_ITEM_H

#include "../ecs/ecs.h"
#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Item Stats
// ============================================================================

typedef struct ItemStats {
    float health;
    float mana;
    float health_regen;
    float mana_regen;
    float attack_damage;
    float ability_power;
    float armor;
    float magic_resist;
    float attack_speed;      // Percentage bonus
    float move_speed;        // Flat bonus
    float crit_chance;       // Percentage
    float life_steal;        // Percentage
    float spell_vamp;        // Percentage
    float cooldown_reduction;// Percentage
} ItemStats;

// ============================================================================
// Item Definition
// ============================================================================

typedef enum ItemType {
    ITEM_TYPE_BASIC,         // Can't be upgraded
    ITEM_TYPE_COMPONENT,     // Used to build other items
    ITEM_TYPE_LEGENDARY,     // Full item
    ITEM_TYPE_BOOTS,         // Movement boots
    ITEM_TYPE_CONSUMABLE     // Single use
} ItemType;

typedef struct ItemDef {
    uint32_t id;
    const char* name;
    ItemType type;
    uint32_t cost;
    ItemStats stats;
    
    // Build path (up to 3 components)
    uint32_t components[3];
    uint8_t component_count;
    
    // Passive/Active effect ID (0 = none)
    uint32_t passive_id;
    uint32_t active_id;
} ItemDef;

// ============================================================================
// Inventory
// ============================================================================

#define INVENTORY_SIZE 6

typedef struct Inventory {
    uint32_t items[INVENTORY_SIZE];     // Item IDs (0 = empty)
    uint32_t gold;
} Inventory;

// ============================================================================
// Item API
// ============================================================================

// Registry
void item_registry_init(void);
const ItemDef* item_get_def(uint32_t id);
uint32_t item_register(ItemDef def);

// Inventory management
void inventory_init(Inventory* inv);
bool inventory_add_item(Inventory* inv, uint32_t item_id);
bool inventory_remove_item(Inventory* inv, uint8_t slot);
bool inventory_has_item(const Inventory* inv, uint32_t item_id);
int inventory_find_item(const Inventory* inv, uint32_t item_id);

// Shopping
bool item_can_buy(const Inventory* inv, uint32_t item_id);
bool item_buy(Inventory* inv, uint32_t item_id);
void item_sell(Inventory* inv, uint8_t slot);

// Stats calculation
ItemStats inventory_get_total_stats(const Inventory* inv);

// Built-in item IDs
#define ITEM_LONG_SWORD      1
#define ITEM_DAGGER          2
#define ITEM_CLOTH_ARMOR     3
#define ITEM_RUBY_CRYSTAL    4
#define ITEM_BOOTS           5
#define ITEM_HEALTH_POTION   6
#define ITEM_MANA_POTION     7
#define ITEM_BF_SWORD        10
#define ITEM_INFINITY_EDGE   20

#endif

