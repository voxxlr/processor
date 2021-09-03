///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Geist Software Labs. All rights reserved.

///////////////////////////////////////////////////////////////////////////

#include <queue>
//
//
//
template <typename ATTRIBUTE> KdTree<ATTRIBUTE>::KdTree(PointCloud& iPointCloud, uint32_t iSplitSize)
: mPointCloud(iPointCloud)
, mSplitSize(iSplitSize)
, mRoot(0)
, mMemoryUsed(0)
, mAccessor(iPointCloud)
{
}

template <typename ATTRIBUTE> KdTree<ATTRIBUTE>::~KdTree()
{
	if (mRoot)
	{
		delete mRoot;
	}
}

template <typename ATTRIBUTE> void KdTree<ATTRIBUTE>::construct()
{
	if (mRoot)
	{
		delete mRoot;
		mIndex.clear();
	}

	mIndex.reserve(mPointCloud.size());
	mMemoryUsed += sizeof(uint32_t)*mPointCloud.size();

	// setup median cut
	mMemoryUsed += sizeof(Block);
	mRoot = new Block(0);
	mRoot->mLower = 0; 
	mRoot->mUpper = mPointCloud.size(); 
 	for(size_t i=0; i<mPointCloud.size(); i++)
    {
		float* lPosition = mAccessor.getPosition(i);
		float cx = lPosition[0];
		float cy = lPosition[1];
		float cz = lPosition[2];

		mRoot->min[0] = std::min(cx, mRoot->min[0]);
		mRoot->min[1] = std::min(cy, mRoot->min[1]);
		mRoot->min[2] = std::min(cz, mRoot->min[2]);

		mRoot->max[0] = std::max(cx, mRoot->max[0]);
		mRoot->max[1] = std::max(cy, mRoot->max[1]);
		mRoot->max[2] = std::max(cz, mRoot->max[2]);

		mIndex.push_back(i);
	}
//	shrink(*mRoot);

	// run median cut
	std::priority_queue<Block*, std::vector<Block*>, Block::Comparator> lBlockList;
	lBlockList.push(mRoot);
	while (1)
	{
		Block* lBlock = lBlockList.top();
		if (lBlock->size() <  2*mSplitSize)
		{
			break;
		}

		float lWx = lBlock->max[0] - lBlock->min[0];
		float lWy = lBlock->max[1] - lBlock->min[1];
		float lWz = lBlock->max[2] - lBlock->min[2];

		if (lWx >= lWy && lWx >= lWz) lBlock->mAxis = Block::X;
		if (lWy >= lWx && lWy >= lWz) lBlock->mAxis = Block::Y;
		if (lWz >= lWx && lWz >= lWy) lBlock->mAxis = Block::Z;

		mAccessor.axisSort(mIndex, lBlock->mLower, lBlock->mUpper, lBlock->mAxis);

		if (lBlock->mUpper - lBlock->mLower == 0)
		{
			break;
		}

		// compute median value
		double lMedianValue = 0;
		for (size_t i=lBlock->mLower; i<lBlock->mUpper; i++)
		{	
			lMedianValue += mAccessor.getAxis(mIndex[i], lBlock->mAxis);
		}
		lMedianValue /= (lBlock->mUpper - lBlock->mLower);

		// find median element and split
		uint32_t lMedianIndex;
		for (lMedianIndex=lBlock->mLower; lMedianIndex<lBlock->mUpper; lMedianIndex++)
		{
			if (mAccessor.getAxis(mIndex[lMedianIndex], lBlock->mAxis) > lMedianValue)
			{
				break;
			}
		}
		if (lMedianIndex == lBlock->mUpper)
		{
			break;
		}

		lBlock->mSplit = lMedianValue;

		lBlockList.pop();

		mMemoryUsed += sizeof(Block);
		Block* lBlockA = new Block(lBlock); 
		lBlockA->mLeaf = true;
		lBlockA->mLower = lBlock->mLower;
		lBlockA->mUpper = lMedianIndex;
		memcpy(lBlockA->min, lBlock->min, sizeof(lBlock->min));
		memcpy(lBlockA->max, lBlock->max, sizeof(lBlock->max));
		lBlockA->max[lBlock->mAxis] = lBlock->mSplit;
		//shrink(*lBlockA);
		lBlockList.push(lBlockA);
		lBlock->mChildLow = lBlockA;

		mMemoryUsed += sizeof(Block);
		Block* lBlockB = new Block(lBlock); 
		lBlockB->mLeaf = true;
		lBlockB->mLower = lMedianIndex;
		lBlockB->mUpper = lBlock->mUpper;
		memcpy(lBlockB->min, lBlock->min, sizeof(lBlock->min));
		memcpy(lBlockB->max, lBlock->max, sizeof(lBlock->max));
		lBlockB->min[lBlock->mAxis] = lBlock->mSplit;
		//shrink(*lBlockB);
		lBlockList.push(lBlockB);
		lBlock->mChildHigh = lBlockB;

		lBlock->mLeaf = false;
	}
}


