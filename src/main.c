#define FLECS_GAME_IMPL

#include <flecs_game.h>

#define VARIATION_SLOTS_MAX (20)

ECS_DECLARE(EcsCameraController);

void FlecsGameCameraControllerImport(ecs_world_t *world);
void FlecsGameLightControllerImport(ecs_world_t *world);

ECS_CTOR(EcsParticleEmitter, ptr, {
    ptr->particle = 0;
    ptr->spawn_interval = 1.0;
    ptr->lifespan = 1000.0;
    ptr->size_decay = 1.0;
    ptr->color_decay = 1.0;
    ptr->velocity_decay = 1.0;
    ptr->t = 0;
})

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
    ecs_entity_t variations[VARIATION_SLOTS_MAX];
    ecs_entity_t prefab;
} flecs_grid_params_t;

static
ecs_entity_t get_prefab(
    ecs_world_t *world, 
    ecs_entity_t parent,
    ecs_entity_t prefab) 
{
    if (!prefab) {
        return 0;
    }

    /* If prefab is a script/assembly, create a private instance of the
     * assembly for the grid with default values. This allows applications to
     * use assemblies directly vs. having to create a dummy prefab */
    ecs_entity_t result = prefab;
    if (ecs_has(world, prefab, EcsScript) && ecs_has(world, prefab, EcsComponent)) {
        result = ecs_new(world);
        ecs_add_pair(world, result, EcsChildOf, parent);
        ecs_add_id(world, result, EcsPrefab);
        ecs_add_id(world, result, prefab);
    }

    return result;
}

static
ecs_entity_t generate_tile(
    ecs_world_t *world,
    ecs_entity_t parent,
    const EcsGrid *grid,
    float xc,
    float yc,
    float zc,
    const flecs_grid_params_t *params)
{
    if (params->x_var) {
        xc += randf(params->x_var) - params->x_var / 2;
    }
    if (params->y_var) {
        yc += randf(params->y_var) - params->y_var / 2;
    }
    if (params->z_var) {
        zc += randf(params->z_var) - params->z_var / 2;
    }

    ecs_entity_t slot = 0;
    if (params->prefab) {
        slot = params->prefab;
    } else {
        float p = randf(params->variations_total), cur = 0;
        for (int v = 0; v < params->variations_count; v ++) {
            cur += grid->variations[v].chance;
            if (p <= cur) {
                slot = params->variations[v];
                break;
            }
        }
    }
    
    ecs_entity_t inst = ecs_new_w_pair(world, EcsChildOf, parent);
    ecs_add_pair(world, inst, EcsIsA, slot);
    ecs_set(world, inst, EcsPosition3, {xc, yc, zc});
    return inst;
}

static
void generate_grid(
    ecs_world_t *world, 
    ecs_entity_t parent, 
    const EcsGrid *grid) 
{
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
            params.variations[i] = get_prefab(world, parent, 
                grid->variations[i].prefab);
            params.variations_total += grid->variations[i].chance;
            params.variations_count ++;
        }
    } else {
        prefab = params.prefab = get_prefab(world, parent, prefab);
    }

    if (!prefab && !params.variations_count) {
        return;
    }

    if (!border) {
        for (int32_t x = 0; x < params.x_count; x ++) {
            for (int32_t y = 0; y < params.y_count; y ++) {
                for (int32_t z = 0; z < params.z_count; z ++) {
                    float xc = (float)x * params.x_spacing - params.x_half;
                    float yc = (float)y * params.y_spacing - params.y_half;
                    float zc = (float)z * params.z_spacing - params.z_half;
                    generate_tile(world, parent, grid, xc, yc, zc, &params);
                }
            }
        }
    } else {
        for (int32_t x = 0; x < params.x_count; x ++) {
            float xc = (float)x * params.x_spacing - params.x_half;
            float zc = grid->border.z / 2 + grid->border_offset.z;
            generate_tile(world, parent, grid, xc, 0, -zc, &params);
            generate_tile(world, parent, grid, xc, 0, zc, &params);
        }

        for (int32_t x = 0; x < params.z_count; x ++) {
            float xc = grid->border.x / 2 + grid->border_offset.x;
            float zc = (float)x * params.z_spacing - params.z_half;
            ecs_entity_t inst;
            inst = generate_tile(world, parent, grid, xc, 0, zc, &params);
            ecs_set(world, inst, EcsRotation3, {0, GLM_PI / 2, 0});
            inst = generate_tile(world, parent, grid, -xc, 0, zc, &params);
            ecs_set(world, inst, EcsRotation3, {0, GLM_PI / 2, 0});
        }
    }
}

static
void SetGrid(ecs_iter_t *it) {
    EcsGrid *grid = ecs_field(it, EcsGrid, 0);

    for (int i = 0; i < it->count; i ++) {
        ecs_entity_t g = it->entities[i];
        ecs_delete_with(it->world, ecs_pair(EcsChildOf, g));
        generate_grid(it->world, g, &grid[i]);
    }
}

