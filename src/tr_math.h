#define PI32 3.1415926535f
#define TAU32 6.2831853071f
#define HALF_PI32 1.5707963267f
#define E32 2.7182818284f

typedef union F32_Bits
{
    f32 flt;
    u32 bits;
} F32_Bits;

typedef union F64_Bits
{
    f64 flt;
    u64 bits;
} F64_Bits;

internal inline f32
F32_Inf()
{
    return (F32_Bits){ .bits = 0x7F800000 }.flt;
}

internal inline f32
Squared(f32 n)
{
    return n * n;
}

internal inline f32
Cubed(f32 n)
{
    return n * n * n;
}

internal inline f32
Sqrt(f32 n)
{
    // TODO: replace with intrinsic
    
    f32 acc = n / 2;
    
    for (umm i = 0; i < 5; ++i)
    {
        acc -= (Squared(acc) - n) / (2*acc);
    }
    
    return acc;
}

internal inline f32
Sgn(f32 x)
{
    return (F32_Bits){ .bits = (F32_Bits){ .flt = x }.bits & (1 << 31) }.flt;
}

internal inline bool
IsNegative(f32 x)
{
    return !!((F32_Bits){ .flt = x }.bits & (1 << 31));
}

internal inline f32
Abs(f32 x)
{
    // TODO: replace with intrinsic
    return (F32_Bits){ .bits = (F32_Bits){ .flt = x }.bits & ~((u32)1 << 31) }.flt;
}

internal inline f32
Sin(f32 x)
{
    // TODO: replace with intrinsic
    
    imm q = (imm)(x / HALF_PI32) - IsNegative(x);
    imm b = (imm)(x / PI32)      - IsNegative(x);
    f32 r = x - q*HALF_PI32;
    
    f32 h = Abs((q & 1)*HALF_PI32 - r);
    
    f32 f0 = h*h*h/6;
    f32 f1 = f0*h*h/20;
    f32 f2 = f1*h*h/42;
    
    return (1 - ((b & 1) << 1))*(h - f0 + f1 - f2);
}

internal inline f32
Cos(f32 x)
{
    // TODO: replace with intrinsic
    
    return Sin(x + HALF_PI32);
}

internal inline f32
Tan(f32 x)
{
    // TODO: replace with intrinsic
    
    return Sin(x) / Cos(x);
}

typedef union  V2
{
    struct { f32 x, y; };
    struct { f32 u, v; };
} V2;

typedef union  V3
{
    struct { f32 x, y, z; };
    struct { f32 r, g, b; };
    
    struct { V2 xy; f32 _0; };
    struct { f32 _1; V2 yz; };
    
    struct { V2 rg; f32 _2; };
    struct { f32 _3; V2 gb; };
} V3;

typedef union  V4
{
    struct { f32 x, y, z, w; };
    struct { f32 r, g, b, a; };
    
    struct { V2 xy; f32 _0; f32 _1; };
    struct { f32 _2; V2 yz; f32 _3; };
    struct { f32 _4; f32 _5; V2 zw; };
    struct { V3 xyz; f32 _6; };
    struct { f32 _7; V2 yzw; };
    
    struct { V2 rg;   f32 _8;  f32 _9;  };
    struct { f32 _10; V2 gb;   f32 _11; };
    struct { f32 _12; f32 _13; V2 ba;   };
    struct { V3 rgb;  f32 _14; };
    struct { f32 _15; V2 gba;  };
} V4;

typedef union M2
{
    struct { V2 i, j; };
    f32 e[4];
} M2;

typedef union M3
{
    struct { V3 i, j, k; };
    f32 e[9];
} M3;

typedef union M4
{
    struct { V4 i, j, k, w; };
    f32 e[16];
} M4;

internal V2
Vec2(f32 x, f32 y)
{
    return (V2){x, y};
}

internal V3
Vec3(f32 x, f32 y, f32 z)
{
    return (V3){x, y, z};
}

internal V4
Vec4(f32 x, f32 y, f32 z, f32 w)
{
    return (V4){x, y, z, w};
}

internal V2
V2_Add(V2 v0, V2 v1)
{
    return (V2){v0.x + v1.x, v0.y + v1.y};
}

internal V3
V3_Add(V3 v0, V3 v1)
{
    return (V3){v0.x + v1.x, v0.y + v1.y, v0.z + v1.z};
}

internal V4
V4_Add(V4 v0, V4 v1)
{
    return (V4){v0.x + v1.x, v0.y + v1.y, v0.z + v1.z, v0.w + v1.w};
}

internal V2
V2_Sub(V2 v0, V2 v1)
{
    return (V2){v0.x - v1.x, v0.y - v1.y};
}

internal V3
V3_Sub(V3 v0, V3 v1)
{
    return (V3){v0.x - v1.x, v0.y - v1.y, v0.z - v1.z};
}

