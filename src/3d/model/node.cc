#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/epsilon.hpp>

#include "node.h"
#include "mesh.h"


UniqueNode::UniqueNode()
: mId(sId++)
{
}

int32_t UniqueNode::sId = 1;





InstancedNode::InstancedNode(Geometry* iGeometry)
: mGeometry(iGeometry)
{
	mGeometry->incRef();
}

std::tuple<int, int> InstancedNode::addInstance(glm::dmat4& iMatrix)
{
	mInstanceMatrix.push_back(iMatrix);
	return std::tuple<int, int>(mId, mInstanceMatrix.size() - 1);
};

AABB& InstancedNode::finalize(Model& iModel)
{
	mGeometry->pushGeometry(iModel.mGeometries);

	BOOST_LOG_TRIVIAL(info) << "********** InstancedNode::finalize ";
	if (iModel.mTransform != glm::dmat4(1.0))
	{
		for (std::vector<glm::dmat4>::iterator lIter = mInstanceMatrix.begin(); lIter != mInstanceMatrix.end(); lIter++)
		{
			*lIter = iModel.mTransform*(*lIter);
		}
	}

	for (std::vector<glm::dmat4>::iterator lIter = mInstanceMatrix.begin(); lIter != mInstanceMatrix.end(); lIter++)
	{
		AABB lAABB;
		mGeometry->getAABB(*lIter, lAABB);
		mInstanceAABB.push_back(lAABB);
		mAABB.grow(lAABB);
	}

	return mAABB;
}

void InstancedNode::computeBVH()
{
	mGeometry->computeBVH();
}

void InstancedNode::translate(glm::dvec3& iVector)
{
	for (std::vector<glm::dmat4>::iterator lIter = mInstanceMatrix.begin(); lIter != mInstanceMatrix.end(); lIter++)
	{
		(*lIter)[3][0] += iVector[0];
		(*lIter)[3][1] += iVector[1];
		(*lIter)[3][2] += iVector[2];
	}	
	
	for (std::vector<AABB>::iterator lIter = mInstanceAABB.begin(); lIter != mInstanceAABB.end(); lIter++)
	{
		lIter->translate(iVector);
	}

	mAABB.translate(iVector);
};

json_spirit::mObject InstancedNode::toJson(int32_t& iBufferOffset, std::string iIndent)
{
	json_spirit::mArray lAabbArray;
	for (std::vector<AABB>::iterator lIter = mInstanceAABB.begin(); lIter != mInstanceAABB.end(); lIter++)
	{
		json_spirit::mObject lAabb;
		lAabb["min"] = (*lIter).minJson();
		lAabb["max"] = (*lIter).maxJson();
		lAabbArray.push_back(lAabb);
	}
	json_spirit::mObject lBuffer;
	lBuffer["type"] = "float";
	lBuffer["size"] = (long)mInstanceMatrix.size()*16;
	lBuffer["offset"] = iBufferOffset;
	iBufferOffset += mInstanceMatrix.size()*16*sizeof(float);

	json_spirit::mObject lObject;
	lObject["matrix"] = lBuffer;
	lObject["aabb"] = lAabbArray;
	lObject["geometry"] = mGeometry->mIndex;
	lObject["min"] = mAABB.minJson();
	lObject["max"] = mAABB.maxJson();
	lObject["id"] =  "i" + std::to_string(mId);;
	return lObject;
};

void InstancedNode::write(FILE* iFile, int32_t& iBufferOffset)
{
	std::vector<float> lBuffer;
	lBuffer.reserve(mInstanceMatrix.size()*16);

	for (std::vector<glm::dmat4>::iterator lIter = mInstanceMatrix.begin(); lIter != mInstanceMatrix.end(); lIter++)
	{
		for (int i=0; i<16; i++)
		{
			lBuffer.push_back((*lIter)[i/4][i%4]);
		}
	}

	fwrite(&lBuffer[0], 1, sizeof(float)*mInstanceMatrix.size()*16, iFile);
	iBufferOffset += sizeof(float)*mInstanceMatrix.size()*16;
}

void InstancedNode::print(std::string iIndent)
{
	BOOST_LOG_TRIVIAL(info) << iIndent << "INSTANCED - " << "i" + std::to_string(mId) << "  "  << mInstanceMatrix.size() << " objects ";
};




CombinedNode::CombinedNode()
: mGeometry(new Geometry("Combined", new Mesh()))
, mObjectCount(0)
, mAABB()
{
	mGeometry->incRef();
};

