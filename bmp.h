#ifndef BMP_BMP_HEADER

// NOTE: Requires
// #include <stdio.h>
// #include <stdlib.h>

#ifdef _MSC_VER
#define PACK_1(x) __pragma(pack(push, 1)) x __pragma(pack(pop))
#else
#define PACK_1(x) x __attribute__((__packed__))
#endif

// NOTE: Change the order if neeeded.
#define BMP_RGBA(r, g, b, a) (unsigned int)(((a) << 24) | ((r) << 16) | ((g) << 8)  | ((b) << 0))
#define BMP_BYTES_PER_PIXEL 4

struct BMP_Bmp
{
	void* pixels;
	int width;
	int height;
};

// NOTE: BMP_BmpHeader has to be tightly packed to read in BMP_LoadBmp.
PACK_1(
	   struct BMP_BmpHeader
	   {
		   // NOTE: BITMAPFILEHEADER, size 14.
		   unsigned short file_type;        /* File type, always 4D42h ("BM") */
		   unsigned int file_size;          /* Size of the file in bytes */
		   unsigned short reserved_1;       /* Always 0 */
		   unsigned short reserved_2;       /* Always 0 */
		   unsigned int bitmap_offset;      /* Starting position of image data in bytes */

		   // NOTE: BITMAPV4HEADER, size 108.
		   unsigned int size;               /* Size of this header in bytes */
		   int width;                       /* Image width in pixels */
		   int height;                      /* Image height in pixels */
		   unsigned short planes;           /* Number of color planes */
		   unsigned short bits_per_pixel;   /* Number of bits per pixel */
		   unsigned int compression;        /* Compression methods used */
		   unsigned int size_of_bitmap;     /* Size of bitmap in bytes */
		   int horizontal_resolution;       /* Horizontal resolution in pixels per meter */
		   int vertical_resolution;         /* Vertical resolution in pixels per meter */
		   unsigned int colors_used;        /* Number of colors in the image */
		   unsigned int colors_important;   /* Minimum number of important colors */

		   unsigned int red_mask;           /* Mask identifying bits of red component */
		   unsigned int green_mask;         /* Mask identifying bits of green component */
		   unsigned int blue_mask;          /* Mask identifying bits of blue component */
		   unsigned int alpha_mask;         /* Mask identifying bits of alpha component */
		   unsigned int colorspace_type;    /* Color space type */

		   int red_x;                       /* X coordinate of red endpoint */
		   int red_y;                       /* Y coordinate of red endpoint */
		   int red_z;                       /* Z coordinate of red endpoint */
		   int green_x;                     /* X coordinate of green endpoint */
		   int green_y;                     /* Y coordinate of green endpoint */
		   int green_z;                     /* Z coordinate of green endpoint */
		   int blue_x;                      /* X coordinate of blue endpoint */
		   int blue_y;                      /* Y coordinate of blue endpoint */
		   int blue_z;                      /* Z coordinate of blue endpoint */
		   unsigned int gamma_red;          /* Gamma red coordinate scale value */
		   unsigned int gamma_green;        /* Gamma green coordinate scale value */
		   unsigned int gamma_blue;         /* Gamma blue coordinate scale value */
	   };
	   );


/* 

   NOTE: Function declarations.
   
*/

// NOTE: Loads .bmp file from file and returns it as BMP_Bmp.
static BMP_Bmp BMP_LoadBmp(char* file_name);

// NOTE: Creates .bmp file by given width, height and pixel_data, returns 0 if it fails.
static int BMP_CreateBmp(char* file_name, void* source_bmp_pixels, int width, int height);
static int BMP_CreateBmp(char* file_name, BMP_Bmp* source_bmp);

// NOTE: Slices source_bmp_pixels starting from x_offset and y_offset by slices_width and slices_height, returns sliced bitmap as BMP_Bmp.
static BMP_Bmp BMP_SliceBmp(void* source_bmp_pixels, int source_width, int source_height,
							int x_offset, int y_offset, int slice_width, int slice_height);
static BMP_Bmp BMP_SliceBmp(BMP_Bmp* source_bmp, int x_offset, int y_offset, int slice_width, int slice_height);

// NOTE: Checks if slice is empty.
static int BMP_IsBmpSliceEmpty(void* source_bmp_pixels, int source_width, int source_height,
							   int x_offset, int y_offset, int slice_width, int slice_height);
static int BMP_IsBmpSliceEmpty(BMP_Bmp* source_bmp, int x_offset, int y_offset, int slice_width, int slice_height);

// NOTE: Slices source_bmp_pixels by size slice_width and slice_height from top to bottom.
// Creates different file for every slice and skip empty slices.
// File naming scheme is:
// file_base_name000.bmp, file_base_name001.bmp, ..., file_base_name999.bmp (if max_digit_count is 3)
// If there is more than 1000 slices set max_digit_count parameter higher than 3.
static int BMP_BatchSliceBmpAndWriteToSeperateFiles(char* file_base_name,
													void* source_bmp_pixels, int source_width, int source_height,
													int slice_width, int slice_height, int max_digit_count=3);
