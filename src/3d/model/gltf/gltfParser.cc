#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue>
#include <strstream>

#define _USE_MATH_DEFINES
#include <math.h>

#include "../node.h"
#include "../parser.h"
#include "../base64.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "gltfParser.h"

GltfParser::GltfParser()
: Parser()
{
};

Node* GltfParser::createNode(json_spirit::mObject& iJsonNode, std::string iIndent)
{
	Node* lNode = new Node(0, Node::GEOMETRY);
	if (iJsonNode.find("name") != iJsonNode.end())
	{
		lNode->mInfo["name"] =  iJsonNode["name"].get_str();
		BOOST_LOG_TRIVIAL(info) << iIndent << "NODE " + lNode->mInfo["name"].get_str();
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << iIndent << "NODE unnamed";
	}
	
	if (iJsonNode.find("matrix") != iJsonNode.end())
	{
		json_spirit::mArray lArray = iJsonNode["matrix"].get_array();
			
		for (int i=0; i<16; i++)
		{
			lNode->mMatrix[i/4][i%4] = (float)lArray[i].get_real();
		}
	}
	else
	{
		glm::dmat4 lR(1.0);
		glm::dmat4 lT(1.0);
		glm::dmat4 lS(1.0);
		
		if (iJsonNode.find("translation") != iJsonNode.end())
		{
			json_spirit::mArray lArray = iJsonNode["translation"].get_array();

			lT = glm::translate(glm::mat4(1.0f), glm::vec3(lArray[0].get_real(), lArray[1].get_real(), lArray[2].get_real()));
		}
		if (iJsonNode.find("rotation") != iJsonNode.end())
		{
			json_spirit::mArray lArray = iJsonNode["rotation"].get_array();

			lR = glm::mat4_cast(glm::quat(lArray[3].get_real(), lArray[0].get_real(), lArray[1].get_real(), lArray[2].get_real()));
		}
		if (iJsonNode.find("scale") != iJsonNode.end())
		{
			json_spirit::mArray lArray = iJsonNode["scale"].get_array();

			lS = glm::scale(glm::mat4(1.0f), glm::vec3(lArray[0].get_real(), lArray[1].get_real(), lArray[2].get_real()));
		}
		
		lNode->mMatrix = lT*lR*lS;
	}

	if (iJsonNode.find("mesh") != iJsonNode.end())
	{
		json_spirit::mObject lMesh = mMeshes[iJsonNode["mesh"].get_int()].get_obj();
			
		json_spirit::mArray lPrimitives = lMesh["primitives"].get_array();
		for (size_t i=0; i<lPrimitives.size(); i++)
		{
			json_spirit::mObject lPrimitive = lPrimitives[i].get_obj();
			int lMode = GltfParser::TRIANGLES;
			if (lPrimitive.find("mode") != lPrimitive.end())
			{
				lMode = lPrimitive["mode"].get_int();
			}
				
			if (lMode == GltfParser::TRIANGLES)
			{
				BOOST_LOG_TRIVIAL(info) << iIndent << "  MESH " << iJsonNode["mesh"].get_int();

				lNode->addNode(mMeshNodes[iJsonNode["mesh"].get_int()]);
				if (lPrimitive.find("material") != lPrimitive.end())
				{
					lNode->mMaterial = lPrimitive["material"].get_int();

					json_spirit::mObject lAttributes = lPrimitive["attributes"].get_obj();
					if (lAttributes.find("TANGENT") == lAttributes.end())
					{
						// remove normal texture from material if there is no tangent space
						json_spirit::mObject lMaterial = mMaterials[lNode->mMaterial].get_obj();
						if (lMaterial.find("normalTexture") != lMaterial.end())
						{
							lMaterial.erase(lMaterial.find("normalTexture"));
							mMaterials[lNode->mMaterial] = lMaterial;
						}
					}
				}
				else
				{
					lNode->mMaterial = mMaterials.size();
				}
			}
		}
	}
		
	if (iJsonNode.find("children") != iJsonNode.end())
	{
		json_spirit::mArray lChildren = iJsonNode["children"].get_array();
			
		for (size_t i=0; i<lChildren.size(); i++)
		{
			lNode->addNode(createNode(mNodes[lChildren[i].get_int()].get_obj(), iIndent + "   "));
		}
	}
		
	return lNode;
}

