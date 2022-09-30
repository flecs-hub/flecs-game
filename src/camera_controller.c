#include <flecs_game.h>

static const float CameraAccelerationFactor = 40.0;
static const float CameraDecelerationC = 0.3;
static const float CameraAngularAccelerationFactor = 5.0;
static const float CameraAngularDecelerationC = 2.0;
static const float CameraMaxAngularSpeed = 1.5;
static const float CameraGearFactor = 5.0;

static
void camera_controller_decel(float *v_ptr, float a) {
    float v = v_ptr[0];

    if (v > 0) {
        v = glm_clamp(v - a, 0, v);
    }
    if (v < 0) {
        v = glm_clamp(v + a, v, 0);
    }

    v_ptr[0] = v;
}

static
void CameraControllerAddPosition(ecs_iter_t *it) {
    for (int i = 0; i < it->count; i ++) {
        ecs_set(it->world, it->entities[i], EcsPosition3, {0, -1.5});
    }
}

static
void CameraControllerAddRotation(ecs_iter_t *it) {
    for (int i = 0; i < it->count; i ++) {
        ecs_set(it->world, it->entities[i], EcsRotation3, {0, 0});
    }
}

static
void CameraControllerAddVelocity(ecs_iter_t *it) {
    for (int i = 0; i < it->count; i ++) {
        ecs_set(it->world, it->entities[i], EcsVelocity3, {0, 0});
    }
}

static
void CameraControllerAddAngularVelocity(ecs_iter_t *it) {
    for (int i = 0; i < it->count; i ++) {
        ecs_set(it->world, it->entities[i], EcsAngularVelocity, {0, 0});
    }
}

static
void CameraControllerSyncPosition(ecs_iter_t *it) {
    EcsCamera *camera = ecs_field(it, EcsCamera, 1);
    EcsPosition3 *p = ecs_field(it, EcsPosition3, 2);

    for (int i = 0; i < it->count; i ++) {
        camera[i].position[0] = p[i].x;
        camera[i].position[1] = p[i].y;
        camera[i].position[2] = p[i].z;
    }
}

static
void CameraControllerSyncRotation(ecs_iter_t *it) {
    EcsCamera *camera = ecs_field(it, EcsCamera, 1);
    EcsPosition3 *p = ecs_field(it, EcsPosition3, 2);
    EcsRotation3 *r = ecs_field(it, EcsRotation3, 3);

    for (int i = 0; i < it->count; i ++) {
        camera[i].lookat[0] = p[i].x + sin(r[i].y) * cos(r[i].x);
        camera[i].lookat[1] = p[i].y + sin(r[i].x);
        camera[i].lookat[2] = p[i].z + cos(r[i].y) * cos(r[i].x);;
    }
}