static
void ParticleEmit(ecs_iter_t *it) {
    EcsParticleEmitter *e = ecs_field(it, EcsParticleEmitter, 0);
    EcsBox *box = ecs_field(it, EcsBox, 1);

    for (int i = 0; i < it->count; i ++) {
        e->t += it->delta_time;
        if (e->t > e->spawn_interval) {
            e->t -= e->spawn_interval;

            ecs_entity_t p = ecs_insert(it->world, 
                {ecs_childof(it->entities[i])},
                {ecs_isa(e->particle)},
                ecs_value(EcsParticle, {
                    .t = e->lifespan
                }));

            if (box) {
                EcsPosition3 pos = {0, 0, 0};
                pos.x = randf(box[i].width) - box[i].width / 2;
                pos.y = randf(box[i].height) - box[i].height / 2;
                pos.z = randf(box[i].depth) - box[i].depth / 2;
                ecs_set_ptr(it->world, p, EcsPosition3, &pos);
            }

            ecs_set(it->world, p, EcsRotation3, { 0, randf(4 * 3.1415926), 0 });
        }
    }
}

static
void ParticleProgress(ecs_iter_t *it) {
    EcsParticle *p = ecs_field(it, EcsParticle, 0);
    EcsParticleEmitter *e = ecs_field(it, EcsParticleEmitter, 1);
    EcsBox *box = ecs_field(it, EcsBox, 2);
    EcsRgb *color = ecs_field(it, EcsRgb, 3);
    EcsVelocity3 *vel = ecs_field(it, EcsVelocity3, 4);

    for (int i = 0; i < it->count; i ++) {
        p[i].t -= it->delta_time;
        if (p[i].t <= 0) {
            ecs_delete(it->world, it->entities[i]);
        }
    }

    if (box) {
        for (int i = 0; i < it->count; i ++) {
            box[i].width *= pow(e->size_decay, it->delta_time);
            box[i].height *= pow(e->size_decay, it->delta_time);
            box[i].depth *= pow(e->size_decay, it->delta_time);

            if ((box[i].width + box[i].height + box[i].depth) < 0.1) {
                ecs_delete(it->world, it->entities[i]);
            }
        }
    }
    if (color) {
        for (int i = 0; i < it->count; i ++) {
            color[i].r *= pow(e->color_decay, it->delta_time);
            color[i].g *= pow(e->color_decay, it->delta_time);
            color[i].b *= pow(e->color_decay, it->delta_time);
        } 
    }
    if (vel) {
        for (int i = 0; i < it->count; i ++) {
            vel[i].x *= pow(e->velocity_decay, it->delta_time);
            vel[i].y *= pow(e->velocity_decay, it->delta_time);
            vel[i].z *= pow(e->velocity_decay, it->delta_time);
        }
    }
}

void FlecsGameImport(ecs_world_t *world) {
    ECS_MODULE(world, FlecsGame);

    ECS_IMPORT(world, FlecsComponentsTransform);
    ECS_IMPORT(world, FlecsComponentsPhysics);
    ECS_IMPORT(world, FlecsComponentsGraphics);
    ECS_IMPORT(world, FlecsComponentsGui);
    ECS_IMPORT(world, FlecsComponentsInput);
    ECS_IMPORT(world, FlecsSystemsPhysics);

    ecs_set_name_prefix(world, "Ecs");

    ECS_TAG_DEFINE(world, EcsCameraController);
    ECS_META_COMPONENT(world, EcsCameraAutoMove);
    ECS_META_COMPONENT(world, EcsTimeOfDay);
    ECS_META_COMPONENT(world, ecs_grid_slot_t);
    ECS_META_COMPONENT(world, ecs_grid_coord_t);
    ECS_META_COMPONENT(world, EcsGrid);
    ECS_META_COMPONENT(world, EcsParticleEmitter);
    ECS_META_COMPONENT(world, EcsParticle);

    FlecsGameCameraControllerImport(world);
    FlecsGameLightControllerImport(world);

    ecs_set_hooks(world, EcsTimeOfDay, {
        .ctor = flecs_default_ctor
    });

    ECS_OBSERVER(world, SetGrid, EcsOnSet, Grid);

    ecs_set_hooks(world, EcsParticleEmitter, {
        .ctor = ecs_ctor(EcsParticleEmitter)
    });

    ECS_SYSTEM(world, ParticleEmit, EcsOnUpdate, 
        ParticleEmitter,
        ?flecs.components.geometry.Box);

    ECS_SYSTEM(world, ParticleProgress, EcsOnUpdate, 
        Particle, 
        ParticleEmitter(up),
        ?flecs.components.geometry.Box(self),
        ?flecs.components.graphics.Rgb(self),
        ?flecs.components.physics.Velocity3(self));
}
