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
} Camera;

typedef struct Scene
{
    Entity* entities;
    u32 entity_count;
    V3 sky_color;
    V3 sun_color;
    V3 sun_dir;
} Scene;

typedef struct Hit_Data
{
    V3 point;
    V3 normal;
    Entity* entity;
} Hit_Data;

internal Hit_Data
CastRay(Scene* scene, Hit_Data* ignored_hit, V3 origin, V3 dir)
{
    Hit_Data hit_data = {0};
    
    f32 closest_hit_dist_sq = F32_Inf();
    for (umm i = 0; i < scene->entity_count; ++i)
    {
        V3 pos          = V3_Sub(scene->entities[i].pos, origin);
        V3 hit          = {0};
        V3 normal       = {0};
        f32 hit_dist_sq = F32_Inf();
        
        switch (scene->entities[i].kind)
        {
            case Entity_Spheroid:
            {
                // TODO: handle oblate spheroids
                f32 depth = V3_Inner(pos, dir);
                f32 par_t = Squared(depth) - V3_Inner(pos, pos) + Squared(scene->entities[i].spheroid.radius);
                
                if (depth > 0 && par_t >= 0)
                {
                    f32 t = depth - Sqrt(par_t);
                    
                    hit         = V3_Scale(dir, t);
                    hit_dist_sq = V3_LengthSq(hit);
                    normal      = V3_Normalize(V3_Sub(hit, pos));
                }
            } break;
            
            case Entity_Cuboid:
            {
                NOT_IMPLEMENTED;
                
            } break;
            
            case Entity_Plane:
            {
                normal = Vec3(0, 1, 0); // TODO: rotate normal
                
                f32 par_t = V3_Inner(dir, normal);
                f32 t     = V3_Inner(pos, normal) / par_t;
                
                if (par_t != 0 && t > 0)
                {
                    hit         = V3_Scale(dir, t);
                    hit_dist_sq = V3_LengthSq(hit);
                }
            } break;
            
            INVALID_DEFAULT_CASE;
        }
        
        if (hit_dist_sq < closest_hit_dist_sq && (ignored_hit == 0 || &scene->entities[i] != ignored_hit->entity))
        {
            hit_data.entity     = &scene->entities[i];
            hit_data.point      = hit;
            hit_data.normal     = normal;
            closest_hit_dist_sq = hit_dist_sq;
        }
    }
    
    return hit_data;
}

internal V3
ComputeLight(Scene* scene, Hit_Data* ignored_hit, V3 pos, V3 ray, imm ttl)
{
    V3 light = {0};
    
    if (ttl-- > 0)
    {
        Hit_Data hit_data = CastRay(scene, ignored_hit, pos, ray);
        
        if (hit_data.entity == 0)
        {
            
        }
        
        else
        {
            Hit_Data sun_data = CastRay(scene, &hit_data, hit_data.point, scene->sun_dir);
            
            f32 sun_light = (sun_data.entity == 0 ? V3_Inner(hit_data.normal, scene->sun_dir) : 0);
            sun_light = MAX(sun_light, 0);
            
            V3 albedo = V3_FromRGBU32(hit_data.entity->color);
            
            f32 light_strength = 0.2f + sun_light;
            light = V3_Scale(albedo, MIN(MAX(light_strength, 0.2f), 1));
        }
    }
    
    return light;
}

