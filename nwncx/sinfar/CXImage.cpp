#include "CXImage.h"
#include "pch.h"
#define RGB2GRAY(r,g,b) (((b)*117 + (g)*601 + (r)*306) >> 10)
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

using namespace nwncx::sinfar;

namespace nwncx
{
	namespace sinfar
	{
		////////////////////////////////////////////////////////////////////////////////
		/**
		* Apply a look up table to the image.
		* \param pLut: uint8_t[256] look up table
		* \return true if everything is ok
		*/
		void CXImage::Lut(uint8_t* pLut)
		{
			for (uint32_t pixel_index=0; pixel_index<num_pixels; pixel_index++)
			{
				TGAPixel_32* pixel = pixels + pixel_index;
				pixel->red = pLut[pixel->red];
				pixel->green = pLut[pixel->green];
				pixel->blue = pLut[pixel->blue];
			}
		}

		////////////////////////////////////////////////////////////////////////////////
		/**
		* Adjusts the color balance of the image
		* \param gamma can be from 0.1 to 5.
		* \return true if everything is ok
		* \sa GammaRGB
		*/
		void CXImage::Gamma(float gamma)
		{
			if (gamma <= 0.0f) return;
			double dinvgamma = 1 / gamma;
			double dMax = std::pow(255.0, dinvgamma) / 255.0;
			uint8_t cTable[256]; //<nipper>
			for (int32_t i = 0; i < 256; i++) 
			{
				cTable[i] = (uint8_t)max(0, min(255, (int32_t)(std::pow((double)i, dinvgamma) / dMax)));
			}
			Lut(cTable);
		}

		////////////////////////////////////////////////////////////////////////////////
		/**
		* Solarize: convert all colors above a given lightness level into their negative
		* \param  level : lightness threshold. Range = 0 to 255; default = 128.
		* \param  bLinkedChannels: true = compare with luminance, preserve colors (default)
		*                         false = compare with independent R,G,B levels
		* \return true if everything is ok
		* \author [Priyank Bolia] (priyank_bolia(at)yahoo(dot)com); changes [DP]
		*/
		void CXImage::Solarize(uint8_t level, bool bLinkedChannels)
		{
			for (uint32_t pixel_index = 0; pixel_index<num_pixels; pixel_index++)
			{
				TGAPixel_32* pixel = pixels + pixel_index;
				if (bLinkedChannels) 
				{
					if ((uint8_t)RGB2GRAY(pixel->red, pixel->green, pixel->blue)>level) 
					{
						pixel->red = (uint8_t)(255 - pixel->red);
						pixel->green = (uint8_t)(255 - pixel->green);
						pixel->blue = (uint8_t)(255 - pixel->blue);
					}
				}
				else 
				{
					if (pixel->blue>level) pixel->blue = (uint8_t)(255 - pixel->blue);
					if (pixel->green>level)	pixel->green = (uint8_t)(255 - pixel->green);
					if (pixel->red>level) pixel->red = (uint8_t)(255 - pixel->red);
				}
			}
		}

		/**
		* Adds an uniform noise to the image
		* \param level: can be from 0 (no noise) to 255 (lot of noise).
		* \return true if everything is ok
		*/
		void CXImage::Noise(int32_t level)
		{
			for (uint32_t pixel_index = 0; pixel_index<num_pixels; pixel_index++)
			{
				TGAPixel_32* pixel = pixels + pixel_index;
				int32_t n = (int32_t)((rand() / (float)RAND_MAX - 0.5)*level);
				pixel->red = (uint8_t)max(0, min(255, (int32_t)(pixel->red + n)));
				n = (int32_t)((rand() / (float)RAND_MAX - 0.5)*level);
				pixel->green = (uint8_t)max(0, min(255, (int32_t)(pixel->green + n)));
				n = (int32_t)((rand() / (float)RAND_MAX - 0.5)*level);
				pixel->blue = (uint8_t)max(0, min(255, (int32_t)(pixel->blue + n)));
			}
		}

