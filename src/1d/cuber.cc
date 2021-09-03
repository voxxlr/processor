#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>
//#define PNG_DEBUG 1

#include "png.h"

#include "jpeglib.h"

#include "cuber.h"

struct jpeg_error
{
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct jpeg_error * my_error_ptr;

METHODDEF(void) jpeg_exit(j_common_ptr lInfo)
{
	struct jpeg_error * lError = (struct jpeg_error *)lInfo->err;
	(*lInfo->err->output_message) (lInfo);
	longjmp(lError->setjmp_buffer, 1);
}


class Level
{
	static const char* FRONT;
	static const char* BACK;
	static const char* LEFT;
	static const char* RIGHT;
	static const char* UP;
	static const char* DOWN;

	static const char* FACES[6];
	static const int KERNEL = 5;
	static float sKernel[KERNEL][KERNEL];

	public:

		static const uint16_t TILE_SIZE = 256;

		Level(uint8_t iZ)
		: mParent(0)
		, mZ(iZ)
		, mEdge((uint32_t)(pow(2, iZ)*TILE_SIZE))
		{
			if (iZ)
			{
				mParent = new Level(iZ - 1);
			}
		}

		void process(uint32_t iImageHeight, unsigned char* iBuffer)
		{
			std::string lDir(std::to_string((boost::int64_t)mZ));

			boost::filesystem::create_directory(boost::filesystem::path(lDir.c_str()));
			chdir(lDir.c_str());

			uint16_t lEdge = (uint16_t)pow(2, mZ);
			for (int f = 0; f < 6; f++)
			{
				for (int ty = 0; ty < lEdge; ty++)
				{
					for (int tx = 0; tx < lEdge; tx++)
					{
						createTile(tx, ty, iImageHeight, iBuffer, FACES[f]);
					}
				}
			};

			chdir("..");

			if (mParent)
			{
				uint32_t lImageHeight = iImageHeight / 2.0 + 0.5;
				uint32_t lImageWidth = 2 * lImageHeight;
				BOOST_LOG_TRIVIAL(info) << "Doensampling to " << lImageWidth << ", " << lImageHeight;

				unsigned char* lBuffer = new unsigned char[lImageWidth* lImageHeight *3];

				float dx = 1.0 / lImageWidth;
				float dy = 1.0 / lImageHeight;
				uint32_t iImageWidth = iImageHeight * 2;
				for (uint32_t y = 0; y < lImageHeight; y++)
				{
					uint32_t ly = iImageHeight*dy*y + 0.5;
					for (uint32_t x = 0; x < lImageWidth; x++)
					{
						uint32_t lx = iImageWidth*dx*x + 0.5;

						float lR = 0;
						float lG = 0;
						float lB = 0;
						for (int ky=-2; ky<3; ky++)
						{
							for (int kx=-2; kx<3; kx++)
							{
								uint32_t lP = (ly+ky+iImageHeight)% iImageHeight * iImageWidth + (lx+kx+iImageWidth)% iImageWidth;

								lR += sKernel[kx+2][ky+2] * iBuffer[lP*3+0];
								lG += sKernel[kx+2][ky+2] * iBuffer[lP*3+1];
								lB += sKernel[kx+2][ky+2] * iBuffer[lP*3+2];
							}
						}
	
						lBuffer[(y*lImageWidth + x) * 3 + 0] = ((int)(lR+0.5))%256;
						lBuffer[(y*lImageWidth + x) * 3 + 1] = ((int)(lG+0.5))%256;
						lBuffer[(y*lImageWidth + x) * 3 + 2] = ((int)(lB+0.5))%256;
					}
				}

				delete[] iBuffer;

				// downsample here
				mParent->process(lImageHeight, lBuffer);
			}
		}

	private:

