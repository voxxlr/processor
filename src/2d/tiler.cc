#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

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

//#include "half.h"

#include "tiler.h"

PngImage::PngImage(std::string iFile, uint32_t iWidth, uint32_t iHeight)
: mWidth(iWidth)
, mHeight(iHeight)
{
	std::string lName = iFile + ".png";
	mFile = fopen(lName.c_str(), "wb");
	mPointer = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	mInfo = png_create_info_struct(mPointer);
	if (setjmp (png_jmpbuf (mPointer))) 
	{
		BOOST_LOG_TRIVIAL(info) << "png_jmpbuf (lPointer) failed ";			
		exit(-1);
	}
	png_init_io(mPointer, mFile);
	//png_set_IHDR(mPointer, mInfo, iWidth, iHeight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	//png_set_IHDR(lPointer, lInfo, lColor.mImageW, lColor.mImageH, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	//png_write_info(mPointer, mInfo);
}

PngImage::~PngImage()
{
	png_destroy_write_struct(&mPointer, &mInfo);
	fclose(mFile);
}


PngRgbImage::PngRgbImage(std::string iFile, uint32_t iWidth, uint32_t iHeight)
: PngImage(iFile, iWidth, iHeight)
{
	png_set_IHDR(mPointer, mInfo, mWidth, mHeight, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(mPointer, mInfo);
}

void PngRgbImage::writeRow(png_bytep lLine)
{
	png_write_row(mPointer, lLine);
}


PngFloatImage::PngFloatImage(std::string iFile, uint32_t iWidth, uint32_t iHeight, double iMin, double iMax)
: PngImage(iFile, iWidth, iHeight)
, mLine(new uint16[iWidth])
, mMin(iMin)
, mMax(iMax)
{
	png_set_IHDR(mPointer, mInfo, mWidth, mHeight, 16, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_write_info(mPointer, mInfo);
}

PngFloatImage::~PngFloatImage()
{
	delete [] mLine;
}

void PngFloatImage::writeRow(png_bytep iLine)
{
	float* lLine = (float*)iLine;
	for (int i=0; i<mWidth; i++)
	{
		mLine[i] = (uint16)((lLine[i]-mMin)/(mMax-mMin)*std::numeric_limits<uint16>::max());
	}
	png_write_row(mPointer, (png_bytep)mLine);
}


















PngWriter::PngWriter(unsigned char iValues[256*256*3])
: mImageRows(new png_bytep[256])
{
	for (int i=0; i<256; i++)
	{
		mImageRows[i] = iValues + i*256*3;
	}
}

PngWriter::~PngWriter()
{
	delete [] mImageRows;
}

void PngWriter::write(std::string iName)
{
	std::string lName = iName + ".png";
	FILE* lFile = fopen(lName.c_str(), "wb");

	png_structp lPointer = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop lInfo = png_create_info_struct(lPointer);
	if (setjmp (png_jmpbuf (lPointer))) 
	{
		BOOST_LOG_TRIVIAL(info) << "png_jmpbuf (lPointer) failed ";			
		exit(-1);
	}
	png_init_io(lPointer, lFile);
	png_set_IHDR(lPointer, lInfo, 256, 256, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_rows (lPointer, lInfo, mImageRows);
	png_write_png (lPointer, lInfo, PNG_TRANSFORM_IDENTITY, NULL);
	png_destroy_write_struct(&lPointer, &lInfo);

	fclose(lFile);
}


#define UTM_HEIGHT 10000000


TifFile::TifFile(std::string iFile)
: mFile(XTIFFOpen(iFile.c_str(), "r"))
{
	TIFFGetField(mFile, TIFFTAG_IMAGEWIDTH, &mImageW);
	TIFFGetField(mFile, TIFFTAG_IMAGELENGTH, &mImageH);
	TIFFGetField(mFile, TIFFTAG_SAMPLESPERPIXEL, &mSamplesPerPixel);
	TIFFGetField(mFile, TIFFTAG_PLANARCONFIG, &mConfig);
	TIFFGetField(mFile, TIFFTAG_BITSPERSAMPLE, &mBitsPerSample);
	TIFFGetField(mFile, TIFFTAG_SAMPLEFORMAT, &mFormat);

	mBytesPerPixel = mSamplesPerPixel*mBitsPerSample/8;

	BOOST_LOG_TRIVIAL(info)  << "File " << iFile << " (" << mImageW << "," << mImageH << ") channels " << mSamplesPerPixel << " bits " << mBitsPerSample;
	
	if (TIFFIsTiled(mFile))
	{
		mReader = new TifTiledFormat(*this);
 	}
	else
	{
		mReader = new TifStripedFormat(*this);
	}

	GTIF* lGTiff = GTIFNew(mFile);
//	GTIFPrint(lGTiff, 0, 0);
	GTIFDefn lDefinition;
	if (GTIFGetDefn(lGTiff, &lDefinition))
	{
		GTIFPrintDefn(&lDefinition, stdout);
			
		//lLat = 41.85;
		//lLon = -87.64999999999998;

		// upper left
		Point& lP0 = mInfo.p0;
		lP0.utmX = 0;
		lP0.utmY = 0;
		GTIFImageToPCS(lGTiff, &lP0.utmX, &lP0.utmY);
		lP0.lon = lP0.utmX;
		lP0.lat = lP0.utmY;
 		GTIFProj4ToLatLong(&lDefinition, 1, &lP0.lon, &lP0.lat);
		  
		// lower right
		Point& lP1 = mInfo.p1;
		lP1.utmX = mImageW;
		lP1.utmY = mImageH;
		GTIFImageToPCS(lGTiff, &lP1.utmX, &lP1.utmY);
		lP1.lon = lP1.utmX;
		lP1.lat = lP1.utmY;
		GTIFProj4ToLatLong(&lDefinition, 1, &lP1.lon, &lP1.lat);
	
		mInfo.worldPixelW = (lP1.utmX - lP0.utmX)/mImageW;
		mInfo.worldPixelH = (lP0.utmY - lP1.utmY)/mImageH;

		mInfo.maxZoom = ceil(log(UTM_HEIGHT/(256*mInfo.worldPixelH))/log(2.));  
		mInfo.minZoom = mInfo.maxZoom - ceil(log(std::max(mImageW/256.0, mImageH/256.0)/log(2.)));
		mInfo.mapPixelS = UTM_HEIGHT/(256*pow(2,mInfo.maxZoom));

		lP0.mapX = lP0.utmX/UTM_HEIGHT*256;
		lP0.mapY = (UTM_HEIGHT-lP0.utmY)/UTM_HEIGHT*256;
		lP1.mapX = lP1.utmX/UTM_HEIGHT*256;
		lP1.mapY = (UTM_HEIGHT-lP1.utmY)/UTM_HEIGHT*256;

		//std::cout << lP1.mapY - lP0.mapY << "\0";

		std::string lString = GTIFGetProj4Defn(&lDefinition);
		size_t lIndex = lString.find("latlong");
		if (lIndex != std::string::npos)
		{
			lString.replace(lIndex, 7, "longlat");
		}
		mInfo.proj = lString;
		std::cout << lString << "\n";
		//lProj["def"] = lString;
	}
	else
	{
		mInfo.worldPixelW = 1;
		mInfo.worldPixelH = 1;
		mInfo.mapPixelS = 1;

		double d = std::max(mImageW, mImageH);
		mInfo.maxZoom = ceil(log(d/256)/log(2.));  
		mInfo.minZoom = std::min<uint16>(mInfo.maxZoom, 2);

		Point& lP0 = mInfo.p0;
		lP0.mapX = 0;
		lP0.mapY = 0;
		Point& lP1 = mInfo.p1;
		lP1.mapX = mImageW/d*256;
		lP1.mapY = mImageH/d*256;

		lP0.utmX = lP0.mapX;
		lP0.utmY = lP0.mapY;
		lP1.utmX = lP1.mapX;
		lP1.utmY = lP1.mapY;

		mInfo.proj = "undef";
	}
};

TifFile::~TifFile()
{
	TIFFClose(mFile);
}


void TifFile::readLine(uint32_t iLine, uint8* iBuffer)
{
	mReader->readLine(iLine, iBuffer);
};



TifTiledFormat::TifTiledFormat(TifFile& iFile)
: mFile(iFile)
, mTileY(-1)
{
	TIFFGetField(iFile.mFile, TIFFTAG_TILEWIDTH, &mTileW);
	TIFFGetField(iFile.mFile, TIFFTAG_TILELENGTH, &mTileH);
	
	mBuffer = new tdata_t[iFile.mImageW/mTileW+1];
	for (uint32_t i = 0; i < iFile.mImageW/mTileW+1; i++)
	{
		mBuffer[i] = _TIFFmalloc(TIFFTileSize(iFile.mFile));
	}
}

TifTiledFormat::~TifTiledFormat()
{
	for (uint32_t i = 0; i < mFile.mImageW/mTileW+1; i++)
	{
		_TIFFfree(mBuffer[i]);
	}
	delete [] mBuffer;
}

void TifTiledFormat::readLine(uint32_t iLine, uint8* iBuffer)
{
	if (mTileY != iLine/mTileH)
	{
		mTileY = iLine/mTileH;
		// read tile row
  		for (uint32_t x = 0; x < mFile.mImageW; x += mTileW)
		{
			//BOOST_LOG_TRIVIAL(info) << x/lTileW << "," << y/lTileH;
	  		TIFFReadTile(mFile.mFile, mBuffer[x/mTileW], x, mTileY*mTileH, 0, 0);
		}
	}

	uint32_t lTileI = iLine%mTileH;

	for (int x=0; x<mFile.mImageW; x++)
	{
		tdata_t lTile = mBuffer[x/mTileW];
		int lPx = x%mTileW;

		for (int i=0; i<mFile.mBytesPerPixel; i++)
		{
			iBuffer[x*mFile.mBytesPerPixel + i] = ((uint8*)lTile)[(lTileI*mTileW+lPx)*mFile.mBytesPerPixel+i];
		}
	}
}





TifStripedFormat::TifStripedFormat(TifFile& iFile)
: mFile(iFile)
{
	mBuffer = _TIFFmalloc(mFile.mImageW*mFile.mBytesPerPixel);
}

TifStripedFormat::~TifStripedFormat()
{
	_TIFFfree(mBuffer);
}

void TifStripedFormat::readLine(uint32_t iLine, uint8* iBuffer)
{
	TIFFReadScanline (mFile.mFile, mBuffer, iLine);

	for (int x=0; x<mFile.mImageW; x++)
	{
		for (int i=0; i<mFile.mBytesPerPixel; i++)
		{
			iBuffer[x*mFile.mBytesPerPixel + i] = ((uint8*)mBuffer)[x*mFile.mBytesPerPixel+i];
		}
	}
}









template <typename T, int S> class ImageWindow
{
	static const int KERNEL = 5;
	static float sKernel[KERNEL][KERNEL];

	public: 

		ImageWindow(uint32_t iImageWidth, uint32_t iImageHeight, T iInit)
		: mWidth(iImageWidth)
		, mInputLine(new T[iImageWidth*S])
		, mInit(iInit)
		, mPngImage(0)
		{
			for (int i=0; i<KERNEL; i++)
			{
				uint32_t lWidth = (mWidth+KERNEL)*S;
				mWindow[i] = new T[lWidth];
				for (int x=0; x<lWidth; x++)
				{
					mWindow[i][x] = mInit;
				}
			}
			// smooth lines
			mSmoothedLine[0] = new T[mWidth*S];
			mSmoothedLine[1] = new T[mWidth*S];

			clearLine();

			//mPngImage = new PngFloatImage(std::to_string((boost::int64_t)iImageWidth), iImageWidth, iImageHeight, 48.6313, 103.142);
			//mPngImage = new PngRgbImage(std::to_string((boost::int64_t)iImageWidth), iImageWidth, iImageHeight);

		}

		~ImageWindow()
		{
			delete [] mSmoothedLine[0];
			delete [] mSmoothedLine[1];
			delete [] mInputLine;
			for (int i=0; i<KERNEL; i++)
			{
				delete mWindow[i];
			}

			if (mPngImage)
			{
				delete mPngImage;
			}
		}

		void processLine(uint32_t iLineIndex)
		{
			// rotate scan line buffer
			T* lWindow = mWindow[KERNEL-1];
			for (int i=KERNEL-1; i>0; i--)
			{
				mWindow[i] = mWindow[i-1];
			}
			mWindow[0] = lWindow;

			// read scanline into window 0
			memcpy(lWindow+(KERNEL/2)*S,  mInputLine, mWidth*S*sizeof(T));
			// extend edges
			for (int i=0; i<KERNEL/2; i++)
			{
				uint32_t lIndex = KERNEL/2;
				for (int s=0; s<S; s++)
				{
					lWindow[i*S+s] = lWindow[lIndex*S+s];
				}
				lIndex += mWidth -1;
				for (int s=0; s<S; s++)
				{
					lWindow[(lIndex+i+1)*S+s] = lWindow[lIndex*S+s];
				}
			}

			// smooth window into Line
			T* lLine = mSmoothedLine[iLineIndex%2];
			for (int i=0; i<mWidth; i++)
			{
				smoothWindow(lLine, i);
			}

			if (mPngImage)
			{
				mPngImage->writeRow((png_bytep)lLine);
			}
		}
		
		void sampleLine(ImageWindow<T,S>& iWindow);

		void clearLine()
		{
			for (int x=0; x<mWidth*S; x++)
			{
				mInputLine[x] = mInit;
			}
		}


		T* mInputLine;
		T mInit;
		uint32_t mWidth;

	private:

		void smoothWindow(T* iLine, uint32_t iY);
		bool isValid(T* iPixel);

		PngImage* mPngImage;


		T* mSmoothedLine[2];
		T* mWindow[KERNEL];
		float mChannel[S];
};

template <typename T, int S>
float ImageWindow<T,S>::sKernel[5][5] = {{1/273.0,4/273.0,7/273.0,4/273.0,1/273.0},
										{4/273.0,16/273.0,26/273.0,16/273.0,4/273.0},
										{7/273.0,26/273.0,41/273.0,26/273.0,7/273.0},
										{4/273.0,16/273.0,26/273.0,16/273.0,4/273.0},
										{1/273.0,4/273.0,7/273.0,4/273.0,1/273.0}};

template <> bool ImageWindow<unsigned char, 3>::isValid(unsigned char* iPixel)
{
	return iPixel[0] != 255 || iPixel[1] != 255 || iPixel[2] != 255;
}

template <> void ImageWindow<unsigned char, 3>::sampleLine(ImageWindow<unsigned char, 3>& iWindow)
{
	for (int i=0; i<mWidth; i++)
	{
		uint8 lCount = 0;
		float lR = 0;
		float lG = 0;
		float lB = 0;

		if (isValid(&iWindow.mSmoothedLine[0][(i*2+0)*3]))
		{
			lR += iWindow.mSmoothedLine[0][(i*2+0)*3+0];
			lG += iWindow.mSmoothedLine[0][(i*2+0)*3+1];
			lB += iWindow.mSmoothedLine[0][(i*2+0)*3+2];
			lCount++;
		}
		if (isValid(&iWindow.mSmoothedLine[0][(i*2+1)*3]))
		{
			lR += iWindow.mSmoothedLine[0][(i*2+1)*3+0];
			lG += iWindow.mSmoothedLine[0][(i*2+1)*3+1];
			lB += iWindow.mSmoothedLine[0][(i*2+1)*3+2];
			lCount++;
		}

		if (isValid(&iWindow.mSmoothedLine[1][(i*2+0)*3]))
		{
			lR += iWindow.mSmoothedLine[1][(i*2+0)*3+0];
			lG += iWindow.mSmoothedLine[1][(i*2+0)*3+1];
			lB += iWindow.mSmoothedLine[1][(i*2+0)*3+2];
			lCount++;
		}
		if (isValid(&iWindow.mSmoothedLine[1][(i*2+1)*3]))
		{
			lR += iWindow.mSmoothedLine[1][(i*2+1)*3+0];
			lG += iWindow.mSmoothedLine[1][(i*2+1)*3+1];
			lB += iWindow.mSmoothedLine[1][(i*2+1)*3+2];
			lCount++;
		}

		if (lCount)
		{
			mInputLine[i*3+0] = lR/lCount;
			mInputLine[i*3+1] = lG/lCount;
			mInputLine[i*3+2] = lB/lCount;
		}
		else
		{
			mInputLine[i*3+0] = 255;
			mInputLine[i*3+1] = 255;
			mInputLine[i*3+2] = 255;
		}
	}
}


template <> bool ImageWindow<float, 1>::isValid(float* iPixel)
{
	return iPixel[0] != ImageYDepth::NOVALUE_VOXXLR;
}

template <> void ImageWindow<float, 1>::sampleLine(ImageWindow<float, 1>& iWindow)
{
	for (int i=0; i<mWidth; i++)
	{
		uint8 lCount = 0;
		float lValue = 0;

		if (isValid(&iWindow.mSmoothedLine[0][i*2]))
		{
			lValue += iWindow.mSmoothedLine[0][i*2];
			lCount++;
		}
		if (isValid(&iWindow.mSmoothedLine[0][i*2+1]))
		{
			lValue += iWindow.mSmoothedLine[0][i*2+1];
			lCount++;
		}

		if (isValid(&iWindow.mSmoothedLine[1][i*2]))
		{
			lValue += iWindow.mSmoothedLine[1][i*2];
			lCount++;
		}
		if (isValid(&iWindow.mSmoothedLine[1][i*2+1]))
		{
			lValue += iWindow.mSmoothedLine[1][i*2+1];
			lCount++;
		}

		if (lCount)
		{
			mInputLine[i] = lValue/lCount;
		}
		else
		{
			mInputLine[i] = ImageYDepth::NOVALUE_VOXXLR;
		}
	}
}


template <typename T, int S> void ImageWindow<T, S>::smoothWindow(T* iLine, uint32_t iX)
{
	// center pixel
	T lCenter[S];
	for (int s=0; s<S; s++)
	{
		lCenter[s] = mWindow[2][(iX+2)*S+s];
	}
	
	if (isValid(lCenter))
	{
		memset(mChannel, 0, sizeof(mChannel));

		for (int y=0; y<5; y++)
		{
			for (int x=0; x<5; x++)
			{
				int kx = iX+x;

				T lPixel[S];
				for (int s=0; s<S; s++)
				{
					lPixel[s] = mWindow[y][kx*S+s];
				}

				if (isValid(lPixel))
				{
					for (int s=0; s<S; s++)
					{
						mChannel[s] += sKernel[y][x]*lPixel[s];
					}
				}
				else
				{
					for (int s=0; s<S; s++)
					{
						mChannel[s] += sKernel[y][x]*lCenter[s];
					}
				}
			}
		}

		for (int s=0; s<S; s++)
		{
			iLine[iX*S+s] = (T)mChannel[s];
		}
	}
	else
	{
		for (int s=0; s<S; s++)
		{
			iLine[iX*S+s] = lCenter[s];
		}
	}
};





float Sampler::depthFilter(float iPixel)
{
	iPixel = (iPixel == ImageYDepth::NOVALUE_AGISOFT) ? ImageYDepth::NOVALUE_VOXXLR : iPixel;
	iPixel = (iPixel <= ImageYDepth::NOVALUE_PIX4D) ? ImageYDepth::NOVALUE_VOXXLR : iPixel;
	if (iPixel != ImageYDepth::NOVALUE_VOXXLR)
	{
		ImageYDepth::sMin = std::min<float>(ImageYDepth::sMin, iPixel);
		ImageYDepth::sMax = std::max<float>(ImageYDepth::sMax, iPixel);
	}
	return iPixel;
};



UpSampler::UpSampler(TifFile& iFile)
: mFile(iFile)
, mPngImage(0)
{
	mScalarX = iFile.mInfo.mapPixelS/iFile.mInfo.worldPixelW;
	mScalarY = iFile.mInfo.mapPixelS/iFile.mInfo.worldPixelH;

	mImageW = iFile.mImageW/mScalarX;
	mImageH = iFile.mImageH/mScalarY;

	mCache[0].buffer = new uint8[iFile.mBytesPerPixel*iFile.mImageW];
	mCache[0].y = -1;
	mCache[1].buffer = new uint8[iFile.mBytesPerPixel*iFile.mImageW];
	mCache[1].y = -1;

	//mPngImage = new PngFloatImage("UpSampler", mImageW, mImageH);
};

UpSampler::~UpSampler()
{
	delete [] mCache[0].buffer;
	delete [] mCache[1].buffer;

	if (mPngImage)
	{
		delete mPngImage;
	}
}

uint8* UpSampler::getLine(int32 iY)
{
	for (int i=0; i<2; i++)
	{
		if (mCache[i].y == iY)
		{
			return mCache[i].buffer;
		}
	}

	mCache[0].y = mCache[1].y;
	std::swap(mCache[0].buffer, mCache[1].buffer);

	mFile.readLine(iY, mCache[1].buffer);
	mCache[1].y = iY;
	return mCache[1].buffer;
}

// for Colors
void UpSampler::readLine(ImageWindow<unsigned char, 3>& iWindow, int32 iY)
{
	double fY = iY*mScalarY;
	int32 lY0 = fY;
	int32 lY1 = std::min<int32>(lY0+1, mFile.mImageH-1);
	double lFractY = fY - lY0;

	uint8* lLine0 = getLine(lY0);
	uint8* lLine1 = getLine(lY1);

	#define LERP(s,e,t)  (s+(e-s)*t)

	#define CHANNEL(type, c) LERP(LERP((type&)(lLine0[lX0*mFile.mBytesPerPixel+c]), (type&)(lLine0[lX1*mFile.mBytesPerPixel+c]), lFractX), LERP((type&)(lLine1[lX0*mFile.mBytesPerPixel+c]), (type&)(lLine1[lX1*mFile.mBytesPerPixel+c]), lFractX), lFractY)
//	#define CHANNEL(type, c) (unsigned short&)&(lLine0[lX0*mFile.mBytesPerPixel+c])

 	for (int x=0; x<mImageW; x++)
	{
		double fX = x*mScalarX;
		int32 lX0 = fX;
		int32 lX1 = std::min<int32>(lX0+1, mFile.mImageW-1);
		double lFractX = fX - lX0;

		unsigned char r = 0;
		unsigned char g = 0;
		unsigned char b = 0;

		switch (mFile.mBitsPerSample)
		{
		case 8:
			r = CHANNEL(unsigned char, 0);//LERP(LERP(lLine0[lX0*mFile.mBytesPerPixel+0], lLine0[lX1*mFile.mBytesPerPixel+0], lFractX), LERP(lLine1[lX0*mFile.mBytesPerPixel+0], lLine1[lX1*mFile.mBytesPerPixel+0], lFractX), lFractY);
			g = CHANNEL(unsigned char, 1);//LERP(LERP(lLine0[lX0*mFile.mBytesPerPixel+1], lLine0[lX1*mFile.mBytesPerPixel+1], lFractX), LERP(lLine1[lX0*mFile.mBytesPerPixel+1], lLine1[lX1*mFile.mBytesPerPixel+1], lFractX), lFractY);
			b = CHANNEL(unsigned char, 2);//LERP(LERP(lLine0[lX0*mFile.mBytesPerPixel+2], lLine0[lX1*mFile.mBytesPerPixel+2], lFractX), LERP(lLine1[lX0*mFile.mBytesPerPixel+2], lLine1[lX1*mFile.mBytesPerPixel+2], lFractX), lFractY);
			break;

		case 16:
			unsigned short rs = CHANNEL(unsigned short, 0);
			unsigned short gs = CHANNEL(unsigned short, 2);
			unsigned short bs = CHANNEL(unsigned short, 4);

			r = rs/255;
			g = gs/255;
			b = bs/255;

			break;
		}


		if (r == 0 && g == 0 && b == 0)
		{
			r = 255;
			g = 255;
			b = 255;
		}

		iWindow.mInputLine[x*3+0] = r;
		iWindow.mInputLine[x*3+1] = g;
		iWindow.mInputLine[x*3+2] = b;


		/*
		unsigned char r = LERP(LERP(lLine0[lX0*mFile.mBytesPerPixel+0], lLine0[lX1*mFile.mBytesPerPixel+0], lFractX), LERP(lLine1[lX0*mFile.mBytesPerPixel+0], lLine1[lX1*mFile.mBytesPerPixel+0], lFractX), lFractY);
		unsigned char g = LERP(LERP(lLine0[lX0*mFile.mBytesPerPixel+1], lLine0[lX1*mFile.mBytesPerPixel+1], lFractX), LERP(lLine1[lX0*mFile.mBytesPerPixel+1], lLine1[lX1*mFile.mBytesPerPixel+1], lFractX), lFractY);
		unsigned char b = LERP(LERP(lLine0[lX0*mFile.mBytesPerPixel+2], lLine0[lX1*mFile.mBytesPerPixel+2], lFractX), LERP(lLine1[lX0*mFile.mBytesPerPixel+2], lLine1[lX1*mFile.mBytesPerPixel+2], lFractX), lFractY);

		if (r == 0 && g == 0 && b == 0)
		{
			r = 255;
			g = 255;
			b = 255;
		}

		iWindow.mInputLine[x*3+0] = r;
		iWindow.mInputLine[x*3+1] = g;
		iWindow.mInputLine[x*3+2] = b;
		*/
	}


	if (mPngImage)
	{
		mPngImage->writeRow(iWindow.mInputLine);
	}
	#undef LERP
}

// for Depth
void UpSampler::readLine(ImageWindow<float, 1>& iWindow, int32 iY)
{
	double fY = iY*mScalarY;
	int32 lY0 = fY;
	int32 lY1 = std::min<int32>(lY0+1, mFile.mImageH-1);
	double lFractY = fY - lY0;

	uint8* lLine0 = getLine(lY0);
	uint8* lLine1 = getLine(lY1);

	#define LERP(s,e,t)  (s+(e-s)*t)

 	for (int x=0; x<mImageW; x++)
	{
		double fX = x*mScalarX;
		int32 lX0 = fX;
		int32 lX1 = std::min<int32>(lX0+1, mFile.mImageW-1);
		double lFractX = fX - lX0;

        float c00 = depthFilter(*((float*)&lLine0[lX0*mFile.mBytesPerPixel]));
        float c10 = depthFilter(*((float*)&lLine0[lX1*mFile.mBytesPerPixel]));
        float c01 = depthFilter(*((float*)&lLine1[lX0*mFile.mBytesPerPixel]));
        float c11 = depthFilter(*((float*)&lLine1[lX1*mFile.mBytesPerPixel]));
				
		if (c00 == ImageYDepth::NOVALUE_VOXXLR ||
			c10 == ImageYDepth::NOVALUE_VOXXLR ||
			c01 == ImageYDepth::NOVALUE_VOXXLR ||
			c11 == ImageYDepth::NOVALUE_VOXXLR )
		{
			iWindow.mInputLine[x] = ImageYDepth::NOVALUE_VOXXLR;
		}
		else
		{
			iWindow.mInputLine[x] = LERP(LERP(c00, c10, lFractX), LERP(c01, c11, lFractX), lFractY);
		}
	}

	//	std::cout << ImageYDepth::sMin << "," << ImageYDepth::sMax << "\n";

	if (mPngImage)
	{
		mPngImage->writeRow((png_bytep)iWindow.mInputLine);
	}
	#undef LERP
}






NoSampler::NoSampler(TifFile& iFile)
: mFile(iFile)
{
	mImageW = iFile.mImageW;
	mImageH = iFile.mImageH;

	mBuffer = new uint8[iFile.mBytesPerPixel*iFile.mImageW];
};

NoSampler::~NoSampler()
{
	if (mBuffer)
	{
		delete [] mBuffer;
	}
}

// for Colors
void NoSampler::readLine(ImageWindow<unsigned char, 3>& iWindow, int32 iY)
{
	mFile.readLine(iY, mBuffer);

	for (int x=0; x<mImageW; x++)
	{
		unsigned char r = mBuffer[x*mFile.mBytesPerPixel+0];
		unsigned char g = mBuffer[x*mFile.mBytesPerPixel+1];
		unsigned char b = mBuffer[x*mFile.mBytesPerPixel+2];

		if (r == 0 && g == 0 && b == 0)
		{
			r = 255;
			g = 255;
			b = 255;
		}

		iWindow.mInputLine[x*3+0] = r;
		iWindow.mInputLine[x*3+1] = g;
		iWindow.mInputLine[x*3+2] = b;
	}

}

// for Depth
void NoSampler::readLine(ImageWindow<float, 1>& iWindow, int32 iY)
{
	mFile.readLine(iY, mBuffer);

	for (int x=0; x<mImageW; x++)
	{
		iWindow.mInputLine[x] = depthFilter(*(float*)&mBuffer[x*mFile.mBytesPerPixel]);
	}
}





ImageY::ImageY(uint32_t iZ, uint32_t iX, uint32_t iTileX, uint32_t iCount)
: mDirectoryX(std::to_string((boost::int64_t)iX))
, mDirectoryZ(std::to_string((boost::int64_t)iZ))
, mTileX(iTileX)
, mCount(iCount)
{
	boost::filesystem::create_directory(boost::filesystem::path(mDirectoryX.c_str()));
}



ImageYColor::ImageYColor(uint32_t iZ, uint32_t iX, uint32_t iTileX, uint32_t iLineX, uint32_t iCount, ImageWindow<unsigned char, 3>& iWindow)
: ImageY(iZ, iX, iTileX, iCount)
, mWindow(iWindow)
, mLine(&iWindow.mInputLine[iLineX*3])
, mWriter(mValues)
{
	clearImage();
}

void ImageYColor::processLine(uint32_t iLineY, uint32_t iTileY)
{
	if (iLineY > 0 && iLineY % 256 == 0)
	{
		uint32_t lTileY = iTileY + iLineY/256-1;
				
		chdir(mDirectoryZ.c_str());
		chdir(mDirectoryX.c_str());

		std::string lName = std::to_string((boost::int64_t)(lTileY));
		mWriter.write(lName);
		clearImage();

		chdir("..");
		chdir("..");
	}

	memcpy(&mValues[((iLineY%256)*256+mTileX)*3], mLine, mCount*3*sizeof(unsigned char));
}

void ImageYColor::clearImage()
{
	for (int i=0; i<256*256*3; i++)
	{
		mValues[i] = mWindow.mInit;
	}
}


/*
ImageYDepth::ImageYDepth(uint32_t iZ, uint32_t iX, uint32_t iTileX, uint32_t iLineX, uint32_t iCount, ImageWindow<float, 1>& iWindow)
: ImageY(iZ, iX, iTileX, iCount)
, mWindow(iWindow)
, mLine(&iWindow.mInputLine[iLineX])
, mLineX(iLineX)
{
	clearImage();
}

void ImageYDepth::processLine(uint32_t iLineY, uint32_t iTileY)
{
	if (iLineY > 0 && iLineY % 256 == 0)
	{
		uint32_t lTileY = iTileY + iLineY/256-1;
				
		chdir(mDirectoryZ.c_str());
		chdir(mDirectoryX.c_str());

		std::string lName = std::to_string((boost::int64_t)(lTileY)) + ".bin";
		FILE* lFile = fopen(lName.c_str(), "wb");
		fwrite(mValues, sizeof(float), 256*256, lFile);
		fclose(lFile);

		clearImage();

		chdir("..");
		chdir("..");
	}

	memcpy(&mValues[(((iLineY+iPixelY)%256)*256+mTileX)], mLine, mCount*sizeof(float));
}

void ImageYDepth::clearImage()
{
	for (int i=0; i<256*256; i++)
	{
		mValues[i] = mWindow.mInit;
	}
}

float ImageYDepth::sMin;
float ImageYDepth::sMax;

float ImageYDepth::NOVALUE_AGISOFT = -32767;
float ImageYDepth::NOVALUE_PIX4D = -10000.00;
float ImageYDepth::NOVALUE_VOXXLR = -10000.00;
*/


ImageYDepth::ImageYDepth(uint32_t iZ, uint32_t iX, uint32_t iTileX, int32 iLineX, uint32_t iCount, ImageWindow<float, 1>& iWindow)
: ImageY(iZ, iX, iTileX, iCount)
, mWindow(iWindow)
, mLineX(iLineX)
{
	for (int i=0; i<260*260; i++)
	{
		mValues[i] = NOVALUE_VOXXLR;
	}
}

void ImageYDepth::processLine(uint32_t iLineY, uint32_t iTileY)
{
	if (iLineY > 2 && (iLineY-2)%256 == 0)
	{
		uint32_t lTileY = iTileY + iLineY/256-1;
				
		chdir(mDirectoryZ.c_str());
		chdir(mDirectoryX.c_str());
		
		std::string lName = std::to_string((boost::int64_t)(lTileY)) + ".bin";
		FILE* lFile = fopen(lName.c_str(), "wb");
		fwrite(mValues, sizeof(float), 260*260, lFile);
		fclose(lFile);
		/*
		PngFloatImage lImage(std::to_string((boost::int64_t)(lTileY))+".png", 260, 260, 498.392, 664.311);
		for (int i=0; i<260; i++)
		{
			lImage.writeRow((png_bytep)&mValues[i*260]);
		}
		*/
		// copy last two four first into first four
		for (int i=0; i<4*260; i++)
		{
			mValues[i] = mValues[256*260+i];
		}
		
		// clear rest
		for (int i=4*260; i<260*260; i++)
		{
			mValues[i] = NOVALUE_VOXXLR;
		}

		chdir("..");
		chdir("..");
	}

	//memcpy(&mValues[(lPixelY%256+2)*260+mTileX+2], mLine, mCount*sizeof(float));
	float* lLine = &mValues[((iLineY-2)%256+4)*260+mTileX+2];
	*(lLine-2) = mWindow.mInputLine[std::max(0, mLineX-2)];
	*(lLine-1) = mWindow.mInputLine[std::max(0, mLineX-1)];
	memcpy(lLine, &mWindow.mInputLine[mLineX], mCount*sizeof(float));
	lLine[mCount] = mWindow.mInputLine[std::min(mWindow.mWidth-1, mLineX+mCount)];
	lLine[mCount+1] = mWindow.mInputLine[std::min(mWindow.mWidth-1, mLineX+mCount+1)];
}

float ImageYDepth::sMin;
float ImageYDepth::sMax;

float ImageYDepth::NOVALUE_AGISOFT = -32767;
float ImageYDepth::NOVALUE_PIX4D = -10000.00;
float ImageYDepth::NOVALUE_VOXXLR = -10000.00;



template <typename T, int S, class C> class ImageXY
{
	private:

		std::string mDirectory;
		std::vector<C*> mColumn;

		uint32_t mLineY;
		uint32_t mPixelY;
		uint32_t mTileY;

		ImageXY<T,S,C>* mParent;

	public:
	
		ImageWindow<T,S> mLineWindow;

		ImageXY(uint32_t iImageWidth, uint32_t iImageHeight, uint32_t iZoom, uint32_t iMinZoom, double iWorldX, double iWorldY, T iInitial)
		: mLineWindow(iImageWidth, iImageHeight, iInitial)
		, mDirectory(std::to_string((boost::int64_t)iZoom))
		, mParent(0)
		, mLineY(0)
		, mTileY((uint64)(iWorldY*pow(2, iZoom))/256)
		, mPixelY((uint64)(iWorldY*pow(2, iZoom))%256)
		{
			uint64 lLevelX = iWorldX*pow(2, iZoom);
			uint64 lTileX = lLevelX/256;
			uint16 lPixelX = lLevelX%256;

			uint16 lGridWidth = ceil((iImageWidth+lPixelX)/256.0);
		
			// columns
			boost::filesystem::create_directory(boost::filesystem::path(mDirectory.c_str()));
			chdir(mDirectory.c_str());
			uint32_t lLineX = 256-lPixelX;
			mColumn.push_back(new C(iZoom, lTileX++, lPixelX, 0, lLineX, mLineWindow));
			for (int x=1; x<lGridWidth-1; x++)
			{
				mColumn.push_back(new C(iZoom, lTileX++, 0, lLineX, 256, mLineWindow));
				lLineX += 256;
			}
			mColumn.push_back(new C(iZoom, lTileX++, 0, lLineX, iImageWidth - lLineX, mLineWindow));
			chdir("..");

			// lower zoom levels
			if (iZoom > iMinZoom)
			{
				mParent = new ImageXY(iImageWidth/2, iImageHeight/2, iZoom-1, iMinZoom, iWorldX, iWorldY, iInitial);
			}

			BOOST_LOG_TRIVIAL(info) << "level " << iZoom << "  pixels (" << iImageWidth << "," << iImageHeight << ")   files - " << (iImageWidth/256) << "," << (iImageHeight/256);
		}

		~ImageXY()
		{
			for (int i=0; i<mColumn.size(); i++)
			{
				delete mColumn[i];
			}
			if (mParent)
			{
				delete mParent;
			}
		}

		void processLine()
		{
			for (int i=0; i<mColumn.size(); i++) 
			{
				mColumn[i]->processLine(mLineY + mPixelY, mTileY);
			}
			
			mLineWindow.processLine(mLineY);
			mLineY++;
			
			if (mParent)
			{
				if (mLineY%2==0)
				{
					mParent->mLineWindow.sampleLine(mLineWindow);
					mParent->processLine();
				}
			}
		}

		void close()
		{
			// fill in remaing strips
			mLineWindow.clearLine();

			BOOST_LOG_TRIVIAL(info)  << "filling in from " << mLineY << " to " << (mLineY/256+1)*256;
			uint32_t lUpper = ((mLineY+mPixelY)/256+1)*256+2;
			for (uint32_t i=mLineY+mPixelY; i<=lUpper; i++)
			{
				processLine();
			}

			if (mParent)
			{
				mParent->close();
			}
		}
};



TmsTiler::TmsTiler()
{
};

void TmsTiler::process(std::string iFile)
{
	BOOST_LOG_TRIVIAL(info)  << "Processing file " << iFile;

	TifFile lFile(iFile);
	
	mRoot["x0"] = 256*(double)lFile.mInfo.p0.utmX/UTM_HEIGHT;
	mRoot["x1"] = 256*(double)lFile.mInfo.p1.utmX/UTM_HEIGHT;
	mRoot["y0"] = 256*(UTM_HEIGHT-(double)lFile.mInfo.p0.utmY)/UTM_HEIGHT;
	mRoot["y1"] = 256*(UTM_HEIGHT-(double)lFile.mInfo.p1.utmY)/UTM_HEIGHT;

	mRoot["maxZoom"] = (long)lFile.mInfo.maxZoom;
	mRoot["minZoom"] = (long)lFile.mInfo.minZoom;
	mRoot["proj"] = lFile.mInfo.proj;
	
	boost::filesystem::create_directory(boost::filesystem::path("root"));
	chdir("root");

	UpSampler lSampler(lFile);
	if (lFile.mFormat == SAMPLEFORMAT_IEEEFP)
	{
		// depth
		boost::filesystem::create_directory(boost::filesystem::path("elevation"));
		chdir("elevation");

		mRoot["size"] = 1; 
		mRoot["type"] = "float32";
		mRoot["format"] = ".bin";

		ImageYDepth::sMin = std::numeric_limits<float>::max();
		ImageYDepth::sMax = -std::numeric_limits<float>::max();
		ImageXY<float, 1, ImageYDepth> lLevel0(lSampler.mImageW, lSampler.mImageH, lFile.mInfo.maxZoom, lFile.mInfo.minZoom, lFile.mInfo.p0.mapX, lFile.mInfo.p0.mapY, ImageYDepth::NOVALUE_VOXXLR);
		for (uint32_t y = 0; y < lSampler.mImageH; y ++)   
		{
			lSampler.readLine(lLevel0.mLineWindow, y);
			lLevel0.processLine();

			if (y % 256 == 1)
			{
				BOOST_LOG_TRIVIAL(info) << " row " << y/ 256  << "/" <<lSampler.mImageH/256;
			}
		}
		lLevel0.close();
		
		BOOST_LOG_TRIVIAL(info) << "min/max " << ImageYDepth::sMin << "/" << ImageYDepth::sMax;

		mRoot["min"] = ImageYDepth::sMin;
		mRoot["max"] = ImageYDepth::sMax;

		chdir("..");
	}
	else
	{
		// color
		boost::filesystem::create_directory(boost::filesystem::path("color"));
		chdir("color");

		mRoot["size"] = 3;
		mRoot["type"] = "uint8";
		mRoot["format"] = ".png";

		ImageXY<unsigned char, 3, ImageYColor> lLevel0(lSampler.mImageW, lSampler.mImageH, lFile.mInfo.maxZoom, lFile.mInfo.minZoom, lFile.mInfo.p0.mapX, lFile.mInfo.p0.mapY, 255);
		for (uint32_t y = 0; y < lSampler.mImageH; y ++)
		{
			lSampler.readLine(lLevel0.mLineWindow, y);
			lLevel0.processLine();

			if (y % 256 == 1)
			{
				BOOST_LOG_TRIVIAL(info) << " row " << y/256  << "/" << lSampler.mImageH/256;
			}
		}

		lLevel0.close();
		chdir("..");
	}

	chdir("..");
};



/*


		{
			PngFloatImage lTemp("temp", lFile.mImageW, lFile.mImageH);
			float* lLine = new float[lFile.mImageW];
			for (int y = 0; y < lFile.mImageH; y++) 
			{
				lFile.readLine(y, (uint8*)lLine);
				lTemp.writeRow((uint8*)lLine);
			}        
		}

		exit(0);

*/