#ifndef GAME_H
#define GAME_H

/* This generated file contains includes for project dependencies */
#include "flecs-game/bake_config.h"

#ifdef __cplusplus
extern "C" {
#endif

FLECS_GAME_API
extern ECS_DECLARE(EcsCameraController);

FLECS_GAME_API
void FlecsGameImport(ecs_world_t *world);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#ifndef FLECS_NO_CPP

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

