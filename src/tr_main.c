#include "tr_platform.h"

// Random number generator yoinked from https://iquilezles.org/www/articles/sfrand/sfrand.htm
float frand( int *seed )
{
    union
    {
        float fres;
        unsigned int ires;
    } f;
    
    seed[0] *= 16807;
    f.ires = ((((unsigned int)seed[0])>>9 ) | 0x3f800000);
    return f.fres - 1.0f;
}

V3
RandomPointInUnitSphere(int* seed)
{
	V3 p = {1, 1, 1};
	while (p.x*p.x + p.y*p.y + p.z*p.z> 1) p = (V3){ 2*frand(seed) - 1, 2*frand(seed) - 1, 2*frand(seed) - 1 };
    
	return p;
}

typedef enum ENTITY_KIND
{
    Entity_Sphere,
    Entity_Plane,
} ENTITY_KIND;

typedef struct Entity
{
    ENTITY_KIND kind;
    V3 pos;
    
    union
    {
        struct
        {
            f32 r;
        } sphere;
        
        struct
        {
            V3 normal;
        } plane;
    };
} Entity;

typedef struct Scene
{
    int seed;
    
    u32 skybox_width;
    u32 skybox_height;
    u32* skybox_data;
    
    u32 entity_count;
    Entity entities[];
} Scene;

typedef struct Ray_Hit
{
    Entity* entity;
    V3 point;
    V3 normal;
    bool backface;
} Ray_Hit;

Ray_Hit
Raycast(Scene* scene, V3 origin, V3 ray, f32 epsilon)
{
    Ray_Hit hit = {0};
    
    f32 min_dist_sq = F32_Inf();
    for (umm i = 0; i < scene->entity_count; ++i)
    {
        Entity* entity = &scene->entities[i];
        
        f32 t         = -1;
        V3 point      = {0};
        V3 normal     = {0};
        f32 dist_sq   = F32_Inf();
        bool backface = false;
        
        switch (entity->kind)
        {
            case Entity_Sphere:
            {
                V3 p_to_o        = V3_Sub(origin, entity->pos);
                f32 b            = V3_Inner(ray, p_to_o);
                f32 discriminant = Squared(b) - V3_Inner(p_to_o, p_to_o) + Squared(entity->sphere.r);
                
                if (discriminant <= 0) t = -1;
                else
                {
                    f32 t_p = -b + Sqrt(discriminant);
                    f32 t_m = -b - Sqrt(discriminant);
                    
                    if (t_m < epsilon)
                    {
                        t = t_p;
                        V3 scaled_ray = V3_Scale(ray, t);
                        point         = V3_Add(origin, scaled_ray);
                        dist_sq       = V3_LengthSq(scaled_ray);
                        normal        = V3_Sub(entity->pos, point);
                        backface      = true;
                    }
                    else
                    {
                        t = t_m;
                        V3 scaled_ray = V3_Scale(ray, t);
                        point         = V3_Add(origin, scaled_ray);
                        normal        = V3_Sub(point, entity->pos);
                        dist_sq       = V3_LengthSq(scaled_ray);
                        backface      = false;
                    }
                    
                    // HACK
                    backface = (backface ^ (entity->sphere.r < 0));
                }
            } break;
            
            case Entity_Plane:
            {
                f32 dividend = V3_Inner(V3_Sub(entity->pos, origin), entity->plane.normal);
                f32 divisor  = V3_Inner(ray, entity->plane.normal);
                
                if (divisor == 0) t = -1;
                else
                {
                    t = dividend / divisor;
                    V3 scaled_ray = V3_Scale(ray, t);
                    point         = V3_Add(origin, scaled_ray);
                    dist_sq       = V3_LengthSq(scaled_ray);
                    
                    if (divisor > 0)
                    {
                        normal   = V3_Neg(entity->plane.normal);
                        backface = true;
                    }
                    else
                    {
                        normal   = entity->plane.normal;
                        backface = false;
                    }
                }
            } break;
            
            INVALID_DEFAULT_CASE;
        }
        
        if (t > epsilon && dist_sq < min_dist_sq)
        {
            min_dist_sq  = dist_sq;
            hit.entity   = entity;
            hit.point    = point;
            hit.normal   = V3_Normalize(normal);
            hit.backface = backface;
        }
    }
    
    return hit;
}

bool
Lambertian(int* seed, f32 epsilon, V3 normal, V3* scattered_ray, V3* attenuation)
{
    *scattered_ray = V3_Normalize(V3_Add(normal, V3_Normalize(RandomPointInUnitSphere(seed))));
    if (Abs(scattered_ray->x) < epsilon && Abs(scattered_ray->y) < epsilon && Abs(scattered_ray->z) < epsilon) *scattered_ray = normal;
    *attenuation = (V3){ 0.5f, 0.5f, 0.5f };
    return true;
}

