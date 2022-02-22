#include "tr_platform.h"

static f32 time = 0;

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
} Camera;

typedef struct Scene
{
    Entity* entities;
    u32 entity_count;
    V3 sun_dir;
    f32 ambient;
    u32* skybox_data;
    u32 skybox_width;
    u32 skybox_height;
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
                    
                    V3 t_dir = V3_Scale(dir, t);
                    hit         = V3_Add(origin, t_dir);
                    hit_dist_sq = V3_LengthSq(hit);
                    normal      = V3_Normalize(V3_Sub(t_dir, pos));
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
                    hit         = V3_Add(origin, V3_Scale(dir, t));
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
ComputeLight(Scene* scene, Hit_Data* ignored_hit, V3 pos, V3 ray, V3 energy)
{
    V3 light = {0};
    
    Hit_Data hit_data = CastRay(scene, ignored_hit, pos, ray);
    
    if (hit_data.entity == 0)
    {
        f32 speed = 1;
        f32 a = time * speed;
        f32 cos_a = Cos(a);
        f32 sin_a = Sin(a);
        V3 v = {
            .x = ray.x * cos_a - ray.z * sin_a,
            .y = ray.y,
            .z = ray.x * sin_a + ray.z * cos_a,
        };
        
        V2 p     = {0};
        umm cell = 0;
        
        f32 min_dist_sq = 2*2;
        f32 normals[]   = {1, 0, 0};
        umm cells[]     = {
            5, 1, 6,
            7, 9, 4
        };
        
        umm xs[]  = {2, 2, 0};
        umm ys[]  = {1, 0, 1};
        imm sgn[] = {1, -1, 1};
        
        for (umm i = 0; i < 3; ++i)
        {
            V3 normal = Vec3(-normals[i], -normals[(i + 2) % 3], -normals[(i + 1) % 3]);
            
            f32 par_t = V3_Inner(v, normal);
            f32 t     = V3_Inner(Vec3(normals[i], normals[(i + 2) % 3], -normals[(i + 1) % 3]), normal) / par_t;
            
            if (par_t != 0)
            {
                V3 hit          = V3_Scale(v, t);
                f32 hit_dist_sq = V3_LengthSq(hit);
                
                if (hit_dist_sq < min_dist_sq)
                {
                    cell = cells[i + (t > 0 ? 0 : 3)];
                    p = (V2){-t*(cell == 9 ? -1 : 1)*v.e[xs[i]], -sgn[i]*ABS(t)*v.e[ys[i]]};
                    
                    min_dist_sq = hit_dist_sq;
                    
                }
            }
        }
        
        p.x = (p.x + 1) / 2;
        p.y = (p.y + 1) / 2;
        
#if 1
        umm cell_size = scene->skybox_width / 4;
        
        umm x = (umm)(((cell % 4) + p.x) * cell_size);
        umm y = (umm)(((cell / 4) + p.y) * cell_size);
        light = V3_FromRGBU32(scene->skybox_data[y * scene->skybox_width + x]);
#else
        u32 colors[] = {
            0x000000,
            0x0000FF, // 1
            0x000000,
            0x000000,
            0x00FF00, // 4
            0xFF0000, // 5
            0x00FFFF, // 6
            0xFFFF00, // 7
            0x000000,
            0xFF00FF, // 9
            0x000000,
            0x000000,
            0x000000,
        };
        
        //light = V3_FromRGBU32(colors[cell]);
        light = V3_Scale(V3_FromRGBU32(colors[cell]), p.x*p.y);
#endif
        
    }
    else
    {
        /*Hit_Data shadow_data = CastRay(scene, &hit_data, hit_data.point, scene->sun_dir);
        
        f32 sun_light = 0;
        if (shadow_data.entity == 0)
        {
            sun_light = V3_Inner(hit_data.normal, scene->sun_dir);
        }
        
        sun_light = MAX(scene->ambient, sun_light);
        
        light = V3_Scale(V3_FromRGBU32(hit_data.entity->color), sun_light);*/
        V3 reflected_ray = V3_Normalize(V3_Add(ray, V3_Scale(hit_data.normal, -2*V3_Inner(hit_data.normal, ray))));
        light = ComputeLight(scene, &hit_data, hit_data.point, reflected_ray, V3_Hadamard(energy, Vec3(0.7f, 0.7f, 0.7f)));
    }
    
    return V3_Hadamard(light, energy);
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
            
            V3 light = ComputeLight(scene, 0, camera->pos, V3_Normalize(ray_dir), (V3){1,1,1});
            
            V3 color = light;
            
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
};

global Entity Entities[] = {
    /*{
        .kind  = Entity_Plane,
        .pos   = {0, -1.2f, 0},
        .color = 0x00212121,
    },*/
    
    {
        .kind  = Entity_Spheroid,
        .pos   = {0, 0, 5},
        .spheroid.radius = 1,
        .color = 0x00FF0000,
    },
    
    {
        .kind  = Entity_Spheroid,
        .pos   = {2.5f, 0, 7},
        .spheroid.radius = 1,
        .color = 0x00FF0000,
    },
    
    {
        .kind  = Entity_Spheroid,
        .pos   = {-2.5f, 0, 7},
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
    .ambient       = 0.2f,
};

void
Tick(Platform_Data* platform_data, Platform_Input input)
{
    Platform = platform_data;
    
    if (MainScene.skybox_data == 0)
    {
        MainScene.sun_dir = V3_Normalize(Vec3(Sin(40), 1, Cos(40)));
        
        String contents;
        Targa_Header header;
        if (Platform->ReadEntireFile(Platform->persistent_arena, STRING("skybox.tga"), &contents) &&
            Targa_ReadHeader(contents, &header) &&
            header.image_format == TargaImageFormat_UncompressedTrueColor)
        {
            MainScene.skybox_width  = header.width;
            MainScene.skybox_height = header.height;
            MainScene.skybox_data   = (u32*)header.data;
        }
        
        else Platform->Print("Failed to load skybox\n");
    }
    
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
        MainCamera.pos.xy = V2_Add(MainCamera.pos.xy, V2_Scale(input.dir, input.dt));
        
        MainCamera.fragment_size = 64;
        should_rerender = true;
    }
    
    time += input.dt;
    MainCamera.fragment_size = 4;
    should_rerender = true;
    
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
