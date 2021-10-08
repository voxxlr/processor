#pragma once

#include "../kdFileTree.h"

class NormalProcessor : public KdFileTree::InorderOperation
{
	public:

		NormalProcessor();

		static void computeNormals(PointCloud& iPoints, uint32_t iNormalIndex);
		void initTraveral(PointCloudAttributes& iAttributes);

	protected:

		uint8_t mInstruction;
	
		static void computeCovariance(PointCloud& iCloud, std::vector<std::pair<uint32_t, float>>& iIndex, glm::dmat3& iMatrix);
		static bool computeEigen(glm::dmat3& iMatrix, glm::dmat3& iVectors, glm::dvec3& iValues, unsigned maxIterationCount = 50);

		struct EdgeComparator
		{
			std::map<std::pair<uint32_t,uint32_t>, float>& mWeights;

			EdgeComparator(std::map<std::pair<uint32_t,uint32_t>, float>& iWeights) 
			: mWeights(iWeights) 
			{ 
			}

			bool operator()(std::pair<uint32_t,uint32_t>& a, std::pair<uint32_t,uint32_t>& b) const
			{

				return mWeights[a] < mWeights[b];
			}
		};

		// Xie Criterion
		inline float xie(float* n1, float* n2, float* p1, float* p2)
		{
			float e[3];
			e[0] = p1[0]-p2[0];
			e[1] = p1[1]-p2[1];
			e[2] = p1[2]-p2[2];

			float el = sqrt(e[0]*e[0]+e[1]*e[1]+e[2]*e[2]);
			e[0]/=el;
			e[1]/=el;
			e[2]/=el; 

			float dot = e[0]*n1[0] + e[1]*n1[1] + e[2]*n1[2];
			
			float n[3];
			n[0] = n1[0] - 2*dot*e[0];
			n[1] = n1[1] - 2*dot*e[1];
			n[2] = n1[2] - 2*dot*e[2];

			return n[0]*n2[0] + n[1]*n2[1] + n[2]*n2[2];
		}

		inline float hoppe(float* n1, float* n2, float* p1, float* p2)
		{
			return n1[0]*n2[0] + n1[1]*n2[1] + n1[2]*n2[2];
		}
	


		void processLeaf(KdFileTreeNode& iNode, PointCloud& iCloud);
		void processInternal(KdFileTreeNode& iNode, PointCloud& iCloud);
};
