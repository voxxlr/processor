#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include "json_spirit/json_spirit.h"
#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue>
#include <strstream>

#define _USE_MATH_DEFINES
#include <math.h>

#define GLM_FORCE_CTOR_INIT
#include <glm/glm.hpp>

#include "node.h"
#include "parser.h"


Node::Node(int iId, uint8_t iType)
: mMaterial(-1)
, mGeometry(0)
, mRefCount(0)
, mType(iType)
, mLogicalParent(-1)
, mMatrix(1.0)
{
	mInfo["id"] = iId;
};


Node::Node(Node& iNode)
: mMaterial(iNode.mMaterial)
, mGeometry(iNode.mGeometry)
, mRefCount(0)
, mType(iNode.mType)
, mLogicalParent(-1)
, mMatrix(1.0)
{
	mInfo = iNode.mInfo;

	for (std::vector<Node*>::iterator lIter = iNode.mChildren.begin(); lIter != iNode.mChildren.end(); lIter++)
	{
		addNode(new Node(*(*lIter)));
	}

	if (mGeometry)
	{
		iNode.mGeometry->incRef();
	}
}


int32_t Node::sId = 1;

glm::dmat4 Node::I(1.0);

Node::~Node()
{
	for (std::vector<Node*>::iterator lIter = mChildren.begin(); lIter != mChildren.end(); lIter++)
	{
		delete (*lIter);
	}

	mChildren.clear();
}

Node* Node::getChild(std::string iName)
{
	for (std::vector<Node*>::iterator lIter = mChildren.begin(); lIter != mChildren.end(); lIter++)
	{
		if ((*lIter)->mInfo["name"] == iName)
		{
			return *lIter;
		}
	}
	Node* lNode = new Node(sId, true);
	lNode->mInfo["name"] = iName;
	lNode->mInfo["id"] = "g" + std::to_string(sId++);
	mChildren.push_back(lNode);
	return lNode;
};


void Node::accumulate(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix)
{
	if (mGeometry)
	{
		mGeometry->accumulate(iMesh, iPrecision, iMatrix);
	}

	for (std::vector<Node*>::iterator lIter = mChildren.begin(); lIter != mChildren.end(); lIter++)
	{
		glm::dmat4 lMatrix = iMatrix*mMatrix; 

		(*lIter)->accumulate(iMesh, iPrecision, lMatrix);
	}
};

void Node::subtract(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix)
{
	if (mGeometry)
	{
		mGeometry->subtract(iMesh, iPrecision, iMatrix);
	}

	for (std::vector<Node*>::iterator lIter = mChildren.begin(); lIter != mChildren.end(); lIter++)
	{
		glm::dmat4 lMatrix = iMatrix*mMatrix; 

		(*lIter)->subtract(iMesh, iPrecision, lMatrix);
	}
};

Node* Node::addNode(Node* iNode)
{
	mChildren.push_back(iNode);
	iNode->mRefCount++;
	return iNode;
}



class Comparator0
{
	public:
			
		bool operator() (Node* a, Node*  b)
		{ 
			if (a->mMaterial != b->mMaterial)
			{
				return a->mMaterial < b->mMaterial;
			}
			else
			{
				return a->mGeometry < b->mGeometry;
			}
		}
};



void Node::finalize(Parser& iParser, InternalNode& iNode)
{
	std::vector<Node*>::iterator lIter = mChildren.begin();
	while (lIter != mChildren.end())
	{
		if ((*lIter)->is(Node::PHYSICAL))
		{
			InternalNode* lInternal = new InternalNode((*lIter)->mInfo);
			(*lIter)->finalize(iParser, *lInternal);
			iNode.addInternal(lInternal);
			lIter = mChildren.erase(lIter);
		}
		else
		{
			lIter++;
		}
	}
	
	resolveReferences();

	glm::dmat4 lMatrix(1.0);
	std::vector<Node*> lChildren;
	resolveHierarchy(iNode, lMatrix, -1, lChildren);
	mChildren = lChildren;

	// Pass 1 - Check if there are unassigned materials
	for (std::vector<Node*>::iterator lIter = mChildren.begin(); lIter != mChildren.end(); lIter++)
  	{  
		if ((*lIter)->mMaterial == -1)
		{ 
			(*lIter)->mMaterial = iParser.mMaterials.size()-1;
		}
	}

	// sort by material
	Comparator0 lComparator;
	std::sort (mChildren.begin(), mChildren.end(), lComparator);     

	// group by material
	lIter = mChildren.begin();
	while (lIter != mChildren.end())
  	{  
 		Node* lNode = (*lIter);

		MaterialNode* lMaterial = new MaterialNode(lNode->mMaterial);

		std::tuple<int, int> lRecord;

		InstancedNode* lInstance = 0;
		if (lNode->mMatrix != Node::I)
		{
			// group same material
			lInstance = new InstancedNode(lNode->mGeometry);
			lRecord = lInstance->addInstance(lNode->mMatrix);

			lMaterial->addInstance(lInstance);
		}
		else
		{
			lRecord = lMaterial->addGeometry(lNode->mGeometry);
		}
		
		// link to logical node
		if (lNode->mLogicalParent != -1)
		{
			iNode.addGeometryRecord(lNode->mLogicalParent, lRecord);
		}

		// group same instance
		lIter++;
		while (lIter != mChildren.end())
		{
			lNode = (*lIter);
			if (lNode->mMaterial == lMaterial->mMaterial)
			{
				if (lNode->mMatrix != Node::I)
				{
					if (!lInstance)
					{
						lInstance = new InstancedNode(lNode->mGeometry);
						lRecord = lInstance->addInstance(lNode->mMatrix);
						lMaterial->addInstance(lInstance);
					}
					else if (lNode->mGeometry == lInstance->mGeometry)
					{
						lRecord = lInstance->addInstance(lNode->mMatrix);
					}
					else
					{
						lInstance = new InstancedNode(lNode->mGeometry);
						lRecord = lInstance->addInstance(lNode->mMatrix);
						lMaterial->addInstance(lInstance);
					}
				}
				else
				{
					lRecord = lMaterial->addGeometry(lNode->mGeometry);
				}

				// link to logical node
				if (lNode->mLogicalParent != -1)
				{
					iNode.addGeometryRecord(lNode->mLogicalParent, lRecord);
				}

				lIter++;
			}
			else
			{
				break;
			}
		}

		iNode.addMaterial(lMaterial);
	}
}