internal V4
V4_Sub(V4 v0, V4 v1)
{
    return (V4){v0.x - v1.x, v0.y - v1.y, v0.z - v1.z, v0.w - v1.w};
}

internal V2
V2_Neg(V2 v)
{
    return (V2){-v.x, -v.y};
}

internal V3
V3_Neg(V3 v)
{
    return (V3){-v.x, -v.y, -v.z};
}

internal V4
V4_Neg(V4 v)
{
    return (V4){-v.x, -v.y, -v.z, -v.w};
}

internal V4
V4_Conjugate(V4 v)
{
    return (V4){v.x, -v.y, -v.z, -v.w};
}

internal V2
V2_Scale(V2 v, f32 n)
{
    return (V2){v.x * n, v.y * n};
}

internal V3
V3_Scale(V3 v, f32 n)
{
    return (V3){v.x * n, v.y * n, v.z * n};
}

internal V4
V4_Scale(V4 v, f32 n)
{
    return (V4){v.x * n, v.y * n, v.z * n, v.w * n};
}

internal f32
V2_LengthSq(V2 v)
{
    return Squared(v.x) + Squared(v.y);
}

internal f32
V3_LengthSq(V3 v)
{
    return Squared(v.x) + Squared(v.y) + Squared(v.z);
}

internal f32
V4_LengthSq(V4 v)
{
    return Squared(v.x) + Squared(v.y) + Squared(v.z) + Squared(v.w);
}

internal f32
V2_Length(V2 v)
{
    return Sqrt(V2_LengthSq(v));
}

internal f32
V3_Length(V3 v)
{
    return Sqrt(V3_LengthSq(v));
}

internal f32
V4_Length(V4 v)
{
    return Sqrt(V4_LengthSq(v));
}

internal V2
V2_Normalize(V2 v)
{
    return V2_Scale(v, 1 / V2_Length(v));
}

internal V3
V3_Normalize(V3 v)
{
    return V3_Scale(v, 1 / V3_Length(v));
}

internal V4
V4_Normalize(V4 v)
{
    return V4_Scale(v, 1 / V4_Length(v));
}

internal V2
V2_Hadamard(V2 v0, V2 v1)
{
    return (V2){v0.x * v1.x, v0.y * v1.y};
}

internal V3
V3_Hadamard(V3 v0, V3 v1)
{
    return (V3){v0.x * v1.x, v0.y * v1.y, v0.z * v1.z};
}

internal V4
V4_Hadamard(V4 v0, V4 v1)
{
    return (V4){v0.x * v1.x, v0.y * v1.y, v0.z * v1.z, v0.w * v1.w};
}

internal f32
V2_Inner(V2 v0, V2 v1)
{
    return v0.x * v1.x + v0.y * v1.y;
}

internal f32
V3_Inner(V3 v0, V3 v1)
{
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}

internal f32
V4_Inner(V4 v0, V4 v1)
{
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
}

internal M2
V2_Outer(V2 v0, V2 v1)
{
    return (M2){.i = V2_Scale(v0, v1.x), .j = V2_Scale(v0, v1.y)};
}

internal M3
V3_Outer(V3 v0, V3 v1)
{
    return (M3){.i = V3_Scale(v0, v1.x), .j = V3_Scale(v0, v1.y), .k = V3_Scale(v0, v1.z)};
}

internal M4
V4_Outer(V4 v0, V4 v1)
{
    return (M4){.i = V4_Scale(v0, v1.x), .j = V4_Scale(v0, v1.y), .k = V4_Scale(v0, v1.z), .w = V4_Scale(v0, v1.w)};
}

internal inline V3
V3_FromRGBU32(u32 rgb)
{
    return (V3){
        (f32)((rgb & 0xFF0000) >> 16) / 255,
        (f32)((rgb & 0x00FF00) >>  8) / 255,
        (f32)((rgb & 0x0000FF) >>  0) / 255,
    };
}

internal inline u32
V3_ToRGBU32(V3 color)
{
    return (u32)(color.r*255) << 16 | (u32)(color.g*255) << 8 | (u32)(color.b*255);
}

internal M2
Mat2(V2 i, V2 j)
{
    return (M2){.i = i, .j = j};
}

internal M3
Mat3(V3 i, V3 j, V3 k)
{
    return (M3){.i = i, .j = j, .k = k};
}

internal M4
Mat4(V4 i, V4 j, V4 k, V4 w)
{
    return (M4){.i = i, .j = j, .k = k, .w = w};
}

internal M2
M2_Add(M2 m0, M2 m1)
{
    return (M2){.i = V2_Add(m0.i, m1.i), .j = V2_Add(m0.j, m1.j)};
}

