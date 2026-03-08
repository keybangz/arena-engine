#include "item.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// Item Registry
// ============================================================================

#define MAX_ITEM_DEFS 64

static ItemDef g_item_defs[MAX_ITEM_DEFS];
static uint32_t g_item_count = 0;

void item_registry_init(void) {
    g_item_count = 0;
    memset(g_item_defs, 0, sizeof(g_item_defs));

    // Basic components
    item_register((ItemDef){
        .id = ITEM_LONG_SWORD, .name = "Long Sword",
        .type = ITEM_TYPE_COMPONENT, .cost = 350,
        .stats = {.attack_damage = 10}
    });

    item_register((ItemDef){
        .id = ITEM_DAGGER, .name = "Dagger",
        .type = ITEM_TYPE_COMPONENT, .cost = 300,
        .stats = {.attack_speed = 0.12f}
    });

    item_register((ItemDef){
        .id = ITEM_CLOTH_ARMOR, .name = "Cloth Armor",
        .type = ITEM_TYPE_COMPONENT, .cost = 300,
        .stats = {.armor = 15}
    });

    item_register((ItemDef){
        .id = ITEM_RUBY_CRYSTAL, .name = "Ruby Crystal",
        .type = ITEM_TYPE_COMPONENT, .cost = 400,
        .stats = {.health = 150}
    });

    item_register((ItemDef){
        .id = ITEM_BOOTS, .name = "Boots",
        .type = ITEM_TYPE_BOOTS, .cost = 300,
        .stats = {.move_speed = 25}
    });

    // Consumables
    item_register((ItemDef){
        .id = ITEM_HEALTH_POTION, .name = "Health Potion",
        .type = ITEM_TYPE_CONSUMABLE, .cost = 50,
        .stats = {.health = 150}  // Restored over time
    });

    item_register((ItemDef){
        .id = ITEM_MANA_POTION, .name = "Mana Potion",
        .type = ITEM_TYPE_CONSUMABLE, .cost = 50,
        .stats = {.mana = 100}
    });

    // Advanced items
    item_register((ItemDef){
        .id = ITEM_BF_SWORD, .name = "B.F. Sword",
        .type = ITEM_TYPE_COMPONENT, .cost = 1300,
        .stats = {.attack_damage = 40}
    });

    // Legendary items
    item_register((ItemDef){
        .id = ITEM_INFINITY_EDGE, .name = "Infinity Edge",
        .type = ITEM_TYPE_LEGENDARY, .cost = 3400,
        .stats = {.attack_damage = 70, .crit_chance = 0.25f},
        .components = {ITEM_BF_SWORD, ITEM_LONG_SWORD, ITEM_LONG_SWORD},
        .component_count = 3
    });

    printf("Item registry initialized with %u items\n", g_item_count);
}

const ItemDef* item_get_def(uint32_t id) {
    for (uint32_t i = 0; i < g_item_count; i++) {
        if (g_item_defs[i].id == id) {
            return &g_item_defs[i];
        }
    }
    return NULL;
}

uint32_t item_register(ItemDef def) {
    if (g_item_count >= MAX_ITEM_DEFS) {
        fprintf(stderr, "Item registry full\n");
        return 0;
    }
    g_item_defs[g_item_count++] = def;
    return def.id;
}

// ============================================================================
// Inventory Management
// ============================================================================

void inventory_init(Inventory* inv) {
    memset(inv, 0, sizeof(Inventory));
}

bool inventory_add_item(Inventory* inv, uint32_t item_id) {
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->items[i] == 0) {
            inv->items[i] = item_id;
            return true;
        }
    }
    return false;  // Inventory full
}

bool inventory_remove_item(Inventory* inv, uint8_t slot) {
    if (slot >= INVENTORY_SIZE) return false;
    if (inv->items[slot] == 0) return false;
    inv->items[slot] = 0;
    return true;
}

bool inventory_has_item(const Inventory* inv, uint32_t item_id) {
    return inventory_find_item(inv, item_id) >= 0;
}

int inventory_find_item(const Inventory* inv, uint32_t item_id) {
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->items[i] == item_id) {
            return i;
        }
    }
    return -1;
}

// ============================================================================
// Shopping
// ============================================================================

bool item_can_buy(const Inventory* inv, uint32_t item_id) {
    const ItemDef* def = item_get_def(item_id);
    if (!def) return false;

    // Check gold
    if (inv->gold < def->cost) return false;

    // Check inventory space
    bool has_space = false;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->items[i] == 0) {
            has_space = true;
            break;
        }
    }

    return has_space;
}

bool item_buy(Inventory* inv, uint32_t item_id) {
    if (!item_can_buy(inv, item_id)) return false;

    const ItemDef* def = item_get_def(item_id);
    inv->gold -= def->cost;
    inventory_add_item(inv, item_id);

    return true;
}

void item_sell(Inventory* inv, uint8_t slot) {
    if (slot >= INVENTORY_SIZE) return;
    if (inv->items[slot] == 0) return;

    const ItemDef* def = item_get_def(inv->items[slot]);
    if (def) {
        inv->gold += def->cost * 60 / 100;  // 60% sell value
    }

    inv->items[slot] = 0;
}

// ============================================================================
// Stats Calculation
// ============================================================================

ItemStats inventory_get_total_stats(const Inventory* inv) {
    ItemStats total = {0};

    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->items[i] == 0) continue;

        const ItemDef* def = item_get_def(inv->items[i]);
        if (!def) continue;

        total.health += def->stats.health;
        total.mana += def->stats.mana;
        total.health_regen += def->stats.health_regen;
        total.mana_regen += def->stats.mana_regen;
        total.attack_damage += def->stats.attack_damage;
        total.ability_power += def->stats.ability_power;
        total.armor += def->stats.armor;
        total.magic_resist += def->stats.magic_resist;
        total.attack_speed += def->stats.attack_speed;
        total.move_speed += def->stats.move_speed;
        total.crit_chance += def->stats.crit_chance;
        total.life_steal += def->stats.life_steal;
        total.spell_vamp += def->stats.spell_vamp;
        total.cooldown_reduction += def->stats.cooldown_reduction;
    }

    return total;
}