void Node::resolveReferences()
{
	for (std::vector<Node*>::iterator lIter = mChildren.begin(); lIter != mChildren.end(); lIter++)
	{
		Node* lChild = (*lIter);
		lChild->resolveReferences();
		if (lChild->mRefCount > 1)
		{
			lChild->mRefCount--;
			lChild = new Node(*lChild);
			lChild->mRefCount++;
			(*lIter) = lChild;
		}
	}
}

void Node::resolveHierarchy(InternalNode& iParent, glm::dmat4& iMatrix, int32_t iMaterial, std::vector<Node*>& iList)
{
	int32_t lMaterial = iMaterial;
	if (mMaterial != -1)
	{
		lMaterial = mMaterial;
	}

	// remove degenerates
	if (mGeometry)
	{
		if (mGeometry->primitiveCount() == 0)
		{
			mGeometry = 0;
		}
	}

	glm::dmat4 lMatrix = iMatrix*mMatrix;
	if (mGeometry)
	{
		// resolve single references
		if (!mGeometry->refCount())
		{
			mGeometry->transform(lMatrix);
			mMatrix = Node::I;
		}
		else
		{
			mMatrix = lMatrix;
		}

		// remember logical parent index
		mLogicalParent = iParent.currentLogcialNode();
		mMaterial = lMaterial;
		iList.push_back(this);
	}
	else 	
	{
		mMatrix = Node::I;
	}

	if (is(Node::LOGICAL))
	{
		iParent.addLogcialNode(mInfo);
	}

	for (std::vector<Node*>::iterator lIter = mChildren.begin(); lIter != mChildren.end(); lIter++)
	{
		(*lIter)->resolveHierarchy(iParent, lMatrix, lMaterial, iList);
	}
}



void Node::print(std::string iIndent)
{
	BOOST_LOG_TRIVIAL(info) << iIndent << "(" << (intptr_t)this << ")," << mMaterial << ((mMatrix != Node::I) ? ",(M)" : "");
	for (std::vector<Node*>::iterator lIter = mChildren.begin(); lIter != mChildren.end(); lIter++)
	{
		(*lIter)->print(iIndent + "  ");
	}
}



Parser::Parser()
{
};


#define DEBUG 1

Model* Parser::convert(Node* iRoot)
{
	mMaterials.push_back(mDefaultMaterial);

	// Pass 2 - Create scene instances 
	json_spirit::mObject lInfo;
	lInfo["name"] = "root";
	lInfo["id"] = iRoot->mInfo["id"];
	InternalNode* lInternal = new InternalNode(lInfo);
	iRoot->finalize(*this, *lInternal);
	lInternal->print("  ");

	Model* lModel = new Model(lInternal);

	lModel->mImages = mImages;
	lModel->mTextures = mTextures;
	lModel->mSamplers = mSamplers;
	lModel->mMaterials = mMaterials;

	//BOOST_LOG_TRIVIAL(info) << "....XXXXXXXXXXXXX ...";
	//std::stringstream lStream;
	//json_spirit::write_stream(json_spirit::mValue(mMaterials), lStream, json_spirit::pretty_print);
	//std::cout << lStream.str() << "\n";
	//BOOST_LOG_TRIVIAL(info) << "....YYYYYYYYYY ...";

	BOOST_LOG_TRIVIAL(info) << "....done converting ...";
	BOOST_LOG_TRIVIAL(info) << "**********************************";
	return lModel;
};



