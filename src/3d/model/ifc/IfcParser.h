#pragma once

#define GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>

#include "../node.h"
#include "../parser.h"
#include "../mesh.h"
#include "../line.h"
#include "../face.h"
#include "../extrusion.h"

#include "Ifc4.h"
#include "IfcBindings.h"
#include "IfcLoader.h"

class IfcParser : public Parser
{
	public:

		IfcParser();

		void processIfcWorkplan(IfcWorkPlan& iWorkplan, json_spirit::mObject& iJson);

		Model* processIfcProject(IfcProject& iProject);
	
	protected:

		std::map<uint32_t, glm::dmat3> mFrames;
		IfcGeometricRepresentationContext* mContext;

		void parseIfcWorkSchedule(IfcWorkSchedule& iSchedule, json_spirit::mObject& iJson);
		void parseIfcWorkCalendar(IfcWorkCalendar& iSchedule, json_spirit::mObject& iJson);
		void parseIfcTask(IfcTask& iTask, json_spirit::mObject& iJson);
		void parseIfcPerson(IfcPerson& iPerson, json_spirit::mObject& iJson);
		void parseIfcWorkTime(IfcWorkTime& iSchedule, json_spirit::mObject& iJson);
		void parseIfcRelSequence(IfcRelSequence& iTask, json_spirit::mObject& iJson);
		void parseIfcTaskTime(IfcTaskTime& iTask, json_spirit::mObject& iJson);
	//	void parseIfcGeometricRepresentationSubContext(IfcGeometricRepresentationSubContext& iItem, std::string iIndent);

		// units
		static double sPrefix[IfcSIPrefix::IfcSIPrefix_ATTO+1];
		double mAngleUnit; // 1 === radians
		double mLengthUnit;// 1 === meter
		double mTimehUnit;// 1 === seconds
		void parseIfcUnits(IfcUnitAssignment& iItem);

		// geometries
		void convertIfcOpenShell(IfcOpenShell& iItem, Mesh& iMesh, std::string iIndent);	
		void convertIfcClosedShell(IfcClosedShell& iItem, Mesh& iMesh, std::string iIndent);	
		void convertIfcFace(IfcFace& iItem, Mesh& iMesh, std::string iIndent, uint8_t iSide = Mesh::FRONT);	

		Node* convertIfcShapeModel(IfcShapeModel& iRepresentation, std::string iIndent);

			void convertIfcGeometricRepresentationItem(IfcGeometricCurveSet& iItem, Line& iLine, std::string iIndent);
			void convertIfcGeometricRepresentationItem(IfcTriangulatedFaceSet& iItem, Mesh& iMesh, std::string iIndent);
			void convertIfcGeometricRepresentationItem(IfcPolygonalFaceSet& iItem, Mesh& iMesh, std::string iIndent);
			void convertIfcGeometricRepresentationItem(IfcBooleanResult& iItem, Mesh& iMesh, std::string iIndent);
			void convertIfcGeometricRepresentationItem(IfcCsgPrimitive3D& iItem, Mesh& iMesh, std::string iIndent);
			void convertIfcGeometricRepresentationItem(IfcSolidModel& iItem, Mesh& iMesh, std::string iIndent);
				void convertIfcSweptAreaSolid(IfcExtrudedAreaSolid& iItem, Mesh& iMesh, std::string iIndent);
				void convertIfcSweptAreaSolid(IfcRevolvedAreaSolid& iItem, Mesh& iMesh, std::string iIndent);
				void convertIfcSweptAreaSolid(IfcSurfaceCurveSweptAreaSolid& iItem, Mesh& iMesh, std::string iIndent);
				void convertIfcSweptAreaSolid(IfcFixedReferenceSweptAreaSolid& iItem, Mesh& iMesh, std::string iIndent);

				Profile* convertIfcProfileDef(IfcProfileDef& iItem, std::string iIndent);
					Profile* convertIfcParameterizedProfileDef(IfcRectangleProfileDef& iItem);
						Profile* convertIfcParameterizedProfileDef(IfcRectangleHollowProfileDef& iItem);
						Profile* convertIfcParameterizedProfileDef(IfcRoundedRectangleProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcCircleProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcEllipseProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcCircleHollowProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcIShapeProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcTrapeziumProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcAsymmetricIShapeProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcTShapeProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcLShapeProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcCShapeProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcZShapeProfileDef& iItem);
					Profile* convertIfcParameterizedProfileDef(IfcUShapeProfileDef& iItem);
					Profile* convertIfcCenterLineProfileDef(IfcCenterLineProfileDef& iItem);	

			static const uint8_t TRIM_NONE;
			static const uint8_t TRIM_PARAM;
			static const uint8_t TRIM_POINT;
			static const uint8_t TRIM_BOTH;
			typedef std::tuple<double, glm::dvec3, uint8_t> Trim;  // = Trim(0,glm::dvec2(0),TRIM_NONE)