		bool CXImage::IsInside(uint32_t x, uint32_t y)
		{
			return (y < height && x < width);
		}

		////////////////////////////////////////////////////////////////////////////////
		/**
		* Adds a random offset to each pixel in the image
		* \param radius: maximum pixel displacement
		* \return true if everything is ok
		*/
		void CXImage::Jitter(int32_t radius)
		{
			for (uint32_t y = 0; y<height; y++)
			{
				for (uint32_t x = 0; x<width; x++)
				{
					uint32_t nx = x + (uint32_t)((rand() / (float)RAND_MAX - 0.5)*(radius * 2));
					uint32_t ny = y + (uint32_t)((rand() / (float)RAND_MAX - 0.5)*(radius * 2));
					if (IsInside(nx, ny)) 
					{
						pixels[x*y] = pixels[nx*ny];
					}
				}
			}
		}

		/**
		* Changes the brightness and the contrast of the image.
		* \param brightness: can be from -255 to 255, if brightness is negative, the image becomes dark.
		* \param contrast: can be from -100 to 100, the neutral value is 0.
		* \return true if everything is ok
		*/
		void CXImage::Light(int32_t brightness, int32_t contrast)
		{
			float c = (100 + contrast) / 100.0f;
			brightness += 128;

			uint8_t cTable[256]; //<nipper>
			for (int32_t i = 0; i<256; i++) {
				cTable[i] = (uint8_t)max(0, min(255, (int32_t)((i - 128)*c + brightness + 0.5f)));
			}

			return Lut(cTable);
		}


		float CXImage::HueToRGB(float n1, float n2, float hue)
		{
			//<F. Livraghi> fixed implementation for HSL2RGB routine
			float rValue;

			if (hue > 360)
				hue = hue - 360;
			else if (hue < 0)
				hue = hue + 360;

			if (hue < 60)
				rValue = n1 + (n2 - n1)*hue / 60.0f;
			else if (hue < 180)
				rValue = n2;
			else if (hue < 240)
				rValue = n1 + (n2 - n1)*(240 - hue) / 60;
			else
				rValue = n1;

			return rValue;
		}
		RGBQUAD CXImage::HSLtoRGB(RGBQUAD lHSLColor)
		{
			//<F. Livraghi> fixed implementation for HSL2RGB routine
			float h, s, l;
			float m1, m2;
			uint8_t r, g, b;

			h = (float)lHSLColor.rgbRed * 360.0f / 255.0f;
			s = (float)lHSLColor.rgbGreen / 255.0f;
			l = (float)lHSLColor.rgbBlue / 255.0f;

			if (l <= 0.5)	m2 = l * (1 + s);
			else			m2 = l + s - l*s;

			m1 = 2 * l - m2;

			if (s == 0) {
				r = g = b = (uint8_t)(l*255.0f);
			}
			else {
				r = (uint8_t)(HueToRGB(m1, m2, h + 120) * 255.0f);
				g = (uint8_t)(HueToRGB(m1, m2, h) * 255.0f);
				b = (uint8_t)(HueToRGB(m1, m2, h - 120) * 255.0f);
			}

			RGBQUAD rgb = { b,g,r,0 };
			return rgb;
		}
		////////////////////////////////////////////////////////////////////////////////
#define  HSLMAX   255	/* H,L, and S vary over 0-HSLMAX */
#define  RGBMAX   255   /* R,G, and B vary over 0-RGBMAX */
		/* HSLMAX BEST IF DIVISIBLE BY 6 */
		/* RGBMAX, HSLMAX must each fit in a uint8_t. */
		/* Hue is undefined if Saturation is 0 (grey-scale) */
		/* This value determines where the Hue scrollbar is */
		/* initially set for achromatic colors */
#define HSLUNDEFINED (HSLMAX*2/3)
		////////////////////////////////////////////////////////////////////////////////
		RGBQUAD CXImage::RGBtoHSL(const TGAPixel_32* pixel)
		{
			uint8_t R, G, B;					/* input RGB values */
			uint8_t H, L, S;					/* output HSL values */
			uint8_t cMax, cMin;				/* max and min RGB values */
			uint16_t Rdelta, Gdelta, Bdelta;	/* intermediate value: % of spread from max*/

			R = pixel->red;	/* get R, G, and B out of uint32_t */
			G = pixel->green;
			B = pixel->blue;

			cMax = max(max(R, G), B);	/* calculate lightness */
			cMin = min(min(R, G), B);
			L = (uint8_t)((((cMax + cMin)*HSLMAX) + RGBMAX) / (2 * RGBMAX));

			if (cMax == cMin) {			/* r=g=b --> achromatic case */
				S = 0;					/* saturation */
				H = HSLUNDEFINED;		/* hue */
			}
			else {					/* chromatic case */
				if (L <= (HSLMAX / 2))	/* saturation */
					S = (uint8_t)((((cMax - cMin)*HSLMAX) + ((cMax + cMin) / 2)) / (cMax + cMin));
				else
					S = (uint8_t)((((cMax - cMin)*HSLMAX) + ((2 * RGBMAX - cMax - cMin) / 2)) / (2 * RGBMAX - cMax - cMin));
				/* hue */
				Rdelta = (uint16_t)((((cMax - R)*(HSLMAX / 6)) + ((cMax - cMin) / 2)) / (cMax - cMin));
				Gdelta = (uint16_t)((((cMax - G)*(HSLMAX / 6)) + ((cMax - cMin) / 2)) / (cMax - cMin));
				Bdelta = (uint16_t)((((cMax - B)*(HSLMAX / 6)) + ((cMax - cMin) / 2)) / (cMax - cMin));

				if (R == cMax)
					H = (uint8_t)(Bdelta - Gdelta);
				else if (G == cMax)
					H = (uint8_t)((HSLMAX / 3) + Rdelta - Bdelta);
				else /* B == cMax */
					H = (uint8_t)(((2 * HSLMAX) / 3) + Gdelta - Rdelta);

				//		if (H < 0) H += HSLMAX;     //always false
				if (H > HSLMAX) H -= HSLMAX;
			}
			RGBQUAD hsl = { L,S,H,0 };
			return hsl;
		}