template <class T> void GltfParser::createIndexedAttibutes(Mesh& iMesh, json_spirit::mObject& iAttributes, T* iIndex, uint32_t iCount)
{
	iMesh.appendVertex(createIndexedAttibute(mAccessors[iAttributes["POSITION"].get_int()].get_obj(), iIndex, iCount), iCount);

	if (iAttributes.find("NORMAL") != iAttributes.end())
	{
		iMesh.appendNormal(createIndexedAttibute(mAccessors[iAttributes["NORMAL"].get_int()].get_obj(), iIndex, iCount), iCount);
	}

	if (iAttributes.find("TANGENT") != iAttributes.end())
	{
		iMesh.appendTangent(createIndexedAttibute(mAccessors[iAttributes["TANGENT"].get_int()].get_obj(), iIndex, iCount), iCount);
	}

	if (iAttributes.find("TEXCOORD_0") != iAttributes.end())
	{
		iMesh.appendUv(createIndexedAttibute(mAccessors[iAttributes["TEXCOORD_0"].get_int()].get_obj(), iIndex, iCount), iCount);
	}
}

template <class T> void endswap(T *objp)
{
	unsigned char *memp = reinterpret_cast<unsigned char*>(objp);
	std::reverse(memp, memp + sizeof(T));
}

template <class T> float* GltfParser::createIndexedAttibute(json_spirit::mObject& iAccessor, T* iIndex, uint32_t iCount)
{
	BufferView* lBufferView = mBufferViews[iAccessor["bufferView"].get_int()];
	float* lIndexed = (float*)lBufferView->read(iAccessor); 

	uint8_t lElements = BufferView::sElements[iAccessor["type"].get_str()];

	float* lUnindexed = new float[iCount*lElements];
	for (uint32_t e=0; e<iCount; e++)
	{
		T lIndex = (T)iIndex[e];
		for (int c=0; c<lElements; c++)
		{
			lUnindexed[e*lElements+c] = lIndexed[lIndex*lElements+c];
		}
	}
	return lUnindexed;
}


Geometry* GltfParser::createMesh(json_spirit::mObject& iPrimitive)
{
	Mesh* lMesh = new Mesh();
	
	if (iPrimitive.find("indices") != iPrimitive.end())
	{
		json_spirit::mObject lAccessor = mAccessors[iPrimitive["indices"].get_int()].get_obj();
		int lIndexCount = lAccessor["count"].get_int();

		BufferView* lBufferView = mBufferViews[lAccessor["bufferView"].get_int()];

		json_spirit::mObject lAttributes = iPrimitive["attributes"].get_obj();
		switch (lAccessor["componentType"].get_int())
		{
		case BufferView::UNSIGNED_BYTE:
			createIndexedAttibutes(*lMesh, lAttributes, (uint8_t*)lBufferView->read(lAccessor), lIndexCount);
			break;
		case BufferView::UNSIGNED_SHORT:
			createIndexedAttibutes(*lMesh, lAttributes, (uint16_t*)lBufferView->read(lAccessor), lIndexCount);
			break;
		case BufferView::UNSIGNED_INT:
			createIndexedAttibutes(*lMesh, lAttributes, (uint32_t*)lBufferView->read(lAccessor), lIndexCount);
			break;
		}
	}
	else
	{
		json_spirit::mObject lAttributes = iPrimitive["attributes"].get_obj();

		json_spirit::mObject lAccessor = mAccessors[lAttributes["POSITION"].get_int()].get_obj();
		uint32_t lElementCount = lAccessor["count"].get_int();

		BufferView* lBufferView = mBufferViews[lAccessor["bufferView"].get_int()];
		lMesh->appendVertex((float*)lBufferView->read(lAccessor), lElementCount); //createAttribute(lAccessor, lIndex, 3);

		if (lAttributes.find("NORMAL") != lAttributes.end())
		{
			json_spirit::mObject lAccessor = mAccessors[lAttributes["NORMAL"].get_int()].get_obj();
			BufferView* lBufferView = mBufferViews[lAccessor["bufferView"].get_int()];
			lMesh->appendNormal((float*)lBufferView->read(lAccessor), lElementCount); //createAttribute(lAccessor, lIndex, 3);
		}

		if (lAttributes.find("TANGENT") != lAttributes.end())
		{
			json_spirit::mObject lAccessor = mAccessors[lAttributes["TANGENT"].get_int()].get_obj();
			BufferView* lBufferView = mBufferViews[lAccessor["bufferView"].get_int()];
			lMesh->appendTangent((float*)lBufferView->read(lAccessor), lElementCount); //createAttribute(lAccessor, lIndex, 4);
		}

		if (lAttributes.find("TEXCOORD_0") != lAttributes.end())
		{
			json_spirit::mObject lAccessor = mAccessors[lAttributes["TEXCOORD_0"].get_int()].get_obj();
			BufferView* lBufferView = mBufferViews[lAccessor["bufferView"].get_int()];
			lMesh->appendUv((float*)lBufferView->read(lAccessor), lElementCount); //createAttribute(lAccessor, lIndex, 2);
		}
	}
		
	return new Geometry(iPrimitive.find("name") != iPrimitive.end() ? iPrimitive["name"].get_str() : "unnamed", lMesh);
}