static int BMP_BatchSliceBmpAndWriteToSeperateFiles(char* file_base_name,
													BMP_Bmp* source_bmp,
													int slice_width, int slice_height, int max_digit_count=3);

// NOTE: Utility function for BMP_LoadBmp().
static int BMP_FindLeastSignificantSetBit(int value);

// NOTE: Utility functions to name files in BMP_BatchSliceBmpAndWriteToSeperateFiles()
static int BMP_GetDigitCount(int number);
static int BMP_GetDigitN(int number, int digit_n);

/* 

   NOTE: Function Implementations.
   
*/

static BMP_Bmp BMP_LoadBmp(char* file_name)
{
	BMP_Bmp bmp = {};

	FILE* bmp_file = fopen(file_name, "rb");

	if (bmp_file) {

		fseek(bmp_file , 0 , SEEK_END);
		unsigned int file_size = ftell(bmp_file);
		rewind(bmp_file);

		void* bmp_file_buffer = malloc(file_size);
		fread(bmp_file_buffer, 1, file_size, bmp_file);

		BMP_BmpHeader* bmp_header = (BMP_BmpHeader*)bmp_file_buffer;

		bmp.pixels = (unsigned int*)((unsigned char*)bmp_header + bmp_header->bitmap_offset);
		bmp.width = bmp_header->width;
		bmp.height = bmp_header->height;

		unsigned int red_index = BMP_FindLeastSignificantSetBit(bmp_header->red_mask);
		unsigned int green_index = BMP_FindLeastSignificantSetBit(bmp_header->green_mask);
		unsigned int blue_index = BMP_FindLeastSignificantSetBit(bmp_header->blue_mask);
		unsigned int alpha_index = BMP_FindLeastSignificantSetBit(bmp_header->alpha_mask);

		unsigned int* pixels = (unsigned int*)bmp.pixels;
		for (int y = 0; y < bmp_header->height; ++y) {
			for (int x = 0; x < bmp_header->width; ++x) {
				unsigned int color = *pixels;

				unsigned int r = (color & bmp_header->red_mask) >> red_index;
				unsigned int g = (color & bmp_header->green_mask) >> green_index;
				unsigned int b = (color & bmp_header->blue_mask) >> blue_index;
				unsigned int a = (color & bmp_header->alpha_mask) >> alpha_index;

				*pixels++ = ((a << 24) |
							 (r << 16) |
							 (g << 8)  |
							 (b << 0));
			}
		}

		fclose(bmp_file);
	}

	return bmp;
}

static int BMP_CreateBmp(char* file_name, void* source_bmp_pixels, int width, int height)
{
	int result = false;

	FILE* bmp_file = fopen(file_name, "wb");
	if (bmp_file) {
		BMP_BmpHeader bmp_header = {};

		unsigned int bmp_file_header_size = 14;
		unsigned int bmp_v4_header_size = 108;
		unsigned int source_bmp_pixels_size = width * height * BMP_BYTES_PER_PIXEL;
		bmp_header.file_type = (unsigned short)(('M' << 8) | ('B' << 0));
		bmp_header.file_size = bmp_file_header_size + bmp_v4_header_size + source_bmp_pixels_size;
		bmp_header.bitmap_offset = bmp_file_header_size + bmp_v4_header_size;
		bmp_header.size = bmp_v4_header_size;
		bmp_header.width = width;
		bmp_header.height = height;
		bmp_header.planes = 1;
		bmp_header.bits_per_pixel = BMP_BYTES_PER_PIXEL * 8;
		bmp_header.compression = 3; // BI_BITFIELDS
		bmp_header.size_of_bitmap = source_bmp_pixels_size;
		bmp_header.red_mask = BMP_RGBA(255, 0, 0, 0);
		bmp_header.green_mask = BMP_RGBA(0, 255, 0, 0);
		bmp_header.blue_mask = BMP_RGBA(0, 0, 255, 0);
		bmp_header.alpha_mask =  BMP_RGBA(0, 0, 0, 255);
		bmp_header.colorspace_type = 0x57696E20; // LCS_WINDOWS_COLOR_SPACE;

		fwrite(&bmp_header, sizeof(bmp_header), 1, bmp_file);
		fwrite(source_bmp_pixels, source_bmp_pixels_size, 1, bmp_file);

		fclose(bmp_file);

		result = true;
	}

	return result;
}

static int BMP_CreateBmp(char* file_name, BMP_Bmp* source_bmp)
{
	int result = BMP_CreateBmp(file_name, source_bmp->pixels, source_bmp->width, source_bmp->height);

	return result;
}

static int BMP_IsBmpSliceEmpty(void* source_bmp_pixels, int source_width, int source_height,
							   int x_offset, int y_offset, int slice_width, int slice_height)
{
	int is_empty = true;

	y_offset = source_height - y_offset - slice_height;

	unsigned char* source_pixels = ((unsigned char*)source_bmp_pixels +
									y_offset * source_width * BMP_BYTES_PER_PIXEL +
									x_offset * BMP_BYTES_PER_PIXEL);
	for (int y = 0; is_empty && y < slice_height; ++y) {
		unsigned int* source_color = (unsigned int*)source_pixels;
		for (int x = 0; is_empty && x < slice_width; ++x) {
			if (*source_color) {
				is_empty = false;
			}

			++source_color;
		}

		source_pixels += source_width * BMP_BYTES_PER_PIXEL;
	}

	return is_empty;
}

