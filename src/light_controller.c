#include <flecs_game.h>

static
void LightControllerSyncPosition(ecs_iter_t *it) {
    EcsDirectionalLight *light = ecs_field(it, EcsDirectionalLight, 1);
    EcsPosition3 *p = ecs_field(it, EcsPosition3, 2);

    for (int i = 0; i < it->count; i ++) {
        light[i].position[0] = p[i].x;
        light[i].position[1] = p[i].y;
        light[i].position[2] = p[i].z;
    }
}

static
void LightControllerSyncRotation(ecs_iter_t *it) {
    EcsDirectionalLight *light = ecs_field(it, EcsDirectionalLight, 1);
    EcsPosition3 *p = ecs_field(it, EcsPosition3, 2);
    EcsRotation3 *r = ecs_field(it, EcsRotation3, 3);

    for (int i = 0; i < it->count; i ++) {
        light[i].direction[0] = p[i].x + sin(r[i].y) * cos(r[i].x);
        light[i].direction[1] = p[i].y + sin(r[i].x);
        light[i].direction[2] = p[i].z + cos(r[i].y) * cos(r[i].x);
    }
}

static
void LightControllerSyncColor(ecs_iter_t *it) {
    EcsDirectionalLight *light = ecs_field(it, EcsDirectionalLight, 1);
    EcsRgb *color = ecs_field(it, EcsRgb, 2);

    for (int i = 0; i < it->count; i ++) {
        light[i].color[0] = color[i].r;
        light[i].color[1] = color[i].g;
        light[i].color[2] = color[i].b;
    }
}

static
void LightControllerSyncIntensity(ecs_iter_t *it) {
    EcsDirectionalLight *light = ecs_field(it, EcsDirectionalLight, 1);
    EcsLightIntensity *intensity = ecs_field(it, EcsLightIntensity, 2);

    for (int i = 0; i < it->count; i ++) {
        light[i].intensity = intensity[i].value;
    }
}

static
void TimeOfDayUpdate(ecs_iter_t *it) {
    EcsTimeOfDay *tod = ecs_field(it, EcsTimeOfDay, 1);
    tod->t += it->delta_time * tod->speed;
}

static
float get_time_of_day(float t) {
    return (t + 1.0) * M_PI;
}

static
float get_sun_height(float t) {
    return -sin(get_time_of_day(t));
}

static
void LightControllerTimeOfDay(ecs_iter_t *it) {
    EcsTimeOfDay *tod = ecs_field(it, EcsTimeOfDay, 1);
    EcsRotation3 *r = ecs_field(it, EcsRotation3, 2);
    EcsRgb *color = ecs_field(it, EcsRgb, 3);
    EcsLightIntensity *light_intensity = ecs_field(it, EcsLightIntensity, 4);

    static vec3 day = {0.8, 0.8, 0.6};
    static vec3 twilight = {1.0, 0.1, 0.01};
    float twilight_angle = 0.3;

    for (int i = 0; i < it->count; i ++) {
        r[i].x = get_time_of_day(tod[i].t);

        float t_sin = get_sun_height(tod[i].t);
        float t_sin_low = twilight_angle - t_sin;
        vec3 sun_color;
        if (t_sin_low > 0) {
            t_sin_low *= 1.0 / twilight_angle;
            glm_vec3_lerp(day, twilight, t_sin_low, sun_color);
        } else {
            glm_vec3_copy(day, sun_color);
        }

        /* increase just before sunrise/after sunset*/
        float intensity = t_sin + 0.07;
        if (intensity < 0) {
            intensity = 0;
        }

        color[i].r = sun_color[0];
        color[i].g = sun_color[1];
        color[i].b = sun_color[2];
        light_intensity[i].value = intensity;
    }
}

static
void AmbientLightControllerTimeOfDay(ecs_iter_t *it) {
    EcsTimeOfDay *tod = ecs_field(it, EcsTimeOfDay, 1);
    EcsCanvas *canvas = ecs_field(it, EcsCanvas, 2);

    static vec3 ambient_day = {0.03, 0.06, 0.09};
    static vec3 ambient_night = {0.001, 0.008, 0.016};
    static vec3 ambient_twilight = {0.01, 0.017, 0.02};
    static float twilight_zone = 0.2;

    for (int i = 0; i < it->count; i ++) {
        float t_sin = get_sun_height(tod[i].t);
        t_sin = (t_sin + 1.0) / 2;

        float t_twilight = glm_max(0.0, twilight_zone - fabs(t_sin - 0.5));
        t_twilight *= (1.0 / twilight_zone);

        vec3 ambient_color;
        glm_vec3_lerp(ambient_night, ambient_day, t_sin, ambient_color);
        glm_vec3_lerp(ambient_color, ambient_twilight, t_twilight, ambient_color);
        canvas[i].ambient_light.r = ambient_color[0];
        canvas[i].ambient_light.g = ambient_color[1];
        canvas[i].ambient_light.b = ambient_color[2];
    }
}

void FlecsGameLightControllerImport(ecs_world_t *world) {
    ECS_SYSTEM(world, LightControllerSyncPosition, EcsOnUpdate,
        [out]    flecs.components.graphics.DirectionalLight, 
        [in]     flecs.components.transform.Position3);

    ECS_SYSTEM(world, LightControllerSyncRotation, EcsOnUpdate,
        [out]    flecs.components.graphics.DirectionalLight, 
        [in]     flecs.components.transform.Position3,
        [in]     flecs.components.transform.Rotation3);

    ECS_SYSTEM(world, LightControllerSyncIntensity, EcsOnUpdate,
        [out]    flecs.components.graphics.DirectionalLight, 
        [in]     flecs.components.graphics.LightIntensity);

    ECS_SYSTEM(world, LightControllerSyncColor, EcsOnUpdate,
        [out]    flecs.components.graphics.DirectionalLight, 
        [in]     flecs.components.graphics.Rgb);

    ECS_SYSTEM(world, TimeOfDayUpdate, EcsOnUpdate,
        [inout]   TimeOfDay($));

    ECS_SYSTEM(world, LightControllerTimeOfDay, EcsOnUpdate,
        [in]      TimeOfDay($), 
        [out]     flecs.components.transform.Rotation3,
        [out]     flecs.components.graphics.Rgb,
        [out]     flecs.components.graphics.LightIntensity,
        [none]    flecs.components.graphics.Sun);

    ECS_SYSTEM(world, AmbientLightControllerTimeOfDay, EcsOnUpdate,
        [in]      TimeOfDay($), 
        [out]     flecs.components.gui.Canvas);

    ecs_add_pair(world, EcsSun, EcsWith, ecs_id(EcsRotation3));
    ecs_add_pair(world, EcsSun, EcsWith, ecs_id(EcsDirectionalLight));
    ecs_add_pair(world, EcsSun, EcsWith, ecs_id(EcsRgb));
    ecs_add_pair(world, EcsSun, EcsWith, ecs_id(EcsLightIntensity));
}