bool
Reflective(int* seed, f32 epsilon, V3 normal, V3 ray, f32 fuzz, V3* scattered_ray, V3* attenuation)
{
    // TODO: internal reflection?
    *scattered_ray = V3_Normalize(V3_Add(V3_Add(ray, V3_Scale(normal, -2*V3_Inner(ray, normal))),
                                         V3_Scale(V3_Normalize(RandomPointInUnitSphere(seed)), fuzz)));
    *attenuation = (V3){ 0.5f, 0.5f, 0.5f };
    return (V3_Inner(normal, *scattered_ray) > 0);
}

bool
Dielectric(int* seed, f32 epsilon, V3 normal, V3 ray, f32 fuzz, bool backface, V3* scattered_ray, V3* attenuation)
{
    f32 refraction_ratio = (backface ? 1.5f : 1 / 1.5f);
    
    f32 cos_theta = -V3_Inner(ray, normal);
    
    f32 schlick = Squared((1 - refraction_ratio) / (1 + refraction_ratio));
    schlick += (1 - schlick) * Squared(Squared(1 - cos_theta))*(1 - cos_theta);
    
    if (refraction_ratio * Sqrt(1 - Squared(cos_theta)) > 1 || schlick > frand(seed))
    {
        *scattered_ray = V3_Normalize(V3_Add(V3_Add(ray, V3_Scale(normal, -2*V3_Inner(ray, normal))),
                                             V3_Scale(V3_Normalize(RandomPointInUnitSphere(seed)), fuzz)));
    }
    else
    {
        V3 perpendicular = V3_Scale(V3_Add(ray, V3_Scale(normal, cos_theta)), refraction_ratio);
        V3 parallel      = V3_Scale(normal, -Sqrt(Abs(1 - V3_LengthSq(perpendicular))));
        
        *scattered_ray = V3_Normalize(V3_Add(perpendicular, parallel));
    }
    
    *attenuation = (V3){ 1, 1, 1 };
    
    return true;
}


V3
Raytrace(Scene* scene, V3 origin, V3 ray, imm depth, f32 epsilon)
{
    V3 color = {0};
    
    if (depth > 0)
    {
        Ray_Hit hit = Raycast(scene, origin, ray, epsilon);
        
        if (hit.entity == 0)
        {
#if 1
            f32 t = (ray.y + 1) / 2;
            color = V3_Add(V3_Scale((V3){ 1, 1, 1 }, 1 - t),  V3_Scale((V3){ 0.5f, 0.7f, 1.0f }, t));
#else
            V3 v = ray;
            
            V2 p     = {0};
            umm cell = 0;
            
            f32 min_dist_sq = 2*2;
            
            if (v.x != 0)
            {
                f32 t = 1 / v.x;
                
                f32 hit_dist_sq = V3_LengthSq(V3_Scale(v, t));
                
                if (hit_dist_sq < min_dist_sq)
                {
                    cell = (t > 0 ? 5 : 7);
                    p = (V2){-t*v.z, -ABS(t)*v.y};
                    
                    min_dist_sq = hit_dist_sq;
                    
                }
            }
            
            if (v.y != 0)
            {
                f32 t = 1 / v.y;
                
                f32 hit_dist_sq = V3_LengthSq(V3_Scale(v, t));
                
                if (hit_dist_sq < min_dist_sq)
                {
                    cell = (t > 0 ? 1 : 9);
                    p = (V2){-ABS(t)*v.z, t*v.x};
                    
                    min_dist_sq = hit_dist_sq;
                    
                }
            }
            
            if (v.z != 0)
            {
                f32 t = -1 / v.z;
                
                f32 hit_dist_sq = V3_LengthSq(V3_Scale(v, t));
                
                if (hit_dist_sq < min_dist_sq)
                {
                    cell = (t > 0 ? 6 : 4);
                    p = (V2){-t*v.x, -ABS(t)*v.y};
                    
                    min_dist_sq = hit_dist_sq;
                    
                }
            }
            
            p.x = (p.x + 1) / 2;
            p.y = (p.y + 1) / 2;
            
#if 1
            umm cell_size = scene->skybox_width / 4;
            
            umm cell_x = (cell % 4) * cell_size;
            umm cell_y = (cell / 4) * cell_size;
            
            umm x = MIN(MAX(0, (umm)(p.x*cell_size)), cell_size - 1);
            umm y = MIN(MAX(0, (umm)(p.y*cell_size)), cell_size - 1);
            
            color= V3_FromRGBU32(scene->skybox_data[(cell_y + y) * scene->skybox_width + (cell_x + x)]);
#else
            u32 colors[] = {
                0xFF00FF,
                0x0000FF, // 1
                0xFF00FF,
                0xFF00FF,
                0x00FF00, // 4
                0xFF0000, // 5
                0x00FFFF, // 6
                0xFFFF00, // 7
                0xFF00FF,
                0xFFFFFF, // 9
                0xFF00FF,
                0xFF00FF,
                0xFF00FF,
            };
            
            color = V3_Scale(V3_FromRGBU32(colors[cell]), p.x*p.y);
#endif
#endif
        }
        else
        {
            bool should_recurse;
            V3 scattered_ray;
            V3 attenuation;
            if (hit.entity->kind == Entity_Plane)
            {
                should_recurse = Reflective(&scene->seed, epsilon, hit.normal, ray, 0, &scattered_ray, &attenuation);
            }
            else should_recurse = Dielectric(&scene->seed, epsilon, hit.normal, ray, 0, hit.backface, &scattered_ray, &attenuation);
            if (should_recurse) color = V3_Hadamard(Raytrace(scene, hit.point, scattered_ray, depth - 1, epsilon), attenuation);
        }
    }
    
    return color;
}

