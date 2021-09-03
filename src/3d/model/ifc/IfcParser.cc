#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif   

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue>

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "IfcLoader.h"
#include "IfcParser.h"

#include "../node.h"
#include "../face.h"
#include "../bsp.h"


#define ERROR(item,comment) BOOST_LOG_TRIVIAL(error) << iIndent << "(#" << item->id() << "," <<  Type::ToString(item->type()) << ") " << comment;
#define IMPLEMENT(item) BOOST_LOG_TRIVIAL(error) << "(#" << item->id() << "," <<  Type::ToString(item->type()) << ") - IMPLEMENT ME";

#define PRCOESSING(item,comment) BOOST_LOG_TRIVIAL(info) << iIndent << "(#" << item.id() << "," <<  Type::ToString(item.type()) << ") " << comment;

double IfcParser::sPrefix[] = { 1E18, 1E15, 1E12, 1E9, 1E6, 1E3, 1E2, 1E1, 1E-1, 1E-2, 1E-3, 1E-6, 1E-9, 1E-12, 1E-15, 1E-18 };

IfcParser::IfcParser()
: Parser()
, mAngleUnit(1)
, mLengthUnit(1)
, mTimehUnit(1)
, mPrecision(0.0000001)
, mContext(0)
{
	mDefaultMaterial["type"] = "IfcSurfaceStyle";
	mDefaultMaterial["name"] = "undefined";
	json_spirit::mObject lColor;
	lColor["r"] = 0.5;
	lColor["g"] = 0.5;
	lColor["b"] = 0.5;
	mDefaultMaterial["surfaceColor"] = lColor;
};



void IfcParser::parseIfcPerson(IfcPerson& iPerson, json_spirit::mObject& iJson)
{
	json_spirit::mObject lJson;
	lJson["IfcPerson"] = iPerson.toJson();
	iJson[std::to_string(iPerson.id())] = lJson;
}


