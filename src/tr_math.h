typedef struct V3
{
    union
    {
        struct { f32 x, y, z; };
        f32 e[3];
    };
} V3;

#define V3(X, Y, Z) (V3){ .x = (X), .y = (Y), .z = (Z) }

V3
V3_Add(V3 a, V3 b)
{
    return V3(a.x + b.x, a.y + b.y, a.z + b.z);
}

V3
V3_Sub(V3 a, V3 b)
{
    return V3(a.x - b.x, a.y - b.y, a.z - b.z);
}

V3
V3_Scale(V3 a, f32 n)
{
    return V3(a.x*n, a.y*n, a.z*n);
}

f32
V3_Inner(V3 a, V3 b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

f32
V3_Length(V3 a)
{
    __m128 inner_prod = _mm_set_ss(V3_Inner(a, a));
    return _mm_cvtss_f32(_mm_mul_ss(inner_prod, _mm_rsqrt_ss(inner_prod))); // NOTE: sqrt(x) = x/sqrt(x), rsqrt produces an approximate result (which is hopefully faster)
}

V3
V3_Normalize(V3 a)
{
    f32 res_length = _mm_cvtss_f32( _mm_rsqrt_ss(_mm_set_ss(V3_Inner(a, a))));
    return V3_Scale(a, res_length);
}

typedef struct V3x8
{
    union
    {
        struct { __m256 x, y, z; };
        __m256 e[3];
    };
} V3x8;

#define V3x8(X, Y, Z) (V3x8){ .x = (X), .y = (Y), .z = (Z) }

V3x8
V3x8_Set1V3(V3 a)
{
    return (V3x8){
        .x = _mm256_set1_ps(a.x),
        .y = _mm256_set1_ps(a.y),
        .z = _mm256_set1_ps(a.z),
    };
}

V3x8
V3x8_Add(V3x8 a, V3x8 b)
{
    return (V3x8){
        .x = _mm256_add_ps(a.x, b.x),
        .y = _mm256_add_ps(a.y, b.y),
        .z = _mm256_add_ps(a.z, b.z),
    };
}

V3x8
V3x8_Sub(V3x8 a, V3x8 b)
{
    return (V3x8){
        .x = _mm256_sub_ps(a.x, b.x),
        .y = _mm256_sub_ps(a.y, b.y),
        .z = _mm256_sub_ps(a.z, b.z),
    };
}

V3x8
V3x8_Scale(V3x8 a, __m256 n)
{
    return (V3x8){
        .x = _mm256_mul_ps(a.x, n),
        .y = _mm256_mul_ps(a.y, n),
        .z = _mm256_mul_ps(a.z, n),
    };
}

__m256
V3x8_Inner(V3x8 a, V3x8 b)
{
    return _mm256_fmadd_ps(a.x, b.x, _mm256_fmadd_ps(a.y, b.y, _mm256_mul_ps(a.z, b.z)));
}

__m256
V3x8_Length(V3x8 a, V3x8 b)
{
    __m256 inner_prod = V3x8_Inner(a, b);
    return _mm256_mul_ps(inner_prod, _mm256_rsqrt_ps(inner_prod));
}

V3x8
V3x8_Normalize(V3x8 a)
{
    __m256 res_length = _mm256_rsqrt_ps(V3x8_Inner(a, a));
    
    return (V3x8){
        .x = _mm256_mul_ps(a.x, res_length),
        .y = _mm256_mul_ps(a.y, res_length),
        .z = _mm256_mul_ps(a.z, res_length),
    };
}