		void createTile(uint32_t iTx, uint32_t iTy, uint32_t iImageHeight, unsigned char* iBuffer, const char* iFace)
		{
			unsigned char lImagePixels[TILE_SIZE * TILE_SIZE * 3];

			for (int y = 0; y < TILE_SIZE; y++)
			{
				for (int x = 0; x < TILE_SIZE; x++)
				{
					double lVx = 0;
					double lVy = 0;
					double lVz = 0;

					double a = (2.0f*(iTx*TILE_SIZE +x)) / mEdge - 1.0f;
					double b = (2.0f*(iTy*TILE_SIZE +y)) / mEdge - 1.0f;

					if (iFace == Level::BACK)
					{
						lVx = -1.0;
						lVy = -a;
						lVz = -b;
					}
					else if (iFace == Level::LEFT)
					{
						lVx = a;
						lVy = -1.0;
						lVz = -b;
					}
					else if (iFace == Level::FRONT)
					{
						lVx = 1.0;
						lVy = a;
						lVz = -b;
					}
					else if (iFace == Level::RIGHT)
					{
						lVx = -a;
						lVy = 1.0;
						lVz = -b;
					}
					else if (iFace == Level::UP)
					{
						lVx = b;
						lVy = a;
						lVz = 1.0;
					}
					else if (iFace == Level::DOWN)
					{
						lVx = -b;
						lVy = a;
						lVz = -1.0;
					}

					double theta = atan2(lVy, lVx); // range -pi to pi
					double r = hypot(lVx, lVy);
					double phi = atan2(lVz, r); // range -pi/2 to pi/2

											   // source img coords
					double uf = 0.5 *2* iImageHeight * (theta + M_PI) / M_PI;
					double vf = 0.5 *2* iImageHeight * (M_PI_2 - phi) / M_PI;

					uint32_t us = (uint32_t)uf;
					uint32_t vs = (uint32_t)vf;

					us %= 2*iImageHeight;
					vs %= iImageHeight;

					lImagePixels[(y*TILE_SIZE + x) * 3 + 0] = iBuffer[(vs* 2*iImageHeight + us) * 3 + 0];
					lImagePixels[(y*TILE_SIZE + x) * 3 + 1] = iBuffer[(vs* 2*iImageHeight + us) * 3 + 1];
					lImagePixels[(y*TILE_SIZE + x) * 3 + 2] = iBuffer[(vs* 2*iImageHeight + us) * 3 + 2];
				}
			}

			//saveJpg(iFace + std::to_string((boost::int64_t)iTx) + "_" + std::to_string((boost::int64_t)iTy), lImagePixels);
			savePng(iFace + std::to_string((boost::int64_t)iTx) + "_" + std::to_string((boost::int64_t)iTy), lImagePixels);

		}


