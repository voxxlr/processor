#pragma once

#include "node.h"

class Bvh
{
	public:

		Bvh(int iI0, int iIN, Mesh& iMesh, std::vector<uint32_t>& iIndex);

		json_spirit::mObject toJson();

		static Bvh* construct(Mesh& iMesh, std::vector<uint32_t>* iIndex = 0);

		AABB mAABB;

	protected:

		int mI0;
		int mIN;
		
		Bvh* mChildL;
		Bvh* mChildH;

	private:

		static void reMapIndex(std::vector<uint32_t>& iTarget, std::vector<uint32_t>& iIndex);
	
		class Comparator
		{
			public:

				Comparator(int iAxis, Mesh& iMesh);

				bool operator() (uint32_t i1, uint32_t i2);

			private:

				int mAxis;
				Mesh& mVertex;
		};

		
		class Selector 
		{
			public:
				
				Selector(int iAxis, Mesh& iMesh);
		
				bool select(uint32_t iI, float iSplit);

			private:

				int mAxis;
				Mesh& mVertex;
		};


		static const int SPLIT_LIMIT = 100;

};