std::tuple<int, int> CombinedNode::addGeometry(Geometry* iGeometry)
{
	for (int i=0; i<iGeometry->primitiveCount(); i++)
	{
		mObjectIndex.push_back(mObjectCount);
	}
	mObjectCount++;
	mGeometry->append(iGeometry);
	return std::tuple<int, int>(mId, mObjectCount - 1);
};


AABB& CombinedNode::finalize(Model& iModel)
{
	mGeometry->pushGeometry(iModel.mGeometries);

	if (iModel.mTransform != glm::dmat4(1.0))
	{
		mGeometry->transform(iModel.mTransform);
	}

	mGeometry->getAABB(mAABB);
	
	return mAABB;
}

void CombinedNode::computeBVH()
{
	mGeometry->computeBVH(&mObjectIndex);
}

void CombinedNode::translate(glm::dvec3& iVector)
{
	mGeometry->translate(iVector);
	mAABB.translate(iVector);
};

json_spirit::mObject CombinedNode::toJson(int32_t& iBufferOffset, std::string iIndent)
{
	json_spirit::mObject lObject;
	lObject["geometry"] = mGeometry->mIndex;
	lObject["min"] = mAABB.minJson();
	lObject["max"] = mAABB.maxJson();
	lObject["parts"] = (long)mObjectCount;
	lObject["id"] = "i" + std::to_string(mId);

	json_spirit::mObject lBuffer;
	lBuffer["type"] = "int";
	lBuffer["size"] = (long)mObjectIndex.size();
	lBuffer["offset"] = iBufferOffset;
	iBufferOffset += mObjectIndex.size()*sizeof(uint32_t);

	lObject["index"] = lBuffer;
	return lObject;
};

void CombinedNode::write(FILE* iFile, int32_t& iBufferOffset)
{
	fwrite(&mObjectIndex[0], 1, sizeof(int32_t)*mObjectIndex.size(), iFile);
	iBufferOffset += mObjectIndex.size()*sizeof(int32_t);
}

void CombinedNode::print(std::string iIndent)
{
	BOOST_LOG_TRIVIAL(info) << iIndent << "COMBINED - " << "i" + std::to_string(mId) << "  " << mGeometry->primitiveCount() << " triangles";
};







MaterialNode::MaterialNode(int32_t iMaterial)
: mMaterial(iMaterial)
, mCombined(0)
{
};

std::tuple<int, int> MaterialNode::addGeometry(Geometry* iGeometry)
{
	if (!mCombined)
	{
		mCombined = new CombinedNode();
	}

	return mCombined->addGeometry(iGeometry);
};

void MaterialNode::addInstance(InstancedNode* iInstance)
{
	mInstanced.push_back(iInstance);
}

void MaterialNode::computeBVH()
{
	for (std::vector<InstancedNode*>::iterator lIter1 = mInstanced.begin(); lIter1 != mInstanced.end(); lIter1++)
	{
		(*lIter1)->computeBVH();
	}

	if (mCombined)
	{
		mCombined->computeBVH();
	}
}

AABB& MaterialNode::finalize(Model& iModel)
{
	for (std::vector<InstancedNode*>::iterator lIter1 = mInstanced.begin(); lIter1 != mInstanced.end(); lIter1++)
	{
		mAABB.grow((*lIter1)->finalize(iModel));
	}

	if (mCombined)
	{
		mAABB.grow(mCombined->finalize(iModel));
	}

	return mAABB;
}

void MaterialNode::translate(glm::dvec3& iVector)
{
	for (std::vector<InstancedNode*>::iterator lIter = mInstanced.begin(); lIter != mInstanced.end(); lIter++)
	{
		(*lIter)->translate(iVector);
	}

	if (mCombined)
	{
		mCombined->translate(iVector);
	}

	mAABB.translate(iVector);
}

json_spirit::mObject MaterialNode::toJson(int32_t& iBufferOffset, std::string iIndent)
{
	json_spirit::mArray lArray0;
	for (std::vector<InstancedNode*>::iterator lIter = mInstanced.begin(); lIter != mInstanced.end(); lIter++)
	{
		lArray0.push_back((*lIter)->toJson(iBufferOffset, iIndent + "  "));
	}

	json_spirit::mObject lObject;
	lObject["instance"] = lArray0;
	if (mCombined)
	{
		lObject["combined"] = mCombined->toJson(iBufferOffset, iIndent + "  ");
	}
	lObject["material"] = mMaterial;
	lObject["min"] = mAABB.minJson();
	lObject["max"] = mAABB.maxJson();
	lObject["id"] = "m" + std::to_string(mId);
	return lObject;
};

