#pragma once

namespace DTXTC
{

typedef struct {
	unsigned char b;
	unsigned char g;
	unsigned char r;
} COLOR;

typedef struct {
	COLOR color;
	unsigned char alpha;
} COLOR_ALPHA;

typedef enum {
	DXT1,
	DXT3,
	DXT5
} compressionType;

void Decompress(unsigned char *compressedData, COLOR_ALPHA *rawData,
                unsigned int width, unsigned int height, compressionType type);
}