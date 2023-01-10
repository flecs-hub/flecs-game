#define FLECS_GAME_IMPL

#include <flecs_game.h>

ECS_CTOR(EcsCameraController, ptr, {
    ecs_os_zeromem(ptr);
    ptr->gear = 5.0;
})

void FlecsGameCameraControllerImport(ecs_world_t *world);
void FlecsGameWorldCellsImport(ecs_world_t *world);

void FlecsGameImport(ecs_world_t *world) {
    ECS_MODULE(world, FlecsGame);

    ECS_IMPORT(world, FlecsComponentsTransform);
    ECS_IMPORT(world, FlecsComponentsPhysics);
    ECS_IMPORT(world, FlecsComponentsGraphics);
    ECS_IMPORT(world, FlecsComponentsInput);
    ECS_IMPORT(world, FlecsSystemsPhysics);

    ecs_set_name_prefix(world, "Ecs");

    ECS_META_COMPONENT(world, EcsCameraController);
    ECS_META_COMPONENT(world, EcsWorldCellCoord);

    ecs_set_hooks(world, EcsCameraController, {
        .ctor = ecs_ctor(EcsCameraController)
    });

    FlecsGameCameraControllerImport(world);
    FlecsGameWorldCellsImport(world);
}