		void savePng(std::string iName, unsigned char* iImagePixels)
		{
			std::string lName = iName + ".png";
			BOOST_LOG_TRIVIAL(info) << lName;

			png_bytep lImageRows[TILE_SIZE];
			for (int i = 0; i<TILE_SIZE; i++)
			{
				lImageRows[i] = iImagePixels + i * TILE_SIZE * 3;
			}

			FILE* lFile = fopen(lName.c_str(), "wb");

			png_structp lPointer = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			png_infop lInfo = png_create_info_struct(lPointer);
			if (setjmp(png_jmpbuf(lPointer)))
			{
				BOOST_LOG_TRIVIAL(info) << "png_jmpbuf (lPointer) failed ";
				exit(-1);
			}
			png_init_io(lPointer, lFile);
			png_set_IHDR(lPointer, lInfo, TILE_SIZE, TILE_SIZE, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			png_set_rows(lPointer, lInfo, lImageRows);
			png_write_png(lPointer, lInfo, PNG_TRANSFORM_IDENTITY, NULL);
			png_destroy_write_struct(&lPointer, &lInfo);
			fclose(lFile);

		}

		void saveJpg(std::string iName, unsigned char* iImagePixels)
		{
			std::string lName = iName + ".jpg";
			BOOST_LOG_TRIVIAL(info) << lName;


			struct jpeg_compress_struct lInfo;
			jpeg_create_compress(&lInfo);

			FILE* lFile = fopen(lName.c_str(), "wb");
			jpeg_stdio_dest(&lInfo, lFile);
			struct jpeg_error lError;
			lInfo.err = jpeg_std_error(&lError.pub);
			lInfo.image_width = TILE_SIZE;
			lInfo.image_height = TILE_SIZE;
			lInfo.input_components = 3;
			lInfo.in_color_space = JCS_RGB;
			jpeg_set_defaults(&lInfo);
			jpeg_set_quality(&lInfo, 75, TRUE);
			jpeg_start_compress(&lInfo, TRUE);

			JSAMPROW lImageRows[TILE_SIZE];
			for (int i = 0; i<TILE_SIZE; i++)
			{
				lImageRows[i] = iImagePixels + i * TILE_SIZE * 3;
			}
			jpeg_write_scanlines(&lInfo, lImageRows, TILE_SIZE);

			jpeg_finish_compress(&lInfo);
			fclose(lFile);
			jpeg_destroy_compress(&lInfo);
		}

		Level* mParent;
		uint8_t mZ;
		uint32_t mEdge;
};

const char* Level::FRONT = "f";
const char* Level::BACK = "b";
const char* Level::LEFT = "l";
const char* Level::RIGHT = "r";
const char* Level::UP = "u";
const char* Level::DOWN = "d";

const char* Level::FACES[6] = { Level::FRONT, Level::BACK, Level::LEFT, Level::RIGHT, Level::UP, Level::DOWN };

float Level::sKernel[5][5] = { { 1 / 273.0,4 / 273.0,7 / 273.0,4 / 273.0,1 / 273.0 },
							 { 4 / 273.0,16 / 273.0,26 / 273.0,16 / 273.0,4 / 273.0 },
							 { 7 / 273.0,26 / 273.0,41 / 273.0,26 / 273.0,7 / 273.0 },
							 { 4 / 273.0,16 / 273.0,26 / 273.0,16 / 273.0,4 / 273.0 },
						 	 { 1 / 273.0,4 / 273.0,7 / 273.0,4 / 273.0,1 / 273.0 } };


Cuber::Cuber()
{
};


void Cuber::process(std::string iFile)
{
	FILE* lFile = fopen(iFile.c_str(), "rb");

	unsigned char* lImage = 0;
	uint32_t lWidth;
	uint32_t lHeight;

	std::string lExt = boost::algorithm::to_lower_copy(iFile.substr(iFile.find_last_of(".") + 1));
	if (lExt == "jpg" || lExt == "jpeg")
	{
		/* We set up the normal JPEG error routines, then override error_exit. */
		struct jpeg_decompress_struct lInfo;
		struct jpeg_error lError;
		lInfo.err = jpeg_std_error(&lError.pub);
		lError.pub.error_exit = jpeg_exit;
		if (setjmp(lError.setjmp_buffer)) 
		{
			jpeg_destroy_decompress(&lInfo);
			fclose(lFile);
			exit(0);
		}

		/* Now we can initialize the JPEG decompression object. */
		jpeg_create_decompress(&lInfo);
		jpeg_stdio_src(&lInfo, lFile);
		jpeg_read_header(&lInfo, TRUE);
		jpeg_start_decompress(&lInfo);
		lWidth = lInfo.output_width;
		lHeight = lInfo.output_height;

		lImage = new unsigned char[lInfo.output_width * lInfo.output_height * 3];
		unsigned char* lLine = new unsigned char[lInfo.output_width * lInfo.output_components];
		uint32_t lY = 0;
		while (lInfo.output_scanline < lInfo.output_height)
		{
			jpeg_read_scanlines(&lInfo, &lLine, 1);
			for (size_t i = 0; i < lInfo.output_width; i++)
			{
				lImage[(lY*lInfo.output_width + i) * 3 + 0] = lLine[i*lInfo.output_components + 0];
				lImage[(lY*lInfo.output_width + i) * 3 + 1] = lLine[i*lInfo.output_components + 1];
				lImage[(lY*lInfo.output_width + i) * 3 + 2] = lLine[i*lInfo.output_components + 2];
			}
			lY++;
		}
		delete[]lLine;
	}
	else
	{
		/*
		unsigned char lHeader[8];
		fread(lHeader, 1, 8, lFile);
		if (png_sig_cmp(lHeader, 0, 8))
		{
			BOOST_LOG_TRIVIAL(error) << "File is not recognized as a PNG file:" << iFile;
			exit(0);
		}
			
		png_structp lPointer = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		png_infop lInfo = png_create_info_struct(lPointer);
	
		setjmp(png_jmpbuf(lPointer));
			
		png_init_io(lPointer, lFile);
		png_set_sig_bytes(lPointer, 8);
		png_read_info(lPointer, lInfo);

		uint32_t lDepth;
		uint32_t lType;
		uint32_t lInterlace;

		png_get_IHDR(lPointer, lInfo, &lWidth, &lHeight, &lDepth, &lType, &lInterlace, NULL, NULL);
		png_set_scale_16(lPointer);
		png_set_strip_alpha(lPointer);
		
		number_of_passes = png_set_interlace_handling(lPointer);
		png_read_update_info(lPointer, lInfo);


		lImage = new unsigned char[lWidth * lHeight * 3];
		uint32_t lY = 0;
		while (lInfo.output_scanline < lInfo.output_height)
		{
			jpeg_read_scanlines(&lInfo, &lLine, 1);
			for (int i = 0; i < lInfo.output_width; i++)
			{
				lImage[(lY*lWidth + i) * 3 + 0] = lLine[i*lInfo.output_components + 0];
				lImage[(lY*lWidth + i) * 3 + 1] = lLine[i*lInfo.output_components + 1];
				lImage[(lY*lWidth + i) * 3 + 2] = lLine[i*lInfo.output_components + 2];
			}
			lY++;
		}


		png_bytep* lRows = (png_bytep*)malloc(sizeof(png_bytep) * lHeight);
		for (int i = 0; i<TILE_SIZE; i++)
		{
			lImageRows[i] = iImagePixels + i * TILE_SIZE * 3;
		}
		jpeg_write_scanlines(&lInfo, lImageRows, TILE_SIZE);

		for (y = 0; y<height; y++)
			lRows[y] = (png_byte*)malloc(png_get_rowbytes(png_ptr, info_ptr));

		png_read_image(png_ptr, row_pointers);

		fclose(fp);
		*/
	}

	boost::filesystem::create_directory(boost::filesystem::path("root"));
	chdir("root");

	uint32_t lw = log(lWidth/4 - 1) / log(2.) + 1.;
	uint32_t lt = log(Level::TILE_SIZE) / log(2.);
	uint16_t lMaxZ = std::max<long>(0, lw-lt);
	Level lLevel(lMaxZ);

	lLevel.process(lHeight, lImage);

	mRoot["maxZoom"] = lMaxZ;
	mRoot["tileSize"] = Level::TILE_SIZE;
	mRoot["format"] = "png";

	chdir("..");
};