Model* GltfParser::process(std::string iFile, float iScalar)
{
	std::ifstream lStream(iFile);
	json_spirit::mValue lJson;
	json_spirit::read_stream(lStream, lJson);
	lStream.close();

	boost::filesystem::path lDirectory = boost::filesystem::path(iFile).parent_path();
	
	/*
	std::stringstream lString;
	json_spirit::write_stream(json_spirit::mValue(lJson), lString);
	std::string json = lString.str();
	BOOST_LOG_TRIVIAL(info) <<  lString.str();
	*/
	json_spirit::mObject& lModel = lJson.get_obj();

	BOOST_LOG_TRIVIAL(info) << "creating buffers";
	json_spirit::mArray lBuffers = lModel["buffers"].get_array();
	std::istream** lStreams = new std::istream*[lBuffers.size()];
	for (size_t i=0; i<lBuffers.size(); i++)
    {
		json_spirit::mObject lBuffer = lBuffers[i].get_obj();
		std::string lUri = lBuffer["uri"].get_str();

		if (boost::starts_with(lUri, "data:application/octet-stream;base64,"))
		{
			std::vector<unsigned char> lDecoded = base64_decode(lUri.substr(strlen("data:application/octet-stream;base64,")));
			lStreams[i] = new std::istream(new std::strstreambuf(lDecoded.data(), lDecoded.size()));
		}
		else
		{
			std::replace(lUri.begin(), lUri.end(), ' ', '_');
			boost::filesystem::path lPath = lDirectory / boost::filesystem::path(lUri);
			lStreams[i] = new std::ifstream(lPath.c_str(), std::ifstream::in | std::ifstream::binary);
		}
	   	//lStream[i].order(ByteOrder.LITTLE_ENDIAN);
    }

	BOOST_LOG_TRIVIAL(info) << "creating buffer views";
 	json_spirit::mArray lBufferViews = lModel["bufferViews"].get_array();
  	mBufferViews = new BufferView*[lBufferViews.size()];
	for (size_t i=0; i<lBufferViews.size(); i++)
	{
		json_spirit::mObject lBufferView = lBufferViews[i].get_obj();
		mBufferViews[i] = new BufferView(lBufferView, lStreams[lBufferView["buffer"].get_int()]);
	}
		
	// read materials
	if (lModel.find("materials") != lModel.end())
	{
		mMaterials = lModel["materials"].get_array();
	}
	json_spirit::mObject lDefaultMaterial;
	lDefaultMaterial["name"] = "undefined";
	mMaterials.push_back(lDefaultMaterial);
	for (size_t i=0; i<mMaterials.size(); i++)
	{
		json_spirit::mObject lMaterial = mMaterials[i].get_obj();
		lMaterial["type"] = "PbrMaterial";
		mMaterials[i] = lMaterial;
	}

	if (lModel.find("images") != lModel.end())
	{
		json_spirit::mArray lImages = lModel["images"].get_array();
			
		for (size_t i=0; i<lImages.size(); i++)
		{
			json_spirit::mObject lImage = lImages[i].get_obj();
			std::string lUri = lImage["uri"].get_str();
			std::replace(lUri.begin(), lUri.end(), ' ', '_');
			lImage["uri"] = lUri;
				
			mImages.push_back(lImage);
		}
	}
	if (lModel.find("textures") != lModel.end())
	{
		mTextures = lModel["textures"].get_array();
	}
	if (lModel.find("samplers") != lModel.end())
	{
		mSamplers = lModel["samplers"].get_array();
	}

    // parse scene
	mNodes = lModel["nodes"].get_array();
	mMeshes = lModel["meshes"].get_array();
	mAccessors = lModel["accessors"].get_array();

	for (size_t i=0; i<mMeshes.size(); i++)
	{
		json_spirit::mObject lMesh = mMeshes[i].get_obj();
		if (lMesh.find("name") != lMesh.end())
		{
			BOOST_LOG_TRIVIAL(info) << lMesh["name"].get_str() << "\n";
		}
		json_spirit::mArray lPrimitives = lMesh["primitives"].get_array();

		Node* lMeshNode = new Node(0,Node::GEOMETRY);
		for (size_t p=0; p<lPrimitives.size(); p++)
		{
			json_spirit::mObject lPrimitive = lPrimitives[p].get_obj();
				
			int lMode = GltfParser::TRIANGLES;
			if (lPrimitive.find("mode") != lPrimitive.end())
			{
				lMode = lPrimitive["mode"].get_int();
			}
				
			if (lMode == GltfParser::TRIANGLES)
			{
				Node* lNode = new Node(0,Node::GEOMETRY);

				lNode->mGeometry = createMesh(lPrimitive);

				if (iScalar != 1.0)
				{
					lNode->mGeometry->scale(glm::dvec3(iScalar));
   				} 
				lMeshNode->addNode(lNode);
			}
		}

		mMeshNodes.push_back(lMeshNode);
	}

	// Only look at scene 0
	json_spirit::mArray lScenes = lModel["scenes"].get_array();
	json_spirit::mObject lScene = lScenes[0].get_obj();
		
	Node* lRoot = new Node(0, Node::PHYSICAL);
	lRoot->mInfo["name"] = iFile;
	json_spirit::mArray lNodes = lScene["nodes"].get_array();
	for (size_t i=0; i<lNodes.size(); i++)
	{
		int lNode = lNodes[i].get_int();
		//if (lNode == 17 || lNode == 8)
		{
			lRoot->addNode(createNode(mNodes[lNode].get_obj(), ""));
		}
	}
	
	//lRoot->mChildren.push_back(createNode(mNodes[8].get_obj(), ""));

	return GltfParser::convert(lRoot);
};