void
Tick(Platform_Data* platform_data, Platform_Input input)
{
    Platform = platform_data;
    
    Scene* scene = 0;
    if (Platform->user_pointer == 0)
    {
        umm max_entity_count = 128;
        scene = Arena_PushSize(Platform->persistent_arena, sizeof(Scene) + sizeof(Entity) * max_entity_count, 8);
        
        scene->seed         = 0x69420;
#if 0
        scene->entity_count = 3;
        
        scene->entities[0] = (Entity){
            .kind     = Entity_Sphere,
            .pos      = { 0, -100.5f, -1 },
            .sphere.r = 100,
        };
        
        scene->entities[1] = (Entity){
            .kind     = Entity_Sphere,
            .pos      = { -1, 0, -1 },
            .sphere.r = 0.5f,
        };
        
        scene->entities[2] = (Entity){
            .kind     = Entity_Sphere,
            .pos      = { -1, 0, -1 },
            .sphere.r = -0.4f,
        };
#else
        scene->entity_count = 15;
        
        scene->entities[0] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {0, 0, -5},
            .sphere.r = 1,
        };
        
        scene->entities[1] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {2.5f, 0, -7},
            .sphere.r = 1,
        };
        
        scene->entities[2] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {-2.5f, 0, -7},
            .sphere.r = 1,
        };
        
        scene->entities[3] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {-1, 4, -8},
            .sphere.r = 1.5f,
        };
        
        scene->entities[4] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {-10, 3, -20},
            .sphere.r = 2.4f,
        };
        
        scene->entities[5] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {6, 4, -4},
            .sphere.r = 5,
        };
        
        
        
        scene->entities[6] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {0, 0, -5},
            .sphere.r = -0.4f,
        };
        
        scene->entities[7] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {2.5f, 0, -7},
            .sphere.r = -0.9f,
        };
        
        scene->entities[8] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {-2.5f, 0, -7},
            .sphere.r = -0.8f,
        };
        
        scene->entities[9] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {-1, 4, -8},
            .sphere.r = -1.1f,
        };
        
        scene->entities[10] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {-10, 3, -20},
            .sphere.r = -2.0f,
        };
        
        scene->entities[11] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {6, 4, -4},
            .sphere.r = -4.5f,
        };
        
        scene->entities[12] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {6, 4, -4},
            .sphere.r = 4,
        };
        
        scene->entities[13] = (Entity){
            .kind  = Entity_Sphere,
            .pos   = {6, 4, -4},
            .sphere.r = -1,
        };
        
        scene->entities[14] = (Entity){
            .kind  = Entity_Plane,
            .pos   = {0, -1.5f, 0},
            .plane.normal = {0,1,0},
        };
#endif
        String contents;
        Targa_Header header;
        if (Platform->ReadEntireFile(Platform->persistent_arena, STRING("skybox.tga"), &contents) &&
            Targa_ReadHeader(contents, &header) &&
            header.image_format == TargaImageFormat_UncompressedTrueColor)
        {
            scene->skybox_width  = header.width;
            scene->skybox_height = header.height;
            scene->skybox_data   = (u32*)header.data;
        }
        
        Platform->user_pointer = scene;
    }
    
    scene = (Scene*)Platform->user_pointer;
    
    V3 camera_pos  = { 0, 0, 0 };
    
    f32 aspect_ratio = 16.0f / 10;
    f32 focal_length = 1;
    V2 screen_dim    = { 2*aspect_ratio, 2 };
    u32 max_depth    = 50;
    umm ray_count    = 1;
    f32 epsilon      = 1.0e-4f;
    
    V3 screen_origin = { -screen_dim.x / 2, +screen_dim.y / 2, -focal_length };
    V3 screen_step   = { screen_dim.x / Platform->width, -screen_dim.y / Platform->height, 0 };
    
    for (umm iy = 0; iy < Platform->height; ++iy)
    {
        for (umm ix = 0; ix < Platform->width; ++ix)
        {
            V3 color = {0};
            
            for (umm i = 0; i < ray_count; ++i)
            {
                V3 screen_point = V3_Add(screen_origin, V3_Hadamard(screen_step, (V3){ix+frand(&scene->seed), iy+frand(&scene->seed), 0}));
                V3 ray = V3_Normalize(screen_point);
                
                color = V3_Add(color, Raytrace(scene, camera_pos, ray, max_depth, epsilon));
            }
            
            // NOTE: Gamma correction for gamma = 2
            color.x = Sqrt(color.x / ray_count);
            color.y = Sqrt(color.y / ray_count);
            color.z = Sqrt(color.z / ray_count);
            
            Platform->image[iy * Platform->width + ix] = V3_ToRGBU32(color);
        }
    }
    
    Platform->SwapBuffers();
}