void IfcParser::parseIfcTaskTime(IfcTaskTime& iTask, json_spirit::mObject& iJson)
{
	if (iTask.hasDurationType())
	{
		iJson["durationType"] = IfcTaskDurationEnum::ToString(iTask.DurationType());
	}

	#define ADD(Field, name) \
		if (iTask.has##Field())\
		{\
			iJson[name] = iTask.Field();\
		}\

	ADD(Name, "name")
	ADD(DataOrigin, "dataOrigin")
	ADD(ScheduleDuration, "scheduleDuration")
	ADD(ScheduleStart, "scheduleStart")
	ADD(ScheduleFinish, "scheduleFinish")
	ADD(EarlyStart, "earlyStart")
	ADD(EarlyFinish, "earlyFinish")
	ADD(LateStart, "lateStart")
	ADD(LateFinish, "lateFinish")

	ADD(FreeFloat, "freeFloat")
	ADD(TotalFloat, "totalFloat")
	ADD(IsCritical, "isCritical")
	ADD(StatusTime, "statusTime")

	ADD(ActualDuration, "actualDuration")
	ADD(ActualStart, "actualStart")
	ADD(ActualFinish, "actualFinish")
	ADD(RemainingTime, "remainingTime")
	ADD(Completion, "completion")

	#undef ADD

}

void IfcParser::parseIfcRelSequence(IfcRelSequence& iSequence, json_spirit::mObject& iJson)
{
	iJson[std::to_string(iSequence.id())] = iSequence.toJson();
}

void IfcParser::parseIfcTask(IfcTask& iTask, json_spirit::mObject& iJson)
{
	json_spirit::mObject lJson;
	lJson["IfcTask"] = iTask.toJson();

	json_spirit::mObject lSequence;

	IfcRelSequence::list::ptr lSuccessorFrom = iTask.IsSuccessorFrom();
	for (std::vector<IfcRelSequence*>::const_iterator lIter = lSuccessorFrom->begin(); lIter != lSuccessorFrom->end(); lIter++)
	{
		parseIfcRelSequence(*(*lIter), lSequence);
	};

	IfcRelSequence::list::ptr lPredecessorTo = iTask.IsPredecessorTo();
	for (std::vector<IfcRelSequence*>::const_iterator lIter = lPredecessorTo->begin(); lIter != lPredecessorTo->end(); lIter++)
	{
		parseIfcRelSequence(*(*lIter), lSequence);
	};

	if (lSequence.size())
	{
		lJson["IfcRelSequence"] = lSequence;
	}


	if (iTask.hasTaskTime())
	{
		lJson["IfcTaskTime"] = iTask.TaskTime()->toJson();
	}

	json_spirit::mArray lProducts;

	IfcRelAssignsToProcess::list::ptr lOperates = iTask.OperatesOn();
	for (std::vector<IfcRelAssignsToProcess*>::const_iterator lIter = lOperates->begin() ; lIter != lOperates->end(); lIter++)
	{
		if ((*lIter)->is(Type::IfcRelAssignsToProcess))
		{
			IfcRelAssignsToProcess* lAssign = (*lIter)->as<IfcRelAssignsToProcess>();
			IfcTemplatedEntityList<IfcObjectDefinition>::ptr lObject = lAssign->RelatedObjects();
			for (std::vector<IfcObjectDefinition*>::const_iterator lIter = lObject->begin() ; lIter != lObject->end(); lIter++)
			{
				if ((*lIter)->is(Type::IfcProduct))
				{
					IfcProduct* lProduct = (*lIter)->as<IfcProduct>();
					lProducts.push_back((int)lProduct->id());
				}
			}
		}
	}


	IfcRelAssigns::list::ptr lAssignments = iTask.HasAssignments();
	for (std::vector<IfcRelAssigns*>::const_iterator lIter = lAssignments->begin() ; lIter != lAssignments->end(); lIter++)
	{
		if ((*lIter)->is(Type::IfcRelAssignsToProduct))
		{
			IfcRelAssignsToProduct* lAssign = (*lIter)->as<IfcRelAssignsToProduct>();
			IfcProductSelect* lSelect = lAssign->RelatingProduct();
			if (lSelect->is(Type::IfcProduct))
			{
				IfcProduct* lProduct = lSelect->as<IfcProduct>();
				lProducts.push_back((int)lProduct->id());
			}
		}
		else if ((*lIter)->is(Type::IfcRelAssignsToProcess))
		{
			BOOST_LOG_TRIVIAL(error) << "(#" << (*lIter)->id() << "," <<  Type::ToString((*lIter)->type()) << ") Unknown IfcObjectDefinition ";
		}
	}

	if (lProducts.size())
	{
		lJson["IfcRelAssignsToProduct"] = lProducts;
	}


	json_spirit::mObject lTasks;

	IfcRelNests::list::ptr lNest = iTask.IsNestedBy();
	for (std::vector<IfcRelNests*>::const_iterator lIter = lNest->begin() ; lIter != lNest->end(); lIter++)
	{
		IfcTemplatedEntityList<IfcObjectDefinition>::ptr lObjects = (*lIter)->RelatedObjects();
		for (std::vector<IfcObjectDefinition*>::const_iterator lIter1 = lObjects->begin(); lIter1 != lObjects->end(); lIter1++)
		{
			if ((*lIter1)->is(Type::IfcTask))
			{
				parseIfcTask(*(*lIter1)->as<IfcTask>(), lTasks);
			}
		}
	}

	if (lTasks.size())
	{
		lJson["IfcRelNests"] = lTasks;
	}

	iJson[std::to_string(iTask.id())] = lJson;
}


void IfcParser::parseIfcWorkSchedule(IfcWorkSchedule& iSchedule, json_spirit::mObject& iJson)
{
	json_spirit::mObject lJson;
	lJson["IfcWorkSchedule"] = iSchedule.toJson();

	json_spirit::mObject lTasks;

	IfcTemplatedEntityList<IfcRelAssignsToControl>::ptr lAssignments = iSchedule.Controls(); 
	for (std::vector<IfcRelAssignsToControl*>::const_iterator lIter = lAssignments->begin() ; lIter != lAssignments->end(); lIter++)
	{
		IfcTemplatedEntityList<IfcObjectDefinition>::ptr lObjects = (*lIter)->RelatedObjects();
		for (std::vector<IfcObjectDefinition*>::const_iterator lIter = lObjects->begin() ; lIter != lObjects->end(); lIter++)
		{
			if ((*lIter)->is(Type::IfcTask))
			{
				parseIfcTask(*(*lIter)->as<IfcTask>(), lTasks);
			}
			else
			{
				BOOST_LOG_TRIVIAL(error) << "(#" << (*lIter)->id() << "," <<  Type::ToString((*lIter)->type()) << ") Unknown IfcObjectDefinition ";
			}
		}
	}

	lJson["IfcRelAssignsToControl"] = lTasks;

	iJson[std::to_string(iSchedule.id())] = lJson;
}

void IfcParser::parseIfcWorkTime(IfcWorkTime& iWorkTime, json_spirit::mObject& iJson)
{
	json_spirit::mObject lJson;
	BOOST_LOG_TRIVIAL(error) << "parseIfcWorkTime";
	lJson["IfcWorkTime"] = iWorkTime.toJson();

	iJson[std::to_string(iWorkTime.id())] = lJson;

}

void IfcParser::parseIfcWorkCalendar(IfcWorkCalendar& iSchedule, json_spirit::mObject& iJson)
{
	json_spirit::mObject lJson;
	lJson["IfcWorkCalendar"] = iSchedule.toJson();

	if (iSchedule.hasWorkingTimes())
	{
		IfcTemplatedEntityList<IfcWorkTime>::ptr lTimes = iSchedule.WorkingTimes(); 
		for (std::vector<IfcWorkTime*>::const_iterator lIter = lTimes->begin(); lIter != lTimes->end(); lIter++)
		{
			parseIfcWorkTime(*(*lIter), lJson);
		}
	}

	if (iSchedule.hasExceptionTimes())
	{
		IfcTemplatedEntityList<IfcWorkTime>::ptr lTimes = iSchedule.ExceptionTimes(); 
		for (std::vector<IfcWorkTime*>::const_iterator lIter = lTimes->begin(); lIter != lTimes->end(); lIter++)
		{
			parseIfcWorkTime(*(*lIter), lJson);
		}
	}
	
	iJson[std::to_string(iSchedule.id())] = lJson;
}

void IfcParser::processIfcWorkplan(IfcWorkPlan& iPlan, json_spirit::mObject& iJson)
{
	json_spirit::mObject lJson;
	lJson["IfcWorkplan"] = iPlan.toJson();

	json_spirit::mObject lArray;

	// IFC 2x3 ??
	IfcRelAggregates::list::ptr lContained = iPlan.IsDecomposedBy();
	for (std::vector<IfcRelAggregates*>::const_iterator lIter = lContained->begin() ; lIter != lContained->end(); lIter++)
	{
		IfcTemplatedEntityList<IfcObjectDefinition>::ptr lObjects = (*lIter)->RelatedObjects();
		for (std::vector<IfcObjectDefinition*>::const_iterator lIter1 = lObjects->begin(); lIter1 != lObjects->end(); lIter1++)
		{
			if ((*lIter1)->is(Type::IfcWorkSchedule))
			{
				parseIfcWorkSchedule(*(*lIter1)->as<IfcWorkSchedule>(), lArray);
			}
			else if ((*lIter1)->is(Type::IfcWorkCalendar))
			{
				parseIfcWorkCalendar(*(*lIter1)->as<IfcWorkCalendar>(), lArray);
			}
			else
			{
				BOOST_LOG_TRIVIAL(error) << "(#" << (*lIter1)->id() << "," <<  Type::ToString((*lIter1)->type()) << ") Unknown IfcObjectDefinition ";
			}
		}
	};

	// IFC 4
	IfcRelNests::list::ptr lNest = iPlan.IsNestedBy();
	for (std::vector<IfcRelNests*>::const_iterator lIter = lNest->begin() ; lIter != lNest->end(); lIter++)
	{
		IfcTemplatedEntityList<IfcObjectDefinition>::ptr lObjects = (*lIter)->RelatedObjects();
		for (std::vector<IfcObjectDefinition*>::const_iterator lIter1 = lObjects->begin(); lIter1 != lObjects->end(); lIter1++)
		{
			if ((*lIter1)->is(Type::IfcWorkSchedule))
			{
				parseIfcWorkSchedule(*(*lIter1)->as<IfcWorkSchedule>(), lArray);
			}
			else if ((*lIter1)->is(Type::IfcWorkCalendar))
			{
				parseIfcWorkCalendar(*(*lIter1)->as<IfcWorkCalendar>(), lArray);
			}
			else
			{
				BOOST_LOG_TRIVIAL(error) << "(#" << (*lIter1)->id() << "," <<  Type::ToString((*lIter1)->type()) << ") Unknown IfcObjectDefinition ";
			}
		}
	};

	lJson["IfcRelNests"] = lArray;

	if (iPlan.hasCreators())
	{
		json_spirit::mObject lArray;

	    IfcTemplatedEntityList<IfcPerson>::ptr lList = iPlan.Creators();
		for (std::vector<IfcPerson*>::const_iterator lIter = lList->begin() ; lIter != lList->end(); lIter++)
		{
			parseIfcPerson(*(*lIter), lArray);
		}
		lJson["IfcPerson"] = lArray;
	}

	iJson[std::to_string(iPlan.id())] = lJson;

}


//
//
//


Model* IfcParser::processIfcProject(IfcProject& iProject)
{
	std::string lIndent("");

	IfcTemplatedEntityList<IfcRepresentationContext>::ptr lGeometries = iProject.RepresentationContexts();
	for (std::vector<IfcRepresentationContext*>::const_iterator lIter = lGeometries->begin() ; lIter != lGeometries->end(); lIter++)
	{
		if ((*lIter)->is(Type::IfcGeometricRepresentationContext))
		{
			IfcGeometricRepresentationContext* lContext = (*lIter)->as<IfcGeometricRepresentationContext>();
			if (lContext->CoordinateSpaceDimension() == 3)
			{
				if (lContext->hasPrecision())
				{
					mPrecision = lContext->Precision();
				}

				IfcAxis2Placement* lFrame = lContext->WorldCoordinateSystem();

				glm::dmat3 lMatrix(1.0);
				if (lFrame->is(Type::IfcAxis2Placement3D))
				{
					IfcAxis2Placement3D& lPlacement = *lFrame->as<IfcAxis2Placement3D>();

					glm::dvec3 lZ(0.0,0.0,1.0);
					if (lPlacement.hasAxis())
					{
						IfcDirection* lDirection = lPlacement.Axis();
						glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
						lZ = glm::dvec3(lVector[0], lVector[1], lVector[2]);
					}

					glm::dvec3 lRef(1.0,0.0,0.0);
					if (lPlacement.hasRefDirection())
					{
						IfcDirection* lDirection = lPlacement.RefDirection();
						glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
						lRef = glm::dvec3(lVector[0], lVector[1], lVector[2]);
					}

					glm::dvec3 lY = glm::cross(lZ, lRef);
					glm::dvec3 lX = glm::cross(lY, lZ);

					lX = glm::normalize(lX);
					lY = glm::normalize(lY);
					lZ = glm::normalize(lZ);
	
					// do transpose on the rotation for some reason? 
					lMatrix[0][0] = lX.x;
					lMatrix[0][1] = lX.y;
					lMatrix[0][2] = lX.z;

					lMatrix[1][0] = lY.x;
					lMatrix[1][1] = lY.y;
					lMatrix[1][2] = lY.z;

					lMatrix[2][0] = lZ.x;
					lMatrix[2][1] = lZ.y;
					lMatrix[2][2] = lZ.z;
				}
				else if (lFrame->is(Type::IfcAxis2Placement2D))
				{
					IfcAxis2Placement2D& lPlacement = *lFrame->as<IfcAxis2Placement2D>();

					glm::vec2 lX(1.0, 0.0);
					if (lPlacement.hasRefDirection())
					{
						IfcDirection* lDirection = lPlacement.RefDirection();
						glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
						lX.x = lVector[0];
						lX.y = lVector[1];
					}
					else 
					{
						BOOST_LOG_TRIVIAL(error)  << " NO IfcAxis2Placement2D.hasRefDirection()";
					}

					glm::vec2 lY(-lX.y, lX.x);

					// do transpose on the rotation for some reason? 
					lMatrix[0][0] = lX.x;
					lMatrix[0][1] = lX.y;

					lMatrix[1][0] = lY.x;
					lMatrix[1][1] = lY.y;
				}

				mFrames[lContext->id()] = lMatrix;
				mContext = lContext; /// for now
			}
		}
	}

	if (iProject.hasUnitsInContext())
	{
		parseIfcUnits(*iProject.UnitsInContext());
	}

	mRoot = new Node(0, Node::PHYSICAL);
	mRoot->mInfo["name"] = "root";
	mStack.push_back(mRoot);

	BOOST_LOG_TRIVIAL(info) << "************* Scene *************";
	IfcTemplatedEntityList<IfcRelAggregates>::ptr lAggregates = iProject.IsDecomposedBy();
	for (std::vector<IfcRelAggregates*>::const_iterator lIter = lAggregates->begin() ; lIter != lAggregates->end(); lIter++)
	{
		IfcTemplatedEntityList<IfcObjectDefinition>::ptr lObjects = (*lIter)->RelatedObjects();
		for (std::vector<IfcObjectDefinition*>::const_iterator lIter1 = lObjects->begin(); lIter1 != lObjects->end(); lIter1++)
		{
			if ((*lIter1)->is(Type::IfcSpatialElement))
			{
				parseIfcSpatialElement(*(*lIter1)->as<IfcSpatialElement>(), lIndent);
			}
			else
			{
				BOOST_LOG_TRIVIAL(error) << "(#" << (*lIter)->id() << "," <<  Type::ToString((*lIter)->type()) << ") Unknown IfcObjectDefinition ";
			}
		}
	}
	BOOST_LOG_TRIVIAL(info) << "**********************************";
	IfcTemplatedEntityList<IfcRelDeclares>::ptr lDeclares = iProject.Declares();
	for (std::vector<IfcRelDeclares*>::const_iterator lIter = lDeclares->begin() ; lIter != lDeclares->end(); lIter++)
	{
		IfcEntityList::ptr lObjects = (*lIter)->RelatedDefinitions();
		/*
		for (std::vector<IfcObjectDefinition*>::const_iterator lIter1 = lObjects->begin(); lIter1 != lObjects->end(); lIter1++)
		{
			if ((*lIter1)->is(Type::IfcSpatialElement))
			{
				parseIfcSpatialElement(*(*lIter1)->as<IfcSpatialElement>(), lIndent);
			}
			else
			{
				BOOST_LOG_TRIVIAL(error) << "(#" << (*lIter)->id() << "," <<  Type::ToString((*lIter)->type()) << ") Unknown IfcObjectDefinition ";
			}
		}
		*/
	}

	Model* lModel = Parser::convert(mRoot);
	
	// switch to y-up;
	lModel->mTransform[1][1] = 0;
	lModel->mTransform[1][2] =-1;
	lModel->mTransform[2][2] = 0;
	lModel->mTransform[2][1] = 1;
	
	// convert to meters
	if (iProject.hasUnitsInContext())
	{
		lModel->mTransform = glm::scale(lModel->mTransform, glm::dvec3(mLengthUnit));
	}
	
	return lModel;
}

void IfcParser::parseIfcUnits(IfcUnitAssignment& iItem)
{
	IfcEntityList::ptr lList = iItem.Units();

	for(std::vector<IfcBaseClass*>::const_iterator lIter = lList->begin(); lIter != lList->end(); lIter++)
	{
		if ((*lIter)->is(Type::IfcSIUnit))
		{
			IfcSIUnit* lSiUnit = (*lIter)->as<IfcSIUnit>();
			BOOST_LOG_TRIVIAL(info) << "(#" << lSiUnit->id() << "," <<  Type::ToString((*lIter)->type()) << ") ";

			IfcSIUnitName::IfcSIUnitName lName = lSiUnit->Name();

			switch (lSiUnit->UnitType())
			{
				case IfcUnitEnum::IfcUnit_LENGTHUNIT:
				{
					if (lSiUnit->hasPrefix())
					{
						mLengthUnit = IfcParser::sPrefix[lSiUnit->Prefix()];
					}
				}
				case IfcUnitEnum::IfcUnit_AREAUNIT:
				{
				}
				case IfcUnitEnum::IfcUnit_TIMEUNIT:
				{
					if (lSiUnit->hasPrefix())
					{
						mTimehUnit = IfcParser::sPrefix[lSiUnit->Prefix()];
					}
				}
				case IfcUnitEnum::IfcUnit_VOLUMEUNIT:
				{
				}
				case IfcUnitEnum::IfcUnit_PLANEANGLEUNIT:
				{
				}
			}
		}
		else if ((*lIter)->is(Type::IfcConversionBasedUnit))
		{
			IfcConversionBasedUnit* lConversionUnit = (*lIter)->as<IfcConversionBasedUnit>();
			BOOST_LOG_TRIVIAL(info) << "(#" << lConversionUnit->id() << "," <<  Type::ToString((*lIter)->type()) << ") ";

			IfcMeasureWithUnit* lFactor = lConversionUnit->ConversionFactor();
			IfcUnit* lUnit = lFactor->UnitComponent();
			if (lUnit->is(Type::IfcSIUnit))
			{
				IfcSIUnit* lSiUnit = lUnit->as<IfcSIUnit>();
				switch (lSiUnit->UnitType())
				{
					case IfcUnitEnum::IfcUnit_LENGTHUNIT:
					{
						if (lSiUnit->hasPrefix())
						{
							switch (lSiUnit->Prefix())
							{
								case IfcSIPrefix::IfcSIPrefix_MILLI:
									mLengthUnit = 0.001;
								break;
								case IfcSIPrefix::IfcSIPrefix_CENTI:
									mLengthUnit = 0.01;
								break;
								case IfcSIPrefix::IfcSIPrefix_DECI:
									mLengthUnit = 0.1;
								break;
							}
						}

						if (lFactor)
						{
							IfcValue* lValue = lFactor->ValueComponent();
							if (lValue->is(Type::IfcRatioMeasure))
							{
								mLengthUnit = *lValue->as<IfcRatioMeasure>();
							}
							else if (lValue->is(Type::IfcLengthMeasure))
							{
								mLengthUnit = *lValue->as<IfcLengthMeasure>();
							}
						}

						/*
						if (lConversionUnit->Name() == "INCH")
						{
							mLengthUnit = 0.0254;
						}
						else if(lConversionUnit->Name() == "FOOT")
						{
							mLengthUnit = 0.3048;
						}
						else if(lConversionUnit->Name() == "YARD")
						{
							mLengthUnit = 0.9144;
						}
						else if(lConversionUnit->Name() == "MILE")
						{
							mLengthUnit = 1609.34;
						}
						*/
					}
					break;

					case IfcUnitEnum::IfcUnit_AREAUNIT:
					{
					}
					break;

					case IfcUnitEnum::IfcUnit_VOLUMEUNIT:
					{
					}
					break;

					case IfcUnitEnum::IfcUnit_PLANEANGLEUNIT:
					{
						if (lSiUnit->hasPrefix())
						{
							switch (lSiUnit->Prefix())
							{
								//case IfcSIPrefix::IfcSIPrefix_DECI:
								//	mLengthUnit = 0.1;
								//break;
							}
						}

						if (lFactor)
						{
							IfcValue* lValue = lFactor->ValueComponent();
							if (lValue->is(Type::IfcRatioMeasure))
							{
								mAngleUnit = *lValue->as<IfcRatioMeasure>();
							}
							else if (lValue->is(Type::IfcLengthMeasure))
							{
								mAngleUnit = *lValue->as<IfcLengthMeasure>();
							}
							else if (lValue->is(Type::IfcPlaneAngleMeasure))
							{
								mAngleUnit = *lValue->as<IfcPlaneAngleMeasure>();
							}
						}
						else if (lConversionUnit->Name() == "DEGREE")
						{
							mAngleUnit = M_PI/180.0;
						}

						/*
						if (lConversionUnit->Name() == "DEGREE")
						{
							mAngleUnit = M_PI/180.0;
						}
						else
						{
							IfcValue* lValue = lFactor->ValueComponent();
							if (lValue->is(Type::IfcRatioMeasure))
							{
								mAngleUnit = *lValue->as<IfcRatioMeasure>();
							}
							else if (lValue->is(Type::IfcLengthMeasure))
							{
								mAngleUnit = *lValue->as<IfcLengthMeasure>();
							}
							else if (lValue->is(Type::IfcPlaneAngleMeasure))
							{
								mAngleUnit = *lValue->as<IfcPlaneAngleMeasure>();
							}
						}
						*/
					}
					break;
				}
			}
		}
	}
}


static int x=0;


Node* IfcParser::convertIfcShapeModel(IfcShapeModel& iRepresentation, std::string iIndent)
{
	BOOST_LOG_TRIVIAL(info) << iIndent << "(#" << iRepresentation.id() << "," <<  Type::ToString(iRepresentation.type()) << ") " << (iRepresentation.hasRepresentationIdentifier() ? iRepresentation.RepresentationIdentifier() : "") << " - "  << (iRepresentation.hasRepresentationType() ? iRepresentation.RepresentationType() : "");

	if (!mIfcShapeModel[iRepresentation.id()])
	{
		Node* lRoot = new Node(iRepresentation.id(), Node::GEOMETRY);

		mIfcShapeModel[iRepresentation.id()] = lRoot;

		IfcTemplatedEntityList<IfcRepresentationItem>::ptr lItems = iRepresentation.Items();
		for (std::vector<IfcRepresentationItem*>::const_iterator lIter = lItems->begin() ; lIter != lItems->end(); lIter++)
		{
			BOOST_LOG_TRIVIAL(info) << iIndent + " " << "(#" << (*lIter)->id() << "," <<  Type::ToString((*lIter)->type()) << ") ";

			if ((*lIter)->is(Type::IfcGeometricRepresentationItem))
			{
				IfcGeometricRepresentationItem& lItem = *(*lIter)->as<IfcGeometricRepresentationItem> ();

				Mesh* lMesh = 0;
				if (lItem.is(Type::IfcFaceBasedSurfaceModel)) 
				{
					IfcFaceBasedSurfaceModel* lSurface = lItem.as<IfcFaceBasedSurfaceModel>();

					lMesh = new Mesh();
					IfcTemplatedEntityList<IfcConnectedFaceSet>::ptr lItems = lSurface->FbsmFaces();
					for (std::vector<IfcConnectedFaceSet*>::const_iterator lIter = lItems->begin() ; lIter != lItems->end(); lIter++)
					{
						IfcConnectedFaceSet* lGeometry = (*lIter);

						IfcTemplatedEntityList<IfcFace>::ptr lFaces = lGeometry->CfsFaces();
						for (std::vector<IfcFace*>::const_iterator lIter = lFaces->begin() ; lIter != lFaces->end(); lIter++)
						{
							convertIfcFace(*(*lIter), *lMesh, iIndent, Mesh::FRONT | Mesh::BACK);
						}
					}
				}
				else if (lItem.is(Type::IfcShellBasedSurfaceModel)) 
				{
					IfcShellBasedSurfaceModel* lShell = lItem.as<IfcShellBasedSurfaceModel>();
		
					lMesh = new Mesh();
					IfcEntityList::ptr lItems = lShell->SbsmBoundary();
					for (std::vector<IfcBaseClass*>::const_iterator lIter = lItems->begin() ; lIter != lItems->end(); lIter++)
					{
						if ((*lIter)->is(Type::IfcOpenShell))
						{
							convertIfcOpenShell(*(*lIter)->as<IfcOpenShell>(), *lMesh, iIndent);
						}
						else if ((*lIter)->is(Type::IfcClosedShell))
						{
							convertIfcClosedShell(*(*lIter)->as<IfcClosedShell>(), *lMesh, iIndent);
						}
					}
				}
				else if (lItem.is(Type::IfcSolidModel))
				{
					lMesh = new Mesh();
					convertIfcGeometricRepresentationItem(*lItem.as<IfcSolidModel>(), *lMesh, iIndent + "  ");
				}
				else if (lItem.is(Type::IfcTriangulatedFaceSet))
				{
					lMesh = new Mesh();
					convertIfcGeometricRepresentationItem(*lItem.as<IfcTriangulatedFaceSet>(), *lMesh, iIndent + "  ");
				}
				else if (lItem.is(Type::IfcPolygonalFaceSet))
				{
					lMesh = new Mesh();
					convertIfcGeometricRepresentationItem(*lItem.as<IfcPolygonalFaceSet>(), *lMesh, iIndent + "  ");
				}
				else if (lItem.is(Type::IfcBooleanResult))
				{
					lMesh = new Mesh();
					convertIfcGeometricRepresentationItem(*lItem.as<IfcBooleanResult>(), *lMesh, iIndent + "  ");
				}
				else if (lItem.is(Type::IfcGeometricCurveSet))
				{
					//Line* lLine = new Line();
					//convertIfcGeometricCurveSet(*iItem.as<IfcGeometricCurveSet>(), *lLine, iIndent + "  ");
					//lNode = new Geometry(Type::ToString(iItem.type())+":"+std::to_string((int)iItem.id()), lLine);
					BOOST_LOG_TRIVIAL(error) << iIndent << "   Unknown Geometric Representation ";
				}
				else
				{
					BOOST_LOG_TRIVIAL(error) << iIndent << "   Unknown Geometric Representation ";
				}
				
				if (lMesh)
				{
					Node* lNode = new Node(lItem.id(), Node::GEOMETRY);
					lRoot->addNode(lNode);
					lNode->mGeometry = new Geometry(Type::ToString(lItem.type())+":"+std::to_string((int)lItem.id()), lMesh);
				
					IfcStyledItem* lStyledItem = find<IfcStyledItem>(Type::IfcStyledItem, *lItem.StyledByItem());
					if (lStyledItem)
					{
						IfcSurfaceStyle* lSurfaceStyle = find<IfcSurfaceStyle>(Type::IfcSurfaceStyle, *lStyledItem->Styles());
						if (!lSurfaceStyle)
						{
							// deprecated
							IfcPresentationStyleAssignment* lAssignment =  find<IfcPresentationStyleAssignment>(Type::IfcPresentationStyleAssignment, *lStyledItem->Styles());
							if (lAssignment)
							{
								lSurfaceStyle = find<IfcSurfaceStyle>(Type::IfcSurfaceStyle, *lAssignment->Styles());
							}
						}
						
						if (parseIfcSurfaceStyle(*lSurfaceStyle, iIndent + "  "))
						{
							lNode->mMaterial= mMaterialMap[lSurfaceStyle->id()];
						}
					}
				}
			}
			else if ((*lIter)->is(Type::IfcMappedItem))
			{
				IfcMappedItem* lItem = (*lIter)->as<IfcMappedItem> ();
				IfcRepresentationMap* lSource = lItem->MappingSource();
				IfcRepresentation* lRepresentation = lSource->MappedRepresentation();
				uint32_t lId = lRepresentation->id();
				
				if (!mIfcShapeModel[lId])
				{
					if (lRepresentation->is(Type::IfcShapeModel))
					{
						mIfcShapeModel[lId] = convertIfcShapeModel(*lRepresentation->as<IfcShapeModel>(), iIndent);
					}
				}

				Node* lReference = mIfcShapeModel[lId];
				if (lReference)
				{
					// Global Transform
					IfcCartesianTransformationOperator* lTarget = lItem->MappingTarget();
					glm::dmat4 lMat1 = convertIfcCartesianTransformationOperator3D(*lTarget->as<IfcCartesianTransformationOperator3D>());

					// Local Transform
					IfcAxis2Placement* lOrigin = lSource->MappingOrigin();

					glm::dmat4 lMat0;
					if (lOrigin->is(Type::IfcAxis2Placement3D))
					{
						lMat0 = convertIfcAxis2Placement3D(*lOrigin->as<IfcAxis2Placement3D>());
					}
					else if (lOrigin->is(Type::IfcAxis2Placement2D))
					{
						lMat0 = convertIfcAxis2Placement2D(*lOrigin->as<IfcAxis2Placement2D>());
					}

					Node* lNode = new Node(lId, Node::GEOMETRY);

					lNode->addNode(lReference);
					lRoot->addNode(lNode);
					lNode->mMatrix = lMat1*lMat0;

					//lReference->incRef();
				}
				else
				{
					BOOST_LOG_TRIVIAL(error) << iIndent << "(#" << lRepresentation->id() << "," <<  Type::ToString(lRepresentation->type()) << ") Does not exist";
				}
			}
			else
			{
				ERROR((*lIter), "Unhandled Representation ");
			}
		}
	}

	return mIfcShapeModel[iRepresentation.id()];
}

void IfcParser::parseIfcSpatialElement(IfcSpatialElement& iItem, std::string iIndent)
{
	PRCOESSING(iItem, "");
	
	if (iItem.is(Type::IfcSpace))
	{
		return;  // JSTIER deal with this
	}
	
	Node* lNode = new Node(iItem.id(), Node::PHYSICAL);
	lNode->mInfo["type"] = Type::ToString(iItem.type());

	#define ADD(Field, name) \
	if (iItem.has##Field())\
	{\
		lNode->mInfo[name] = iItem.Field();\
	}\
			
	ADD(Name, "name")
	ADD(Description, "description")
	ADD(LongName, "longName")

	#undef ADD
	
	if (iItem.hasObjectPlacement())
	{
		convertIfcObjectPlacement(*iItem.ObjectPlacement(), lNode->mMatrix);
	}

	if (iItem.is(Type::IfcSite))
	{
		IfcSite* lSite = iItem.as<IfcSite>();
	}
	else if (iItem.is(Type::IfcBuilding))
	{
		IfcBuilding* lBuilding = iItem.as<IfcBuilding>();
	}
	else if (iItem.is(Type::IfcBuildingStorey))
	{
		IfcBuildingStorey* lStorey = iItem.as<IfcBuildingStorey>();
		if (lStorey->hasName())
		{
			//if (lStorey->id() != 179)
			{
			//	return;
			}
			if (lStorey->Name() != "Ground Floor")  // JSTIER
			{
			//	return;
			}
		}
	}

	
	mStack.back()->addNode(lNode);
	mStack.push_back(lNode);

	if (iItem.hasRepresentation())
	{
		lNode->addNode(parseIfcProductRepresentation(*iItem.Representation(), iIndent +  "  "));
	}

	static int s=0;
	IfcTemplatedEntityList<IfcRelContainedInSpatialStructure>::ptr lContains = iItem.ContainsElements();
	for (std::vector<IfcRelContainedInSpatialStructure*>::const_iterator lIter = lContains->begin() ; lIter != lContains->end(); lIter++)
	{
		IfcRelContainedInSpatialStructure* lContained = (*lIter);
		static int c=0;
		PRCOESSING((*lContained), (lContained->hasName() ? lContained->Name(): ""));
		IfcTemplatedEntityList<IfcProduct>::ptr lProducts = (*lIter)->RelatedElements();
		for (std::vector<IfcProduct*>::const_iterator lIter1 = lProducts->begin() ; lIter1 != lProducts->end(); lIter1++)
		{
			//BOOST_LOG_TRIVIAL(error) << (*lIter1)->id() << Type::ToString((*lIter1)->type());

			if ((*lIter1)->is(Type::IfcElement))
			{
				IfcElement* lElement = (*lIter)->as<IfcElement>();
				//if ((*lIter1)->is(Type::IfcDoor)) // JSTIER  #727610
				//if ((*lIter1)->is(Type::IfcCurtainWall)) // JSTIER
				//if ((*lIter1)->is(Type::IfcWallStandardCase) && (*lIter1)->Name() == "WALL | Standard(3)" && c++ == 0) // JSTIER
				{
					parseIfcProduct(*(*lIter1)->as<IfcElement>(), iIndent + "   ");
				}
			}
			else if ((*lIter1)->is(Type::IfcSite))
			{
				IfcSite* lElement = (*lIter1)->as<IfcSite>();
				parseIfcSpatialElement(*lElement, iIndent + "  ");
			}
		}
	}

	IfcTemplatedEntityList<IfcRelAggregates>::ptr lAggregates = iItem.IsDecomposedBy();
	for (std::vector<IfcRelAggregates*>::const_iterator lIter = lAggregates->begin() ; lIter != lAggregates->end(); lIter++)
	{
		IfcTemplatedEntityList<IfcObjectDefinition>::ptr lObjects = (*lIter)->RelatedObjects();
		for (std::vector<IfcObjectDefinition*>::const_iterator lIter1 = lObjects->begin(); lIter1 != lObjects->end(); lIter1++)
		{
			if ((*lIter1)->is(Type::IfcSpatialElement))
			{
				parseIfcSpatialElement(*(*lIter1)->as<IfcSpatialElement>(), iIndent + "  ");
			}
			else
			{
				ERROR((*lIter1), "Unknown Object");
			}
		}
	}

	mStack.pop_back();
}

void IfcParser::parseIfcProduct(IfcElement& iItem, std::string iIndent)
{
	PRCOESSING(iItem, (iItem.hasName() ? iItem.Name() : "Product"));

	Node* lNode = new Node(iItem.id(), Node::LOGICAL);

	#define ADD(Field, name) \
	if (iItem.has##Field())\
	{\
		lNode->mInfo[name] = iItem.Field();\
	}\
			
	ADD(Name, "name")
	ADD(Description, "description")

	#undef ADD

	if (iItem.hasObjectPlacement())
	{
		convertIfcObjectPlacement(*iItem.ObjectPlacement(), lNode->mMatrix);
	}

	/*
	std::string lName = (iItem.hasName() ? iItem.Name() : "Product");
	//std::cout << lName << "\n";
	if (lName == "Door #1" || lName == "Spaces")
	{
		return;
	}
	*/

	#define CREATE_PHYSICAL(type, name) if (iItem.is(Type::type)) { lParent = lParent->getChild(name); mStack.push_back(lParent); } else 

	size_t lStackSize = mStack.size();
	Node* lParent = mStack.back();
	
	CREATE_PHYSICAL(IfcWindow, "Windows")
	CREATE_PHYSICAL(IfcColumn, "Columns")
	CREATE_PHYSICAL(IfcCovering, "Coverings")
	CREATE_PHYSICAL(IfcWallStandardCase, "Walls")
	CREATE_PHYSICAL(IfcBuildingElementProxy , "Elements")
	CREATE_PHYSICAL(IfcCurtainWall, "Walls")
	CREATE_PHYSICAL(IfcFooting, "Footings")
	CREATE_PHYSICAL(IfcBeam, "Beams")
	CREATE_PHYSICAL(IfcDoor, "Doors")
	CREATE_PHYSICAL(IfcStair, "Stairs")
	CREATE_PHYSICAL(IfcStairFlight, "Stairs")
	CREATE_PHYSICAL(IfcSlab, "Slabs")
	CREATE_PHYSICAL(IfcFurnishingElement, "Furniture") 
	{
		lParent = lParent->getChild("Other"); 
		mStack.push_back(lParent);
	}

	/*
    ,
    ,IfcMember
    ,IfcPile
    ,IfcPlate
    ,IfcRailing
    ,IfcRamp
    ,IfcRampFlight
    ,IfcRoof
    ,IfcShadingDevice))
	*/


	lParent->addNode(lNode);

	IfcTemplatedEntityList<IfcRelDefinesByProperties>::ptr lList = iItem.IsDefinedBy();
	for (std::vector<IfcRelDefinesByProperties*>::const_iterator lIter = lList->begin() ; lIter != lList->end(); lIter++)
	{
		IfcRelDefinesByProperties* lDefinition = (*lIter);

		IfcPropertySetDefinitionSelect* lProperties = lDefinition->RelatingPropertyDefinition();
		
	};

	static int cc=0;
	cc++;
	if (iItem.hasRepresentation())
	{
		lNode->addNode(parseIfcProductRepresentation(*iItem.Representation(), iIndent + "  "));
		
		std::vector<Node*> lOpenings = getOpenings(iItem, iIndent + "  ");
		if (lOpenings.size())
		{
			for (std::vector<Node*>::iterator lIter = lOpenings.begin() ; lIter != lOpenings.end(); lIter++)
			{
				Mesh lMesh;
				(*lIter)->accumulate(lMesh, mPrecision);
				lNode->subtract(lMesh, mPrecision);
			}
		}
	}

	// add components
	IfcRelAggregates::list::ptr lDecomposedBy = iItem.IsDecomposedBy();
	for (std::vector<IfcRelAggregates*>::const_iterator lIter = lDecomposedBy->begin() ; lIter != lDecomposedBy->end(); lIter++)
	{
		IfcTemplatedEntityList<IfcObjectDefinition>::ptr lList = (*lIter)->RelatedObjects();
		for (std::vector<IfcObjectDefinition*>::const_iterator lIter2 = lList->begin() ; lIter2 != lList->end(); lIter2++)
		{
			IfcObjectDefinition& lObject = *(*lIter2);
			if (lObject.is(Type::IfcElement))
			{
				parseIfcProduct(*lObject.as<IfcElement>(), iIndent + "  ");
			}
		}
	}
	
	lNode->mMaterial = fromLibrary(iItem);

	if (mStack.size() > lStackSize)
	{
		mStack.pop_back();
	}
}

Node* IfcParser::parseIfcProductRepresentation(IfcProductRepresentation& iItem, std::string iIndent)
{
	Node* lNode = new Node(iItem.id(), Node::GEOMETRY);

	IfcTemplatedEntityList<IfcRepresentation>::ptr lList = iItem.Representations();
	for (std::vector<IfcRepresentation*>::const_iterator lIter = lList->begin() ; lIter != lList->end(); lIter++)
	{
		IfcRepresentationContext* lContext = (*lIter)->ContextOfItems();

		if (lContext && lContext->hasContextType())
		{
			std::string lType = lContext->ContextType();
			boost::to_upper(lType);
			if (lType == "MODEL")
			{
				if ((*lIter)->is(Type::IfcShapeModel))
				{
					lNode->addNode(convertIfcShapeModel(*(*lIter)->as<IfcShapeModel>(), iIndent + "  "));
				}
				else
				{
					ERROR((*lIter), "Invalid IfcProductRepresentation");
				}
			}
		}
		else  // this has to be this way for IFC2x
		{
			if ((*lIter)->is(Type::IfcShapeModel))
			{
				lNode->addNode(convertIfcShapeModel(*(*lIter)->as<IfcShapeModel>(), iIndent + "  "));
			}
			else
			{
				ERROR((*lIter), "Invalid IfcProductRepresentation");
			}
		}
	}
	return lNode;
}

void IfcParser::convertIfcObjectPlacement(IfcObjectPlacement& iPlacement, glm::dmat4& iMatrix)
{
	if (iPlacement.is(Type::IfcLocalPlacement))
	{
		IfcLocalPlacement& lLocalPlacement = *iPlacement.as<IfcLocalPlacement>();

		glm::dmat4 lMat0(1.0);
		IfcAxis2Placement* lPlacement = lLocalPlacement.RelativePlacement();
		if (lPlacement->is(Type::IfcAxis2Placement3D))
		{
			lMat0 = convertIfcAxis2Placement3D(*lPlacement->as<IfcAxis2Placement3D>());
		}

		glm::dmat4 lMat1(1.0);
		if (lLocalPlacement.hasPlacementRelTo())
		{
			convertIfcObjectPlacement(*lLocalPlacement.PlacementRelTo(), lMat1);
		}

		iMatrix = lMat1*lMat0;
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "handle GridPlaement here ....";;
	}
};

std::vector<Node*> IfcParser::getOpenings(IfcElement& iElement, std::string iIndent)
{
	std::vector<Node*> lNodes;
	IfcTemplatedEntityList<IfcRelVoidsElement>::ptr lOpenings = iElement.HasOpenings();
	for (std::vector<IfcRelVoidsElement*>::const_iterator lIter = lOpenings->begin() ; lIter != lOpenings->end(); lIter++)
	{
		IfcFeatureElementSubtraction* lSubstraction = (*lIter)->RelatedOpeningElement();
		if (lSubstraction->is(Type::IfcOpeningElement))
		{
			IfcOpeningElement* lOpening = lSubstraction->as<IfcOpeningElement>();
			PRCOESSING((*lOpening), "");

			if (lOpening->hasRepresentation())
			{
				Node* lNode = parseIfcProductRepresentation(*lOpening->Representation(), iIndent + "   ");
				if (lNode)
				{
					lNodes.push_back(lNode);
					if (lOpening->hasObjectPlacement())
					{
						convertIfcObjectPlacement(*lOpening->ObjectPlacement(), lNode->mMatrix);
						//lNode->mMatrix = Shape::toMatrix(*lOpening->ObjectPlacement());
					}
				}
			}
		}
	}

	return lNodes;
}

json_spirit::mObject IfcParser::parseIfcColourRgb(IfcColourRgb& iColor)
{
	json_spirit::mObject lColor;
	lColor["r"] = iColor.Red();
	lColor["g"] = iColor.Green();
	lColor["b"] = iColor.Blue();
//	std::cout << iColor.Red() << ", " << iColor.Green() << ", " << iColor.Blue() << "\n" ;
	return lColor;
}

bool IfcParser::parseIfcSurfaceStyle(IfcSurfaceStyle& iItem, std::string iIndent)
{
	PRCOESSING(iItem, (iItem.hasName() ? iItem.Name() : "unnamed"));

	json_spirit::mObject lMaterial;
	lMaterial["name"] = iItem.hasName() ? iItem.Name() : "unnamed";
	lMaterial["type"] = "IfcSurfaceStyle";

	IfcEntityList::ptr lSurfaceStyles = iItem.Styles();
	for(std::vector<IfcBaseClass*>::const_iterator lIter = lSurfaceStyles->begin(); lIter != lSurfaceStyles->end(); lIter++)
	{
		if ((*lIter)->is(Type::IfcSurfaceStyleShading))
		{
			IfcSurfaceStyleShading* lStyle = (*lIter)->as<IfcSurfaceStyleShading>();

			json_spirit::mObject lColor = parseIfcColourRgb(*lStyle->SurfaceColour());
			if (lStyle->hasTransparency())
			{
				lColor["a"] = 1.0-lStyle->Transparency();
			}
			lMaterial["surfaceColor"] = lColor;
		}
		else if ((*lIter)->is(Type::IfcSurfaceStyleLighting))
		{
			IfcSurfaceStyleLighting* lStyle = (*lIter)->as<IfcSurfaceStyleLighting>();
			lMaterial["diffuseTransmissionColour"] = parseIfcColourRgb(*lStyle->DiffuseTransmissionColour());
			lMaterial["diffuseReflectionColour"] = parseIfcColourRgb(*lStyle->DiffuseReflectionColour());
			lMaterial["transmissionColour"] = parseIfcColourRgb(*lStyle->TransmissionColour());
			lMaterial["reflectanceColour"] = parseIfcColourRgb(*lStyle->ReflectanceColour());
		}
		else if ((*lIter)->is(Type::IfcSurfaceStyleRefraction))
		{
			IfcSurfaceStyleRefraction* lStyle = (*lIter)->as<IfcSurfaceStyleRefraction>();
		}
		else if ((*lIter)->is(Type::IfcSurfaceStyleWithTextures))
		{
			IfcSurfaceStyleWithTextures* lStyle = (*lIter)->as<IfcSurfaceStyleWithTextures>();
		}
		else
		{
			BOOST_LOG_TRIVIAL(error) << iIndent << "(" <<  Type::ToString((*lIter)->type()) << ")  Unknown IfcSurfaceStyleElementSelect";
		}
	}

	if (lMaterial.find("surfaceColor") != lMaterial.end())
	{
		// see if the exact same material already exists;
		json_spirit::mObject lColor = lMaterial["surfaceColor"].get_obj();
		float rs = 	lColor["r"].get_real();
		float gs = 	lColor["g"].get_real();
		float bs = 	lColor["b"].get_real();
		float as = 	lColor.find("a") != lColor.end() ? lColor["a"].get_real() : 1.0;

		for (size_t i=0; i<mMaterials.size(); i++)
		{
			json_spirit::mObject lMaterial = mMaterials[i].get_obj();
			lColor = lMaterial["surfaceColor"].get_obj();
			float rm = 	lColor["r"].get_real();
			float gm = 	lColor["g"].get_real();
			float bm = 	lColor["b"].get_real();
			float am = 	lColor.find("a") != lColor.end() ? lColor["a"].get_real() : 1.0;

			if (fabs(rm-rs) < 0.01 && fabs(gm-gs) < 0.01 && fabs(bm-bs) < 0.01 && fabs(am-as) < 0.01)
			{
				mMaterialMap[iItem.id()] = i;
				return true;
			}
		}

		mMaterialMap[iItem.id()] = mMaterials.size();
		mMaterials.push_back(lMaterial);	
		return true;
	}

	return false;
}


										
void IfcParser::convertIfcOpenShell(IfcOpenShell& iShell, Mesh& iMesh, std::string iIndent)
{
	IfcTemplatedEntityList<IfcFace>::ptr lFaces = iShell.CfsFaces();
	for (std::vector<IfcFace*>::const_iterator lIter = lFaces->begin() ; lIter != lFaces->end(); lIter++)
	{
		convertIfcFace(*(*lIter), iMesh, iIndent, Mesh::FRONT | Mesh::BACK);
	}
}

void IfcParser::convertIfcClosedShell(IfcClosedShell& iShell, Mesh& iMesh, std::string iIndent)
{
	IfcTemplatedEntityList<IfcFace>::ptr lFaces = iShell.CfsFaces();
	for (std::vector<IfcFace*>::const_iterator lIter = lFaces->begin() ; lIter != lFaces->end(); lIter++)
	{
		convertIfcFace(*(*lIter), iMesh, iIndent);
	}
}

void IfcParser::convertIfcLoop(IfcLoop& iLoop, std::vector<glm::dvec3>& iPoints, std::string iIndent)
{
	if (iLoop.is(Type::IfcPolyLoop))
	{
		IfcPolyLoop* lLoop = iLoop.as<IfcPolyLoop>();
		IfcTemplatedEntityList<IfcCartesianPoint>::ptr lPoints = lLoop->Polygon();
		for(std::vector<IfcCartesianPoint*>::const_iterator lIter = lPoints->begin(); lIter != lPoints->end(); lIter++)
		{
			glm::dvec3 lCoords = toVec3((*lIter)->Coordinates());
			iPoints.push_back(lCoords);
		}
	}
	else if (iLoop.is(Type::IfcEdgeLoop)) 
	{
		IfcEdgeLoop* lLoop = iLoop.as<IfcEdgeLoop>();
		IfcTemplatedEntityList<IfcOrientedEdge>::ptr lEdges = lLoop->EdgeList();

		for(std::vector<IfcOrientedEdge*>::const_iterator lIter = lEdges->begin(); lIter != lEdges->end(); lIter++)
		{
			IfcEdge* lEdge = (*lIter)->EdgeElement();

			IfcVertex* lVertex = lEdge->EdgeStart();
			if (lVertex->is(Type::IfcVertexPoint))
			{
				IfcCartesianPoint* lPoint = lVertex->as<IfcVertexPoint>()->VertexGeometry()->as<IfcCartesianPoint>();
				glm::dvec3 lCoords = toVec3(lPoint->Coordinates());
				iPoints.push_back(glm::dvec3(lCoords[0], lCoords[1], lCoords[2]));
			}
		}
	}
	else if (iLoop.is(Type::IfcVertexLoop)) 
	{
		BOOST_LOG_TRIVIAL(info) << iIndent << "(#" << Type::ToString(iLoop.type()) << ") Not implemented ";
	}

	// if first and last point have same coordinates, remove last point
	while(iPoints.size() > 2)
	{
		glm::dvec3& first = iPoints.front();
		glm::dvec3& last = iPoints.back();

		if( std::abs( first.x - last.x ) < 0.00000001 )
		{
			if( std::abs( first.y - last.y ) < 0.00000001 )
			{
				if( std::abs( first.z - last.z ) < 0.00000001 )
				{
					iPoints.pop_back();
					continue;
				}
			}
		}
		break;
	}
}

void IfcParser::convertIfcFace(IfcFace& iFace, Mesh& iMesh, std::string iIndent, uint8_t iSide)
{
	//PRCOESSING(iFace, iIndent);

	Face lFace;

	IfcTemplatedEntityList<IfcFaceBound>::ptr lFaceBounds = iFace.Bounds();
	for (std::vector<IfcFaceBound*>::const_iterator lIter = lFaceBounds->begin(); lIter != lFaceBounds->end(); lIter++)
	{
		//BOOST_LOG_TRIVIAL(info) << iIndent << "(#" << (*lIter)->id() << "," <<  Type::ToString((*lIter)->type()) << ") Face ";

		IfcFaceBound* lBound = *lIter;
		if (lBound->is(Type::IfcFaceOuterBound))
		{
			IfcFaceOuterBound* lOuterBound = lBound->as<IfcFaceOuterBound>();
			convertIfcLoop(*lOuterBound->Bound(), lFace.mBounds, iIndent + "  ");
			if (!lBound->Orientation())
			{
				std::reverse(lFace.mBounds.begin(), lFace.mBounds.end());
			}
		}
		else
		{
			std::vector<glm::dvec3> lPoints;
			convertIfcLoop(*lBound->Bound(), lPoints, iIndent + "  ");
			if (!lBound->Orientation())
			{
				std::reverse(lPoints.begin(), lPoints.end());
			}
			lFace.mHoles.push_back(lPoints);
		}
	}
	
	lFace.triangulate(iSide, iMesh);
}

void IfcParser::convertIfcGeometricRepresentationItem(IfcCsgPrimitive3D& iItem, Mesh& iMesh, std::string iIndent)
{
	uint32_t lStartVertex = iMesh.size();

 	if (iItem.is(Type::IfcBlock))
	{
		IfcBlock* lGeometry = iItem.as<IfcBlock>();
		iMesh.block(lGeometry->XLength(), lGeometry->YLength(), lGeometry->ZLength());
	}
	else if (iItem.is(Type::IfcSphere))
	{
		IfcSphere* lGeometry = iItem.as<IfcSphere>();
		iMesh.sphere(lGeometry->Radius());
	}
	else if (iItem.is(Type::IfcRectangularPyramid))
	{
		IfcRectangularPyramid& lItem = *iItem.as<IfcRectangularPyramid>();
		Mesh::pyramid(iMesh, glm::dvec3(0.0), glm::dvec3(lItem.XLength(), lItem.YLength(), lItem.Height()));
	}
	else if (iItem.is(Type::IfcRightCircularCone))
	{
		IfcRightCircularCone& lItem = *iItem.as<IfcRightCircularCone>();
		Mesh::cylinder(iMesh, glm::dvec3(0.0), glm::dvec2(lItem.BottomRadius(), lItem.Height()));
	}
	else if (iItem.is(Type::IfcRightCircularCylinder))	
	{
		IfcRightCircularCylinder& lItem = *iItem.as<IfcRightCircularCylinder>();
		Mesh::cylinder(iMesh, glm::dvec3(0.0), glm::dvec2(lItem.Radius(), lItem.Height()));
	}

	iMesh.transform(convertIfcAxis2Placement3D(*iItem.Position()), lStartVertex);

}

static int t=0;
void IfcParser::convertIfcGeometricRepresentationItem(IfcBooleanResult& iItem, Mesh& iMesh, std::string iIndent)
{
	IfcBooleanOperand* lOperand0 = iItem.FirstOperand();
	IfcBooleanOperand* lOperand1 = iItem.SecondOperand();

	Mesh lMesh0;
	if (lOperand0->is(Type::IfcSolidModel))
	{
		convertIfcGeometricRepresentationItem(*lOperand0->as<IfcSolidModel>(), lMesh0, iIndent + " ");
	}
	else if (lOperand0->is(Type::IfcBooleanResult))
	{
		convertIfcGeometricRepresentationItem(*lOperand0->as<IfcBooleanResult>(), lMesh0, iIndent + " ");
	}
	else if (lOperand0->is(Type::IfcCsgPrimitive3D))
	{
		convertIfcGeometricRepresentationItem(*lOperand0->as<IfcCsgPrimitive3D>(), lMesh0, iIndent);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "(#??," <<  Type::ToString(lOperand1->type()) << ") Unknown Boolean Operand";
	}
	
	 
	if (lOperand1->is(Type::IfcHalfSpaceSolid))
	{
		IfcHalfSpaceSolid* lHalfspace = lOperand1->as<IfcHalfSpaceSolid>();

		IfcSurface* lSurface = lHalfspace->BaseSurface();
		glm::dmat4 lPlane = convertIfcAxis2Placement3D(*lSurface->as<IfcPlane>()->Position());

		AABB lAABB(lMesh0.vertexBuffer());
		double lA = round(lAABB.range());

		Mesh lSolid;
		if (lOperand1->is(Type::IfcBoxedHalfSpace))
		{
			IfcBoxedHalfSpace* lGeometry = lOperand1->as<IfcBoxedHalfSpace>();
			ERROR((&iItem), "Unknown IfcBoxedHalfSpace");
		}
		else if (lOperand1->is(Type::IfcPolygonalBoundedHalfSpace))
		{
			IfcPolygonalBoundedHalfSpace* lGeometry = lOperand1->as<IfcPolygonalBoundedHalfSpace>();
			//if (t < 2)
			{
				// extrude solid big enought to cover entire lMesh0
				std::vector<glm::dvec3> lBounds;
				convertIfcCurve(*lGeometry->PolygonalBoundary(), lBounds);

				Extrusion lExtrusion(new Profile(lBounds));
				lExtrusion.extrude(glm::dvec3(0,0,2*lA), lSolid);

				lSolid.transform(glm::translate(glm::dmat4(1.0f), glm::dvec3(0,0,-lA)));

				// transfrom plane into solid coordinates
				glm::dmat4 lMatrix = convertIfcAxis2Placement3D(*(lGeometry->Position()));
				glm::dmat4 lInverse = glm::inverse(lMatrix);
				lPlane = lInverse*lPlane;

				// clip extruded solid by base plane
				Mesh lBaseplane;
				glm::dvec3 lP0 = glm::dvec3(lPlane*glm::dvec4(-lA, -lA, 0.0, 1.0));
				glm::dvec3 lP1 = glm::dvec3(lPlane*glm::dvec4( lA, -lA, 0.0, 1.0));
				glm::dvec3 lP2 = glm::dvec3(lPlane*glm::dvec4( lA,  lA, 0.0, 1.0));
				glm::dvec3 lP3 = glm::dvec3(lPlane*glm::dvec4(-lA,  lA, 0.0, 1.0));

				if (lHalfspace->AgreementFlag())
				{
					lBaseplane.pushTriangle(lP2, lP1, lP0, true);
					lBaseplane.pushTriangle(lP0, lP3, lP2, true);
				}
				else 
				{
					lBaseplane.pushTriangle(lP0, lP1, lP2, true);
					lBaseplane.pushTriangle(lP2, lP3, lP0, true);
				}
				BspTree::subtract(lSolid, lBaseplane, mPrecision, t);

				// transfrom solid into plane coordinates
				lSolid.transform(lMatrix);
			}
		}
		else
		{
			glm::dvec3 lP0 = glm::dvec3(lPlane*glm::dvec4(-lA, -lA, 0.0, 1.0));
			glm::dvec3 lP1 = glm::dvec3(lPlane*glm::dvec4( lA, -lA, 0.0, 1.0));
			glm::dvec3 lP2 = glm::dvec3(lPlane*glm::dvec4( lA,  lA, 0.0, 1.0));
			glm::dvec3 lP3 = glm::dvec3(lPlane*glm::dvec4(-lA,  lA, 0.0, 1.0));

			if (!lHalfspace->AgreementFlag())
			{
				lSolid.pushTriangle(lP2, lP1, lP0, true);
				lSolid.pushTriangle(lP0, lP3, lP2, true);
			}
			else 
			{
				lSolid.pushTriangle(lP0, lP1, lP2, true);
				lSolid.pushTriangle(lP2, lP3, lP0, true);
			}
		}

		if (lSolid.size())
		{
			BspTree::subtract(lMesh0, lSolid, mPrecision);
		}
	}
	else 
	{
		Mesh lMesh1;
		if (lOperand1->is(Type::IfcSolidModel))
		{
			convertIfcGeometricRepresentationItem(*lOperand1->as<IfcSolidModel>(), lMesh1, iIndent + " ");
		}
		else if (lOperand1->is(Type::IfcBooleanResult))
		{
			convertIfcGeometricRepresentationItem(*lOperand1->as<IfcBooleanResult>(), lMesh1, iIndent + " ");
		}
		else if (lOperand1->is(Type::IfcCsgPrimitive3D))
		{
			convertIfcGeometricRepresentationItem(*lOperand1->as<IfcCsgPrimitive3D>(), lMesh1, iIndent);
		}
		else 
		{
			BOOST_LOG_TRIVIAL(error) << "(#??," <<  Type::ToString(lOperand1->type()) << ") Unknown Boolean Operand";
		}

		IfcBooleanOperator::IfcBooleanOperator lOperator = iItem.Operator();
	
		//if (t++ < 1)
		{
			switch (lOperator)
			{
			case IfcBooleanOperator::IfcBooleanOperator_UNION:
				BspTree::combine(lMesh0, lMesh1, mPrecision);
				break;
			case IfcBooleanOperator::IfcBooleanOperator_INTERSECTION:
				BspTree::intersect(lMesh0, lMesh1, mPrecision);
				break;
			case IfcBooleanOperator::IfcBooleanOperator_DIFFERENCE:
				BspTree::subtract(lMesh0, lMesh1, mPrecision);
				break;
			}
		}
		//else
		{
		//	lMesh0.append(lMesh1);
		}
	}
	
	iMesh.append(lMesh0);
};


void IfcParser::convertIfcGeometricRepresentationItem(IfcGeometricCurveSet& iItem, Line& iLine, std::string iIndent)
{
	IfcEntityList::ptr lList = iItem.Elements();
	
	for (std::vector<IfcBaseClass*>::const_iterator lIter = lList->begin() ; lIter != lList->end(); lIter++)
	{
		if ((*lIter)->is(Type::IfcPolyline))
		{
			IfcPolyline* lCurve = (*lIter)->as<IfcPolyline>();

			IfcTemplatedEntityList< IfcCartesianPoint >::ptr lPoints = lCurve->Points();
			std::vector<glm::dvec3>& lLine = iLine.vertexBuffer();
			for(std::vector<IfcCartesianPoint*>::const_iterator lIter = lPoints->begin(); lIter != lPoints->end(); lIter++)
			{
				glm::dvec3 lCoords = toVec3((*lIter)->Coordinates());
				lLine.push_back(lCoords);
			}
		}
		else
		{
			BOOST_LOG_TRIVIAL(error) << "(#??," <<  Type::ToString((*lIter)->type()) << ") Unknown Curve";
		}
	}
}

void IfcParser::convertIfcGeometricRepresentationItem(IfcTriangulatedFaceSet& iItem, Mesh& iMesh, std::string iIndent)
{
	IfcCartesianPointList3D* lCoordinates = iItem.Coordinates();
	
	std::vector<std::vector<int>> lTriangleArray = iItem.CoordIndex();
	std::vector<std::vector<double>> lVertexArray = lCoordinates->CoordList();

	bool lClosed = false;
	if (iItem.hasClosed())
	{
		lClosed = iItem.Closed();
	}


	// vertex
	int lPointer = 0;
	for (size_t t=0; t<lTriangleArray.size(); t++)
	{
		std::vector<int> lIndex = lTriangleArray[t];

		std::vector<double>& la0 = lVertexArray[lIndex[0]-1];
		std::vector<double>& la1 = lVertexArray[lIndex[1]-1];
		std::vector<double>& la2 = lVertexArray[lIndex[2]-1];

		glm::dvec3 lV0(la0[0], la0[1], la0[2]);
		glm::dvec3 lV1(la1[0], la1[1], la1[2]);
		glm::dvec3 lV2(la2[0], la2[1], la2[2]);

		iMesh.pushTriangle(lV0, lV1, lV2, !iItem.hasNormals());
		if (!lClosed)
		{
			iMesh.pushTriangle(lV2, lV1, lV0, !iItem.hasNormals());
		}
	}
	
	// normal
	if (iItem.hasNormals())
	{
		std::vector<std::vector<double>> lNormalArray = iItem.Normals();

		std::vector<int> lPnIndex;
		if (iItem.hasPnIndex())
		{
			lPnIndex = iItem.PnIndex();
		}

		int lPointer = 0;
		for (size_t t=0; t<lTriangleArray.size(); t++)
		{
			std::vector<int> lIndex = lTriangleArray[t];

			//if (lPnIndex.size())
			{
			//	lIndex[0] = lPnIndex[lIndex[0]];
			//	lIndex[1] = lPnIndex[lIndex[1]];
			//	lIndex[1] = lPnIndex[lIndex[2]];
			}
			//

			std::vector<double>& la0 = lNormalArray[lIndex[0]-1];
			std::vector<double>& la1 = lNormalArray[lIndex[1]-1];
			std::vector<double>& la2 = lNormalArray[lIndex[2]-1];

			glm::dvec3 lN0(la0[0], la0[1], la0[2]);
			glm::dvec3 lN1(la1[0], la1[1], la1[2]);
			glm::dvec3 lN2(la2[0], la2[1], la2[2]);

			iMesh.pushNormals(lN0, lN1, lN2);
		}
	}
}

void IfcParser::convertIfcGeometricRepresentationItem(IfcPolygonalFaceSet& iItem, Mesh& iMesh, std::string iIndent)
{
	IfcCartesianPointList3D* lCoordinates = iItem.Coordinates();
	std::vector<std::vector<double>> lPoints = lCoordinates->CoordList();
	
	std::vector<int> lPnIndex;
	if (iItem.hasPnIndex())
	{
		lPnIndex = iItem.PnIndex();
	}

	#define READ_LOOP(buffer,loop)\
		for (std::vector<int>::iterator lIterI = loop.begin(); lIterI != loop.end(); lIterI++)\
		{\
			int lIndex = lPnIndex.size() ? lPnIndex[*lIterI-1] : *lIterI-1;\
			buffer.push_back(glm::make_vec3(&lPoints[lIndex][0]));\
		}\

	IfcTemplatedEntityList<IfcIndexedPolygonalFace>::ptr lList = iItem.Faces();
	for (std::vector<IfcIndexedPolygonalFace*>::const_iterator lIter = lList->begin() ; lIter != lList->end(); lIter++)
	{
		IfcIndexedPolygonalFace* lItem = (*lIter);

		std::vector<int> lCoordIndex = lItem->CoordIndex();

		Face lFace;
		READ_LOOP(lFace.mBounds, lCoordIndex);

		if (lItem->is(Type::IfcIndexedPolygonalFaceWithVoids))
		{
			IfcIndexedPolygonalFaceWithVoids* lFaceVoids = lItem->as<IfcIndexedPolygonalFaceWithVoids>();
			std::vector<std::vector<int>> lVoids = lFaceVoids->InnerCoordIndices();
			for (std::vector<std::vector<int>>::iterator lIter2 = lVoids.begin(); lIter2 != lVoids.end(); lIter2++)
			{
				std::vector<glm::dvec3>& lHole = lFace.newHole();
				READ_LOOP(lHole, (*lIter2))
			}
		}

		lFace.triangulate(Mesh::FRONT | (iItem.Closed() ? 0 : Mesh::BACK), iMesh);
	}
}


void IfcParser::convertIfcGeometricRepresentationItem(IfcSolidModel& iItem, Mesh& iMesh, std::string iIndent)
{
	if (iItem.is(Type::IfcManifoldSolidBrep))
	{
		IfcManifoldSolidBrep& lModel = *iItem.as<IfcManifoldSolidBrep>();

		if (iItem.is(Type::IfcFacetedBrep))
		{
			convertIfcClosedShell(*lModel.Outer(), iMesh, iIndent);
		}
		else if (iItem.is(Type::IfcAdvancedBrep))
		{
			IfcAdvancedBrep* lAdvanced = iItem.as<IfcAdvancedBrep>();

			IfcClosedShell* lShell = lModel.Outer();

			ERROR((&iItem), "Implement Me");

			IfcTemplatedEntityList<IfcFace>::ptr lFaces = lShell->CfsFaces();
			for (std::vector<IfcFace*>::const_iterator lIter = lFaces->begin() ; lIter != lFaces->end(); lIter++)
			{
				IfcFaceOuterBound* lBound = (*(*lIter)->Bounds()->begin())->as<IfcFaceOuterBound>();
			}
		}
	}
	else if (iItem.is(Type::IfcSweptDiskSolid))
	{
		IfcSweptDiskSolid& lSolid = *iItem.as<IfcSweptDiskSolid>();

		std::vector<glm::dvec3> lPath;
		convertIfcGeometricRepresentationItem(*lSolid.Directrix(), lPath);

		Profile* lProfile = new Profile();
		if (lSolid.hasInnerRadius())
		{
			Profile::circle(*lProfile, lSolid.Radius(), lSolid.InnerRadius());
		}
		else
		{
			Profile::circle(*lProfile, lSolid.Radius());
		}

		Extrusion lExtrusion(lProfile);
		lExtrusion.sweep(Mesh::FRONT, lPath, iMesh);
	}
	else if (iItem.is(Type::IfcSweptAreaSolid))
	{
		IfcSweptAreaSolid& lSolid = *iItem.as<IfcSweptAreaSolid>();

		uint32_t lStartVertex = iMesh.size();

		if (lSolid.is(Type::IfcExtrudedAreaSolid))
		{
			convertIfcSweptAreaSolid(*iItem.as<IfcExtrudedAreaSolid>(), iMesh, iIndent + " ");
		}
		else if (lSolid.is(Type::IfcRevolvedAreaSolid))
		{
			convertIfcSweptAreaSolid(*iItem.as<IfcRevolvedAreaSolid>(), iMesh, iIndent + " ");
		}
		else if (lSolid.is(Type::IfcSurfaceCurveSweptAreaSolid))
		{
			convertIfcSweptAreaSolid(*iItem.as<IfcSurfaceCurveSweptAreaSolid>(), iMesh, iIndent + " ");
		}
		else if (lSolid.is(Type::IfcFixedReferenceSweptAreaSolid))
		{
			convertIfcSweptAreaSolid(*iItem.as<IfcFixedReferenceSweptAreaSolid>(), iMesh, iIndent + " ");
		}

		if (lSolid.hasPosition())
		{
			iMesh.transform(convertIfcAxis2Placement3D(*lSolid.Position()), lStartVertex);
		};
	}
	else if (iItem.is(Type::IfcCsgSolid))
	{
		IfcCsgSolid* lSolid = iItem.as<IfcCsgSolid>();

		IfcCsgSelect* lSelect = lSolid->TreeRootExpression();

		if (lSelect->is(Type::IfcBooleanResult))
		{
			convertIfcGeometricRepresentationItem(* lSelect->as<IfcBooleanResult>(), iMesh, iIndent + "  ");
		}
		else if (lSelect->is(Type::IfcCsgPrimitive3D))
		{
			convertIfcGeometricRepresentationItem(*lSelect->as<IfcCsgPrimitive3D>(), iMesh, iIndent + "  ");
		}
	}
	else
	{
		ERROR((&iItem), "Implement Me");
	}
}

void IfcParser::convertIfcSweptAreaSolid(IfcExtrudedAreaSolid& iItem, Mesh& iMesh, std::string iIndent)
{
	IfcDirection* lDirection = iItem.ExtrudedDirection();
	glm::dvec3 lRatios = toVec3(lDirection->DirectionRatios());
	glm::dvec3 lAxis = glm::normalize(glm::dvec3(lRatios[0], lRatios[1], lRatios[2]))*iItem.Depth();

	Extrusion lExtrusion(convertIfcProfileDef(*iItem.SweptArea(), iIndent+"   "));
	lExtrusion.extrude(lAxis, iMesh);
}

void IfcParser::convertIfcSweptAreaSolid(IfcRevolvedAreaSolid& iItem, Mesh& iMesh, std::string iIndent)
{
	IfcAxis1Placement* lPlacement = iItem.Axis();

	glm::dvec3 lDirection(0,0,1);
	if (lPlacement->hasAxis())
	{
		IfcDirection& lAxis = *lPlacement->Axis();
		lDirection = glm::normalize(toVec3(lAxis.DirectionRatios()));
	}

	IfcCartesianPoint& lLocation = *lPlacement->Location();
	Extrusion lExtrusion(convertIfcProfileDef(*iItem.SweptArea(), iIndent));
	lExtrusion.revolve(toVec3(lLocation.Coordinates()), lDirection, iItem.Angle()*mAngleUnit, iMesh);
}

void IfcParser::convertIfcSweptAreaSolid(IfcSurfaceCurveSweptAreaSolid& iItem, Mesh& iMesh, std::string iIndent)
{
	ERROR((&iItem), "Implement Me");
}

void IfcParser::convertIfcSweptAreaSolid(IfcFixedReferenceSweptAreaSolid& iItem, Mesh& iMesh, std::string iIndent)
{
	ERROR((&iItem), "Implement Me");
}




void IfcParser::convertIfcGeometricRepresentationItem(IfcCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	if (iItem.is(Type::IfcBoundedCurve))
	{
		convertIfcCurve(*iItem.as<IfcBoundedCurve>(), iBounds, iTrim0, iTrim1);
	}
	else if (iItem.is(Type::IfcConic))
	{
		convertIfcCurve(*iItem.as<IfcConic>(), iBounds, iTrim0, iTrim1);
	}
	else if (iItem.is(Type::IfcLine))
	{
		convertIfcCurve(*iItem.as<IfcLine>(), iBounds, iTrim0, iTrim1);
	}
	else if (iItem.is(Type::IfcOffsetCurve2D))
	{
		convertIfcCurve(*iItem.as<IfcOffsetCurve2D>(), iBounds, iTrim0, iTrim1);
	}
	else if (iItem.is(Type::IfcOffsetCurve3D))
	{
		convertIfcCurve(*iItem.as<IfcOffsetCurve3D>(), iBounds, iTrim0, iTrim1);
	}
	else if (iItem.is(Type::IfcPcurve))
	{
		convertIfcCurve(*iItem.as<IfcPcurve>(), iBounds, iTrim0, iTrim1);
	}

	std::vector<glm::dvec3>::iterator lIter = iBounds.begin();
	if (lIter != iBounds.end())
	{
		glm::dvec3 lPrev = *lIter;
		lIter++;
		while (lIter != iBounds.end())
		{
			glm::dvec3 lCurr = *lIter;
			if (glm::all(glm::epsilonEqual(lCurr, lPrev, 0.00001)))
			{
				lIter = iBounds.erase(lIter);
			}
			else
			{
				lPrev = *lIter;
				lIter++;
			}
		}
	}
}

void IfcParser::convertIfcCurve(IfcBoundedCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	if (iItem.is(Type::IfcPolyline))
	{
		convertIfcBoundedCurve(*iItem.as<IfcPolyline>(), iBounds, iTrim0, iTrim1);
	}
	else if (iItem.is(Type::IfcIndexedPolyCurve))
	{
		convertIfcBoundedCurve(*iItem.as<IfcIndexedPolyCurve>(), iBounds, iTrim0, iTrim1);
	}
	else if (iItem.is(Type::IfcBSplineCurve))
	{
		convertIfcBoundedCurve(*iItem.as<IfcBSplineCurve>(), iBounds, iTrim0, iTrim1);
	}
	else if (iItem.is(Type::IfcCompositeCurve))
	{
		convertIfcBoundedCurve(*iItem.as<IfcCompositeCurve>(), iBounds, iTrim0, iTrim1);
	}
	else if (iItem.is(Type::IfcTrimmedCurve))
	{
		convertIfcBoundedCurve(*iItem.as<IfcTrimmedCurve>(), iBounds, iTrim0, iTrim1);
	}
}

void IfcParser::convertIfcBoundedCurve(IfcPolyline& iCurve, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)
{
	IfcTemplatedEntityList< IfcCartesianPoint >::ptr lPoints = iCurve.Points();

	iBounds.reserve(lPoints->size());
	for (std::vector<IfcCartesianPoint*>::const_iterator lIter = lPoints->begin(); lIter != lPoints->end(); lIter++)
	{
		std::vector<double> lCoords = (*lIter)->Coordinates();
		addVec3(iBounds, lCoords);
	}
};

void IfcParser::convertIfcBoundedCurve(IfcIndexedPolyCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	IfcCartesianPointList* lPoints = iItem.Points();

 	std::vector<std::vector<double>> lList;
	if (lPoints->is(Type::IfcCartesianPointList2D))
	{
		IfcCartesianPointList2D* lPoints2D = lPoints->as<IfcCartesianPointList2D>();
		lList = lPoints2D->CoordList();
	}
	else if (lPoints->is(Type::IfcCartesianPointList3D))
	{
		IfcCartesianPointList3D* lPoints3D = lPoints->as<IfcCartesianPointList3D>();
		lList = lPoints3D->CoordList();
	}

	if (iItem.hasSegments())
	{
		IfcEntityList::ptr lSegments = iItem.Segments();

		if (lSegments->begin() != lSegments->end())
		{
			// push first point
			IfcBaseClass* lFirst = *lSegments->begin();
			if (lFirst->is(Type::IfcLineIndex))
			{
				IfcLineIndex& lLine = *lFirst->as<IfcLineIndex>();
				std::vector<int> lIndex = lLine;
				addVec3(iBounds, lList[lIndex[0]-1]);
			}
			else if (lFirst->is(Type::IfcArcIndex))
			{
				IfcArcIndex& lArc = *lFirst->as<IfcArcIndex>();
				std::vector<int> lIndex = lArc;
				addVec3(iBounds, lList[lIndex[0]-1]);
			}
		}
		for(std::vector<IfcBaseClass*>::const_iterator lIter = lSegments->begin(); lIter != lSegments->end(); lIter++)
		{
			// push all but first
			if ((*lIter)->is(Type::IfcLineIndex))
			{
				IfcLineIndex& lLine = *(*lIter)->as<IfcLineIndex>();
				std::vector<int> lIndex = lLine;
				for (size_t i=1; i<lIndex.size(); i++)
				{
					addVec3(iBounds, lList[lIndex[i]-1]);
				}
			}
			else if ((*lIter)->is(Type::IfcArcIndex))
			{
				IfcArcIndex& lArc = *(*lIter)->as<IfcArcIndex>();
				std::vector<int> lIndex = lArc;
				addVec3(iBounds, lList[lIndex[1]-1]);
				addVec3(iBounds, lList[lIndex[2]-1]);
			}
		}
	}
	else
	{
		for (size_t i=0; i<lList.size(); i++)
		{
			addVec3(iBounds, lList[i]);
		}
	}
}

void IfcParser::convertIfcBoundedCurve(IfcBSplineCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	IMPLEMENT((&iItem));
	//IfcBSplineCurve
	/*
	Vector BSpline::interpolate(double u, const Vector& P0, const Vector& P1, const Vector& P2, const Vector& P3)
	{
		Vector point;
		point=u*u*u*((-1) * P0 + 3 * P1 - 3 * P2 + P3) / 6;
		point+=u*u*(3*P0 - 6 * P1+ 3 * P2) / 6;
		point+=u*((-3) * P0 + 3 * P2) / 6;
		point+=(P0+4*P1+P2) / 6;

		return point;
	}
	*/
}

void IfcParser::convertIfcBoundedCurve(IfcCompositeCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	IfcTemplatedEntityList<IfcCompositeCurveSegment>::ptr lSegments = iItem.Segments();
	static int s0 = 0;
	for(std::vector<IfcCompositeCurveSegment*>::const_iterator lIter = lSegments->begin(); lIter != lSegments->end(); lIter++)
	{
		IfcCompositeCurveSegment& lSegment = *(*lIter);

		std::vector<glm::dvec3> lBounds;
		convertIfcGeometricRepresentationItem(*(*lIter)->ParentCurve(), lBounds);
		if(!lSegment.SameSense())
		{
			std::reverse(lBounds.begin(), lBounds.end());
		}

		std::vector<glm::dvec3>::iterator lIterB = lBounds.begin();
		if (lIterB != lBounds.end())
		{
			if (iBounds.size())
			{
				if (glm::all(glm::epsilonEqual(iBounds.back(), *lIterB, 0.0001)))
				{
					lIterB++;
				}
			}

			iBounds.insert(iBounds.end(), lIterB, lBounds.end());
		}
	}
}

const uint8_t IfcParser::TRIM_NONE = 0x0;
const uint8_t IfcParser::TRIM_PARAM = 0x1;
const uint8_t IfcParser::TRIM_POINT = 0x2;
const uint8_t IfcParser::TRIM_BOTH = 0x3;

void IfcParser::convertIfcBoundedCurve(IfcTrimmedCurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	#define GET_TRIM(trim,list)\
	{\
		IfcEntityList::ptr lList = list;\
		for(std::vector<IfcBaseClass*>::const_iterator lIter = lList->begin(); lIter != lList->end(); lIter++)\
		{\
			if ((*lIter)->is(Type::IfcParameterValue))\
			{\
				IfcParameterValue* lValue = (*lIter)->as<IfcParameterValue>();\
				std::get<2>(trim) |= TRIM_PARAM;\
				std::get<0>(trim) = (double)(*lValue);\
			}\
			else if ((*lIter)->is(Type::IfcCartesianPoint))\
			{\
				IfcCartesianPoint* lValue = (*lIter)->as<IfcCartesianPoint>();\
				std::get<2>(trim) |= TRIM_POINT;\
				std::vector<double> lPoint = lValue->Coordinates();\
				std::get<1>(trim) = toVec3(lPoint);\
			}\
		}\
	}\

	Trim lTrim0(0,glm::dvec3(0), TRIM_NONE);
	Trim lTrim1(0,glm::dvec3(0), TRIM_NONE);
	GET_TRIM(lTrim0, iItem.Trim1());
	GET_TRIM(lTrim1, iItem.Trim2());

	#undef GET_TRIM

	if( !iItem.SenseAgreement())
	{
		std::swap(lTrim0, lTrim1);
	}

	std::vector<glm::dvec3> lBounds;
	convertIfcGeometricRepresentationItem(*iItem.BasisCurve(), lBounds, lTrim0, lTrim1);

	if( !iItem.SenseAgreement())
	{
		iBounds.insert(iBounds.end(), lBounds.rbegin(), lBounds.rend());
	}
	else
	{
		iBounds.insert(iBounds.end(), lBounds.begin(), lBounds.end());
	}
}

void IfcParser::convertIfcCurve(IfcConic& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	std::vector<glm::dvec3> lBounds;

	#define TO_RADIANS(trim)\
	if (std::get<2>(trim) & TRIM_PARAM)\
	{\
		std::get<0>(trim) = fmod(std::get<0>(trim)*mAngleUnit, 2.0*M_PI);\
	}\
	else if (std::get<2>(trim) & TRIM_POINT)\
	{\
		glm::dvec2 lVector = glm::normalize(std::get<1>(trim));\
		if (std::abs(lVector.x) < 0)\
		{\
			if( lVector.y > 0 )\
			{\
				std::get<0>(trim) = M_PI_2;\
			}\
			else if (lVector.y < 0 )\
			{\
				std::get<0>(trim) = M_PI*1.5;\
			}\
		}\
		else \
		{\
			if (lVector.y > 0 )\
			{\
				std::get<0>(trim) = acos(lVector.x);\
			}\
			else if (lVector.y < 0 )\
			{\
				std::get<0>(trim) = 2.0*M_PI - acos(lVector.x);\
			}\
		}\
	}\

	TO_RADIANS(iTrim0);
	TO_RADIANS(iTrim1);

	//if( !sense_agreement )
	//{
	//	std::swap(trim_angle1, trim_angle2);
	//}
	if (iItem.is(Type::IfcEllipse))
	{
		convertIfcConic(*iItem.as<IfcEllipse>(), lBounds, std::get<0>(iTrim0), std::get<0>(iTrim1));
	}
	else if (iItem.is(Type::IfcCircle))
	{
		convertIfcConic(*iItem.as<IfcCircle>(), lBounds, std::get<0>(iTrim0), std::get<0>(iTrim1));
	}
	
	IfcAxis2Placement* lPosition = iItem.Position();
	if (lPosition->is(Type::IfcAxis2Placement2D))
	{
		glm::dmat3 lMatrix = convertIfcAxis2Placement2D(*lPosition->as<IfcAxis2Placement2D>());
		for (std::vector<glm::dvec3>::iterator lIter = lBounds.begin(); lIter != lBounds.end(); lIter++)
		{
			(*lIter) = lMatrix*glm::dvec3((*lIter).x, (*lIter).y, 1.0);
			(*lIter).z = 0;
		};
	}
	else if (lPosition->is(Type::IfcAxis2Placement3D)) 
	{
		glm::dmat3 lMatrix = convertIfcAxis2Placement3D(*lPosition->as<IfcAxis2Placement3D>());

		for (std::vector<glm::dvec3>::iterator lIter = lBounds.begin(); lIter != lBounds.end(); lIter++)
		{
			(*lIter) = lMatrix*glm::dvec4((*lIter).x, (*lIter).y, (*lIter).z, 1.0);
		};
	}
	
	iBounds.insert(iBounds.end(), lBounds.begin(), lBounds.end());

	#undef TO_RADIANS
}

void IfcParser::convertIfcConic(IfcEllipse& iItem, std::vector<glm::dvec3>& iBounds, double iA0, double iA1)  
{
	static double kN = 12;
	
	double dR = fmod(iA1+2.0*M_PI-iA0, 2.0*M_PI)/kN;
	double lRx = iItem.SemiAxis1();
	double lRy = iItem.SemiAxis2();
	for (int i=0; i<kN; i++)
	{
		iBounds.push_back(glm::dvec3(cos(iA0+i*dR)*lRx, lRy*sin(iA0+i*dR), 0));
	}
}

void IfcParser::convertIfcConic(IfcCircle& iItem, std::vector<glm::dvec3>& iBounds, double iA0, double iA1)  
{
	static double kN = 12;
	
	double dR = fmod(iA1+2.0*M_PI-iA0, 2.0*M_PI)/kN;
	double lR = iItem.Radius();
	for (int i=0; i<=kN; i++)
	{
		iBounds.push_back(lR*glm::dvec3(cos(iA0+i*dR), sin(iA0+i*dR), 0));
	}
}

void IfcParser::convertIfcCurve(IfcLine& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	IfcVector* lVector = iItem.Dir();
	IfcCartesianPoint* lPnt = iItem.Pnt();
	IfcDirection* lOrientation = lVector->Orientation();

	glm::dvec3 p = toVec3(lPnt->Coordinates());
	glm::dvec3 v = toVec3(lOrientation->DirectionRatios())*lVector->Magnitude();

	if (std::get<2>(iTrim0) & TRIM_PARAM)
	{
		double t = std::get<0>(iTrim0);
		iBounds.push_back(glm::dvec3(p[0]+v[0]*t, p[1]+v[1]*t, p[2]+v[2]*t));
	}
	else if (std::get<2>(iTrim0) & TRIM_POINT)
	{
		double t = glm::dot(p-std::get<1>(iTrim0),v)/glm::dot(v,v);
		if (t <= 0)
		{
			iBounds.push_back(p);
		}
		else if (t >= 1)
		{
			iBounds.push_back(p + v);
		}
		else
		{
			iBounds.push_back(p + v*t);
		}
	}
	else 
	{
		iBounds.push_back(p);
	}

	if (std::get<2>(iTrim1) & TRIM_PARAM)
	{
		double t = std::get<0>(iTrim1);
		iBounds.push_back(glm::dvec3(p[0]+v[0]*t, p[1]+v[1]*t, p[2]+v[2]*t));
	}
	else if (std::get<2>(iTrim1) & TRIM_POINT)
	{
		double t = glm::dot(p-std::get<1>(iTrim1),v)/glm::dot(v,v);
		if (t <= 0)
		{
			iBounds.push_back(p);
		}
		else if (t >= 1)
		{
			iBounds.push_back(p + v);
		}
		else
		{
			iBounds.push_back(p + v*t);
		}
	}
	else
	{
		iBounds.push_back(p+v);
	}
}

void IfcParser::convertIfcCurve(IfcOffsetCurve2D& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	IMPLEMENT((&iItem));
}

void IfcParser::convertIfcCurve(IfcOffsetCurve3D& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	IMPLEMENT((&iItem));
}

void IfcParser::convertIfcCurve(IfcPcurve& iItem, std::vector<glm::dvec3>& iBounds, Trim iTrim0, Trim iTrim1)  
{
	IMPLEMENT((&iItem));
}








Profile* IfcParser::convertIfcProfileDef(IfcProfileDef& iItem, std::string iIndent)
{
	Profile* lProfile = 0;

	if (iItem.is(Type::IfcArbitraryClosedProfileDef))
	{
		IfcArbitraryClosedProfileDef& lItem = *iItem.as<IfcArbitraryClosedProfileDef>();

		std::vector<glm::dvec3> lBounds;
		convertIfcGeometricRepresentationItem(*lItem.OuterCurve(), lBounds);
		lProfile = new Profile();
		toVec2(lBounds, lProfile->mBounds);
		
		if (lItem.is(Type::IfcArbitraryProfileDefWithVoids))
		{
			IfcArbitraryProfileDefWithVoids& lItem = *iItem.as<IfcArbitraryProfileDefWithVoids>();

			IfcTemplatedEntityList<IfcCurve>::ptr lList = lItem.InnerCurves();
			for(std::vector<IfcCurve*>::const_iterator lIter = lList->begin(); lIter != lList->end(); lIter++)
			{
				std::vector<glm::dvec3> lHole;
				convertIfcGeometricRepresentationItem(*(*lIter), lHole);
				toVec2(lHole, lProfile->newHole());
			}
		}
	}
	else if (iItem.is(Type::IfcParameterizedProfileDef))
	{
		IfcParameterizedProfileDef& lItem = *iItem.as<IfcParameterizedProfileDef>();

		if (lItem.is(Type::IfcRectangleProfileDef))
		{
			if (lItem.is(Type::IfcRectangleHollowProfileDef))
			{
				lProfile = convertIfcParameterizedProfileDef(*lItem.as<IfcRectangleHollowProfileDef>());
			}
			else if (lItem.is(Type::IfcRoundedRectangleProfileDef))
			{
				lProfile = convertIfcParameterizedProfileDef(*lItem.as<IfcRoundedRectangleProfileDef>());
			}
			else
			{
				lProfile = convertIfcParameterizedProfileDef(*iItem.as<IfcRectangleProfileDef>());
			}
		}
		else if (lItem.is(Type::IfcCircleProfileDef))
		{
			if (lItem.is(Type::IfcCircleHollowProfileDef))
			{
				lProfile = convertIfcParameterizedProfileDef(*lItem.as<IfcCircleHollowProfileDef>());
			}
			else
			{
				lProfile = convertIfcParameterizedProfileDef(*lItem.as<IfcCircleProfileDef>());
			}
		}
		else if (lItem.is(Type::IfcEllipseProfileDef))
		{
			lProfile = convertIfcParameterizedProfileDef(*lItem.as<IfcEllipseProfileDef>());
		}
		else if (lItem.is(Type::IfcIShapeProfileDef))
		{
			lProfile = convertIfcParameterizedProfileDef(*iItem.as<IfcIShapeProfileDef>());
		}
		else if (lItem.is(Type::IfcTShapeProfileDef))
		{
			lProfile = convertIfcParameterizedProfileDef(*iItem.as<IfcTShapeProfileDef>());
		}
		else if (lItem.is(Type::IfcCShapeProfileDef))
		{
			lProfile = convertIfcParameterizedProfileDef(*iItem.as<IfcCShapeProfileDef>());
		}
		else if (lItem.is(Type::IfcAsymmetricIShapeProfileDef))
		{
			lProfile = convertIfcParameterizedProfileDef(*iItem.as<IfcAsymmetricIShapeProfileDef>());
		}
		else if (lItem.is(Type::IfcLShapeProfileDef))
		{
			lProfile = convertIfcParameterizedProfileDef(*iItem.as<IfcLShapeProfileDef>());
		}
		else if (lItem.is(Type::IfcZShapeProfileDef))
		{
			lProfile = convertIfcParameterizedProfileDef(*iItem.as<IfcZShapeProfileDef>());
		}
		else if (lItem.is(Type::IfcUShapeProfileDef))
		{
			lProfile = convertIfcParameterizedProfileDef(*iItem.as<IfcUShapeProfileDef>());
		}
		else if (lItem.is(Type::IfcTrapeziumProfileDef))
		{
			lProfile = convertIfcParameterizedProfileDef(*iItem.as<IfcTrapeziumProfileDef>());
		}
		else
		{
			BOOST_LOG_TRIVIAL(error) << iIndent << "(#" << iItem.id() << "," <<  Type::ToString(iItem.type()) << ") IfcProfileDef not implemented ";
		}

		if (lItem.hasPosition())
		{
			if (lProfile)
			{
				lProfile->transform(convertIfcAxis2Placement2D(*lItem.Position()));
			}
		}
	}
	else if (iItem.is(Type::IfcCenterLineProfileDef))
	{
		lProfile = convertIfcCenterLineProfileDef(*iItem.as<IfcCenterLineProfileDef>());
	}
	else if (iItem.is(Type::IfcCompositeProfileDef))
	{
		IfcCompositeProfileDef& lItem = *iItem.as<IfcCompositeProfileDef>();
		IfcTemplatedEntityList<IfcProfileDef>::ptr lList = lItem.Profiles();

		std::vector<IfcProfileDef*>::const_iterator lIter = lList->begin();
		lProfile = convertIfcProfileDef(*(*lIter), "  ");
		for (std::vector<IfcProfileDef*>::const_iterator lIter = lList->begin(); lIter != lList->end(); lIter++)
		{
			lProfile->append(convertIfcProfileDef(*(*lIter), "  "));
		}
	}
	else if (iItem.is(Type::IfcDerivedProfileDef))
	{
		IfcDerivedProfileDef& lItem = *iItem.as<IfcDerivedProfileDef>();
		lProfile = convertIfcProfileDef(*lItem.ParentProfile(), "  ");
		lProfile->transform(convertIfcCartesianTransformationOperator2D(*lItem.Operator()));
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << iIndent << "(#" << iItem.id() << "," <<  Type::ToString(iItem.type()) << ") IfcProfileDef not implemented ";
	}

	return lProfile;
}


Profile* IfcParser::convertIfcParameterizedProfileDef(IfcRectangleProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lXdim = iItem.XDim();
	double lYdim = iItem.YDim();

	lProfile->mBounds.push_back(glm::dvec2(-lXdim/2.0, -lYdim/2.0));
	lProfile->mBounds.push_back(glm::dvec2( lXdim/2.0, -lYdim/2.0));
	lProfile->mBounds.push_back(glm::dvec2( lXdim/2.0,  lYdim/2.0));
	lProfile->mBounds.push_back(glm::dvec2(-lXdim/2.0,  lYdim/2.0));
	return lProfile;
};

void IfcParser::fillet(std::vector<glm::dvec2>& iList, uint8_t iQuad, double iX, double iY, double iR)
{
	switch (iQuad)
	{
		case 0:
			iX -= iR;
			iY -= iR;
			break;
		case 1:
			iX += iR;
			iY -= iR;
			break;
		case 2:
			iX += iR;
			iY += iR;
			break;
		case 3:
			iX -= iR;
			iY += iR;
			break;
	}

	for (int i=0; i<5; i++)
	{
		double lAngle = iQuad*M_PI_2+i*M_PI/8.0;
		iList.push_back(glm::dvec2(iX+iR*cos(lAngle), iY+iR*sin(lAngle)));
	}
}

Profile* IfcParser::convertIfcParameterizedProfileDef(IfcRectangleHollowProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lXdim = iItem.XDim();
	double lYdim = iItem.YDim();

	if (iItem.hasOuterFilletRadius())
	{
		double lRadius = iItem.OuterFilletRadius();

		fillet(lProfile->mBounds, 0, lXdim/2.0, lYdim/2.0, lRadius);
		fillet(lProfile->mBounds, 1,-lXdim/2.0, lYdim/2.0, lRadius);
		fillet(lProfile->mBounds, 2,-lXdim/2.0,-lYdim/2.0, lRadius);
		fillet(lProfile->mBounds, 3, lXdim/2.0,-lYdim/2.0, lRadius);
	}
	else
	{
		lProfile->mBounds.push_back(glm::dvec2(-lXdim/2.0, -lYdim/2.0));
		lProfile->mBounds.push_back(glm::dvec2( lXdim/2.0, -lYdim/2.0));
		lProfile->mBounds.push_back(glm::dvec2( lXdim/2.0,  lYdim/2.0));
		lProfile->mBounds.push_back(glm::dvec2(-lXdim/2.0,  lYdim/2.0));
	}

	lXdim -= iItem.WallThickness();
	lYdim -= iItem.WallThickness();
	
	std::vector<glm::dvec2> lHole;
	if (iItem.hasInnerFilletRadius())
	{
		double lRadius = iItem.InnerFilletRadius();

		fillet(lHole, 0, lXdim/2.0, lYdim/2.0, lRadius);
		fillet(lHole, 1,-lXdim/2.0, lYdim/2.0, lRadius);
		fillet(lHole, 2,-lXdim/2.0,-lYdim/2.0, lRadius);
		fillet(lHole, 3, lXdim/2.0,-lYdim/2.0, lRadius);
	}
	else
	{
		lHole.push_back(glm::dvec2(-lXdim/2.0, -lYdim/2.0));
		lHole.push_back(glm::dvec2( lXdim/2.0, -lYdim/2.0));
		lHole.push_back(glm::dvec2( lXdim/2.0,  lYdim/2.0));
		lHole.push_back(glm::dvec2(-lXdim/2.0,  lYdim/2.0));
	}
	lProfile->mHoles.push_back(lHole);
	return lProfile;
};

Profile* IfcParser::convertIfcParameterizedProfileDef(IfcRoundedRectangleProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lXdim = iItem.XDim();
	double lYdim = iItem.YDim();
	double lRadius = iItem.RoundingRadius();

	fillet(lProfile->mBounds, 0, lXdim/2.0, lYdim/2.0, lRadius);
	fillet(lProfile->mBounds, 1, -lXdim/2.0, lYdim/2.0, lRadius);
	fillet(lProfile->mBounds, 2, -lXdim/2.0,-lYdim/2.0, lRadius);
	fillet(lProfile->mBounds, 3, lXdim/2.0,-lYdim/2.0, lRadius);
	return lProfile;
};

Profile* IfcParser::convertIfcParameterizedProfileDef(IfcCircleProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	static double POINTS = 20;
	static double DR = 2.0*M_PI/POINTS;

	double lRadius = iItem.Radius();
	for (int i=0; i<POINTS; i++)
	{
		double lCos = cos(i*DR);
		double lSin = sin(i*DR);
		lProfile->mBounds.push_back(glm::dvec2(lCos*lRadius, lSin*lRadius));
	}
	return lProfile;
};

Profile* IfcParser::convertIfcParameterizedProfileDef(IfcCircleHollowProfileDef& iItem)
{
	static double POINTS = 20;
	static double DR = 2.0*M_PI/POINTS;

	Profile* lProfile = new Profile();
	lProfile->mHoles.resize(1);

	double lOuter = iItem.Radius();
	double lInner = lOuter - iItem.WallThickness();
	for (int i=0; i<POINTS; i++)
	{
		double lCos = cos(i*DR);
		double lSin = sin(i*DR);
		lProfile->mBounds.push_back(glm::dvec2(lCos*lOuter, lSin*lOuter));
		lProfile->mHoles[0].push_back(glm::dvec2(lCos*lInner, lSin*lInner));
	}
	return lProfile;
};


Profile* IfcParser::convertIfcParameterizedProfileDef(IfcEllipseProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	static double POINTS = 20;
	static double DR = 2.0*M_PI/POINTS;

	double lRx = iItem.SemiAxis1();
	double lRy = iItem.SemiAxis2();
	for (int i=0; i<POINTS; i++)
	{
		double lCos = cos(i*DR);
		double lSin = sin(i*DR);
		lProfile->mBounds.push_back(glm::dvec2(lCos*lRx, lSin*lRy));
	}
	return lProfile;
};

Profile* IfcParser::convertIfcParameterizedProfileDef(IfcTrapeziumProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	
	double lBottomXDim = iItem.BottomXDim();
	double lTopXDim = iItem.TopXDim();
	double lYDim = iItem.YDim();
	double lTopXOffset = iItem.TopXOffset();

	lProfile->mBounds.push_back(glm::dvec2(-lBottomXDim/2,-lYDim/2));
	lProfile->mBounds.push_back(glm::dvec2( lBottomXDim/2,-lYDim/2));
	lProfile->mBounds.push_back(glm::dvec2(-lBottomXDim/2+lTopXOffset+lTopXDim, lYDim/2));
	lProfile->mBounds.push_back(glm::dvec2(-lBottomXDim/2+lTopXOffset, lYDim/2));

	return lProfile;
}



Profile* IfcParser::convertIfcParameterizedProfileDef(IfcIShapeProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lOverallWidth = iItem.OverallWidth();
	double lOverallDepth = iItem.OverallDepth();
	double lWebThickness = iItem.WebThickness();
	double lFlangeThickness = iItem.FlangeThickness();

	/*
	if (iItem.hasFlangeEdgeRadius() || iItem.hasFilletRadius() || iItem.hasFlangeSlope())
	{
		BOOST_LOG_TRIVIAL(error) << "Implement me ... IfcIShapeProfileDef  ";
	}
	*/

	lProfile->mBounds.push_back(glm::dvec2(-lOverallWidth/2,-lOverallDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lOverallWidth/2,-lOverallDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lOverallWidth/2,-lOverallDepth/2+lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lWebThickness/2,-lOverallDepth/2+lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lWebThickness/2, lOverallDepth/2-lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lOverallWidth/2, lOverallDepth/2-lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lOverallWidth/2, lOverallDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lOverallWidth/2, lOverallDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lOverallWidth/2, lOverallDepth/2-lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWebThickness/2, lOverallDepth/2-lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWebThickness/2,-lOverallDepth/2+lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lOverallWidth/2,-lOverallDepth/2+lFlangeThickness));
	return lProfile;
}

Profile* IfcParser::convertIfcParameterizedProfileDef(IfcTShapeProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lDepth = iItem.Depth();
	double lFlangeWidth = iItem.FlangeWidth();
	double lWebThickness = iItem.WebThickness();
	double lFlangeThickness = iItem.FlangeThickness();

	if (iItem.hasFlangeEdgeRadius() || iItem.hasFilletRadius() || iItem.hasFlangeSlope())
	{
		BOOST_LOG_TRIVIAL(error) << "Implement me ... IfcIShapeProfileDef  ";
	}

	lProfile->mBounds.push_back(glm::dvec2(-lWebThickness/2,-lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lWebThickness/2,-lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lWebThickness/2, lDepth/2-lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lFlangeWidth/2, lDepth/2-lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lFlangeWidth/2, lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lFlangeWidth/2, lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lFlangeWidth/2, lDepth/2-lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWebThickness/2, lDepth/2-lFlangeThickness));
	return lProfile;
}


Profile* IfcParser::convertIfcParameterizedProfileDef(IfcZShapeProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lDepth = iItem.Depth();
	double lFlangeWidth = iItem.FlangeWidth();
	double lWebThickness = iItem.WebThickness();
	double lFlangeThickness = iItem.FlangeThickness();

	lProfile->mBounds.push_back(glm::dvec2( lFlangeWidth-lWebThickness/2,-lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lFlangeWidth-lWebThickness/2,-lDepth/2+lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lWebThickness/2,-lDepth/2+lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lWebThickness/2, lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lFlangeWidth+lWebThickness/2, lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lFlangeWidth+lWebThickness/2, lDepth/2-lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWebThickness/2, lDepth/2-lFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWebThickness/2,-lDepth/2));
	return lProfile;
}


Profile* IfcParser::convertIfcParameterizedProfileDef(IfcUShapeProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lDepth = iItem.Depth();
	double lFlangeWidth = iItem.FlangeWidth();
	double lWebThickness = iItem.WebThickness();
	double lFlangeThickness = iItem.FlangeThickness();

	lProfile->mBounds.push_back(glm::dvec2(-lFlangeWidth/2,-lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lFlangeWidth/2,-lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lFlangeWidth/2,-lDepth/2+lWebThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lFlangeWidth/2+lWebThickness,-lDepth/2+lWebThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lFlangeWidth/2+lWebThickness, lDepth/2-lWebThickness));
	lProfile->mBounds.push_back(glm::dvec2( lFlangeWidth/2, lDepth/2-lWebThickness));
	lProfile->mBounds.push_back(glm::dvec2( lFlangeWidth/2, lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lFlangeWidth/2, lDepth/2));
	return lProfile;
}

Profile* IfcParser::convertIfcParameterizedProfileDef(IfcAsymmetricIShapeProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lOverallDepth = iItem.OverallDepth();
	double lWebThickness = iItem.WebThickness();
	double lBottomFlangeWidth = iItem.BottomFlangeWidth();
	double lBottomFlangeThickness = iItem.BottomFlangeThickness();
	double lTopFlangeWidth = iItem.TopFlangeWidth();
	double lTopFlangeThickness = iItem.TopFlangeThickness();

	lProfile->mBounds.push_back(glm::dvec2(-lBottomFlangeWidth/2,-lOverallDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lBottomFlangeWidth/2,-lOverallDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lBottomFlangeWidth/2,-lOverallDepth/2+lBottomFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lWebThickness/2, -lOverallDepth/2+lBottomFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lWebThickness/2,  lOverallDepth/2-lTopFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lTopFlangeWidth/2, lOverallDepth/2-lTopFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2( lTopFlangeWidth/2, lOverallDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lTopFlangeWidth/2, lOverallDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lTopFlangeWidth/2, lOverallDepth/2-lTopFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWebThickness/2, lOverallDepth/2-lTopFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWebThickness/2,-lOverallDepth/2+lBottomFlangeThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lBottomFlangeWidth/2,-lOverallDepth/2+lBottomFlangeThickness));
	return lProfile;
}

Profile* IfcParser::convertIfcParameterizedProfileDef(IfcCShapeProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lGirth = iItem.Girth();
	double lWallThickness = iItem.WallThickness();
	double lDepth = iItem.Depth();
	double lWidth = iItem.Width();

	lProfile->mBounds.push_back(glm::dvec2( lWidth/2,-lDepth/2+lGirth));
	lProfile->mBounds.push_back(glm::dvec2( lWidth/2,-lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lWidth/2,-lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lWidth/2, lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lWidth/2, lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lWidth/2, lDepth/2-lGirth));
	lProfile->mBounds.push_back(glm::dvec2( lWidth/2-lWallThickness, lDepth/2-lGirth));
	lProfile->mBounds.push_back(glm::dvec2( lWidth/2-lWallThickness, lDepth/2-lWallThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWidth/2+lWallThickness, lDepth/2-lWallThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWidth/2+lWallThickness,-lDepth/2+lWallThickness));
	lProfile->mBounds.push_back(glm::dvec2( lWidth/2-lWallThickness,-lDepth/2+lWallThickness));
	lProfile->mBounds.push_back(glm::dvec2( lWidth/2-lWallThickness,-lDepth/2+lGirth));
	return lProfile;
}


Profile* IfcParser::convertIfcParameterizedProfileDef(IfcLShapeProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lDepth = iItem.Depth();
	double lWidth = iItem.hasWidth() ? iItem.Width() : lDepth;
	double lThickness = iItem.Thickness();

	lProfile->mBounds.push_back(glm::dvec2(-lWidth/2,-lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lWidth/2,-lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2( lWidth/2,-lDepth/2+lThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWidth/2+lThickness, -lDepth/2+lThickness));
	lProfile->mBounds.push_back(glm::dvec2(-lWidth/2+lThickness, lDepth/2));
	lProfile->mBounds.push_back(glm::dvec2(-lWidth/2, lDepth/2));
	return lProfile;
}

Profile* IfcParser::convertIfcCenterLineProfileDef(IfcCenterLineProfileDef& iItem)
{
	Profile* lProfile = new Profile();
	double lThickness = iItem.Thickness()/2;

	std::vector<glm::dvec3> lPath;
	convertIfcCurve(*iItem.Curve(), lPath); 

	std::vector<glm::dvec2> lPoints;
	for (size_t i=0; i<lPath.size(); i++)
	{
		lPoints.push_back(lPath[i]);
	}

	if (lPoints.front() == lPoints.back())
	{
		std::vector<glm::dvec2> lTangent;
		// closed loop
		uint32_t lSize = lPath.size();
		for (size_t i=0; i<lPath.size(); i++)
		{
			uint32_t p = (i-1+lSize)%lSize;
			uint32_t n = (i+1)%lSize;
			lTangent.push_back(glm::normalize((lPath[i]-lPath[p]) + (lPath[n]-lPath[p])));
		}

		for (size_t i=0; i<lPath.size(); i++)
		{
			glm::dvec2& p = lPoints[i];
			glm::dvec2& t = lTangent[i];

			p.x += t.y*lThickness;
			p.y += t.x*lThickness;
			lProfile->mBounds.push_back(p);
		}
	}
	else
	{
		std::vector<glm::dvec2> lTangent;
		// open loop
		lTangent.push_back(glm::normalize(lPath[1] - lPath[0]));
		for (size_t i=1; i<lPath.size()-1; i++)
		{
			lTangent.push_back(glm::normalize((lPath[i]-lPath[i-1]) + (lPath[i+1]-lPath[i-1])));
		}
		lTangent.push_back(glm::normalize(lPath[lPath.size()-1]-lPath[lPath.size()-2]));

		for (size_t i=0; i<lPath.size(); i++)
		{
			glm::dvec2 p = lPoints[i];
			glm::dvec2 t = lTangent[i];

			p.x += t.y*lThickness;
			p.y +=-t.x*lThickness;
			lProfile->mBounds.push_back(p); 
		}

		for (size_t i=lPath.size()-1; i>=0; i--)
		{
			glm::dvec2 p = lPoints[i];
			glm::dvec2 t = lTangent[i];

			p.x +=-t.y*lThickness;
			p.y += t.x*lThickness;
			lProfile->mBounds.push_back(p);
		}
	}
	return lProfile;
}

glm::dmat3& IfcParser::convertIfcAxis2Placement2D(IfcAxis2Placement2D& iPlacement)
{
	static glm::dmat3 sMatrix(1.0);

	glm::dmat3& lWorld = mFrames[mContext->id()];

	IfcCartesianPoint* lLocation = iPlacement.Location();
	glm::dvec3 lT = toVec3(lLocation->Coordinates());
	
	glm::vec2 lX(lWorld[0]);
	if (iPlacement.hasRefDirection())
	{
		IfcDirection* lDirection = iPlacement.RefDirection();
		
		glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
		lX.x = lVector[0];
		lX.y = lVector[1];
	}
	else 
	{
		BOOST_LOG_TRIVIAL(error)  << " NO IfcAxis2Placement2D.hasRefDirection()";
	}

	glm::vec2 lY(-lX.y, lX.x);

	// do transpose on the rotation for some reason? 
	sMatrix[0][0] = lX.x;
	sMatrix[0][1] = lX.y;
	sMatrix[2][0] = lT[0];

	sMatrix[1][0] = lY.x;
	sMatrix[1][1] = lY.y;
	sMatrix[2][1] = lT[1];

	return sMatrix;
}

glm::dmat4& IfcParser::convertIfcAxis2Placement3D(IfcAxis2Placement3D& iPlacement)
{
	static glm::dmat4 sMatrix(1.0);

	glm::dmat3& lWorld = mFrames[mContext->id()];

	IfcCartesianPoint* lLocation = iPlacement.Location();
	glm::dvec3 lT = toVec3(lLocation->Coordinates());

	glm::dvec3 lZ(lWorld[2]);
	if (iPlacement.hasAxis())
	{
		IfcDirection* lDirection = iPlacement.Axis();
		glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
		lZ = glm::dvec3(lVector[0], lVector[1], lVector[2]);
	}

	glm::dvec3 lRef(lWorld[0]);
	if (iPlacement.hasRefDirection())
	{
		IfcDirection* lDirection = iPlacement.RefDirection();
		glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
		lRef = glm::dvec3(lVector[0], lVector[1], lVector[2]);
	}

	glm::dvec3 lY = glm::cross(lZ, lRef);
	glm::dvec3 lX = glm::cross(lY, lZ);

	lX = glm::normalize(lX);
	lY = glm::normalize(lY);
	lZ = glm::normalize(lZ);
	
	// do transpose on the rotation for some reason? 
	sMatrix[0][0] = lX.x;
	sMatrix[0][1] = lX.y;
	sMatrix[0][2] = lX.z;
	sMatrix[3][0] = lT[0];

	sMatrix[1][0] = lY.x;
	sMatrix[1][1] = lY.y;
	sMatrix[1][2] = lY.z;
	sMatrix[3][1] = lT[1];

	sMatrix[2][0] = lZ.x;
	sMatrix[2][1] = lZ.y;
	sMatrix[2][2] = lZ.z;
	sMatrix[3][2] = lT[2];
	
	return sMatrix;
}

glm::dmat3& IfcParser::convertIfcCartesianTransformationOperator2D(IfcCartesianTransformationOperator2D& iPlacement)
{
	static glm::dmat3 sMatrix(1.0);
	
	glm::dmat3& lWorld = mFrames[mContext->id()];

	IfcCartesianPoint* lLocation = iPlacement.LocalOrigin();
	glm::dvec3 lT = toVec3(lLocation->Coordinates());

	glm::dvec2 lX(lWorld[0]);
	if (iPlacement.hasAxis1())
	{
		IfcDirection* lDirection = iPlacement.Axis1();
		glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
		lX = glm::dvec2(lVector[0], lVector[1]);
	}

	glm::dvec2 lY(lWorld[1]);
	if (iPlacement.hasAxis2())
	{
		IfcDirection* lDirection = iPlacement.Axis2();
		glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
		lY = glm::dvec2(lVector[0], lVector[1]);
	}


	lX = glm::normalize(lX);
	lY = glm::normalize(lY);
	
	sMatrix[0][0] = lX.x;
	sMatrix[0][1] = lX.y;
	sMatrix[2][0] = lT[0];

	sMatrix[1][0] = lY.x;
	sMatrix[1][1] = lY.y;
	sMatrix[2][1] = lT[1];

	glm::dmat3 lS(1.0);
	
	if (iPlacement.hasScale())
	{
		lS[0][0] = iPlacement.Scale();
		lS[1][1] = iPlacement.Scale();
	}

	if (iPlacement.is(Type::IfcCartesianTransformationOperator2DnonUniform))
	{
		IfcCartesianTransformationOperator3DnonUniform* lNonUniform = iPlacement.as<IfcCartesianTransformationOperator3DnonUniform>();
		if (lNonUniform->hasScale2())
		{
			lS[1][1] = lNonUniform->Scale2();
		}
	}
	
	sMatrix = sMatrix*lS;

	return sMatrix;
//	std::cout << "\n" << iMatrix << "\n";
}


glm::dmat4& IfcParser::convertIfcCartesianTransformationOperator3D(IfcCartesianTransformationOperator3D& iPlacement)
{
	static glm::dmat4 sMatrix(1.0);

	glm::dmat3& lWorld = mFrames[mContext->id()];

	IfcCartesianPoint* lLocation = iPlacement.LocalOrigin();
	glm::dvec3 lT = toVec3(lLocation->Coordinates());

	glm::dvec3 lX(lWorld[0]);
	if (iPlacement.hasAxis1())
	{
		IfcDirection* lDirection = iPlacement.Axis1();
		glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
		lX = glm::dvec3(lVector[0], lVector[1], lVector[2]);
	}

	glm::dvec3 lY(lWorld[1]);
	if (iPlacement.hasAxis2())
	{
		IfcDirection* lDirection = iPlacement.Axis2();
		glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
		lY = glm::dvec3(lVector[0], lVector[1], lVector[2]);
	}

	glm::dvec3 lZ(lWorld[2]);
	if (iPlacement.hasAxis3())
	{
		IfcDirection* lDirection = iPlacement.Axis3();
		glm::dvec3 lVector = toVec3(lDirection->DirectionRatios());
		lZ = glm::dvec3(lVector[0], lVector[1], lVector[2]);
	}

	lX = glm::normalize(lX);
	lY = glm::normalize(lY);
	lZ = glm::normalize(lZ);

	if (glm::dot(glm::cross(lX, lY), lZ) < 0)
	{
		lZ = -lZ;
	}

	sMatrix[0][0] = lX.x;
	sMatrix[0][1] = lX.y;
	sMatrix[0][2] = lX.z;
	sMatrix[3][0] = lT[0];

	sMatrix[1][0] = lY.x;
	sMatrix[1][1] = lY.y;
	sMatrix[1][2] = lY.z;
	sMatrix[3][1] = lT[1];

	sMatrix[2][0] = lZ.x;
	sMatrix[2][1] = lZ.y;
	sMatrix[2][2] = lZ.z;
	sMatrix[3][2] = lT[2];
	
	/*

	sMatrix[0][0] = lX.x;
	sMatrix[1][0] = lX.y;
	sMatrix[2][0] = lX.z;
	sMatrix[3][0] = lT[0];

	sMatrix[0][1] = lY.x;
	sMatrix[1][1] = lY.y;
	sMatrix[2][1] = lY.z;
	sMatrix[3][1] = lT[1];

	sMatrix[0][2] = lZ.x;
	sMatrix[1][2] = lZ.y;
	sMatrix[2][2] = lZ.z;
	sMatrix[3][2] = lT[2];
	*/
	glm::dvec3 lS(1.0);
	
	if (iPlacement.hasScale())
	{
		lS.x = iPlacement.Scale();
		lS.y = iPlacement.Scale();
		lS.z = iPlacement.Scale();
	}

	if (iPlacement.is(Type::IfcCartesianTransformationOperator3DnonUniform))
	{
		IfcCartesianTransformationOperator3DnonUniform* lNonUniform = iPlacement.as<IfcCartesianTransformationOperator3DnonUniform>();
		if (lNonUniform->hasScale2())
		{
			lS.y = lNonUniform->Scale2();
		}
		if (lNonUniform->hasScale3())
		{
			lS.z = lNonUniform->Scale3();
		}
	}
	
	sMatrix = glm::scale(sMatrix, glm::dvec3(lS));
	return sMatrix;
}







uint32_t IfcParser::fromLibrary(IfcElement& iElement)
{
	#define R(hex) (float)((hex >> 16) & 0xff) / 255.0
	#define G(hex) (float)((hex >> 8) & 0xff) / 255.0
	#define B(hex) (float)((hex >> 0) & 0xff) / 255.0

	#define MATERIAL(NAME,hex,a) if (iElement.is(Type::NAME))\
	{\
		if (mMaterialLib.find(#NAME) == mMaterialLib.end())\
		{\
			json_spirit::mObject lMaterial;\
			lMaterial["type"] = "IfcSurfaceStyle";\
			lMaterial["name"] = #NAME;\
			json_spirit::mObject lColor;\
			lColor["r"] = R(hex);\
			lColor["g"] = G(hex);\
			lColor["b"] = B(hex);\
			lColor["a"] = a;\
			lMaterial["surfaceColor"] = lColor;\
			mMaterials.push_back(lMaterial);\
			mMaterialLib[#NAME] = mMaterials.size()-1;\
		}\
		return mMaterialLib[#NAME];\
	} else\

	MATERIAL(IfcColumn,0xCCCDCD,1.0)
	MATERIAL(IfcSlab,0xE5F8FF,1.0)
	MATERIAL(IfcBeam,0xC0B097,1.0)
	MATERIAL(IfcFooting,0x939393,1.0)
	MATERIAL(IfcStair,0x7ABCB1,1.0)
	MATERIAL(IfcStairFlight,0x7ABCB1,1.0)
	MATERIAL(IfcWall,0xFFF0D7,1.0)
	MATERIAL(IfcWindow,0x5EB5DA,0.5)
	MATERIAL(IfcCovering,0xC7C7C7,1.0)
	MATERIAL(IfcCurtainWall,0x5EB5DA,0.5)
	MATERIAL(IfcDoor,0x6E3F27,1.0)
	MATERIAL(IfcPipeSegment,0x3F49FF,1.0)
	MATERIAL(IfcPipeFitting,0x0008FF,1.0)
	MATERIAL(IfcDuctSegment,0xFFA866,1.0)
	MATERIAL(IfcDuctFitting,0xFF7200,1.0)
	MATERIAL(IfcFurnishingElement,0x5E69A8,1.0)
	{
		return -1;
	}
}




	/*
	// colors
	IfcTemplatedEntityList<IfcIndexedColourMap>::ptr lColors = lGeometry->HasColours(); 
	if (lColors->size())
	{
		lNode->mColors = new uint32_t[lNode->mElementCount];
		IfcIndexedColourMap* lMap = *lColors->begin(); // only use one colormap
			
		uint8_t lOpacity = lMap->hasOpacity() ? lMap->Opacity()*255 : 255;
		std::vector<int> lIndex = lMap->ColourIndex();
		std::vector<std::vector<double>> lColors = lMap->Colours()->ColourList();
		for (int t=0; t<lIndex.size(); t++)
		{
			std::vector<double> lColor = lColors[lIndex[t]-1];

			uint8_t r = (uint8_t)(lColor[0]*255);
			uint8_t g = (uint8_t)(lColor[1]*255);
			uint8_t b = (uint8_t)(lColor[2]*255);
			lNode->mColors[t*3+0] = lNode->mColors[t*3+1] = lNode->mColors[t*3+2] = (r << 24) + (g << 16) + (b << 8) + lOpacity;
		}
	}

		
	IfcTemplatedEntityList<IfcIndexedTextureMap >::ptr lTextures = lGeometry->HasTextures(); // INVERSE IfcIndexedTextureMap::MappedTo
	if (lTextures->size())
	{
		IfcIndexedTextureMap* lMap = *lTextures->begin(); // only use one texture

		IfcIndexedTriangleTextureMap* lTextureMap;
	}
	*/

/*
	BOOST_LOG_TRIVIAL(info) << "************* Geometries *************";
	IfcTemplatedEntityList<IfcRepresentationContext>::ptr lGeometries = iProject.RepresentationContexts();
	for (std::vector<IfcRepresentationContext*>::const_iterator lIter = lGeometries->begin() ; lIter != lGeometries->end(); lIter++)
	{
		if ((*lIter)->is(Type::IfcGeometricRepresentationContext))
		{
			IfcGeometricRepresentationContext* lContext = (*lIter)->as<IfcGeometricRepresentationContext>();
			if (lContext->CoordinateSpaceDimension() == 3)
			{
				lContext3d = lContext;

				parseIfcRepresentation(lContext->RepresentationsInContext(), "");

				IfcTemplatedEntityList<IfcGeometricRepresentationSubContext>::ptr lSubContexts = lContext->HasSubContexts();
				for (std::vector<IfcGeometricRepresentationSubContext*>::const_iterator lIter = lSubContexts->begin() ; lIter != lSubContexts->end(); lIter++)
				{
					parseIfcGeometricRepresentationSubContext(*(*lIter), "  ");
				}

				IfcAxis2Placement* lPlacement = lContext->WorldCoordinateSystem();
				glm::dmat4 lMatrix = Shape::toMatrix(*lPlacement->as<IfcAxis2Placement3D>());
			}
		}
		else
		{
			BOOST_LOG_TRIVIAL(info) << "(#" << (*lIter)->id() << "," <<  Type::ToString((*lIter)->type()) << ") Unknown IfcRepresentationContext";
		}
	}
	*/
/*
void IfcParser::parseIfcGeometricRepresentationSubContext(IfcGeometricRepresentationSubContext& iItem, std::string iIndent)
{
	PRCOESSING(iItem, "");
	
	if (iItem.TargetView() == IfcGeometricProjectionEnum::IfcGeometricProjection_MODEL_VIEW)
	{
		parseIfcRepresentation(iItem.RepresentationsInContext(), iIndent);
	}

	IfcTemplatedEntityList<IfcGeometricRepresentationSubContext>::ptr lSubContexts = iItem.HasSubContexts();
	for (std::vector<IfcGeometricRepresentationSubContext*>::const_iterator lIter = lSubContexts->begin() ; lIter != lSubContexts->end(); lIter++)
	{
		parseIfcGeometricRepresentationSubContext(*(*lIter), iIndent + "  ");
	}
}

*/



/*
int32_t IfcParser::getMaterial(IfcElement& iElement, std::string iIndent)
{
	IfcMaterial* lMaterial = 0;

	IfcRelAssociates::list::ptr lAssociations = iElement.HasAssociations();
	for (std::vector<IfcRelAssociates*>::const_iterator lIter = lAssociations->begin() ; lIter != lAssociations->end(); lIter++)
	{
		if ((*lIter)->is(Type::IfcRelAssociatesMaterial))
		{
			IfcRelAssociatesMaterial* lAssociation = (*lIter)->as<IfcRelAssociatesMaterial>();
			IfcMaterialSelect* lSelection = lAssociation->RelatingMaterial();

			if (lSelection->is(Type::IfcMaterial))
			{
				ERROR((*lIter), "IfcMaterial");
			}
			else if (lSelection->is(Type::IfcMaterialLayerSetUsage))
			{
				IfcMaterialLayerSetUsage* lUsage = lSelection->as<IfcMaterialLayerSetUsage>();
				IfcMaterialLayerSet* lSet = lUsage->ForLayerSet();

				//IfcTemplatedEntityList<IfcMaterialLayer>::ptr lList = lSet->MaterialLayers();
				//for (std::vector<IfcMaterialLayer*>::const_iterator lIter2 = lList->begin(); lIter2 != lList->end(); lIter2++)
				{
					IfcMaterialLayer* lLayer = *lSet->MaterialLayers()->begin();
					if (lLayer->hasMaterial())
					{
						lMaterial = lLayer->Material();
					}
				}
			}
			else if (lSelection->is(Type::IfcMaterialProfileSetUsage))
			{
				IfcMaterialProfileSetUsage* lUsage = lSelection->as<IfcMaterialProfileSetUsage>();
				IfcMaterialProfileSet* lSet = lUsage->ForProfileSet();

				//IfcTemplatedEntityList<IfcMaterialProfile>::ptr lList = lSet->MaterialProfiles();
				//for (std::vector<IfcMaterialProfile*>::const_iterator lIter2 = lList->begin(); lIter2 != lList->end(); lIter2++)
				{
					IfcMaterialProfile* lLayer = *lSet->MaterialProfiles()->begin();
					if (lLayer->hasMaterial())
					{
						lMaterial = lLayer->Material();
					}
				}
			}
			else if (lSelection->is(Type::IfcMaterialList))
			{
				ERROR((*lIter), "IfcMaterialList");
			}
			else if (lSelection->is(Type::IfcMaterialLayerSet))
			{
				ERROR((*lIter), "IfcMaterialLayerSet");
			}
			else if (lSelection->is(Type::IfcMaterialLayer))
			{
				ERROR((*lIter), "IfcMaterialLayer");
			}
		}
		else
		{
			ERROR((*lIter), "");
		}
	}

	IfcSurfaceStyle* lStyle = 0;

	if (lMaterial)
	{
		PRCOESSING((*lMaterial), lMaterial->Name());
		if (mMaterialMap.find(lMaterial->id()) == mMaterialMap.end())
		{
			IfcMaterialDefinitionRepresentation* lRepresentation = first<IfcMaterialDefinitionRepresentation>(*lMaterial->HasRepresentation());
			if (lRepresentation)
			{
				IfcTemplatedEntityList<IfcRepresentation>::ptr lList = lRepresentation->Representations();
				for (std::vector<IfcRepresentation*>::const_iterator lIter = lList->begin(); lIter != lList->end(); lIter++)
				{
					IfcRepresentationContext* lContext = (*lIter)->ContextOfItems();
					if (lContext->hasContextType())
					{
						if (lContext->ContextType() == "Model")
						{
							lStyle = getSurfaceStyle(*(*lIter)->as<IfcStyledRepresentation>(), iIndent);
							if (lStyle)
							{
								break;
							}
						}
					}
					else
					{
						lStyle = getSurfaceStyle(*(*lIter)->as<IfcStyledRepresentation>(), iIndent);
						if (lStyle)
						{
							break;
						}
					}
				}
			}
		}
	}
	
	if (lStyle)
	{
		if (mMaterialMap.find(lStyle->id()) == mMaterialMap.end())
		{
			parseIfcSurfaceStyle(*lStyle, iIndent + "  ");
		}

		if (mMaterialMap.find(lStyle->id()) == mMaterialMap.end())
		{
			return mMaterialMap[lStyle->id()];
		}
	}
	else if (lMaterial)
	{
		for (int i=0; i<mMaterials.size(); i++)
		{
			json_spirit::mObject lObject = mMaterials[i].get_obj();
			if (lObject["name"].get_str() == lMaterial->Name())
			{
				return i;
			}
		}
	}

	return fromLibrary(iElement);
}



IfcSurfaceStyle* IfcParser::getSurfaceStyle(IfcStyledRepresentation& iRepresentation, std::string iIndent)
{
	IfcStyledItem* lStyledItem = find<IfcStyledItem, IfcRepresentationItem>(Type::IfcStyledItem, *iRepresentation.Items());

	IfcSurfaceStyle* lSurfaceStyle = find<IfcSurfaceStyle>(Type::IfcSurfaceStyle, *lStyledItem->Styles());
	if (!lSurfaceStyle)
	{
		// deprecated
		IfcPresentationStyleAssignment* lAssignment =  find<IfcPresentationStyleAssignment>(Type::IfcPresentationStyleAssignment, *lStyledItem->Styles());
		if (lAssignment)
		{
			lSurfaceStyle = find<IfcSurfaceStyle>(Type::IfcSurfaceStyle, *lAssignment->Styles());
		}
	}

	return lSurfaceStyle;
}


void IfcParser::parseIfcGeometricRepresentationSubContext(IfcGeometricRepresentationSubContext& iItem, std::string iIndent)
{
	BOOST_LOG_TRIVIAL(info) << iIndent <<  iItem.id() << " -- " << (iItem.hasContextType() ? iItem.ContextType() : "?") << " : " << (iItem.hasContextIdentifier() ? iItem.ContextIdentifier() : "?");
	switch (iItem.TargetView())
	{
		case IfcGeometricProjectionEnum::IfcGeometricProjection_MODEL_VIEW: BOOST_LOG_TRIVIAL(info) << iIndent << "MODEL_VIEW"; break;
		case IfcGeometricProjectionEnum::IfcGeometricProjection_PLAN_VIEW: BOOST_LOG_TRIVIAL(info) << iIndent << "PLAN_VIEW"; break;
		case IfcGeometricProjectionEnum::IfcGeometricProjection_SECTION_VIEW: BOOST_LOG_TRIVIAL(info) << iIndent << "SECTION_VIEW"; break;
		case IfcGeometricProjectionEnum::IfcGeometricProjection_REFLECTED_PLAN_VIEW: BOOST_LOG_TRIVIAL(info) << iIndent<< "REFLECTED_PLAN_VIEW"; break;
		case IfcGeometricProjectionEnum::IfcGeometricProjection_GRAPH_VIEW: BOOST_LOG_TRIVIAL(info) << iIndent << "GRAPH_VIEW"; break;
		case IfcGeometricProjectionEnum::IfcGeometricProjection_ELEVATION_VIEW: BOOST_LOG_TRIVIAL(info) << iIndent << "ELEVATION_VIEW"; break;
	}
	IfcTemplatedEntityList<IfcGeometricRepresentationSubContext>::ptr lSubContexts = iItem.HasSubContexts();
	for (std::vector<IfcGeometricRepresentationSubContext*>::const_iterator lIter = lSubContexts->begin() ; lIter != lSubContexts->end(); lIter++)
	{
		parseIfcGeometricRepresentationSubContext(*(*lIter), iIndent + "  ");
	}
}

*/
	/*
	IfcTemplatedEntityList<IfcRepresentationContext>::ptr lGeometries = iProject.RepresentationContexts();
	for (std::vector<IfcRepresentationContext*>::const_iterator lIter = lGeometries->begin() ; lIter != lGeometries->end(); lIter++)
	{
		if ((*lIter)->is(Type::IfcGeometricRepresentationContext))
		{
			IfcGeometricRepresentationContext* lContext = (*lIter)->as<IfcGeometricRepresentationContext>();
			if (lContext->CoordinateSpaceDimension() == 3)
			{
				BOOST_LOG_TRIVIAL(info) <<  lContext->id() << " -- " << (lContext->hasContextType() ? lContext->ContextType() : "?") << " : " << (lContext->hasContextIdentifier() ? lContext->ContextIdentifier() : "?");

				IfcTemplatedEntityList<IfcGeometricRepresentationSubContext>::ptr lSubContexts = lContext->HasSubContexts();
				for (std::vector<IfcGeometricRepresentationSubContext*>::const_iterator lIter = lSubContexts->begin() ; lIter != lSubContexts->end(); lIter++)
				{
					parseIfcGeometricRepresentationSubContext(*(*lIter), "  ");
				}

				IfcAxis2Placement* lPlacement = lContext->WorldCoordinateSystem();
				glm::dmat4 lMatrix = Shape::toMatrix(*lPlacement->as<IfcAxis2Placement3D>());
			}
		}
		else
		{
			BOOST_LOG_TRIVIAL(info) << "(#" << (*lIter)->id() << "," <<  Type::ToString((*lIter)->type()) << ") Unknown IfcRepresentationContext";
		}
	}
	*/