static
void CameraControllerAccelerate(ecs_iter_t *it) {
    EcsInput *input = ecs_field(it, EcsInput, 1);
    EcsRotation3 *r = ecs_field(it, EcsRotation3, 2);
    EcsVelocity3 *v = ecs_field(it, EcsVelocity3, 3);
    EcsAngularVelocity *av = ecs_field(it, EcsAngularVelocity, 4);
    EcsCameraController *cc = ecs_field(it, EcsCameraController, 5);

    vec3 zero = {0};
    float dt = it->delta_time;
    float dt_accel = dt * CameraAccelerationFactor;
    float dt_angular_accel = dt * CameraAngularAccelerationFactor;
    float dt_decel = dt * CameraAccelerationFactor * CameraDecelerationC;
    float dt_angular_decel = dt * CameraAngularAccelerationFactor * CameraAngularDecelerationC;

    for (int i = 0; i < it->count; i ++) {
        float gear = cc[i].gear;
        float max_speed = gear * CameraGearFactor;
        float accel = gear * dt_accel;
        float angular_accel = dt_angular_accel;
        float angle = r[i].y;
        vec3 v3 = {v[i].x, v[i].y, v[i].z}, vn3;
        vec3 av3 = {av[i].x, av[i].y, av[i].z};
        float speed = glm_vec3_distance(zero, v3);
        float angular_speed = glm_vec3_distance(zero, av3);
        bool did_accel = false, did_angular_accel = false;

        // Camera XZ movement
        if (speed < max_speed) {
            if (input->keys[ECS_KEY_W].state) {
                v[i].x += sin(angle) * accel;
                v[i].z += cos(angle) * accel;
                did_accel = true;
            }
            if (input->keys[ECS_KEY_S].state) {
                v[i].x += sin(angle + GLM_PI) * accel;
                v[i].z += cos(angle + GLM_PI) * accel;
                did_accel = true;
            }

            if (input->keys[ECS_KEY_D].state) {
                v[i].x += cos(angle) * accel;
                v[i].z -= sin(angle) * accel;
                did_accel = true;
            }
            if (input->keys[ECS_KEY_A].state) {
                v[i].x += cos(angle + GLM_PI) * accel;
                v[i].z -= sin(angle + GLM_PI) * accel;
                did_accel = true;
            }

            // Camera Y movement
            if (input->keys[ECS_KEY_E].state) {
                v[i].y -= accel;
                did_accel = true;
            }
            if (input->keys[ECS_KEY_Q].state) {
                v[i].y += accel;
                did_accel = true;
            }
        }

        if (speed && !did_accel) {
            float decel = gear * dt_decel;
            glm_vec3_normalize_to(v3, vn3);
            camera_controller_decel(&v[i].x, decel * fabs(vn3[0]));
            camera_controller_decel(&v[i].y, decel * fabs(vn3[1]));
            camera_controller_decel(&v[i].z, decel * fabs(vn3[2]));
        }

        // Camera Y rotation
        if (angular_speed < CameraMaxAngularSpeed) {
            if (input->keys[ECS_KEY_RIGHT].state) {
                av[i].y += angular_accel;
                did_angular_accel = true;
            }
            if (input->keys[ECS_KEY_LEFT].state) {
                av[i].y -= angular_accel;
                did_angular_accel = true;
            }

            // Camera X rotation
            if (input->keys[ECS_KEY_UP].state) {
                av[i].x -= angular_accel;
                did_angular_accel = true;
            }
            if (input->keys[ECS_KEY_DOWN].state) {
                av[i].x += angular_accel;
                did_angular_accel = true;
            }
        }

        if (!did_angular_accel) {
            camera_controller_decel(&av[i].x, dt_angular_decel);
            camera_controller_decel(&av[i].y, dt_angular_decel);
        }

        // Gear selection
        if (input->keys[ECS_KEY_COMMA].state) {
            gear -= dt * 10;
        }
        if (input->keys[ECS_KEY_PERIOD].state) {
            gear += dt * 10;
        }
        cc[i].gear = glm_max(0.1, gear);
    }
}

static
void CameraLimitY(ecs_iter_t *it) {
    EcsPosition3 *p = ecs_field(it, EcsPosition3, 1);
    EcsVelocity3 *v = ecs_field(it, EcsVelocity3, 2);
    EcsCameraController *cc = ecs_field(it, EcsCameraController, 3);

    for (int i = 0; i < it->count; i ++) {
        if (cc[i].limit_y) {
            float y = p[i].y;
            float vy = v[i].y * it->delta_time;
            float max_y = cc[i].max_y;

            if ((y + vy) > max_y) {
                if (vy > 0) {
                    v[i].y = 0;
                }
            }
        }
    }
}

void FlecsGameCameraControllerImport(ecs_world_t *world) {
    ECS_SYSTEM(world, CameraControllerAddPosition, EcsOnLoad,
        [filter] flecs.components.graphics.Camera,
        [filter] CameraController,
        [out]    !flecs.components.transform.Position3);

    ECS_SYSTEM(world, CameraControllerAddRotation, EcsOnLoad,
        [filter] flecs.components.graphics.Camera,
        [filter] CameraController,
        [out]    !flecs.components.transform.Rotation3);

    ECS_SYSTEM(world, CameraControllerAddVelocity, EcsOnLoad,
        [filter] flecs.components.graphics.Camera,
        [filter] CameraController,
        [out]    !flecs.components.physics.Velocity3);

    ECS_SYSTEM(world, CameraControllerAddAngularVelocity, EcsOnLoad,
        [filter] flecs.components.graphics.Camera,
        [filter] CameraController,
        [out]    !flecs.components.physics.AngularVelocity);

    ECS_SYSTEM(world, CameraControllerSyncPosition, EcsOnUpdate,
        [out]    flecs.components.graphics.Camera, 
        [in]     flecs.components.transform.Position3,
        [filter] CameraController);

    ECS_SYSTEM(world, CameraControllerSyncRotation, EcsOnUpdate,
        [out]    flecs.components.graphics.Camera, 
        [in]     flecs.components.transform.Position3,
        [in]     flecs.components.transform.Rotation3,
        [filter] CameraController);

    ECS_SYSTEM(world, CameraControllerAccelerate, EcsOnUpdate,
        [in]     flecs.components.input.Input($),
        [in]     flecs.components.transform.Rotation3,
        [inout]  flecs.components.physics.Velocity3,
        [inout]  flecs.components.physics.AngularVelocity,
        [in]     CameraController);

    ECS_SYSTEM(world, CameraLimitY, EcsOnUpdate,
        [inout]  flecs.components.transform.Position3,
        [inout]  flecs.components.physics.Velocity3,
        [filter] CameraController);
}
