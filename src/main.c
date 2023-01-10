#define FLECS_GAME_IMPL

#include <flecs_game.h>

ECS_DECLARE(EcsCameraController);

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

    ECS_TAG_DEFINE(world, EcsCameraController);
    ECS_META_COMPONENT(world, EcsCameraAutoMove);
    ECS_META_COMPONENT(world, EcsWorldCellCoord);

    FlecsGameCameraControllerImport(world);
    FlecsGameWorldCellsImport(world);
}