internal M3
M3_Add(M3 m0, M3 m1)
{
    return (M3){.i = V3_Add(m0.i, m1.i), .j = V3_Add(m0.j, m1.j), .k = V3_Add(m0.k, m1.k)};
}

internal M4
M4_Add(M4 m0, M4 m1)
{
    return (M4){.i = V4_Add(m0.i, m1.i), .j = V4_Add(m0.j, m1.j), .k = V4_Add(m0.k, m1.k), .w = V4_Add(m0.w, m1.w)};
}

internal M2
M2_Sub(M2 m0, M2 m1)
{
    return (M2){.i = V2_Sub(m0.i, m1.i), .j = V2_Sub(m0.j, m1.j)};
}

internal M3
M3_Sub(M3 m0, M3 m1)
{
    return (M3){.i = V3_Sub(m0.i, m1.i), .j = V3_Sub(m0.j, m1.j), .k = V3_Sub(m0.k, m1.k)};
}

internal M4
M4_Sub(M4 m0, M4 m1)
{
    return (M4){.i = V4_Sub(m0.i, m1.i), .j = V4_Sub(m0.j, m1.j), .k = V4_Sub(m0.k, m1.k), .w = V4_Sub(m0.w, m1.w)};
}

internal M2
M2_Neg(M2 m)
{
    return (M2){.i = V2_Neg(m.i), .j = V2_Neg(m.j)};
}

internal M3
M3_Neg(M3 m)
{
    return (M3){.i = V3_Neg(m.i), .j = V3_Neg(m.j), .k = V3_Neg(m.k)};
}

internal M4
M4_Neg(M4 m)
{
    return (M4){.i = V4_Neg(m.i), .j = V4_Neg(m.j), .k = V4_Neg(m.k), .w = V4_Neg(m.w)};
}

internal M2
M2_Scale(M2 m, f32 n)
{
    return (M2){.i = V2_Scale(m.i, n), .j = V2_Scale(m.j, n)};
}

internal M3
M3_Scale(M3 m, f32 n)
{
    return (M3){.i = V3_Scale(m.i, n), .j = V3_Scale(m.j, n), .k = V3_Scale(m.k, n)};
}

internal M4
M4_Scale(M4 m, f32 n)
{
    return (M4){.i = V4_Scale(m.i, n), .j = V4_Scale(m.j, n), .k = V4_Scale(m.k, n), .w = V4_Scale(m.w, n)};
}

internal V2
M2_Row(M2 m, umm r)
{
    return (V2){m.e[r], m.e[r + 2]};
}

internal V3
M3_Row(M3 m, umm r)
{
    return (V3){m.e[r], m.e[r + 3], m.e[r + 6]};
}

internal V4
M4_Row(M4 m, umm r)
{
    return (V4){m.e[r], m.e[r + 4], m.e[r + 8], m.e[r + 12]};
}

internal M2
M2_Transpose(M2 m)
{
    V2 r0 = M2_Row(m, 0);
    V2 r1 = M2_Row(m, 1);
    
    return (M2){.i = r0, .j = r1};
}

internal M3
M3_Transpose(M3 m)
{
    V3 r0 = M3_Row(m, 0);
    V3 r1 = M3_Row(m, 1);
    V3 r2 = M3_Row(m, 2);
    
    return (M3){.i = r0, .j = r1, .k = r2};
}

internal M4
M4_Transpose(M4 m)
{
    V4 r0 = M4_Row(m, 0);
    V4 r1 = M4_Row(m, 1);
    V4 r2 = M4_Row(m, 2);
    V4 r3 = M4_Row(m, 3);
    
    return (M4){.i = r0, .j = r1, .k = r2, .w = r3};
}

internal M2
M2_Mul(M2 m0, M2 m1)
{
    V2 r0 = M2_Row(m0, 0);
    V2 r1 = M2_Row(m0, 1);
    
    M2 result;
    result.i = (V2){V2_Inner(r0, m1.i), V2_Inner(r1, m1.i)};
    result.j = (V2){V2_Inner(r0, m1.j), V2_Inner(r1, m1.j)};
    
    return result;
}

internal M3
M3_Mul(M3 m0, M3 m1)
{
    V3 r0 = M3_Row(m0, 0);
    V3 r1 = M3_Row(m0, 1);
    V3 r2 = M3_Row(m0, 2);
    
    M3 result;
    result.i = (V3){V3_Inner(r0, m1.i), V3_Inner(r1, m1.i), V3_Inner(r2, m1.i)};
    result.j = (V3){V3_Inner(r0, m1.j), V3_Inner(r1, m1.j), V3_Inner(r2, m1.j)};
    result.k = (V3){V3_Inner(r0, m1.k), V3_Inner(r1, m1.k), V3_Inner(r2, m1.k)};
    
    return result;
}

