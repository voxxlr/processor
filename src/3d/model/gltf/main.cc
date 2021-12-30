#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>

using boost::asio::ip::tcp;

#include "task.h"

#include "gltfParser.h"


bool processFile(json_spirit::mObject& iObject)
{
	GltfParser lParser;
	Model* lModel = lParser.process(iObject["file"].get_str(), iObject["scalar"].get_real());

	boost::filesystem::path lDirectory = boost::filesystem::path(iObject["file"].get_str()).parent_path();
	json_spirit::mObject lRoot = lModel->write(lDirectory);

	// write root
	std::ofstream lOstream("root.json");
	lRoot["type"] = "model";
	json_spirit::write_stream(json_spirit::mValue(lRoot), lOstream);
	lOstream.close();

	// return json object
	std::ofstream lProcess("process.json");
	json_spirit::mObject lResult;
	json_spirit::write_stream(json_spirit::mValue(lResult), lProcess);

	return true;
};

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
/*
int max(int a, int b)
{
	return a>b ? a: b;
}

int sumPath(int** Board, int N, int M, int iCurrentRow, int iCurrentCol, int iDepth, int iSourceRow, int iSourceCol) 
{
	static int dc [4] = { -1, 1, 0, 0 };
	static int dr [4] = { 0, 0, -1, 1 };

	if (iCurrentRow < 0 || iCurrentRow >= N || (iCurrentCol < 0 || iCurrentCol >= M))
	{
		return 0;
	}

	int lMax = 0;

	if (iDepth > 0)
	{
		for (int i=0; i<4; i++)
		{
			int lNextRow = iCurrentRow + dr[i];
			int lNextCol = iCurrentCol + dc[i];

			if (lNextRow != iSourceRow || lNextCol != iSourceCol)
			{
				lMax = max(sumPath(Board,N,M,lNextCol,lNextRow,iDepth-1,iCurrentRow,iCurrentCol), lMax);
			}
		}
	}

	return Board[iCurrentRow][iCurrentCol]*pow(10, iDepth)+lMax;
}

int solution(int **Board, int N, int M) 
{
	int lMax = 0;
	for (int r=0; r<N; r++)
	{
		for (int c=0; c<M; c++)
		{
			lMax = max(sumPath(Board,N,M,r,c,3,-1,-1), lMax);
		}
	}
	return lMax;
}
  
*/ 

    
#ifdef _WIN32

#define TEST 1   

#endif

int main(int argc, char *argv[])
{
	/*
	int** Board = new int* [3];

	Board[0] = new int [5] {9, 1, 1, 0, 7};

	
	std::cout << solution(&Board[0], 3, 5) << "\n";;
	*/




#if defined TEST

	json_spirit::mObject lObject;
		
	//glm::mat4 ll = glm::translate(glm::mat4(1.0), glm::vec3(1 , 2, 3));
	//std::cout << ll;
	boost::filesystem::create_directories(boost::filesystem::path("D:/home/voxxlr/data/gltf/Box"));
	chdir("D:/home/voxxlr/data/gltf/Box");
	GltfParser lParser;
	Model* lModel = lParser.process("Box.gltf", 1);
	lModel->write(boost::filesystem::path("."));
	/*
	boost::filesystem::create_directories(boost::filesystem::path("D:/home/voxxlr/data/gltf/ac"));
	chdir("D:/home/voxxlr/data/gltf/ac");
	GltfParser lParser;
	Model* lModel = lParser.process("scene.gltf", lObject);
	lModel->write(boost::filesystem::path("."));
	*/
	/*
	
	boost::filesystem::create_directories(boost::filesystem::path("D:/home/voxxlr/data/gltf/OrientationTest"));
	chdir("D:/home/voxxlr/data/gltf/OrientationTest");
	GltfParser lParser;
	Model* lModel = lParser.process("OrientationTest.gltf", lObject);
	lModel->write();
	
			
	boost::filesystem::create_directories(boost::filesystem::path("D:/home/voxxlr/data/gltf/Alien"));
	chdir("D:/home/voxxlr/data/gltf/Alien");
	GltfParser lParser;
	Model* lModel = lParser.process("Alien.gltf", lObject);
	lModel->write();
	*/	
	/*
	boost::filesystem::create_directories(boost::filesystem::path("D:/home/voxxlr/data/gltf/gears"));
	chdir("D:/home/voxxlr/data/gltf/gears");
	GltfParser lParser;
	Model* lModel = lParser.process("GearboxAssy.gltf", lObject);
	lModel->write(boost::filesystem::path("."));
	
	boost::filesystem::create_directories(boost::filesystem::path("D:/home/voxxlr/data/gltf/leo/northbldg"));
	chdir("D:/home/voxxlr/data/gltf/leo/northbldg");
	GltfParser lParser;
	Model* lModel = lParser.process("scene.gltf", lObject);
	lModel->write();
		*/	

#else

	task::initialize("gltf", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));

#endif

    return EXIT_SUCCESS;
}