BufferView::BufferView(json_spirit::mObject& iBufferView, std::istream* iBuffer)
: mByteOffset(0)
, mBuffer(*iBuffer)
{
	if (iBufferView.find("byteOffset") != iBufferView.end())
	{
		mByteOffset = iBufferView["byteOffset"].get_int();
	}
}

uint8_t* BufferView::read(json_spirit::mObject& iAccessor)
{
	int lByteOffset = 0;
	if (iAccessor.find("byteOffset") != iAccessor.end())
	{
		lByteOffset = iAccessor["byteOffset"].get_int();
	}

	mBuffer.seekg(mByteOffset+lByteOffset, std::ios_base::beg);

	uint32_t lCount = iAccessor["count"].get_int();
	lCount *= sElements[iAccessor["type"].get_str()];
	lCount *= sSize[iAccessor["componentType"].get_int()];
 
	uint8_t* lBuffer = new uint8_t[lCount];
	mBuffer.read(reinterpret_cast<char*>(lBuffer), lCount);

	return lBuffer;
}

std::map<std::string, int> BufferView::sElements = {
								{ "SCALAR", 1 },
								{ "VEC2", 2 },
								{ "VEC3", 3 },
								{ "VEC4", 4 },
								{ "MAT2", 4 },
								{ "MAT3", 9 },
								{ "MAT4", 16 },
};


std::map<int, int> BufferView::sSize = {

								{ BufferView::BYTE, 1 },
								{ BufferView::UNSIGNED_BYTE, 1 },
								{ BufferView::SHORT, 2 },
								{ BufferView::UNSIGNED_SHORT, 2 },
								{ BufferView::UNSIGNED_INT, 4 },
								{ BufferView::FLOAT, 4 }
};


