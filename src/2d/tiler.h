#pragma once

#include "tiff.h"
#include "xtiffio.h"

#include "geotiff.h"
#include "geo_normalize.h"
#include "geo_tiffp.h"
#include "geo_keyp.h"

#include "png.h"

class PngImage
{
	public:

		PngImage(std::string iFile, uint32 iWidth, uint32 iHeight);
		virtual ~PngImage();

		virtual void writeRow(png_bytep lLine) = 0;

	protected:

		uint32 mWidth;
		uint32 mHeight;

		png_structp mPointer;
		png_infop mInfo;
		FILE* mFile;
};

class PngRgbImage : public PngImage
{
	public:

		PngRgbImage(std::string iFile, uint32 iWidth, uint32 iHeight);

		void writeRow(png_bytep lLine);
};


class PngFloatImage : public PngImage
{
	public:

		PngFloatImage(std::string iFile, uint32 iWidth, uint32 iHeight, double iMin, double iMax);
		~PngFloatImage();

		void writeRow(png_bytep lLine);

	protected:

		uint16* mLine;
		double mMin;
		double mMax;
};


class PngWriter
{
	public:

		PngWriter(unsigned char iValues[256*256*3]);
		~PngWriter();

		void write(std::string iName);

	private:

		png_bytep* mImageRows;
};







class TifFormat;

class TifFile 
{
	public:

		TifFile(std::string iFile);
		~TifFile();

		uint32 mImageW;
		uint32 mImageH;
		uint32 mTileX;
		uint32 mTileY;
		uint32 mPixelX;
		uint32 mPixelY;
		uint16 mFormat;
	
		uint16 mSamplesPerPixel;
		uint16 mBytesPerPixel;
		uint16 mBitsPerSample;

		TIFF* mFile;

		struct Point
		{
			double lon;
			double lat;
			float mapX;
			float mapY;
			double utmX;
			double utmY;
		};

		struct Info
		{
			Point p0;
			Point p1;
			double worldPixelW;
			double worldPixelH;
			double mapPixelS;
			uint16 maxZoom;
			uint16 minZoom;
			std::string proj;
		} mInfo;

		void readLine(uint32_t iLine, uint8* iBuffer);

	protected:

		TifFormat* mReader;

		uint16 mConfig;
};

class TifFormat
{
	public:

		virtual void readLine(uint32_t iLine, uint8* iBuffer) = 0;
};

class TifTiledFormat : public TifFormat
{
	public:

		TifTiledFormat(TifFile& iFile);
		~TifTiledFormat();

		void readLine(uint32_t iLine, uint8* iBuffer);

	protected:

		uint32 mTileW;
		uint32 mTileH;

		int32 mTileY;

		tdata_t* mBuffer;
		TifFile mFile;
};


class TifStripedFormat : public TifFormat
{
	public:

		TifStripedFormat(TifFile& iFile);
		~TifStripedFormat();

		void readLine(uint32_t iLine, uint8* iBuffer);

	protected:

		tdata_t mBuffer;
		TifFile mFile;
};




template <typename T, int S> class ImageWindow;

class Sampler
{
	public:

		virtual void readLine(ImageWindow<unsigned char, 3>& iWindow, int32 iY) = 0;
		virtual void readLine(ImageWindow<float, 1>& iWindow, int32 iY) = 0;

		float depthFilter(float iPixel);
};

class UpSampler : public Sampler
{
	public:

		UpSampler(TifFile& iFile);
		~UpSampler();

		void readLine(ImageWindow<unsigned char, 3>& iWindow, int32 iY);
		void readLine(ImageWindow<float, 1>& iWindow, int32 iY);

		uint32 mImageW;
		uint32 mImageH;

	private:

		TifFile& mFile;

		struct Cache 
		{
			int32 y;
			uint8* buffer;
		};

		Cache mCache[2];
		double mScalarX;
		double mScalarY;

		uint8* getLine(int32 iY);

		PngImage* mPngImage;
};


class NoSampler : public Sampler
{
	public:

		NoSampler(TifFile& iFile);
		~NoSampler();

		void readLine(ImageWindow<unsigned char, 3>& iWindow, int32 iY);
		void readLine(ImageWindow<float, 1>& iWindow, int32 iY);

		uint32 mImageW;
		uint32 mImageH;

	private:
	
		TifFile& mFile;
		uint8* mBuffer;
	
};



class ImageY
{
	public:

		ImageY(uint32 iZ, uint32 iX, uint32 iTileX, uint32 iCount);

		virtual void processLine(uint32 iLineY, uint32 iTileY) = 0;
	
	protected:

		std::string mDirectoryZ;
		std::string mDirectoryX;
		uint32 mTileX;
		uint32 mCount;
};


class ImageYColor : public ImageY
{
	public:

		ImageYColor(uint32 iZ, uint32 iX, uint32 iTileX, uint32 iLineX, uint32 iCount, ImageWindow<unsigned char, 3>& iWindow);

		void processLine(uint32 iLineY, uint32 iTileY);

	protected:

		void clearImage();

		unsigned char* mLine;
		unsigned char mValues[256*256*3];

		PngWriter mWriter;
		
		ImageWindow<unsigned char, 3>& mWindow;
};

class ImageYDepth : public ImageY
{
	public:

		static float sMin;
		static float sMax;

		static float NOVALUE_AGISOFT;
		static float NOVALUE_PIX4D;
		static float NOVALUE_VOXXLR;

		ImageYDepth(uint32 iZ, uint32 iX, uint32 iTileX, int32 iLineX, uint32 iCount, ImageWindow<float, 1>& iWindow);

		void processLine(uint32 iLineY, uint32 iTileY);

	protected:
	
		void clearImage();

		int32 mLineX;

		float mValues[260*260];
		
		ImageWindow<float, 1>& mWindow;
};







class TmsTiler 
{
	public:

		TmsTiler();

		void process(std::string iFile);
		
		json_spirit::mObject mRoot;

	protected:

};

