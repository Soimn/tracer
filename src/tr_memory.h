internal inline umm
RoundUpToAlignment(umm size, umm alignment)
{
    umm overshoot    = size + (alignment - 1);
    umm rounded_down = overshoot & ~(alignment - 1);
    
    return rounded_down;
}

internal inline umm
RoundDownToAlignment(umm size, umm alignment)
{
    umm rounded_down = size & ~(alignment - 1);
    
    return rounded_down;
}

internal inline void*
Align(void* ptr, u8 alignment)
{
    return (void*)RoundUpToAlignment((umm)ptr, alignment);
}

internal inline u8
AlignOffset(void* ptr, u8 alignment)
{
    return (u8)(RoundUpToAlignment((umm)ptr, alignment) - (umm)ptr);
}

internal inline void
Copy(void* src, void* dst, umm size)
{
    u8* bsrc = src;
    u8* bdst = dst;
    
    for (umm i = 0; i < size; ++i)
    {
        *bdst++ = *bsrc++;
    }
}

#define CopyStruct(src, dst) Copy((src), (dst), sizeof(*(src)))

internal inline void
Move(void* src, void* dst, umm size)
{
    u8* bsrc = src;
    u8* bdst = dst;
    
    if (bdst < bsrc || bsrc + size < bdst) Copy(src, dst, size);
    else
    {
        bsrc += size - 1;
        bdst += size - 1;
        
        for (umm i = 0; i < size; ++i)
        {
            *bdst-- = *bsrc--;
        }
    }
}

internal inline void
Zero(void* ptr, umm size)
{
    u8* bptr = ptr;
    
    for (umm i = 0; i < size; ++i)
    {
        *bptr++ = 0;
    }
}

#define ZeroStruct(X) Zero((X), sizeof(*(X)))
#define ZeroArray(X, S) Zero((X), sizeof(*(X)) * (S))

internal inline bool
MemoryCompare(void* a, void* b, umm size)
{
    bool are_equal = true;
    
    u8* ba = a;
    u8* bb = b;
    
    for (umm i = 0; i < size; ++i)
    {
        if (*ba++ != *bb++)
        {
            are_equal = false;
            break;
        }
    }
    
    return are_equal;
}

#define StructCompare(s0, s1) MemoryCompare((s0), (s1), sizeof(*(s0)))
#define ArrayCompare(a0, a1, size) MemoryCompare((a0), (a1), sizeof(*(a0)) * (size))

typedef struct Memory_Arena_Marker
{
    u64 offset;
} Memory_Arena_Marker;

#define MEMORY_ARENA_PAGE_SIZE KB(4)

typedef struct Memory_Arena
{
    u64 base_address;
    u64 offset;
    u64 size;
} Memory_Arena;

internal void*
Arena_PushSize(Memory_Arena* arena, umm size, u8 alignment)
{
    u64 offset = arena->offset + AlignOffset((void*)(arena->base_address + arena->offset), alignment);
    
    if (offset + size > arena->size)
    {
        Platform->CommitMemory((void*)arena->base_address, offset + size);
    }
    
    arena->offset = offset + size;
    
    return (void*)(arena->base_address + offset);
}

internal inline void
Arena_PopSize(Memory_Arena* arena, umm size)
{
    ASSERT(arena->offset > size);
    arena->offset -= size;
}

internal inline void
Arena_Clear(Memory_Arena* arena)
{
    arena->offset = 0;
}

internal Memory_Arena_Marker
Arena_BeginTempMemory(Memory_Arena* arena)
{
    return (Memory_Arena_Marker){arena->offset};
}

internal void
Arena_EndTempMemory(Memory_Arena* arena, Memory_Arena_Marker marker)
{
    arena->offset = marker.offset;
}

internal umm
Arena_UsedSize(Memory_Arena* arena)
{
    return arena->offset;
}

typedef struct Binary_Tree
{
    Memory_Arena* arena;
    void* data;
    u32 node_size;
    u32 node_cap;
} Binary_Tree;

