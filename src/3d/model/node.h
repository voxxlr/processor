#pragma once
#include <boost/filesystem/path.hpp>

#include <glm/glm.hpp>

#include "aabb.h"
#include "geometry.h"


class Model;

class UniqueNode
{
	public: 

		UniqueNode();
		
		int32_t mId;

	private: 

		static int32_t sId;
};

class InstancedNode: public UniqueNode
{
	public:

		InstancedNode(Geometry* iGeometry);

		std::tuple<int, int> addInstance(glm::dmat4& iMatrix);

		AABB& finalize(Model& iModel);
		void translate(glm::dvec3& iVector);
		void computeBVH();

	public:

		json_spirit::mObject toJson(int32_t& iBufferOffset, std::string iIndent);
		void write(FILE* iFile, int32_t& iBufferOffset);
		void print(std::string iIndent);

		Geometry* mGeometry;

	private:

		AABB mAABB;

		std::vector<glm::dmat4> mInstanceMatrix;
		std::vector<AABB> mInstanceAABB;
};

class CombinedNode: public UniqueNode
{
	public:

		CombinedNode();

		std::tuple<int, int> addGeometry(Geometry* iGeometry);

		AABB& finalize(Model& iModel);
		void translate(glm::dvec3& iVector);
		void computeBVH();

	public:

		json_spirit::mObject toJson(int32_t& iBufferOffset, std::string iIndent);
		void write(FILE* iFile, int32_t& iBufferOffset);
		void print(std::string iIndent);

	private:

		AABB mAABB;
		Geometry* mGeometry;

		std::vector<uint32_t> mObjectIndex;
		uint32_t mObjectCount;
};

class MaterialNode: public UniqueNode
{
	public:

		MaterialNode(int32_t iMaterial);

		std::tuple<int, int> addGeometry(Geometry* iGeometry);
		void addInstance(InstancedNode* iInstance);

		int32_t mMaterial;

		AABB& finalize(Model& iModel);
		void translate(glm::dvec3& iVector);
		void computeBVH();

	public:

		json_spirit::mObject toJson(int32_t& iBufferOffset, std::string iIndent);
		void write(FILE* iFile, int32_t& iBufferOffset);
		void print(std::string iIndent);

	private:

		AABB mAABB;

		std::vector<InstancedNode*> mInstanced;   
		CombinedNode* mCombined;				
};


class InternalNode //: public UniqueNode
{
	public:

		InternalNode(json_spirit::mObject& iInfo);

		void addInternal(InternalNode* iInternal);
		void addMaterial(MaterialNode* iMaterial);

		AABB& finalize(Model& iModel);
		void translate(glm::dvec3& iVector);
		void computeBVH();
		
		void print(std::string iIndent);

		void addLogcialNode(json_spirit::mObject iNode);
		int currentLogcialNode();
		void addGeometryRecord(int32_t iIndex, std::tuple<int, int> iRecord);

	public:

		json_spirit::mObject toTree();
		json_spirit::mObject toJson(int32_t& iBufferOffset, std::string iIndent);
		void write(FILE* iFile, int32_t& iBufferOffset);

		std::vector<MaterialNode*> mMaterial;   
		std::vector<InternalNode*> mInternal;   

		std::map<int, std::vector<std::tuple<int, int>>> mLogicalRecords;

	private:

		class AlphaComparator
		{
			public:

				AlphaComparator(json_spirit::mArray& iMaterials);
				bool operator() (MaterialNode* a, MaterialNode*  b);
	
			protected:

				bool hasAlpha(json_spirit::mObject& iMaterial);
				json_spirit::mArray& mMaterials;
		};

		AABB mAABB;

		json_spirit::mObject mInfo;
};

class Model
{
	public:

		Model(InternalNode* iInternal);

		json_spirit::mObject write(boost::filesystem::path iRoot);

		InternalNode* mInternal;

		std::vector<Geometry*> mGeometries;

		json_spirit::mArray mImages;
		json_spirit::mArray mTextures;
		json_spirit::mArray mMaterials;
		json_spirit::mArray mSamplers;

		glm::dmat4 mTransform;

	private:
		

		void getInternals(json_spirit::mArray& iArray, InternalNode* iInternal);

		AABB mAABB;
};


