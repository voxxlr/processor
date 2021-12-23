#ifndef _KdTree
#define _KdTree


#include "pointCloud.h"

typedef struct Block
{  
	size_t  mLower;
	size_t  mUpper; 

	static const int X = 0;
	static const int Y = 1;
	static const int Z = 2;
	uint8_t mAxis;
	float mSplit;

	float min[3];
	float max[3];

	Block* mChildLow;
	Block* mChildHigh;
	Block* mParent;
	bool mLeaf;

	double median[3];

	Block(Block* iParent)
	: mAxis(0)
	, mChildLow(0)
	, mChildHigh(0)
	, mParent(iParent)
	, mLeaf(true)
	, mSplit(0)
	{
		min[0] = std::numeric_limits<float>::max();
		min[1] = std::numeric_limits<float>::max();
		min[2] = std::numeric_limits<float>::max();

		max[0] = -std::numeric_limits<float>::max();
		max[1] = -std::numeric_limits<float>::max();
		max[2] = -std::numeric_limits<float>::max();

		median[0] = 0;
		median[1] = 0;
		median[2] = 0;

	}

	~Block()
	{
		if (mChildLow)
		{
			delete mChildLow;
		}
		if (mChildHigh)
		{
			delete mChildHigh;
		}
	}

	inline size_t size()
	{
		return mUpper - mLower;
	}

	inline float volume()
	{
		return (max[0]-min[0])*(max[1]-min[1])*(max[2]-min[2]);
	}

	struct Comparator
	{
		bool operator() (Block* a, Block* b) 
		{
			return a->size() < b->size();
		}
	};

} Block;



template <typename ATTRIBUTE> class KdTree 
{
	public:
	
		KdTree(PointCloud& iPointCloud, uint32_t iSplitSize = 100);
		~KdTree();

		void construct();
	//	void write();
		
		void search(uint32_t iPoint, float iRadius, std::vector<uint32_t>& iIndex); // return neightbors within radius
		void select(uint32_t iPoint, float iRadius, std::vector<uint32_t>& iIndex); // return unselected neightbors within radius
		uint32_t count(uint32_t iPoint, float iRadius); // count neightbors within radius
		bool detect(uint32_t iPoint, float iRadius, int iCount);// find at least count neightbors within radius

		template<unsigned int N> void knn(uint32_t iPoint, std::vector<std::pair<uint32_t, float>>& iIndex); 		// find N nearest neightbors 

		Block* getRoot()
		{
			return mRoot;
		}

		float volume()
		{
			return mRoot->volume();
		}

		static uint64_t maxMemoryUsage(uint64_t iPoints, uint32_t iSplitSize = 100)
		{
			return 2*(iPoints/iSplitSize)*sizeof(Block) + iPoints*sizeof(uint32_t);
		}

		std::vector<uint32_t> mIndex;
		uint64_t mMemoryUsed;
		
		ATTRIBUTE mAccessor;

	private:

	//	void shrink(Block& iBlock);

		uint32_t mSplitSize;

		PointCloud& mPointCloud;	

		uint32_t count(Block& iNode, Point& iPoint, float iRadius);
		bool detect(Block& iNode, Point& iPoint, float iRadius, int& iCount);
		void search(Block& iNode, Point& iPoint, float iRadius, std::vector<uint32_t>& iIndex);
		void select(Block& iNode, Point& iPoint, float iRadius, std::vector<uint32_t>& iIndex);

	private:

		Block* mRoot;


		/*
		// find neatest
		float nearest(uint32_t iPoint);
		float nearest(Block* iNode, uint32_t iPoint);
		uint32_t nearest(float* iPoint, float iRadius);

		void nearest(Block& iNode, uint32_t iIndex, float* iPoint, float& iNearest);
		void nearest(Block& iNode, float* iPoint, float& iRadius, uint32_t& iIndex);
		*/
}; 

#include "kdTree.inl"






class KdSpatialDomain
{
	public:
		
		KdSpatialDomain(PointCloud& iCloud);

		void axisSort(std::vector<uint32_t>& iIndex, uint32_t iLower, uint32_t iUpper, uint8_t iAxis);

		bool operator() (uint32_t a, uint32_t  b)
		{ 
			return mPointCloud[a]->position[mAxis] < mPointCloud[b]->position[mAxis];
		}

		inline float* getPosition(uint32_t iIndex)
		{
			return mPointCloud[iIndex]->position;
		}
		
		inline float getAxis(uint32_t iIndex, uint8_t iAxis)
		{
			return mPointCloud[iIndex]->position[iAxis];
		};
				
		inline float getAxis(Point& iPoint, uint8_t iAxis)
		{
			return iPoint.position[iAxis];
		};

		inline float distanceSquared(uint32_t iIndex, Point& iPoint)
		{
			Point& lPoint = *mPointCloud[iIndex];
			float dx = iPoint.position[0] - lPoint.position[0];
			float dy = iPoint.position[1] - lPoint.position[1];
			float dz = iPoint.position[2] - lPoint.position[2];
			return dx*dx + dy*dy + dz*dz;
		}

	protected:

