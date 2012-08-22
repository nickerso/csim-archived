/*
 * CellmlSimulator.cpp
 *
 *  Created on: Aug 15, 2012
 *      Author: dnic019
 */
#include <iostream>
#include <string>

#include "CellmlSimulator.hpp"
#include "cellml-utils.hpp"
#include "CellmlCode.hpp"
#include "ModelCompiler.hpp"
#include "ExecutableModel.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
#include "cellml.h"
#include "simulation.h"
#include "outputVariables.h"
#ifdef __cplusplus
}
#endif

CellmlSimulator::CellmlSimulator() :
	mModel(NULL), mSimulation(NULL), mCode(NULL), mExecutableModel(NULL)
{
	std::cout << "Creating cellml simulator." << std::endl;
}

CellmlSimulator::~CellmlSimulator()
{
	std::cout << "Destroying cellml simulator for model url: " << mUrl.c_str() << std::endl;
	if (mModel) DestroyCellMLModel(&mModel);
	if (mSimulation) DestroySimulation(&mSimulation);
	if (mCode) delete mCode;
	if (mExecutableModel) delete mExecutableModel;
}

std::string CellmlSimulator::serialiseCellmlFromUrl(const std::string& url)
{
	std::cout << "Serialising the model from the URL: " << url.c_str() << std::endl;
	std::string modelString = modelUrlToString(url);
	return modelString;
}

int CellmlSimulator::loadModelString(const std::string& modelString)
{
	mModel = CreateCellMLModelFromString(modelString.c_str());
	if (mModel)
	{
		char* url = getCellMLModelURI(mModel);
		mUrl = std::string(url);
		free(url);
		return 0;
	}
	return -1;
}

int CellmlSimulator::createSimulationDefinition()
{
	if (!mModel)
	{
		std::cerr << "CellmlSimulator::createSimulationDefinition: Error, need "
				"a model before creating a simulation" << std::endl;
		return -1;
	}
	mSimulation = CreateSimulation();
	// these are not yet used, but need to be set.
	simulationSetURI(mSimulation, mUrl.c_str());
	simulationSetModelURI(mSimulation, mUrl.c_str());
	simulationSetBvarURI(mSimulation, "bob");

	// Set some reasonable defaults for things?
	simulationSetID(mSimulation, "CellmlSimulator");
	simulationSetBvarStart(mSimulation, 0.0);
	simulationSetBvarEnd(mSimulation, 1.0);
	simulationSetBvarMaxStep(mSimulation, 0.01);
	simulationSetBvarTabStep(mSimulation, 0.1);

	void* list = createOutputVariablesForAllLocalComponents(mModel);
	if (list)
	{
		simulationSetOutputVariables(mSimulation, list);
		outputVariablesDestroy(list);
	}
	else
	{
		std::cerr << "CellmlSimulator::createSimulationDefinition: Missing simulation output variables."
				<< std::endl;
		DestroySimulation(&mSimulation);
		mSimulation = NULL;
		return -2;
	}
	return 0;
}

int CellmlSimulator::compileModel()
{
	int returnCode;
	if (!mModel || !mSimulation)
	{
		std::cerr << "CellmlSimulator::compileModel: Error, need a model and simulation definition before "
				"it can be compiled." << std::endl;
		return -1;
	}

	mCode = new CellmlCode();
	returnCode = mCode->createCodeForSimulation(mModel, mSimulation, /*generateDebugCode*/false);
	if (returnCode != 0)
	{
		std::cerr << "CellmlSimulator::compileModel: Error, unable to generate code to compile."
				<< std::endl;
		return -2;
	}

	// create the LLVM/Clang model compiler
	ModelCompiler compiler(/*executable name*/"CellmlSimulator", /*verbose*/true, /*debug*/false);
	// and the executable model
	mExecutableModel = new ExecutableModel();
	if (mExecutableModel->initialise(&compiler, mCode->codeFileName(), simulationGetBvarStart(mSimulation)) != 0)
	{
		std::cerr << "CellmlSimulator::compileModel: Unable to create the executable model from '"
				<< mCode->codeFileName() << "'" << std::endl;
		return -3;
	}

	return 0;
}
