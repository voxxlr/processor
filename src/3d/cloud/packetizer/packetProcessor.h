#pragma once

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>

#include <vector>
#include <queue>

#include "../point.h" 
#include "../kdTree.h"
#include "../kdFileTree.h"

class PacketProcessor : public KdFileTree::InorderOperation
{
	public:

		PacketProcessor();

		uint64_t mTotalStorage;
		uint64_t mTotalWritten;

		json_spirit::mObject mRootInfo;

		void initTraveral(PointCloudAttributes& iAttributes);

	protected:

		boost::mutex mCountLock;

		// Normal Calculation
		static void computeNormals(PointCloud& iPoints, uint32_t iNormalIndex);
		static void computeCovariance(PointCloud& iCloud, std::vector<std::pair<uint32_t, float>>& iIndex, glm::dmat3& iMatrix);
		static bool computeEigen(glm::dmat3& iMatrix, glm::dmat3& iVectors, glm::dvec3& iValues, unsigned maxIterationCount = 50);

		// KdTree sorting
		void kdTest(float* iMin, float* iMax, std::vector<float>& iTree, int iIndex, float* iTestMin, float* iTestMax);
		void kdTreeSort(PointCloud& iCloud, uint32_t iLower, uint32_t iUpper, std::vector<uint32_t>& iIndex, float* iMin, float* iMax, std::vector<float>& iTree, int iNodeIndex);

		// File I/O
		void writePacket(KdFileTreeNode& iNode, PointCloud& iPoints);

		// traversal
		void processNode(KdFileTreeNode& iNode, PointCloud& iCloud);

		uint32_t align4(FILE* iFile, uint32_t iPointer);
  
};