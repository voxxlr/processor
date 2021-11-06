#ifndef _KdFileTree
#define _KdFileTree

#include <vector>
#include <queue>
#include <stack>

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include "point.h"
#include "pointCloud.h"
#include "kdTree.h"

class KdFileTreeNode
{  
	public:

		KdFileTreeNode(json_spirit::mObject& iConfig);
		KdFileTreeNode(std::string iPath, uint32_t iHeight = 0);
		~KdFileTreeNode();

		inline bool contains(float* iPosition)
		{
			return iPosition[0] >= min[0] &&  iPosition[0] <= max[0] && 
				   iPosition[1] >= min[1] &&  iPosition[1] <= max[1] &&  
				   iPosition[2] >= min[2] &&  iPosition[2] <= max[2];
		}

		std::string mPath;
		bool mThreaded;
		uint8_t mHeight;
		float min[3];
		float max[3];
		float mSplit;
		uint8_t mAxis;
		KdFileTreeNode* mChildLow;
		KdFileTreeNode* mChildHigh;
		uint64_t mCount;
		uint64_t mVolumeLimit;

		void feed(uint8_t* iBuffer, uint32_t iStride, uint32_t iCount, boost::thread_group*& iGroup, int32_t iThreadCount);
		bool grow(std::string iIndent, uint32_t iLeafsize);

		void openFiles(PointCloudAttributes& iAttributes, float iResolution);
		void write(uint8_t* iBuffer, uint32_t iStride, uint32_t iCount, float iOverlap, boost::thread_group*& iGroup, int32_t iThreadCount);
		json_spirit::mObject closeFiles();
		void deleteFiles();

		bool prune();
		uint64_t collapse(FILE* iFile, uint32_t iStride, float* iMin, float* iMax);

	private:

		KdFileTreeNode();
		KdFileTreeNode(KdFileTreeNode& iNode);

		void recordChunk(uint8_t* iBuffer, uint32_t iStride, uint32_t iCount);
		void writeChunk(uint8_t* iBuffer, uint32_t iStride, uint32_t iCount, float iOverlap);

		static const int X = 0;
		static const int Y = 1;
		static const int Z = 2;

		double median[3];

		bool mAlive;

		FILE* mFile;
};


class KdFileTree
{	
	private:

		static const uint64_t POINTS_PER_IO = 10000000;
		static const float MIN_RESOLUTION; // 1 mm

		// File IO
		std::string mName;

		PointCloudAttributes mPointAttributes;

	public:

		// 100000 points in a leaf node i.e. one packet going to browser ~ 2MB uncompressed
		static const float SIGMA;

		KdFileTree();
		void construct(std::string iName, uint32_t iLeafsize, float iOverlap);
		void load(std::string iName);

		uint64_t collapse(std::string iName);
		void remove();

		json_spirit::mArray fill(float iSigma = SIGMA);

		float mResolution;

		operator PointCloudAttributes& ()
		{
			return mPointAttributes;
		}
		bool hasAttribute(const std::string& iName);

		//
		// Operations Api
		//
		
		static const uint8_t LEAVES = 0x01; 
		static const uint8_t INTERNAL = 0x10; 

		class InorderOperation
		{
			public:

				InorderOperation(std::string iName);

				virtual void initTraveral(PointCloudAttributes& iAttributes);
				virtual void completeTraveral(PointCloudAttributes& iAttributes);

				virtual void processNode(KdFileTreeNode& iNode, PointCloud& iCloud) {};

				std::string mName;

			private:

				boost::posix_time::ptime mStartTime;

				InorderOperation();
		};

		void process(InorderOperation& iProcessor, uint8_t iNodes);

	
	private:

		KdFileTreeNode* mRoot;

		void getNodes(std::vector<KdFileTreeNode*>& iList, KdFileTreeNode& iNode, uint8_t iNodes);
		void processNode(KdFileTree::InorderOperation& iProcessor, KdFileTreeNode& iNode);

}; 

class Downsampler : public KdFileTree::InorderOperation
{
	public:

		Downsampler(FILE* iFile, float iResolution);

		uint64_t mWritten;

	protected:

		FILE* mFile;

		float mResolution;

		boost::mutex mWriteLock;

		// traversal
		void processNode(KdFileTreeNode& iNode, PointCloud& iCloud);
};




#endif
