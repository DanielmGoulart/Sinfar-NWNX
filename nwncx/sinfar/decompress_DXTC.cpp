#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "DXTC.h"

namespace DTXTC
{

static __inline int my_round(double d)
{
    return (int)(d + 0.500001);
}
/* convert little-endian to the current arch byte order */
static __inline uint16_t uint16_l2arch(uint16_t ui16l)
{
    uint16_t ui16arch;
    unsigned char *p = (unsigned char *)&ui16l;

    ui16arch  = p[0];
    ui16arch += p[1] <<  8;
    ui16arch += p[2] << 16;
    ui16arch += p[3] << 24;

    return ui16arch;
}


static __inline void DecompressColorBlock(
        uint16_t *compressedData, COLOR_ALPHA *rawData, unsigned int xPos,
        unsigned int yPos, unsigned int lineWidth, compressionType type)
{
    COLOR c[4];
    int i, j;
    unsigned char colorIndex;
    uint16_t word1, word2;
    uint16_t w;
    int oneBitTrans;

    word1 = uint16_l2arch(*compressedData);
    word2 = uint16_l2arch(*(compressedData+1));
    compressedData += 2;

    if (word1 > word2){
        oneBitTrans = 0;
    }
    else {
        if (type == DXT1)
            oneBitTrans = 1;
        else
            oneBitTrans = 0;
    }

    c[0].r = (word1 >> 11) & 0x1F;
    c[0].r = my_round((double)c[0].r*0xFF/0x1F);
    c[0].g = (word1 >> 5) & 0x3F;
    c[0].g = my_round((double)c[0].g*0xFF/0x3F);
    c[0].b = (word1) & 0x1F;
    c[0].b = my_round((double)c[0].b*0xFF/0x1F);

    c[1].r = (word2 >> 11) & 0x1F;
    c[1].r = my_round((double)c[1].r*0xFF/0x1F);
    c[1].g = (word2 >> 5) & 0x3F;
    c[1].g = my_round((double)c[1].g*0xFF/0x3F);
    c[1].b = (word2) & 0x1F;
    c[1].b = my_round((double)c[1].b*0xFF/0x1F);

    if (oneBitTrans) {
        c[2].r = my_round(0.5*c[0].r + 0.5*c[1].r);
        c[2].g = my_round(0.5*c[0].g + 0.5*c[1].g);
        c[2].b = my_round(0.5*c[0].b + 0.5*c[1].b);

        c[3].r = 0;
        c[3].g = 0;
        c[3].b = 0;
    }
    else {
        c[2].r = my_round(2.0/3*c[0].r + 1.0/3*c[1].r);
        c[2].g = my_round(2.0/3*c[0].g + 1.0/3*c[1].g);
        c[2].b = my_round(2.0/3*c[0].b + 1.0/3*c[1].b);

        c[3].r = my_round(1.0/3*c[0].r + 2.0/3*c[1].r);
        c[3].g = my_round(1.0/3*c[0].g + 2.0/3*c[1].g);
        c[3].b = my_round(1.0/3*c[0].b + 2.0/3*c[1].b);
    }

    w = uint16_l2arch(*compressedData);
    for (i = 0; i < 4; i++) {
        if (i == 2) w = uint16_l2arch(*(++compressedData));

        for (j = 0; j < 4; j++) {
            colorIndex = w & 0x03;

            rawData[(yPos+i)*lineWidth+(xPos+j)].color = c[colorIndex];
            if (type == DXT1) {
                if (colorIndex == 3 && oneBitTrans)
                    rawData[(yPos+i)*lineWidth+(xPos+j)].alpha = 0;
                else
                    rawData[(yPos+i)*lineWidth+(xPos+j)].alpha = 255;
            }

            w >>= 2;
        }
    }
}

static __inline void DecompressDXT3AlphaBlock(
        unsigned char *compressedData, COLOR_ALPHA *rawData, unsigned int xPos,
        unsigned int yPos, unsigned int lineWidth)
{
    int i, j;
    unsigned char c=0;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            if (j % 2)
                c >>= 4;
            else
                c = *(compressedData++);

            rawData[(yPos+i)*lineWidth+(xPos+j)].alpha =
                my_round((double)(c&0x0F)*0xFF/0x0F);
        }
    }
}

static __inline void DecompressDXT5AlphaBlock(
        unsigned char *compressedData, COLOR_ALPHA *rawData, unsigned int xPos,
        unsigned int yPos, unsigned int lineWidth)
{
    unsigned char a[8];
    unsigned char alphaIndex;
    int bitPos;
    int i, j;

    a[0] = *compressedData;
    a[1] = *(compressedData+1);

    if (*compressedData > *(compressedData+1)) {
        a[2] = my_round(6.0/7*a[0] + 1.0/7*a[1]);
        a[3] = my_round(5.0/7*a[0] + 2.0/7*a[1]);
        a[4] = my_round(4.0/7*a[0] + 3.0/7*a[1]);
        a[5] = my_round(3.0/7*a[0] + 4.0/7*a[1]);
        a[6] = my_round(2.0/7*a[0] + 5.0/7*a[1]);
        a[7] = my_round(1.0/7*a[0] + 6.0/7*a[1]);
    }
    else {
        a[2] = my_round(4.0/5*a[1] + 1.0/5*a[0]);
        a[3] = my_round(3.0/5*a[1] + 2.0/5*a[0]);
        a[4] = my_round(2.0/5*a[1] + 3.0/5*a[0]);
        a[5] = my_round(1.0/5*a[1] + 4.0/5*a[0]);
        a[6] = 0x00;
        a[7] = 0xFF;
    }

    compressedData += 2;
    bitPos = 0;
#define GET_BIT(X) (((*(compressedData+(X)/8)) >> ((X)%8)) & 0x01)
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            alphaIndex =   (GET_BIT(bitPos+2) << 2)
                         | (GET_BIT(bitPos+1) << 1)
                         | GET_BIT(bitPos);

            rawData[(yPos+i)*lineWidth+(xPos+j)].alpha = a[alphaIndex];

            bitPos += 3;
        }
    }
#undef GET_BIT
}


void Decompress(unsigned char *compressedData, COLOR_ALPHA *rawData,
                unsigned int width, unsigned int height, compressionType type)
{
    unsigned int xPos = 0, yPos = 0;

    while (yPos < height) {
        if (type == DXT3) {
           DecompressDXT3AlphaBlock(compressedData, rawData, xPos, yPos, width);
           compressedData += 8;
        }
        if (type == DXT5) {
           DecompressDXT5AlphaBlock(compressedData, rawData, xPos, yPos, width);
           compressedData += 8;
        }
        DecompressColorBlock((uint16_t *)compressedData, rawData,
                             xPos, yPos, width, type);
        compressedData += 8;

        xPos += 4;
        if (xPos >= width) {
            xPos = 0;
            yPos += 4;
        }
    }
}

}
