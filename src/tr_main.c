#include "tr_platform.h"

enum ENTITY_KIND
{
    Entity_Spheroid,
    Entity_Cuboid,
    Entity_Plane,
};

typedef struct Entity
{
    Enum8(ENTITY_KIND) kind;
    
    V3 pos;
    V4 rot;
    u32 color;
    
    union
    {
        struct
        {
            f32 radius;
        } spheroid;
        
        struct
        {
            V2 dim;
        } cuboid;
    };
} Entity;

typedef struct Camera
{
    u32* data;
    u32 width;
    u32 height;
    u32 target_width;
    u32 target_height;
    u32 fragment_size;
    V3 pos;
    V4 rot;
    f32 fov;
    u32 bounce_count;
    V3 sky_color;
} Camera;

typedef struct Hit_Data
{
    V3 point;
    V3 normal;
    V3 reflection;
    Entity* entity;
} Hit_Data;

internal Hit_Data
CastRay(Entity* entities, umm entity_count, Hit_Data* prev_hit, V3 origin, V3 dir)
{
    Hit_Data hit_data = {0};
    
    f32 closest_hit_dist_sq = F32_Inf();
    for (umm i = 0; i < entity_count; ++i)
    {
        V3 pos = V3_Sub(entities[i].pos, origin);
        
        switch (entities[i].kind)
        {
            case Entity_Spheroid:
            {
                // TODO: handle oblate spheroids
                f32 depth = V3_Inner(pos, dir);
                f32 par_t = Squared(depth) - V3_Inner(pos, pos) + Squared(entities[i].spheroid.radius);
                
                if (depth > 0 && par_t >= 0)
                {
                    f32 t = depth - Sqrt(par_t);
                    
                    V3 hit          = V3_Scale(dir, t);
                    f32 hit_dist_sq = V3_LengthSq(hit);
                    
                    if (hit_dist_sq < closest_hit_dist_sq && (prev_hit == 0 || prev_hit->entity != &entities[i]))
                    {
                        V3 normal = V3_Normalize(V3_Sub(hit, pos));
                        
                        hit_data.entity     = &entities[i];
                        hit_data.point      = hit;
                        hit_data.normal     = normal;
                        hit_data.reflection = V3_Normalize(V3_Add(dir, V3_Scale(normal, -V3_Inner(dir, normal))));
                        closest_hit_dist_sq = hit_dist_sq;
                    }
                }
            } break;
            
            case Entity_Cuboid:
            {
                NOT_IMPLEMENTED;
            } break;
            
            case Entity_Plane:
            {
                V3 normal = Vec3(0, 1, 0); // TODO: rotate normal
                
                f32 par_t = V3_Inner(dir, normal);
                f32 t     = V3_Inner(pos, normal) / par_t;
                
                if (par_t != 0 && t > 0)
                {
                    V3 hit          = V3_Scale(dir, t);
                    f32 hit_dist_sq = V3_LengthSq(hit);
                    
                    if (hit_dist_sq < closest_hit_dist_sq && (prev_hit == 0 || prev_hit->entity != &entities[i]))
                    {
                        hit_data.entity     = &entities[i];
                        hit_data.point      = hit;
                        hit_data.normal     = normal;
                        hit_data.reflection = V3_Normalize(V3_Add(dir, V3_Scale(normal, -V3_Inner(dir, normal))));
                        closest_hit_dist_sq = hit_dist_sq;
                    }
                }
            } break;
            
            INVALID_DEFAULT_CASE;
        }
    }
    
    return hit_data;
}

#define MAX_BOUNCE_COUNT 16
internal void
TraceScene(Camera camera, Entity* entities, umm entity_count)
{
    f32 near_plane_width  = 1;
    f32 near_plane_height = (f32)camera.height / camera.width;
    f32 near_plane_depth  = (near_plane_width / 2) / Tan(camera.fov / 2);
    
    V3 co = { -near_plane_width / 2, +near_plane_height / 2, +near_plane_depth };
    
    f32 cw = near_plane_width / camera.width;
    f32 ch = near_plane_height / camera.height;
    
    V3 sun_dir = V3_Normalize(Vec3(Sin(40), 1, Cos(40)));
    
    for (umm cy = 0; cy < camera.height; ++cy)
    {
        for (umm cx = 0; cx < camera.width; ++cx)
        {
            V3 cell = { (cx + 0.5f) * cw, -((cy + 0.5f) * ch), 0 };
            
            Hit_Data hit_data    = CastRay(entities, entity_count, 0, camera.pos, V3_Normalize(V3_Add(co, cell)));
            Hit_Data bounce_data = {0};
            
            if (hit_data.entity != 0) bounce_data = CastRay(entities, entity_count, &hit_data, hit_data.point, sun_dir);
            
            umm bounce_count = (hit_data.entity != 0) + (bounce_data.entity != 0);
            
#if 0
            u32 color = 0;
            if (bounce_count == 0) color = V3_ToRGBU32(camera.sky_color);
            else if (bounce_count == 1) color = 0xFF0000;
            else if (bounce_count == 2) color = 0xFF00FF;
            
            camera.data[cy * camera.width + cx] = color;
#endif
            
            V3 color = camera.sky_color;
            if (hit_data.entity != 0)
            {
                f32 l = V3_Inner(hit_data.normal, sun_dir);
                
                color = V3_FromRGBU32(hit_data.entity->color);
                
                color = V3_Scale(color, MIN(MAX(0.2f, l), 1));
                
                color = V3_Scale(color, 1.0f / bounce_count);
            }
            
            u32 fragment_width  = camera.target_width / camera.width;
            u32 fragment_height = camera.target_height / camera.height;
            for (umm j = 0; j < fragment_height; ++j)
            {
                for (umm i = 0; i < fragment_width; ++i)
                {
                    camera.data[(cy * fragment_height + j) * camera.target_width + (cx * fragment_width + i)] = V3_ToRGBU32(color);
                }
            }
        }
    }
}

global Camera MainCamera = {
    .data          = 0,
    .width         = 0,
    .height        = 0,
    .target_width  = 0,
    .target_height = 0,
    .pos           = { 0, 0, 0},
    .rot           = { 0, 0, 0, 1 },
    .fov           = HALF_PI32,
    .bounce_count  = 1,
    .sky_color     = {0.1f, 0.5f, 0.9f},
};

global Entity Entities[] = {
    {
        .kind  = Entity_Plane,
        .pos   = {0, -1.2f, 0},
        .color = 0x00212121,
    },
    
    {
        .kind  = Entity_Spheroid,
        .pos   = {0, 0, 5},
        .spheroid.radius = 1,
        .color = 0x00FF0000,
    },
};

void
Tick(Platform_Data* platform_data)
{
    Platform = platform_data;
    
    if (MainCamera.target_width != Platform->width ||
        MainCamera.target_height != Platform->height)
    {
        MainCamera.target_width  = Platform->width;
        MainCamera.target_height = Platform->height;
        MainCamera.fragment_size = 512;
    }
    
    if (MainCamera.width != MainCamera.target_width ||
        MainCamera.height != MainCamera.target_height)
    {
        MainCamera.width  = (u32)((f32)MainCamera.target_width  / MainCamera.fragment_size); 
        MainCamera.height = (u32)((f32)MainCamera.target_height / MainCamera.fragment_size); 
        MainCamera.fragment_size = MAX(MainCamera.fragment_size / 2, 1);
        
        MainCamera.data = Platform->image;
        TraceScene(MainCamera, Entities, ARRAY_SIZE(Entities));
        Platform->SwapBuffers();
    }
}