void MaterialNode::write(FILE* iFile, int32_t& iBufferOffset)
{
	for (std::vector<InstancedNode*>::iterator lIter = mInstanced.begin(); lIter != mInstanced.end(); lIter++)
	{
		(*lIter)->write(iFile, iBufferOffset);
	}

	if (mCombined)
	{
		mCombined->write(iFile, iBufferOffset);
	}
}

void MaterialNode::print(std::string iIndent)
{
	BOOST_LOG_TRIVIAL(info) << iIndent << "MATERIAL " << mMaterial;
	iIndent += "  ";
	for (std::vector<InstancedNode*>::iterator lIter = mInstanced.begin(); lIter != mInstanced.end(); lIter++)
	{
		(*lIter)->print(iIndent);
	}
	if (mCombined)
	{
		mCombined->print(iIndent);
	}
};









InternalNode::InternalNode(json_spirit::mObject& iInfo)
: mInfo(iInfo)
{
	int i=12;
};

void InternalNode::addInternal(InternalNode* iInternal)
{
	mInternal.push_back(iInternal);
};

void InternalNode::addMaterial(MaterialNode* iMaterial)
{
	mMaterial.push_back(iMaterial);
};

AABB& InternalNode::finalize(Model& iModel)
{
	std::string lName;

	if (mInfo.find("name") != mInfo.end())
	{
		lName = mInfo["name"].get_str();
	}

	for (std::vector<InternalNode*>::iterator lIter = mInternal.begin(); lIter != mInternal.end(); lIter++)
	{
		mAABB.grow((*lIter)->finalize(iModel));
	}
	
	for (std::vector<MaterialNode*>::iterator lIter = mMaterial.begin(); lIter != mMaterial.end(); lIter++)
	{
		mAABB.grow((*lIter)->finalize(iModel));
	}

	AlphaComparator lComparator(iModel.mMaterials);
	std::sort (mMaterial.begin(), mMaterial.end(), lComparator);     
	return mAABB;
}

void InternalNode::translate(glm::dvec3& iVector)
{
	for (std::vector<InternalNode*>::iterator lIter = mInternal.begin(); lIter != mInternal.end(); lIter++)
	{
		(*lIter)->translate(iVector);
	}

	for (std::vector<MaterialNode*>::iterator lIter = mMaterial.begin(); lIter != mMaterial.end(); lIter++)
	{
		(*lIter)->translate(iVector);
	}

	mAABB.translate(iVector);
}

void InternalNode::computeBVH()
{
	for (std::vector<InternalNode*>::iterator lIter = mInternal.begin(); lIter != mInternal.end(); lIter++)
	{
		(*lIter)->computeBVH();
	}

	for (std::vector<MaterialNode*>::iterator lIter = mMaterial.begin(); lIter != mMaterial.end(); lIter++)
	{
		(*lIter)->computeBVH();
	}
}

void InternalNode::addLogcialNode(json_spirit::mObject iNode)
{
	if (mInfo.find("children") == mInfo.end())
	{
		mInfo["children"] = json_spirit::mArray();
	}

	json_spirit::mArray& lChildren = mInfo["children"].get_array();
	iNode["records"] = json_spirit::mObject();
	lChildren.push_back(iNode);
};

int InternalNode::currentLogcialNode()
{
	if (mInfo.find("children") == mInfo.end())
	{
		return -1;
	}

	json_spirit::mArray& lChildren = mInfo["children"].get_array();
	return lChildren.size()-1;
};

void InternalNode::addGeometryRecord(int32_t iIndex, std::tuple<int, int> iRecord)
{
	mLogicalRecords[iIndex].push_back(iRecord);
};

