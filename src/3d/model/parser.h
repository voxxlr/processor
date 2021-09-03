#pragma once

#define GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>

#include "node.h"
#include "bvh.h"


class Parser;

class Node
{
	public: 

		Node(int iId, uint8_t iType);
		Node(Node& iNode);
		~Node();
		uint32_t mRefCount;

		glm::dmat4 mMatrix;
		Geometry* mGeometry;
		int32_t mMaterial;
		json_spirit::mObject mInfo;

		Node* addNode(Node* iNode);

		// csg operations
		void accumulate(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix = I);
		void subtract(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix = I);

		//
		Node* getChild(std::string iName);

		//
		static const uint8_t PHYSICAL = 0x01;
		static const uint8_t LOGICAL = 0x02;
		static const uint8_t GEOMETRY = 0x00;

		int32_t mLogicalParent;

		bool is(uint8_t iType)
		{
			return (mType & iType) == iType;
		}


		//
		void finalize(Parser& iParser, InternalNode& iNode);
		
		void print(std::string iIndent);

	protected:

		uint8_t mType;
		
		static glm::dmat4 I;

		void resolveHierarchy(InternalNode& iParent, glm::dmat4& iMatrix, int32_t iMaterial, std::vector<Node*>& iList);
		void resolveReferences();

		std::vector<Node*> mChildren;

		static int32_t sId;
};

class Parser 
{
	public:

		Parser();

		Model* process(std::string iFile, json_spirit::mObject& iSource);

		json_spirit::mArray mImages;
		json_spirit::mArray mMaterials;
		json_spirit::mArray mTextures;
		json_spirit::mArray mSamplers;
		
		json_spirit::mObject mDefaultMaterial;

	protected:

		static const int TRIANGLES = 4;

		Model* convert(Node* iRoot);
};

