#include <flecs_game.h>

ECS_DECLARE(EcsWorldCell);
ECS_COMPONENT_DECLARE(WorldCells);
ECS_COMPONENT_DECLARE(WorldCellCache);

typedef struct ecs_world_quadrant_t {
    ecs_map_t cells;
} ecs_world_quadrant_t;

typedef struct WorldCells {
    ecs_world_quadrant_t quadrants[4];
} WorldCells;

typedef struct WorldCellCache {
    uint64_t cell_id;
    uint64_t old_cell_id;
    int8_t quadrant;
} WorldCellCache;

static
void flecs_game_get_cell_id(
    WorldCellCache *cache,
    float xf, 
    float yf)
{
    int32_t x = xf;
    int64_t y = yf;

    uint8_t left = x < 0;
    uint8_t bottom = y < 0;

    x *= 1 - (2 * left);
    y *= 1 - (2 * bottom);

    x = x >> FLECS_GAME_WORLD_CELL_SHIFT;
    y = y >> FLECS_GAME_WORLD_CELL_SHIFT;

    cache->quadrant = left + bottom * 2;
    cache->cell_id = x + (y << 32);
}

static
ecs_entity_t flecs_game_get_cell(
    ecs_world_t *world,
    WorldCells *wcells,
    const WorldCellCache *wcache)
{
    int8_t quadrant = wcache->quadrant;
    uint64_t cell_id = wcache->cell_id;
    ecs_entity_t *cell_ptr = ecs_map_ensure(&wcells->quadrants[quadrant].cells, 
        ecs_entity_t, cell_id);
    ecs_entity_t cell = *cell_ptr;
    if (!cell) {
        cell = *cell_ptr = ecs_new(world, EcsWorldCell);

        // Decode cell coordinates from spatial hash
        int32_t left = (int32_t)cell_id;
        int32_t bottom = (int32_t)(cell_id >> 32);
        int32_t half_size = (1 << FLECS_GAME_WORLD_CELL_SHIFT) / 2;
        bottom = bottom << FLECS_GAME_WORLD_CELL_SHIFT;
        left = left << FLECS_GAME_WORLD_CELL_SHIFT;
        int32_t x = left + half_size;
        int32_t y = bottom + half_size;
        if (wcache->quadrant & 1) {
            x *= -1;
        }
        if (wcache->quadrant & 2) {
            y *= -1;
        }

        ecs_set(world, cell, EcsWorldCellCoord, {
            .x = x,
            .y = y,
            .size = 1 << FLECS_GAME_WORLD_CELL_SHIFT
        });
    }
    return cell;
}

static
void AddWorldCellCache(ecs_iter_t *it) {
    ecs_world_t *world = it->world;

    for (int i = 0; i < it->count; i ++) {
        ecs_set(world, it->entities[i], WorldCellCache, { 
            .cell_id = 0, .old_cell_id = -1
        });
    }    
}

static
void FindWorldCell(ecs_iter_t *it) {
    while (ecs_query_next_table(it)) {
        if (!ecs_query_changed(NULL, it)) {
            continue;
        }

        ecs_query_populate(it);

        EcsPosition3 *pos = ecs_field(it, EcsPosition3, 1);
        WorldCellCache *wcache = ecs_field(it, WorldCellCache, 2);

        for (int i = 0; i < it->count; i ++) {
            flecs_game_get_cell_id(&wcache[i], pos[i].x, pos[i].z);
        }
    }
}

static
void SetWorldCell(ecs_iter_t *it) {
    while (ecs_query_next_table(it)) {
        if (!ecs_query_changed(NULL, it)) {
            continue;
        }

        ecs_query_populate(it);

        ecs_world_t *world = it->world;
        WorldCellCache *wcache = ecs_field(it, WorldCellCache, 1);
        WorldCells *wcells = ecs_field(it, WorldCells, 2);

        for (int i = 0; i < it->count; i ++) {
            WorldCellCache *cur = &wcache[i];

            if (cur->cell_id != cur->old_cell_id) {
                ecs_entity_t cell = flecs_game_get_cell(world, wcells, cur);
                ecs_add_pair(world, it->entities[i], ecs_id(EcsWorldCell), cell);
            }
        }
    }
}

static
void ResetWorldCellCache(ecs_iter_t *it) {
    while (ecs_query_next_table(it)) {
        if (!ecs_query_changed(NULL, it)) {
            continue;
        }

        ecs_query_populate(it);

        WorldCellCache *wcache = ecs_field(it, WorldCellCache, 1);
        bool changed = false;

        for (int i = 0; i < it->count; i ++) {
            WorldCellCache *cur = &wcache[i];
            if (cur->old_cell_id != cur->cell_id) {
                cur->old_cell_id = cur->cell_id;
                changed = true;
            }
        }

        if (!changed) {
            ecs_query_skip(it);
        }
    }
}

void FlecsGameWorldCellsImport(ecs_world_t *world) {

    ECS_COMPONENT_DEFINE(world, WorldCellCache);
    ECS_COMPONENT_DEFINE(world, WorldCells);
    ECS_ENTITY_DEFINE(world, EcsWorldCell, Tag, Exclusive);

    ecs_set_hooks(world, WorldCells, {
        .ctor = ecs_default_ctor
    });

    ECS_SYSTEM(world, AddWorldCellCache, EcsOnLoad,
        [none] flecs.components.transform.Position3(self),
        [out]  !flecs.game.WorldCellCache(self),
        [out]  !flecs.game.WorldCell(self),
        [none] !flecs.components.transform.Position3(up(ChildOf)));

    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "FindWorldCell",
            .add = { ecs_dependson(EcsOnValidate) }
        }),
        .query = {
            .filter.terms = {{
                .id = ecs_id(EcsPosition3),
                .inout = EcsIn,
                .src.flags = EcsSelf
            }, {
                .id = ecs_id(WorldCellCache),
                .inout = EcsOut,
                .src.flags = EcsSelf
            }}
        },
        .run = FindWorldCell
    });

    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "SetWorldCell",
            .add = { ecs_dependson(EcsOnValidate) }
        }),
        .query = {
            .filter.terms = {{
                .id = ecs_id(WorldCellCache),
                .inout = EcsIn,
                .src.flags = EcsSelf
            }, {
                .id = ecs_id(WorldCells),
                .inout = EcsIn,
                .src.flags = EcsSelf,
                .src.id = ecs_id(WorldCells)
            }}
        },
        .run = SetWorldCell
    });

    ecs_system(world, {
        .entity = ecs_entity(world, {
            .name = "ResetWorldCellCache",
            .add = { ecs_dependson(EcsOnValidate) }
        }),
        .query = {
            .filter.terms = {{
                .id = ecs_id(WorldCellCache),
                .inout = EcsInOut,
                .src.flags = EcsSelf
            }}
        },
        .run = ResetWorldCellCache
    });

    WorldCells *wcells = ecs_singleton_get_mut(world, WorldCells);
    ecs_map_init(&wcells->quadrants[0].cells, ecs_entity_t, NULL, 1);
    ecs_map_init(&wcells->quadrants[1].cells, ecs_entity_t, NULL, 1);
    ecs_map_init(&wcells->quadrants[2].cells, ecs_entity_t, NULL, 1);
    ecs_map_init(&wcells->quadrants[3].cells, ecs_entity_t, NULL, 1);
}
