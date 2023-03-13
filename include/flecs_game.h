#ifndef GAME_H
#define GAME_H

/* This generated file contains includes for project dependencies */
#include "flecs-game/bake_config.h"

// Reflection system boilerplate
#undef ECS_META_IMPL
#ifndef FLECS_GAME_IMPL
#define ECS_META_IMPL EXTERN // Ensure meta symbols are only defined once
#endif

// Number of bits to shift from x/y coordinate before creating the spatial hash.
// Larger numbers create larger cells.
#define FLECS_GAME_WORLD_CELL_SHIFT (8)

// Convenience macro to get size of world cell
#define FLECS_GAME_WORLD_CELL_SIZE (1 << FLECS_GAME_WORLD_CELL_SHIFT)

#ifdef __cplusplus
extern "C" {
#endif

FLECS_GAME_API
extern ECS_DECLARE(EcsCameraController);

FLECS_GAME_API
ECS_STRUCT(EcsCameraAutoMove, {
    float after;
    float speed;
    float t;
});

FLECS_GAME_API
extern ECS_DECLARE(EcsWorldCell);

FLECS_GAME_API
extern ECS_DECLARE(EcsWorldCellRoot);

FLECS_GAME_API
ECS_STRUCT(EcsWorldCellCoord, {
    int64_t x;
    int64_t y;
    int32_t size;
});

FLECS_GAME_API
ECS_STRUCT(ecs_grid_slot_t, {
    ecs_entity_t prefab;
    float chance;
});

FLECS_GAME_API
ECS_STRUCT(ecs_grid_coord_t, {
    int32_t count;
    float spacing;
    float variation;
});

FLECS_GAME_API
ECS_STRUCT(EcsGrid, {
    ecs_grid_coord_t x;
    ecs_grid_coord_t y;
    ecs_grid_coord_t z;

    EcsPosition3 border;

    ecs_entity_t prefab;
    ecs_grid_slot_t variations[20];
});

FLECS_GAME_API
void FlecsGameImport(ecs_world_t *world);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#ifndef FLECS_NO_CPP
#include <iostream>

namespace flecs {

struct game {
    static flecs::entity_t CameraController;

    game(flecs::world& ecs) {
        // Load module contents
        FlecsGameImport(ecs);

        CameraController = EcsCameraController;

        // Bind C++ types with module contents
        ecs.module<flecs::game>();
    }
};

flecs::entity_t game::CameraController = 0;

}

#endif
#endif

#endif