json_spirit::mObject InternalNode::toTree()
{
	if (mInfo.find("children") == mInfo.end())
	{
		mInfo["children"] = json_spirit::mArray();
	}

	json_spirit::mArray& lChildren = mInfo["children"].get_array();
	for (std::vector<InternalNode*>::iterator lIter = mInternal.begin(); lIter != mInternal.end(); lIter++)
	{
		lChildren.push_back((*lIter)->toTree());
	}

	for (std::map<int, std::vector<std::tuple<int, int>>>::iterator lIter = mLogicalRecords.begin(); lIter != mLogicalRecords.end(); lIter++)
	{
		// physical nodes and parts
		std::vector<std::tuple<int, int>>& lVector = lIter->second;
		sort(lVector.begin(), lVector.end());

		// logical node
		json_spirit::mObject& lNode = lChildren[lIter->first].get_obj();
		json_spirit::mObject& lRecords = lNode["records"].get_obj();

		std::vector<std::tuple<int, int>>::iterator lIter1 = lVector.begin();
		json_spirit::mArray lParts;
		do
		{
			lParts.push_back(std::get<1>(*lIter1));

			int lCurrentObject = std::get<0>(*lIter1);
			lIter1++;
			if (lIter1 != lVector.end())
			{
				if (std::get<0>(*lIter1) != lCurrentObject)
				{
					lRecords["i"+std::to_string(lCurrentObject)] = lParts;
					lParts.clear();
				}
			}
			else
			{
				lRecords["i"+std::to_string(lCurrentObject)] = lParts;
			}
		}
		while (lIter1 != lVector.end());
	}

	return mInfo;
};


json_spirit::mObject InternalNode::toJson(int32_t& iBufferOffset, std::string iIndent)
{
	json_spirit::mObject lObject;

	json_spirit::mArray lArray1;
	for (std::vector<InternalNode*>::iterator lIter = mInternal.begin(); lIter != mInternal.end(); lIter++)
	{
		lArray1.push_back((*lIter)->toJson(iBufferOffset, iIndent + "  "));
	}
	lObject["internal"] = lArray1;

	json_spirit::mArray lArray0;
	for (std::vector<MaterialNode*>::iterator lIter = mMaterial.begin(); lIter != mMaterial.end(); lIter++)
	{
		lArray0.push_back((*lIter)->toJson(iBufferOffset, iIndent + "  "));
	}
	lObject["material"] = lArray0;

	lObject["min"] = mAABB.minJson();
	lObject["max"] = mAABB.maxJson();
	lObject["id"] = mInfo["id"];
	return lObject;
};

void InternalNode::write(FILE* iFile, int32_t& iBufferOffset)
{
	for (std::vector<InternalNode*>::iterator lIter = mInternal.begin(); lIter != mInternal.end(); lIter++)
	{
		(*lIter)->write(iFile, iBufferOffset);
	}
	for (std::vector<MaterialNode*>::iterator lIter = mMaterial.begin(); lIter != mMaterial.end(); lIter++)
	{
		(*lIter)->write(iFile, iBufferOffset);
	}
}

void InternalNode::print(std::string iIndent)
{
	if (mInfo.find("name") != mInfo.end())
	{
		BOOST_LOG_TRIVIAL(info) << iIndent << "INTERNAL " << mInfo["name"].get_str();
	}
	iIndent += "  ";
	for (std::vector<InternalNode*>::iterator lIter = mInternal.begin(); lIter != mInternal.end(); lIter++)
	{
		(*lIter)->print(iIndent);
	}
	for (std::vector<MaterialNode*>::iterator lIter = mMaterial.begin(); lIter != mMaterial.end(); lIter++)
	{
		(*lIter)->print(iIndent);
	}
};




InternalNode::AlphaComparator::AlphaComparator(json_spirit::mArray& iMaterials)
: mMaterials(iMaterials)
{
}
			
bool InternalNode::AlphaComparator::operator() (MaterialNode* a, MaterialNode*  b)
{ 
	return hasAlpha(mMaterials[a->mMaterial].get_obj()) < hasAlpha(mMaterials[b->mMaterial].get_obj());
}

bool InternalNode::AlphaComparator::hasAlpha(json_spirit::mObject& iMaterial)
{
	if (iMaterial["type"] == "PbrMaterial")
	{
		if (iMaterial.find("alphaMode") != iMaterial.end())
		{
			std::string a1 = iMaterial["alphaMode"].get_str();
			if (a1 == "BLEND")
			{
     			return true;
			}
		}
	}
	else if (iMaterial["type"] == "IfcSurfaceStyle")
	{
		if (iMaterial.find("surfaceColor") != iMaterial.end())
		{
			json_spirit::mObject lColor = iMaterial["surfaceColor"].get_obj();
			if (lColor.find("a") != lColor.end() && lColor["a"].get_real() != 1.0)
			{
     			return true;
			}
		}
	}

	return false;
}







Model::Model(InternalNode* iInternal)
: mInternal(iInternal)
, mTransform(1.0)
{
}