internal void
TraceScanline(Camera* camera, Scene* scene, V3 cell_origin, V2 cell_dim, umm start, umm end)
{
    u32 fragment_width  = camera->target_width / camera->width;
    u32 fragment_height = camera->target_height / camera->height;
    
    for (umm cy = start; cy < end; ++cy)
    {
        for (umm cx = 0; cx < camera->width; ++cx)
        {
            V3 ray_dir = {
                cell_origin.x + (cx + 0.5f) * cell_dim.x,
                cell_origin.y - ((cy + 0.5f) * cell_dim.y), cell_origin.z
            };
            
            V3 offsets[4] = {
                { Sin((f32)cx)    * cell_dim.x / 2, Cos((f32)cy*cy) * cell_dim.y / 2, 0 },
                { Sin((f32)cy)    * cell_dim.x / 2, Cos((f32)cx*cx) * cell_dim.y / 2, 0 },
                { Sin((f32)cx*cx) * cell_dim.x / 2, Cos((f32)cy)    * cell_dim.y / 2, 0 },
                { Sin((f32)cy*cy) * cell_dim.x / 2, Cos((f32)cx)    * cell_dim.y / 2, 0 },
            };
            
            V3 color = {0};
            
            for (umm i = 0; i < 4; ++i)
            {
                color = V3_Add(color, ComputeLight(scene, 0, camera->pos, V3_Normalize(V3_Add(ray_dir, offsets[i])), camera->bounce_count));
            }
            
            color = V3_Scale(color, 0.25f);
            
            u32 rgb = V3_ToRGBU32(color);
            for (umm j = 0; j < fragment_height; ++j)
            {
                for (umm i = 0; i < fragment_width; ++i)
                {
                    camera->data[(cy * fragment_height + j) * camera->target_width + (cx * fragment_width + i)] = rgb;
                }
            }
        }
    }
}

internal void
TraceScene(Camera* camera, Scene* scene)
{
    f32 near_plane_width  = 1;
    f32 near_plane_height = (f32)camera->height / camera->width;
    f32 near_plane_depth  = (near_plane_width / 2) / Tan(camera->fov / 2);
    
    V3 co = { -near_plane_width / 2, +near_plane_height / 2, +near_plane_depth };
    V2 cd = { near_plane_width  / camera->width, near_plane_height / camera->height };
    
    umm scanline_size = 50;
    imm scanlines     = camera->height / scanline_size;
    
    for (imm scanline = 0; scanline < scanlines - 1; ++scanline)
    {
        TraceScanline(camera, scene, co, cd, scanline * scanline_size, (scanline + 1) * scanline_size);
    }
    
    TraceScanline(camera, scene, co, cd, MAX(scanlines - 1, 0) * scanline_size, camera->height);
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
    .bounce_count  = 10,
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
    
    {
        .kind  = Entity_Spheroid,
        .pos   = {-1, 4, 8},
        .spheroid.radius = 1.5f,
        .color = 0x00128812,
    },
    
    {
        .kind  = Entity_Spheroid,
        .pos   = {-10, 3, 20},
        .spheroid.radius = 2.4f,
        .color = 0x00331266,
    },
    
    {
        .kind  = Entity_Spheroid,
        .pos   = {15, 6, 40},
        .spheroid.radius = 2.5f,
        .color = 0x00008866,
    },
};

global Scene MainScene = {
    .entities      = Entities,
    .entity_count  = ARRAY_SIZE(Entities),
    .sky_color     = {0.1f, 0.5f, 0.9f},
    .sun_color     = {1, 1, 1},
};

void
Tick(Platform_Data* platform_data, Platform_Input input)
{
    Platform = platform_data;
    
    MainScene.sun_dir = V3_Normalize(Vec3(Sin(40), 1, Cos(40)));
    
    bool should_rerender = false;
    
    if (MainCamera.target_width != Platform->width ||
        MainCamera.target_height != Platform->height)
    {
        MainCamera.target_width  = Platform->width;
        MainCamera.target_height = Platform->height;
        MainCamera.fragment_size = 64;
        
        should_rerender = true;
    }
    
    else if (MainCamera.width != MainCamera.target_width ||
             MainCamera.height != MainCamera.target_height)
    {
        MainCamera.fragment_size = MAX(MainCamera.fragment_size / 2, 1);
        
        should_rerender = true;
    }
    
    if (input.dir.x != 0 || input.dir.y != 0)
    {
        MainCamera.pos.xy = V2_Add(MainCamera.pos.xy, V2_Scale(input.dir, Platform->dt));
        
        MainCamera.fragment_size = 64;
        should_rerender = true;
    }
    
    if (should_rerender)
    {
        MainCamera.width  = (u32)((f32)MainCamera.target_width  / MainCamera.fragment_size); 
        MainCamera.height = (u32)((f32)MainCamera.target_height / MainCamera.fragment_size); 
        MainCamera.width  = MAX(MainCamera.width,  1);
        MainCamera.height = MAX(MainCamera.height, 1);
        
        MainCamera.data = Platform->image;
        
        TraceScene(&MainCamera, &MainScene);
        Platform->SwapBuffers();
    }
}