static int BMP_IsBmpSliceEmpty(BMP_Bmp* source_bmp, int x_offset, int y_offset, int slice_width, int slice_height)
{
	int is_empty = BMP_IsBmpSliceEmpty(source_bmp->pixels, source_bmp->width, source_bmp->height,
									   x_offset, y_offset, slice_width, slice_height);

	return is_empty;
}

static BMP_Bmp BMP_SliceBmp(void* source_bmp_pixels, int source_width, int source_height,
							int x_offset, int y_offset, int slice_width, int slice_height)
{
	BMP_Bmp result = {};

	result.pixels = malloc(slice_width * slice_height * BMP_BYTES_PER_PIXEL);
	result.width = slice_width;
	result.height = slice_height;

	y_offset = source_height - y_offset - slice_height;

	unsigned char* source_pixels = ((unsigned char*)source_bmp_pixels +
									y_offset * source_width * BMP_BYTES_PER_PIXEL +
									x_offset * BMP_BYTES_PER_PIXEL);
	unsigned char* destination_pixels = (unsigned char*)result.pixels;
	for (int y = 0; y < slice_height; ++y) {
		unsigned int* source_color = (unsigned int*)source_pixels;
		unsigned int* destination_color = (unsigned int*)destination_pixels;
		for (int x = 0; x < slice_width; ++x) {
			*destination_color++ = *source_color++;
		}

		source_pixels += source_width * BMP_BYTES_PER_PIXEL;
		destination_pixels += slice_width * BMP_BYTES_PER_PIXEL;
	}

	return result;
}

static BMP_Bmp BMP_SliceBmp(BMP_Bmp* source_bmp, int x_offset, int y_offset, int slice_width, int slice_height)
{
	BMP_Bmp sliced_bmp = BMP_SliceBmp(source_bmp->pixels, source_bmp->width, source_bmp->height,
									  x_offset, y_offset, slice_width, slice_height);

	return sliced_bmp;
}

static int BMP_BatchSliceBmpAndWriteToSeperateFiles(char* file_base_name,
													void* source_bmp_pixels, int source_width, int source_height,
													int slice_width, int slice_height, int max_digit_count)
{
	int slice_count = 0;
	for (int y_source = 0; y_source < source_height; y_source += slice_height) {
		for (int x_source = 0; x_source < source_width; x_source += slice_width) {

			if (!BMP_IsBmpSliceEmpty(source_bmp_pixels, source_width, source_height,
									 x_source, y_source, slice_width, slice_height)) {
				
				BMP_Bmp sliced_bmp = BMP_SliceBmp(source_bmp_pixels, source_width, source_height,
												  x_source, y_source, slice_width, slice_height);

				int slice_count_digit_count = BMP_GetDigitCount(slice_count);
				int rest_digit_count = max_digit_count - slice_count_digit_count;

				int buffer_index = 0;
				char file_name_buffer[256] = {};
				char* file_name = file_base_name;
				while (*file_name) { file_name_buffer[buffer_index++] = *file_name++; }

				for (int r_digit = rest_digit_count - 1; r_digit >= 0; --r_digit) {
					file_name_buffer[buffer_index++] = '0';
				}

				for (int s_digit = slice_count_digit_count - 1; s_digit >= 0 ; --s_digit) {
					char digit = (char)(BMP_GetDigitN(slice_count, s_digit) + '0');
					file_name_buffer[buffer_index++] = digit;
				}

				file_name_buffer[buffer_index++] = '.';
				file_name_buffer[buffer_index++] = 'b';
				file_name_buffer[buffer_index++] = 'm';
				file_name_buffer[buffer_index++] = 'p';

				BMP_CreateBmp(file_name_buffer, sliced_bmp.pixels, sliced_bmp.width, sliced_bmp.height);
				free(sliced_bmp.pixels);

				++slice_count;
			}
		}
	}

	return slice_count;
}

static int BMP_BatchSliceBmpAndWriteToSeperateFiles(char* file_base_name,
													BMP_Bmp* source_bmp,
													int slice_width, int slice_height, int max_digit_count)
{
	int slice_count = BMP_BatchSliceBmpAndWriteToSeperateFiles(file_base_name,
															   source_bmp->pixels, source_bmp->width, source_bmp->height,
															   slice_width, slice_height, max_digit_count);
}

static int BMP_FindLeastSignificantSetBit(int value)
{
	int index = 0;

	for (int i = 0; i < 32; ++i) {
		if ((value >> i) & 1) {
			index = i;
			break;
		}
	}

	return index;
}

static int BMP_GetDigitCount(int number)
{
	int result = 0;
	while (++result && (number = number / 10)) {}

	return result;
}

static int BMP_GetDigitN(int number, int digit_n)
{
	int result = 0;

	for (int n = 0; n < digit_n; ++n) {
		number /= 10;
	}

	result = number % 10;

	return result;
}

#define BMP_BMP_HEADER
#endif