// find neightbors within radius
template <typename ATTRIBUTE> void KdTree<ATTRIBUTE>::search(Block& iNode, Point& iPoint, float iRadius, std::vector<uint32_t>& iIndex)
{
	if (iNode.mChildLow && iNode.mChildHigh)
	{
		float lValue = mAccessor.getAxis(iPoint, iNode.mAxis);

		if (lValue - iNode.mChildLow->max[iNode.mAxis] <= iRadius)
		{
			search(*iNode.mChildLow, iPoint, iRadius, iIndex);
		}
		if (iNode.mChildHigh->min[iNode.mAxis] - lValue <= iRadius)
		{
			search(*iNode.mChildHigh, iPoint, iRadius, iIndex);
		}
	}
	else
	{
		float lRadius2 = iRadius*iRadius;
		for (int i=iNode.mLower; i<iNode.mUpper; i++)
		{
			if (mAccessor.distanceSquared(mIndex[i], iPoint) <= lRadius2)
			{
				iIndex.push_back(mIndex[i]);
			}
		}
	}
}

template <typename ATTRIBUTE> void KdTree<ATTRIBUTE>::search(uint32_t iPoint, float iRadius, std::vector<uint32_t>& iIndex)
{
	search(*mRoot, *mPointCloud[iPoint], iRadius, iIndex);
}





// selet unsleced neightbors within radius
template <typename ATTRIBUTE> void KdTree<ATTRIBUTE>::select(Block& iNode, Point& iPoint, float iRadius, std::vector<uint32_t>& iIndex)
{
	if (iNode.mChildLow && iNode.mChildHigh)
	{
		float lValue = mAccessor.getAxis(iPoint, iNode.mAxis);

		if (lValue - iNode.mChildLow->max[iNode.mAxis] <= iRadius)
		{
			select(*iNode.mChildLow, iPoint, iRadius, iIndex);
		}
		if (iNode.mChildHigh->min[iNode.mAxis] - lValue <= iRadius)
		{
			select(*iNode.mChildHigh, iPoint, iRadius, iIndex);
		}
	}
	else
	{
		float lRadius2 = iRadius*iRadius;
		for (int i=iNode.mLower; i<iNode.mUpper; i++)
		{
			if (mAccessor.distanceSquared(mIndex[i], iPoint) <= lRadius2)
			{
				iIndex.push_back(mIndex[i]);
			}
		}
	}
}

template <typename ATTRIBUTE> void KdTree<ATTRIBUTE>::select(uint32_t iPoint, float iRadius, std::vector<uint32_t>& iIndex)
{
	select(*mRoot, *mPointCloud[iPoint], iRadius, iIndex);
}







// use median[0] to store minimum distance to split plane as prio
struct ClosestNode
{
	bool operator()(const Block* iL, const Block* iR) const
	{
		return iL->median[0] > iR->median[0];
	}
};



