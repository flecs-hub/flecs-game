#ifndef GAME_H
#define GAME_H

/* This generated file contains includes for project dependencies */
#include "flecs-game/bake_config.h"

// Reflection system boilerplate
#undef ECS_META_IMPL
#ifndef FLECS_GAME_IMPL
#define ECS_META_IMPL EXTERN // Ensure meta symbols are only defined once
#endif

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
ECS_STRUCT(EcsTimeOfDay, {
    float t;
    float speed;
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
    EcsPosition3 border_offset;

    ecs_entity_t prefab;
    ecs_grid_slot_t variations[20];
});

FLECS_GAME_API
ECS_STRUCT(EcsParticleEmitter, {
    ecs_entity_t particle;
    float spawn_interval;
    float lifespan;
    float size_decay;
    float color_decay;
    float velocity_decay;
    float t;
});

FLECS_GAME_API
ECS_STRUCT(EcsParticle, {
    float t;
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

    game(flecs::world& ecs) {
        // Load module contents
        FlecsGameImport(ecs);

        // Bind C++ types with module contents
        ecs.module<flecs::game>();
    }
};

}

#endif
#endif

#endif
