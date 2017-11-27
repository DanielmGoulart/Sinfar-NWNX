#pragma once

#include "nwncx_sinfar.h"

namespace nwncx
{
	namespace sinfar
	{
		struct RGBQUAD 
		{
			uint8_t rgbBlue;
			uint8_t rgbGreen;
			uint8_t rgbRed;
			uint8_t rgbReserved;
		};

		class CXImage
		{
		private:
			nwncx::sinfar::TGAPixel_32* const pixels;
			uint32_t const width;
			uint32_t const height;
			uint32_t const num_pixels;
		public:
			CXImage(nwncx::sinfar::TGAPixel_32* pixels, uint32_t width, uint32_t height) : pixels(pixels), width(width), height(height), num_pixels(width*height)
			{
			}
			void Gamma(float gamma);
			void Lut(uint8_t* pLut);
			void Solarize(uint8_t level, bool bLinkedChannels=true);
			void Noise(int32_t level);
			void Jitter(int32_t radius);
			bool IsInside(uint32_t x, uint32_t y);
			void Light(int32_t brightness, int32_t contrast=0);
			void Colorize(uint8_t hue, uint8_t sat, float blend);
			RGBQUAD RGBtoHSL(const TGAPixel_32* pixel);
			float HueToRGB(float n1, float n2, float hue);
			RGBQUAD HSLtoRGB(RGBQUAD lHSLColor);
		};
	}
}
