enum TARGA_IMAGE_FORMAT
{
    TargaImageFormat_NoData                   = 0,
    TargaImageFormat_UncompressedColorMapped  = 1,
    TargaImageFormat_UncompressedTrueColor    = 2,
    TargaImageFormat_UncompressedGrayscale    = 3,
    TargaImageFormat_RLECompressedColorMapped = 9,
    TargaImageFormat_RLECompressedTrueColor   = 10,
    TargaImageFormat_RLECompressedGrayscale   = 11,
};

#define TARGA_IS_IMAGE_FORMAT_RLE_COMPRESSED(format) ((format) & 0x8)

typedef struct Targa_Header
{
    u16 width;
    u16 height;
    u8 depth;
    Enum8(TARGA_IMAGE_FORMAT) image_format;
    u8* id;
    u8* data;
} Targa_Header;

typedef struct Targa_File_Header
{
    u8 id_length;
    u8 color_map_type;
    u8 image_type;
    u8 color_map_specification[5];
    u8 image_specification[10];
} Targa_File_Header;

internal bool
Targa_ReadHeader(String file_contents, Targa_Header* header)
{
    bool is_valid = true;
    
    if (file_contents.size < sizeof(Targa_File_Header)) is_valid = false;
    else
    {
        Targa_File_Header* file_header = (Targa_File_Header*)file_contents.data;
        
        // NOTE: no image data is represented by all zeros in the header, which implies a width, height and depth of 0
        ZeroStruct(header);
        if (file_header->image_type != 0)
        {
            header->image_format = file_header->image_type;
            
            // NOTE: error on color mapped images missing a color map
            if ((file_header->image_type & ~0x8) == 1 && file_header->color_map_type != 1) is_valid = false;
            
            // NOTE: error on non color mapped images with a color map
            if ((file_header->image_type & ~0x8) != 1 && file_header->color_map_type == 1) is_valid = false;
            
            header->width  = *(u16*)&file_header->image_specification[4];
            header->height = *(u16*)&file_header->image_specification[6];
            header->depth  = file_header->image_specification[8];
            
            u8* advance = (u8*)file_header + sizeof(Targa_File_Header);
            header->id = (file_header->id_length != 0 ? advance : 0);
            advance += file_header->id_length;
            
            advance += *(u32*)&file_header->color_map_specification[2] * file_header->color_map_specification[4];
            
            header->data = advance;
        }
    }
    
    if (!is_valid) ZeroStruct(header);
    
    return is_valid;
}