		/**
		* Replaces the original hue and saturation values.
		* \param hue: hue
		* \param sat: saturation
		* \param blend: can be from 0 (no effect) to 1 (full effect)
		* \return true if everything is ok
		*/
		void CXImage::Colorize(uint8_t hue, uint8_t sat, float blend)
		{
			if (blend < 0.0f) blend = 0.0f;
			if (blend > 1.0f) blend = 1.0f;
			bool full_blend = blend > 0.999f;
			int32_t a0 = (int32_t)(256 * blend);
			int32_t a1 = 256 - a0;

			for (uint32_t pixel_index = 0; pixel_index<num_pixels; pixel_index++)
			{
				TGAPixel_32* pixel = pixels + pixel_index;
				if (full_blend) 
				{
					RGBQUAD color = RGBtoHSL(pixel);
					color.rgbRed = hue;
					color.rgbGreen = sat;
					RGBQUAD new_color = HSLtoRGB(color);
					pixel->blue = new_color.rgbBlue;
					pixel->green = new_color.rgbGreen;
					pixel->red = new_color.rgbRed;
				}
				else {
					RGBQUAD hsl;
					hsl.rgbRed = hue;
					hsl.rgbGreen = sat;
					hsl.rgbBlue = (uint8_t)RGB2GRAY(pixel->red, pixel->green, pixel->blue);
					hsl = HSLtoRGB(hsl);
					pixel->red = (uint8_t)((hsl.rgbRed * a0 + pixel->red * a1) >> 8);
					pixel->blue = (uint8_t)((hsl.rgbBlue * a0 + pixel->blue * a1) >> 8);
					pixel->green = (uint8_t)((hsl.rgbGreen * a0 + pixel->green * a1) >> 8);
				}
			}
		}
	}
}