internal inline Binary_Tree
BT__Init(Memory_Arena* arena, umm node_size)
{
    ASSERT(node_size <= U32_MAX);
    
    return (Binary_Tree){
        .arena     = arena,
        .data      = 0,
        .node_size = (u32)node_size,
        .node_cap  = 0,
    };
}

#define BT_Init(arena, T) BT__Init((arena), sizeof(T))

internal inline void*
BT_LeftChild(Binary_Tree* tree, void* ptr)
{
    ASSERT((u64)ptr >= (u64)tree->data);
    
    umm index = ((u64)ptr - (u64)tree->data) / tree->node_size;
    ASSERT((u64)ptr == index * tree->node_size + (u64)tree->data);
    
    umm left = index * 2 + 1;
    
    return (left < tree->node_cap ? (void*)(left * tree->node_size + (u64)tree->data) : 0);
}

internal inline void*
BT_RightChild(Binary_Tree* tree, void* ptr)
{
    ASSERT((u64)ptr >= (u64)tree->data);
    
    umm index = ((u64)ptr - (u64)tree->data) / tree->node_size;
    ASSERT((u64)ptr == index * tree->node_size + (u64)tree->data);
    
    umm right = (index + 1) * 2;
    
    return (right < tree->node_cap ? (void*)(right * tree->node_size + (u64)tree->data) : 0);
}

internal inline void*
BT_Parent(Binary_Tree* tree, void* ptr)
{
    ASSERT((u64)ptr >= (u64)tree->data);
    
    umm index = ((u64)ptr - (u64)tree->data) / tree->node_size;
    ASSERT((u64)ptr == index * tree->node_size + (u64)tree->data);
    
    umm parent = MAX(0, ((imm)index - 1) / 2);
    
    return (parent == index ? 0 : (void*)(parent * tree->node_size + (u64)tree->data));
}

enum BINARY_TREE_NODE_KIND
{
    BinaryTreeNode_Root,
    BinaryTreeNode_Parent,
    BinaryTreeNode_LeftChild,
    BinaryTreeNode_RightChild,
    
    BINARY_TREE_NODE_KIND_COUNT
};

internal inline void*
BT_GetNode(Binary_Tree* tree, void* ref_node, Enum8(BINARY_TREE_NODE_KIND) kind)
{
    ASSERT(kind != BinaryTreeNode_Root);
    
    void* node;
    if      (kind == BinaryTreeNode_Parent)    node = BT_Parent(tree, ref_node);
    else if (kind == BinaryTreeNode_LeftChild) node = BT_LeftChild(tree, ref_node);
    else                                       node = BT_RightChild(tree, ref_node);
    
    return node;
}

internal void*
BT_AddNode(Binary_Tree* tree, void* parent, Enum8(BINARY_TREE_NODE_KIND) kind)
{
    ASSERT(kind != BinaryTreeNode_Parent);
    ASSERT((u64)parent >= (u64)tree->data);
    
    umm index = ((u64)parent - (u64)tree->data) / tree->node_size;
    umm place;
    if (kind == BinaryTreeNode_Root) place = 0;
    else                             place = 2 * (index + (kind == BinaryTreeNode_RightChild)) + (kind == BinaryTreeNode_LeftChild);
    
    if (place >= tree->node_cap)
    {
        // NOTE: in case the binary tree is using temporary memory from an arena it should align the first allocation
        //       to the maximum alignment, it is otherwise tightly packed
        u8 alignment = (tree->node_cap == 0 ? ALIGNOF(u64) : ALIGNOF(u8));
        
        void* memory = Arena_PushSize(tree->arena, (umm)((place - tree->node_cap) + 1) * tree->node_size, alignment);
        
        tree->node_cap = (u32)place + 1;
        
        if (tree->data == 0) tree->data = memory;
    }
    
    return (void*)((u64)tree->data + place * tree->node_size);
}

internal inline umm
BT_HeightOfNode(Binary_Tree* tree, void* ptr)
{
    ASSERT((u64)ptr >= (u64)tree->data);
    
    umm index = ((u64)ptr - (u64)tree->data) / tree->node_size;
    ASSERT((u64)ptr == index * tree->node_size + (u64)tree->data);
    
    umm height = 0;
    for (; index != 0; index >>= 1, height += 1);
    
    return height;
}