		uint8_t mAxis;
		PointCloud& mPointCloud;
};


class KdColorDomain
{
	public:
		
		KdColorDomain(PointCloud& iCloud);

		void axisSort(std::vector<uint32_t>& iIndex, uint32_t iLower, uint32_t iUpper, uint8_t iAxis);

		bool operator() (uint32_t a, uint32_t  b)
		{ 
			ColorType* lA = (ColorType*)mPointCloud[a]->getAttribute(mColorIndex);
			ColorType* lB = (ColorType*)mPointCloud[b]->getAttribute(mColorIndex);
			return lA->mValue[mAxis] < lB->mValue[mAxis];
		}

		inline float* getPosition(uint32_t iIndex)
		{
			ColorType* lAttribute = (ColorType*)mPointCloud[iIndex]->getAttribute(mColorIndex);
			mValues[0] = lAttribute->mValue[0];
			mValues[1] = lAttribute->mValue[1];
			mValues[2] = lAttribute->mValue[2];
			return mValues;
		}

		inline float getAxis(uint32_t iIndex, uint8_t iAxis)
		{
			ColorType* lAttribute = (ColorType*)mPointCloud[iIndex]->getAttribute(mColorIndex);
			return lAttribute->mValue[iAxis];
		};
		
		inline float getAxis(Point& iPoint, uint8_t iAxis)
		{
			ColorType* lAttribute = (ColorType*)iPoint.getAttribute(mColorIndex);
			return lAttribute->mValue[iAxis];
		};

		inline float distanceSquared(uint32_t iIndex, Point& iPoint)
		{
			ColorType* lColorA = (ColorType*)mPointCloud[iIndex]->getAttribute(mColorIndex);
			ColorType* lColorB = (ColorType*)iPoint.getAttribute(mColorIndex);
			int32_t dr = lColorA->mValue[0] - lColorB->mValue[0];
			int32_t dg = lColorA->mValue[1] - lColorB->mValue[1];
			int32_t db = lColorA->mValue[2] - lColorB->mValue[2];
			return (float)(dr*dr + dg*dg + db*db);
		}

	protected:

		float mValues[3];
		uint8_t mAxis;
		uint8_t mColorIndex;
		PointCloud& mPointCloud;

};



//
//
//


class KdSpatialColorDomain : public KdSpatialDomain
{
	public:
		
		KdSpatialColorDomain(PointCloud& iCloud);

		inline float distanceSquared(uint32_t iIndex, Point& iPoint)
		{
			Point& lPoint = *mPointCloud[iIndex];

			ColorType* lColorA = (ColorType*)mPointCloud[iIndex]->getAttribute(mColorIndex);
			ColorType* lColorB = (ColorType*)iPoint.getAttribute(mColorIndex);
			float dr = (lColorA->mValue[0] - lColorB->mValue[0])/255.0f;
			float dg = (lColorA->mValue[1] - lColorB->mValue[1])/255.0f;
			float db = (lColorA->mValue[2] - lColorB->mValue[2])/255.0f;
			float dc = 1.0f-sqrt(dr*dr + dg*dg + db*db)/sqrt(3.0f);

			return KdSpatialDomain::distanceSquared(iIndex, iPoint)*dc;
		}

	private:
		
		uint8_t mColorIndex;

};


class KdSpatialNormalDomain : public KdSpatialDomain
{
	public:
		
		KdSpatialNormalDomain(PointCloud& iCloud);

		inline float distanceSquared(uint32_t iIndex, Point& iPoint)
		{
			Point& lPoint = *mPointCloud[iIndex];
			NormalType* lNormalA = (NormalType*)lPoint.getAttribute(mNormalIndex);
			NormalType* lNormalB = (NormalType*)iPoint.getAttribute(mNormalIndex);
			float lAngle = 1.0f-fabs(lNormalA->mValue[0]*lNormalB->mValue[0]+lNormalA->mValue[1]*lNormalB->mValue[1]+lNormalA->mValue[2]*lNormalB->mValue[2]);
			if (lAngle > 0)
			{
				return KdSpatialDomain::distanceSquared(iIndex, iPoint)*exp(1-1/(lAngle*lAngle));
			}
			return 0;
		}

	private:
		
		uint8_t mNormalIndex;

};


class KdColorNormalDomain : public KdColorDomain
{
	public:
		
		KdColorNormalDomain(PointCloud& iCloud);

		inline float distanceSquared(uint32_t iIndex, Point& iPoint)
		{
			Point& lPoint = *mPointCloud[iIndex];
			NormalType* lNormalA = (NormalType*)lPoint.getAttribute(mNormalIndex);
			NormalType* lNormalB = (NormalType*)iPoint.getAttribute(mNormalIndex);
			float lAngle = 1.0f-fabs(lNormalA->mValue[0]*lNormalB->mValue[0]+lNormalA->mValue[1]*lNormalB->mValue[1]+lNormalA->mValue[2]*lNormalB->mValue[2]);
			return KdColorDomain::distanceSquared(iIndex, iPoint)*exp(1-1/(lAngle*lAngle));
		}

	private:

		uint8_t mNormalIndex;
};




#endif