			void convertIfcGeometricRepresentationItem(IfcCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0 = Trim(0,glm::dvec3(0),TRIM_NONE), Trim iTrim1 = Trim(0,glm::dvec3(0),TRIM_NONE));
				void convertIfcCurve(IfcBoundedCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0 = Trim(0,glm::dvec3(0),TRIM_NONE), Trim iTrim1 = Trim(0,glm::dvec3(0),TRIM_NONE));
					void convertIfcBoundedCurve(IfcPolyline& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);
					void convertIfcBoundedCurve(IfcIndexedPolyCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);
					void convertIfcBoundedCurve(IfcBSplineCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);
					void convertIfcBoundedCurve(IfcCompositeCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);
					void convertIfcBoundedCurve(IfcTrimmedCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);

				void convertIfcCurve(IfcConic& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);
					void convertIfcConic(IfcEllipse& iItem, std::vector<glm::dvec3>& iBounds, double iA0, double iA1);
					void convertIfcConic(IfcCircle& iItem, std::vector<glm::dvec3>& iBounds, double iA0, double iA1);
				void convertIfcCurve(IfcLine& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);
				void convertIfcCurve(IfcOffsetCurve2D& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);
				void convertIfcCurve(IfcOffsetCurve3D& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);
				void convertIfcCurve(IfcPcurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1);

		std::map<uint32_t, Node*> mIfcShapeModel;

		std::vector<Node*> mStack;


		// scene
		Node* mRoot;
		void parseIfcSpatialElement(IfcSpatialElement& iItem, std::string iIndent);
		void parseIfcProduct(IfcElement& iItem, std::string iIndent);
		Node* parseIfcProductRepresentation(IfcProductRepresentation& iItem, std::string iIndent);

		void convertIfcObjectPlacement(IfcObjectPlacement& iPlacement, glm::dmat4& iMatrix);
		void convertIfcLoop(IfcLoop& iLoop, std::vector<glm::dvec3>& iPoints, std::string iIndent);
		glm::dmat4& convertIfcCartesianTransformationOperator3D(IfcCartesianTransformationOperator3D& iPlacement);
		glm::dmat4& convertIfcAxis2Placement3D(IfcAxis2Placement3D& iPlacement);
		glm::dmat3& convertIfcCartesianTransformationOperator2D(IfcCartesianTransformationOperator2D& iPlacement);
		glm::dmat3& convertIfcAxis2Placement2D(IfcAxis2Placement2D& iPlacement);

		std::vector<Node*> getOpenings(IfcElement& iElement, std::string iIndent);


		// materials
		bool parseIfcSurfaceStyle(IfcSurfaceStyle& iItem, std::string iIndent);
		json_spirit::mObject parseIfcColourRgb(IfcColourRgb& iColor);
		std::map<uint32_t, uint32_t> mMaterialMap;

		std::map<std::string, uint32_t> mMaterialLib;
		uint32_t fromLibrary(IfcElement& iElement);

		template <class T, class S> T* find(Type::Enum iType, IfcTemplatedEntityList<S>& iList)
		{
			for (typename std::vector<S*>::const_iterator lIter = iList.begin(); lIter != iList.end();  lIter++)
			{
				if ((*lIter)->is(iType))
				{
					return (T*)(*lIter);
				}
			}
			return 0;
		}

		template <class T> T* find(Type::Enum iType, IfcEntityList& iList)
		{
			for (typename std::vector<IfcBaseClass*>::const_iterator lIter = iList.begin(); lIter != iList.end();  lIter++)
			{
				if ((*lIter)->is(iType))
				{
					return (T*)(*lIter);
				}
			}
			return 0;
		}

		template <class T> T* first(IfcTemplatedEntityList<T>& iList)
		{
			if (iList.begin() != iList.end())
			{
				return *iList.begin();
			}
			return 0;
		}

		double mPrecision;

		inline double round(double iValue)
		{
			return iValue;
			//return std::floor(iValue / mPrecision +0.5) * mPrecision;
		}

		inline void addVec3(std::vector<glm::dvec3>& iBounds, std::vector<double>& iCoords)
		{
			glm::dvec3 lPoint(0,0,0);
			for (size_t i=0; i<iCoords.size(); i++)
			{
				lPoint[i] = round(iCoords[i]);
			}
			iBounds.push_back(lPoint);
		};

		inline glm::dvec3 toVec3(std::vector<double> iCoords)
		{
			glm::dvec3 lPoint(0,0,0);
			for (size_t i=0; i<iCoords.size(); i++)
			{
				lPoint[i] = round(iCoords[i]);
			}
			return lPoint;
		};

		inline void toVec2(std::vector<glm::dvec3>& iList3d, std::vector<glm::dvec2>& iList2d)
		{
			iList2d.reserve(iList2d.size() + iList3d.size());
			for (std::vector<glm::dvec3>::iterator lIter = iList3d.begin(); lIter != iList3d.end(); lIter++)
			{
				iList2d.push_back(*lIter);
			}
		};

		void fillet(std::vector<glm::dvec2>& iList, uint8_t iQuad, double iX, double iY, double iR);

		inline json_spirit::mArray toJson(std::vector<int>& iVector)
		{
			json_spirit::mArray lJson;
			for (std::vector<int>::iterator lIter = iVector.begin(); lIter != iVector.end(); lIter++)
			{
				lJson.push_back(*lIter);
			}
			return lJson;
		}
};