json_spirit::mObject Model::write(boost::filesystem::path iRoot)
{
	BOOST_LOG_TRIVIAL(info) << "**********************************";

	// collaps internal hierachy
	while (mInternal->mInternal.size() == 1 && mInternal->mMaterial.size() == 0)
	{
		mInternal = mInternal->mInternal[0];
	}

	mAABB.grow(mInternal->finalize(*this));

	// center Scene
	glm::dvec3 lCenter(-(mAABB.max[0] + mAABB.min[0])/2,-(mAABB.max[1] + mAABB.min[1])/2,-(mAABB.max[2] + mAABB.min[2])/2);
	json_spirit::mObject lTranslate;
	lTranslate["x"] = -lCenter[0];
	lTranslate["y"] = -lCenter[1];
	lTranslate["z"] = -lCenter[2];
	mInternal->translate(lCenter);
	mAABB.translate(lCenter);

	BOOST_LOG_TRIVIAL(info) << "....Computing BVH....";
	mInternal->computeBVH();


	//
	// create Json
	//
	BOOST_LOG_TRIVIAL(info) << "....Creating Json....";
	std::string lIndent("  ");

	json_spirit::mObject lObject;
	lObject["min"] = mAABB.minJson();
	lObject["max"] = mAABB.maxJson();
	lObject["translate"] = lTranslate;
	lObject["materials"] = mMaterials;
	lObject["textures"] = mTextures;
	lObject["images"] = mImages;
	lObject["samplers"] = mSamplers;
	lObject["type"] = 2; //  M.GeometryNode.TRIANGLES in Javascript

	int32_t lBufferOffset = 0;

	json_spirit::mArray lArray0;
	for (std::vector<Geometry*>::iterator lIter = mGeometries.begin(); lIter != mGeometries.end(); lIter++)
	{
		lArray0.push_back((*lIter)->toJson(lBufferOffset, lIndent));
	}
	lObject["geometries"] = lArray0;
	lObject["internal"] = mInternal->toJson(lBufferOffset, lIndent);
	lObject["hierarchy"] = mInternal->toTree();

	//
	// create Binary  
	//
	boost::filesystem::create_directory(boost::filesystem::path("root"));

	// copy images
	BOOST_LOG_TRIVIAL(info) << "....Copying Images....";
	for (int i=0; i<mImages.size(); i++)
	{
		json_spirit::mObject lImage = mImages[i].get_obj();
		std::string lLocalPath = lImage["uri"].get_str();
		boost::filesystem::create_directories(boost::filesystem::path("./root/"+lLocalPath).parent_path());
		if ( !boost::filesystem::exists("./root/"+lLocalPath))
		{
			boost::filesystem::path lPath = iRoot / lLocalPath;
			if (boost::filesystem::exists(lPath))
			{
				boost::filesystem::copy_file(lPath, "./root/"+lLocalPath);
			}
		}
	}

	std::stringstream lStream;
	json_spirit::write_stream(json_spirit::mValue(lObject), lStream);
	std::string lString = lStream.str();

	// write buffers
	FILE* lFile = fopen("./root/model.bin", "wb");

	// write scene tree
	uint32_t lHeaderSize = lString.size();
	fwrite(&lHeaderSize, sizeof(uint32_t), 1, lFile);
	fwrite(lString.c_str(), 1, lString.length(), lFile);
	// align to 4 byte
	int lLength = (lString.length()+sizeof(uint32_t))%4;
	if (lLength != 0)
	{
		char FILL[4] = {0};
		fwrite(FILL, 1, 4-lLength, lFile);
	}

	lBufferOffset = 0;
	for (std::vector<Geometry*>::iterator lIter = mGeometries.begin(); lIter != mGeometries.end(); lIter++)
	{
		(*lIter)->write(lFile, lBufferOffset);
	}
	mInternal->write(lFile, lBufferOffset);

	fclose(lFile);

	BOOST_LOG_TRIVIAL(info) << "....Wrting Done....";
	BOOST_LOG_TRIVIAL(info) << "**********************************";
	
	// JSTIER deal with Root some time... seems superfluous
	json_spirit::mObject lRoot;
	lRoot["min"] = mAABB.minJson();
	lRoot["max"] = mAABB.maxJson();;
	lRoot["translate"] = lTranslate;
	lRoot["textures"] = mTextures;
	lRoot["images"] = mImages;
	lRoot["samplers"] = mSamplers;

	return lRoot;
}

void Model::getInternals(json_spirit::mArray& iArray, InternalNode* iInternal)
{
}