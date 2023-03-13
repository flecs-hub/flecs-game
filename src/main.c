#define FLECS_GAME_IMPL

#include <flecs_game.h>

#define VARIATION_SLOTS_MAX (20)

ECS_DECLARE(EcsCameraController);

void FlecsGameCameraControllerImport(ecs_world_t *world);
void FlecsGameWorldCellsImport(ecs_world_t *world);

static
float randf(float max) {
    return max * (float)rand() / (float)RAND_MAX;
}

typedef struct {
    float x_count;
    float y_count;
    float z_count;
    float x_spacing;
    float y_spacing;
    float z_spacing;
    float x_half;
    float y_half;
    float z_half;
    float x_var;
    float y_var;
    float z_var;
    float variations_total;
    int32_t variations_count;
} flecs_grid_params_t;

static
void generate_tile(
    ecs_world_t *world,
    const EcsGrid *grid,
    int32_t x,
    int32_t y,
    int32_t z,
    const flecs_grid_params_t *params)
{
    float xc = (float)x * params->x_spacing - params->x_half;
    float yc = (float)y * params->y_spacing - params->y_half;
    float zc = (float)z * params->z_spacing - params->z_half;

    if (params->x_var) {
        xc += randf(params->x_var / 2) - params->x_var;
    }
    if (params->y_var) {
        yc += randf(params->y_var / 2) - params->y_var;
    }
    if (params->z_var) {
        zc += randf(params->z_var / 2) - params->z_var;
    }

    ecs_entity_t slot = 0;
    if (grid->prefab) {
        slot = grid->prefab;
    } else {
        float p = randf(params->variations_total), cur = 0;
        for (int v = 0; v < params->variations_count; v ++) {
            cur += grid->variations[v].chance;
            if (p <= cur) {
                slot = grid->variations[v].prefab;
                break;
            }
        }
    }

    ecs_entity_t inst = ecs_new_w_pair(world, EcsIsA, slot);
    ecs_set(world, inst, EcsPosition3, {xc, yc, zc});
}

static
void generate_grid(ecs_world_t *world, ecs_entity_t parent, const EcsGrid *grid) {
    flecs_grid_params_t params = {0};

    params.x_count = glm_max(1, grid->x.count);
    params.y_count = glm_max(1, grid->y.count);
    params.z_count = glm_max(1, grid->z.count);

    bool border = false;
    if (grid->border.x || grid->border.y || grid->border.z) {
        params.x_spacing = grid->border.x / params.x_count;
        params.y_spacing = grid->border.y / params.y_count;
        params.z_spacing = grid->border.z / params.z_count;
        border = true;
    } else {
        params.x_spacing = glm_max(0.001, grid->x.spacing);
        params.y_spacing = glm_max(0.001, grid->y.spacing);
        params.z_spacing = glm_max(0.001, grid->z.spacing);
    }

    params.x_half = ((params.x_count - 1) / 2.0) * params.x_spacing;
    params.y_half = ((params.y_count - 1) / 2.0) * params.y_spacing;
    params.z_half = ((params.z_count - 1) / 2.0) * params.z_spacing;
    
    params.x_var = grid->x.variation;
    params.y_var = grid->y.variation;
    params.z_var = grid->z.variation;

    ecs_entity_t prefab = grid->prefab;
    params.variations_total = 0;
    params.variations_count = 0;
    if (!prefab) {
        for (int i = 0; i < VARIATION_SLOTS_MAX; i ++) {
            if (!grid->variations[i].prefab) {
                break;
            }
            params.variations_total += grid->variations[i].chance;
            params.variations_count ++;
        }
    }

    if (!prefab && !params.variations_count) {
        return;
    }

    if (!border) {
        for (int32_t x = 0; x < params.x_count; x ++) {
            for (int32_t y = 0; y < params.y_count; y ++) {
                for (int32_t z = 0; z < params.z_count; z ++) {
                    generate_tile(world, grid, x, y, z, &params);
                }
            }
        }
    } else {
        for (int32_t x = 0; x < params.x_count; x ++) {
            generate_tile(world, grid, x, 0, 0, &params);
            generate_tile(world, grid, 
                x, params.y_count - 1, params.z_count - 1, &params);
        }
        for (int32_t y = 1; y < params.y_count - 1; y ++) {
            generate_tile(world, grid, 0, y, 0, &params);
            generate_tile(world, grid, 
                params.x_count - 1, y, params.z_count - 1, &params);
        }
        for (int32_t z = 1; z < params.z_count - 1; z ++) {
            generate_tile(world, grid, 0, 0, z, &params);
            generate_tile(world, grid, 
                params.x_count - 1, params.z_count - 1, z, &params);
        }
    }
}

static
void SetGrid(ecs_iter_t *it) {
    EcsGrid *grid = ecs_field(it, EcsGrid, 1);

    for (int i = 0; i < it->count; i ++) {
        ecs_entity_t g = it->entities[0];
        ecs_delete_with(it->world, ecs_pair(EcsChildOf, g));
        generate_grid(it->world, g, &grid[i]);
    }
}

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
    ECS_META_COMPONENT(world, ecs_grid_slot_t);
    ECS_META_COMPONENT(world, ecs_grid_coord_t);
    ECS_META_COMPONENT(world, EcsGrid);

    FlecsGameCameraControllerImport(world);
    FlecsGameWorldCellsImport(world);

    ECS_OBSERVER(world, SetGrid, EcsOnSet, Grid);
}