template <typename ATTRIBUTE> template<unsigned int N> void KdTree<ATTRIBUTE>::knn(uint32_t iPoint, std::vector<std::pair<uint32_t, float>>& iResult)
{
	Point& lPoint = *mPointCloud[iPoint];

	iResult.resize(N);
	for (int i=0; i<N; i++)
	{
		iResult[i].first = iPoint;
		iResult[i].second = FLT_MAX;
	}

	float lSearchRadius = FLT_MAX;
	std::priority_queue<Block*, std::vector<Block*>, ClosestNode> lQueue;
	mRoot->median[0] = 0; // use median[0] to store search distance used in prio
	lQueue.push(mRoot);
	while (!lQueue.empty())
	{
		Block* lNode = lQueue.top();
		lQueue.pop();

		if (!lNode->mLeaf)
		{
			if (lNode->median[0] < lSearchRadius)
			{
				float dS = lNode->mSplit - mAccessor.getAxis(lPoint, lNode->mAxis);

				if (dS*dS < lSearchRadius)
				{
					// search both halves
					if (dS > 0)
					{
						lNode->mChildLow->median[0] = 0;
						lNode->mChildHigh->median[0] = dS*dS;
					}
					else
					{
						lNode->mChildLow->median[0] = dS*dS;
						lNode->mChildHigh->median[0] = 0;
					}

	
					lQueue.push(lNode->mChildLow);
 					lQueue.push(lNode->mChildHigh);
				}
				else
				{
					// only search one half
					if (dS > 0)
					{
						lNode->mChildLow->median[0] = 0;
						lQueue.push(lNode->mChildLow);
					}
					else
					{
						lNode->mChildHigh->median[0] = 0;
 						lQueue.push(lNode->mChildHigh);
					}
				}
			}
		}
		else
		{
			if (lNode->median[0] < lSearchRadius)
			{
				for (size_t i=lNode->mLower; i<lNode->mUpper; i++)
				{
					uint32_t lIndex = mIndex[i];
					if (lIndex != iPoint)
					{
						std::pair<uint32_t, float> lD(lIndex, mAccessor.distanceSquared(lIndex, lPoint));
						if (lD.second < lSearchRadius)
						{
							for (int n=0; n<N; n++)
							{
								if (lD.second < iResult[n].second)
								{
									std::pair<uint32_t, float> lTemp = iResult[n];
									iResult[n] = lD;
									lD = lTemp;
								}
							}
						}
					}
				}

				lSearchRadius = 0;
				for (int i=0; i<N; i++)
				{
					lSearchRadius = std::max(lSearchRadius, iResult[i].second);
				}
			}
		}
	}

	for (int i=0; i<N; i++)
	{
		iResult[i].second = sqrt(iResult[i].second);
	}
}
















// count neighbors
template <typename ATTRIBUTE> uint32_t KdTree<ATTRIBUTE>::count(Block& iNode, Point& iPoint, float iRadius)
{
	uint32_t lCount = 0;
	if (iNode.mChildLow && iNode.mChildHigh)
	{
		float lValue = mAccessor.getAxis(iPoint, iNode.mAxis);

		if (lValue - iNode.mChildLow->max[iNode.mAxis] <= iRadius)
		{
			lCount += count(*iNode.mChildLow, iPoint, iRadius);
		}
		if (iNode.mChildHigh->min[iNode.mAxis] - lValue <= iRadius)
		{
			lCount += count(*iNode.mChildHigh, iPoint, iRadius);
		}
	}
	else
	{
		float lRadius2 = iRadius*iRadius;
		for (int i=iNode.mLower; i<iNode.mUpper; i++)
		{
			if (mAccessor.distanceSquared(mIndex[i], iPoint) <= lRadius2)
			{
				lCount++;
			}
		}
	}
	return lCount;
}

template <typename ATTRIBUTE> uint32_t KdTree<ATTRIBUTE>::count(uint32_t iPoint, float iRadius)
{
	return count(*mRoot, mPointCloud[iPoint], iRadius);
};




template <typename ATTRIBUTE> bool KdTree<ATTRIBUTE>::detect(Block& iNode, Point& iPoint, float iRadius, int& iCount)
{
	if (iNode.mChildLow && iNode.mChildHigh)
	{
		float lValue = mAccessor.getAxis(iPoint, iNode.mAxis);

		if (lValue - iNode.mChildLow->max[iNode.mAxis] <= iRadius)
		{
			if (detect(*iNode.mChildLow, iPoint, iRadius, iCount))
			{
				return true;
			}
		}
		if (iNode.mChildHigh->min[iNode.mAxis] - lValue <= iRadius)
		{
			if (detect(*iNode.mChildHigh, iPoint, iRadius, iCount))
			{
				return true;
			}
		}
	}
	else
	{
		float lRadius2 = iRadius*iRadius;

		//std::cout << (iNode.mUpper - iNode.mLower) << " \n";
		for (int i=iNode.mLower; i<iNode.mUpper; i++)
		{
			//if (mPointCloud[mIndex[i]]->dist2(iPoint) <= lRadius2)
			if (mAccessor.distanceSquared(mIndex[i], iPoint) <= lRadius2)
			{
				if (--iCount < 0)
				{
					return true;
				}
			}
		}
	}
	return false;
}

