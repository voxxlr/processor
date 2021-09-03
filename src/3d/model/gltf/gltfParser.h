#pragma once

#define GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>

#include "../node.h"
#include "../parser.h"
#include "../bvh.h"

class BufferView
{
	public:

		static const int BYTE = 5120;
		static const int UNSIGNED_BYTE = 5121;
		static const int SHORT = 5122;
		static const int UNSIGNED_SHORT = 5123;
		static const int UNSIGNED_INT = 5125;
		static const int FLOAT = 5126;
	    
		static std::map<std::string, int> sElements;
		static std::map<int, int> sSize;
			
		BufferView(json_spirit::mObject& iBufferView, std::istream* iBuffer);
		
		uint8_t* read(json_spirit::mObject& iAccessor);

	private:

		std::istream& mBuffer;
		int mByteOffset;
};


class GltfParser : public Parser
{
	public:

		GltfParser();

		Model* process(std::string iFile, float iScalar);

	protected:

		static const int TRIANGLES = 4;

		BufferView** mBufferViews;

		json_spirit::mArray mNodes;
		json_spirit::mArray mMeshes;
		json_spirit::mArray mAccessors;

		//std::vector<Geometry*> mGeometries;
		std::vector<Node*> mMeshNodes;

		template <class T> void createIndexedAttibutes(Mesh& iMesh, json_spirit::mObject& iAttributes, T* iIndex, uint32_t iCount);
		template <class T> float* createIndexedAttibute(json_spirit::mObject& iAccessor, T* iIndex, uint32_t iCount);

		Node* createNode(json_spirit::mObject&, std::string iIndent);
		Geometry* createMesh(json_spirit::mObject& iJsonNode);
};