internal M4
M4_Mul(M4 m0, M4 m1)
{
    V4 r0 = M4_Row(m0, 0);
    V4 r1 = M4_Row(m0, 1);
    V4 r2 = M4_Row(m0, 2);
    V4 r3 = M4_Row(m0, 3);
    
    M4 result;
    result.i = (V4){V4_Inner(r0, m1.i), V4_Inner(r1, m1.i), V4_Inner(r2, m1.i), V4_Inner(r3, m1.i)};
    result.j = (V4){V4_Inner(r0, m1.j), V4_Inner(r1, m1.j), V4_Inner(r2, m1.j), V4_Inner(r3, m1.j)};
    result.k = (V4){V4_Inner(r0, m1.k), V4_Inner(r1, m1.k), V4_Inner(r2, m1.k), V4_Inner(r3, m1.k)};
    result.w = (V4){V4_Inner(r0, m1.w), V4_Inner(r1, m1.w), V4_Inner(r2, m1.w), V4_Inner(r3, m1.w)};
    
    return result;
}

internal M2
M2_Hadamard(M2 m0, M2 m1)
{
    return (M2){
        .i = V2_Hadamard(m0.i, m1.i),
        .j = V2_Hadamard(m0.j, m1.j)};
}

internal M3
M3_Hadamard(M3 m0, M3 m1)
{
    return (M3){
        .i = V3_Hadamard(m0.i, m1.i),
        .j = V3_Hadamard(m0.j, m1.j),
        .k = V3_Hadamard(m0.k, m1.k)
    };
}

internal M4
M4_Hadamard(M4 m0, M4 m1)
{
    return (M4){
        .i = V4_Hadamard(m0.i, m1.i),
        .j = V4_Hadamard(m0.j, m1.j),
        .k = V4_Hadamard(m0.k, m1.k),
        .w = V4_Hadamard(m0.w, m1.w)
    };
}

internal V2
M2_Transform(M2 m, V2 v)
{
    return (V2){
        V2_Inner(M2_Row(m, 0), v),
        V2_Inner(M2_Row(m, 1), v)
    };
}

internal V3
M3_Transform(M3 m, V3 v)
{
    return (V3){
        V3_Inner(M3_Row(m, 0), v),
        V3_Inner(M3_Row(m, 1), v),
        V3_Inner(M3_Row(m, 2), v)
    };
}

internal V4
M4_Transform(M4 m, V4 v)
{
    return (V4){
        V4_Inner(M4_Row(m, 0), v),
        V4_Inner(M4_Row(m, 1), v),
        V4_Inner(M4_Row(m, 2), v),
        V4_Inner(M4_Row(m, 3), v)
    };
}

typedef struct Rect
{
    V2 min;
    V2 max;
} Rect;

internal inline Rect
Rect_FromMinMax(V2 min, V2 max)
{
    return (Rect){min, max};
}

internal inline Rect
Rect_FromPosSize(V2 pos, V2 size)
{
    return (Rect){pos, V2_Add(pos, size)};
}

internal inline Rect
Rect_FromPosScale(V2 pos, V2 scale)
{
    return (Rect){V2_Sub(pos, V2_Scale(scale, 0.5)), V2_Add(pos, V2_Scale(scale, 0.5))};
}

internal inline bool
Rect_PointTest(V2 point, Rect rect)
{
    return (point.x >= rect.min.x && point.x <= rect.max.x &&
            point.y >= rect.min.y && point.y <= rect.max.y);
}

internal inline f32
Rect_Height(Rect rect)
{
    return rect.max.y - rect.min.y;
}

internal inline f32
Rect_Width(Rect rect)
{
    return rect.max.x - rect.min.x;
}

internal inline V2
Rect_Center(Rect rect)
{
    return (V2){
        .x = (rect.min.x + rect.max.x) / 2,
        .y = (rect.min.y + rect.max.y) / 2,
    };
}

internal inline Rect
Rect_ScaleUniformly(Rect rect, f32 scale)
{
    V2 center = Rect_Center(rect);
    V2 dmin = V2_Scale(V2_Sub(rect.min, center), scale);
    V2 dmax = V2_Scale(V2_Sub(rect.max, center), scale);
    return (Rect){V2_Add(center, dmin), V2_Add(center, dmax)};
}

internal inline Rect
Rect_Scale(Rect rect, V2 scale)
{
    V2 center = Rect_Center(rect);
    V2 dmin = V2_Hadamard(V2_Sub(rect.min, center), scale);
    V2 dmax = V2_Hadamard(V2_Sub(rect.max, center), scale);
    return (Rect){V2_Add(center, dmin), V2_Add(center, dmax)};
}

internal inline Rect
Rect_Translate(Rect rect, V2 translation)
{
    return (Rect){
        .min = V2_Add(rect.min, translation),
        .max = V2_Add(rect.max, translation),
    };
}