template <typename ATTRIBUTE> bool KdTree<ATTRIBUTE>::detect(uint32_t iPoint, float iRadius, int iCount)
{
	return detect(*mRoot, *mPointCloud[iPoint], iRadius, iCount);
};


















/*



template <typename ATTRIBUTE> void KdTree<ATTRIBUTE>::nearest(Block& iNode, float* iPoint, float& iRadius, uint32_t& iIndex)
{
	if (iNode.mChildLow && iNode.mChildHigh)
	{
		if (iPoint[iNode.mAxis] - iNode.mChildLow->max[iNode.mAxis] <= iRadius)
		{
			nearest(*iNode.mChildLow, iPoint, iRadius, iIndex);
		}
		if (iNode.mChildHigh->min[iNode.mAxis] - iPoint[iNode.mAxis] <= iRadius)
		{
			nearest(*iNode.mChildHigh, iPoint, iRadius, iIndex);
		}
	}
	else
	{
		float lRadius2 = iRadius*iRadius;

		for (int i=iNode.mLower; i<iNode.mUpper; i++)
		{
			float lDist2 = mPointCloud[mIndex[i]]->dist2(iPoint);
			if (lDist2 <= lRadius2)
			{
				lRadius2 = lDist2;
				iIndex = mIndex[i];
			}
		}

		iRadius = sqrt(lRadius2);
	}
}


template <typename ATTRIBUTE> uint32_t KdTree<ATTRIBUTE>::nearest(float* iPoint, float iRadius)
{
	float lRadius = iRadius;
	uint32_t lIndex = 0;
	nearest(*mRoot, iPoint, lRadius, lIndex);
	return lIndex;
};










// find nearst neightbor
template <typename ATTRIBUTE> void KdTree<ATTRIBUTE>::nearest(Block& iNode, uint32_t iIndex, float* iPoint, float& iNearest)
{
	if (iNode.mChildLow && iNode.mChildHigh)
	{
		if (iPoint[iNode.mAxis] - iNode.mChildLow->max[iNode.mAxis] <= iNearest)
		{
			nearest(*iNode.mChildLow, iIndex, iPoint, iNearest);
		}
		if (iNode.mChildHigh->min[iNode.mAxis] - iPoint[iNode.mAxis] <= iNearest)
		{
			nearest(*iNode.mChildHigh, iIndex, iPoint, iNearest);
		}
	}
	else
	{
		if (iPoint[iNode.mAxis] - iNode.max[iNode.mAxis] <= iNearest)
		{
			float lNearest2 = iNearest*iNearest;

			for (int i=iNode.mLower; i<iNode.mUpper; i++)
			{
				if (mIndex[i] != iIndex)
				{
					float lDist2 = mPointCloud[mIndex[i]]->dist2(iPoint);
					if (lDist2 > 0.001*0.001)  // nothing less than 1mm
					{
						lNearest2 = std::min(lNearest2, mPointCloud[mIndex[i]]->dist2(iPoint));
					}
				}
			}

			iNearest = sqrt(lNearest2);
		}
	}
}

template <typename ATTRIBUTE> float KdTree<ATTRIBUTE>::nearest(uint32_t iPoint)
{
	float lNearest = sqrt(FLT_MAX) - 12;
	nearest(*mRoot, iPoint,  mPointCloud[iPoint]->position, lNearest);
	return lNearest;
}

template <typename ATTRIBUTE> float KdTree<ATTRIBUTE>::nearest(Block* iNode, uint32_t iPoint)
{
	float lNearest = sqrt(FLT_MAX) - 12;
	nearest(*iNode, iPoint,  mPointCloud[iPoint]->position, lNearest);
	return lNearest;
}
*/