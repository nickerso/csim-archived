/*
 * CellmlSimulator.cpp
 *
 *  Created on: Aug 15, 2012
 *      Author: dnic019
 */
#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <cmath>

#include "CellmlSimulator.hpp"
#include "cellml-utils.hpp"
#include "cellml.hpp"
#include "CellmlCode.hpp"
#include "ModelCompiler.hpp"
#include "ExecutableModel.hpp"
#include "integrator.hpp"

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

static std::string formatOutputValues(const std::vector<double>& values)
{
	std::string valueString;
	char n[128];
	std::vector<double>::const_iterator i = values.begin();
	while (i != values.end())
	{
		sprintf(n, "%15.10e", *i);
		valueString += std::string(n);
		++i;
		if (i != values.end()) valueString += "\t";
	}
	return valueString;
}

CellmlSimulator::CellmlSimulator() :
	mModel(NULL), mSimulation(NULL), mCode(NULL), mExecutableModel(NULL),
	mBoundCache(NULL), mRatesCache(NULL), mStatesCache(NULL), mConstantsCache(NULL),
	mAlgebraicCache(NULL), mOutputsCache(NULL)
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
	if (mBoundCache) free(mBoundCache);
	if (mRatesCache) free(mRatesCache);
	if (mStatesCache) free(mStatesCache);
	if (mConstantsCache) free(mConstantsCache);
	if (mAlgebraicCache) free(mAlgebraicCache);
	if (mOutputsCache) free(mOutputsCache);
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

	void* list = createOutputVariablesForAllLocalComponents(mModel, mVariableIds);
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
	ModelCompiler compiler(/*executable name*/"CellmlSimulator", /*verbose*/false, /*debug*/false);
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

int CellmlSimulator::checkpointModelValues()
{
	if (!mExecutableModel)
	{
		std::cerr << "CellmlSimulator::checkpointModelValues(): Error, need to compile the model before "
				"checkpointing." << std::endl;
		return -1;
	}
	if (mBoundCache == NULL)
	{
		mBoundCache = (double*)malloc(sizeof(double)*mExecutableModel->nBound);
		mRatesCache = (double*)malloc(sizeof(double)*mExecutableModel->nRates);
		mStatesCache = (double*)malloc(sizeof(double)*mExecutableModel->nRates);
		mConstantsCache = (double*)malloc(sizeof(double)*mExecutableModel->nConstants);
		mAlgebraicCache = (double*)malloc(sizeof(double)*mExecutableModel->nAlgebraic);
		mOutputsCache = (double*)malloc(sizeof(double)*mExecutableModel->nOutputs);
	}
	memcpy(mBoundCache, mExecutableModel->bound, sizeof(double)*mExecutableModel->nBound);
	memcpy(mRatesCache, mExecutableModel->rates, sizeof(double)*mExecutableModel->nRates);
	memcpy(mStatesCache, mExecutableModel->states, sizeof(double)*mExecutableModel->nRates);
	memcpy(mConstantsCache, mExecutableModel->constants, sizeof(double)*mExecutableModel->nConstants);
	memcpy(mAlgebraicCache, mExecutableModel->algebraic, sizeof(double)*mExecutableModel->nAlgebraic);
	memcpy(mOutputsCache, mExecutableModel->outputs, sizeof(double)*mExecutableModel->nOutputs);
	return 0;
}

int CellmlSimulator::updateModelFromCheckpoint()
{
	if (!mExecutableModel || !mBoundCache)
	{
		std::cerr << "CellmlSimulator::updateModelFromCheckpoint: Error, need to compile the model and "
				"checkpoint the values before updating the model from the checkpoint." << std::endl;
		return -1;
	}
	memcpy(mExecutableModel->bound, mBoundCache, sizeof(double)*mExecutableModel->nBound);
	memcpy(mExecutableModel->rates, mRatesCache, sizeof(double)*mExecutableModel->nRates);
	memcpy(mExecutableModel->states, mStatesCache, sizeof(double)*mExecutableModel->nRates);
	memcpy(mExecutableModel->constants, mConstantsCache, sizeof(double)*mExecutableModel->nConstants);
	memcpy(mExecutableModel->algebraic, mAlgebraicCache, sizeof(double)*mExecutableModel->nAlgebraic);
	memcpy(mExecutableModel->outputs, mOutputsCache, sizeof(double)*mExecutableModel->nOutputs);
	return 0;
}

int CellmlSimulator::setVariableValue(const std::string& variableId, double value)
{
	if (!mExecutableModel)
	{
		std::cerr << "CellmlSimulator::setVariableValue: Error, need to compile the model before "
				"setting the value of any variable." << std::endl;
		return -1;
	}
	std::vector<std::string>::const_iterator iter = mVariableIds.begin();
	int variableIndex = -1, i = 0;
	while (iter != mVariableIds.end())
	{
		if (*iter == variableId)
		{
			variableIndex = i;
			break;
		}
		++i;
		++iter;
	}
	if (variableIndex > -1)
	{
		enum VariableCodeArray array = outputVariablesGetCodeArray(simulationGetOutputVariables(mSimulation),
				variableIndex);
		int index = outputVariablesGetCodeIndex(simulationGetOutputVariables(mSimulation),
				variableIndex);
		switch (array)
		{
		case VOI_ARRAY:
			mExecutableModel->bound[0] = value;
			break;
		case STATE_ARRAY:
			mExecutableModel->states[index] = value;
			break;
		case ALGEBRAIC_ARRAY:
			mExecutableModel->algebraic[index] = value;
			break;
		case CONSTANT_ARRAY:
			mExecutableModel->constants[index] = value;
			break;
		default:
			std::cerr << "CellmlSimulator::setVariableValue: Error finding the array for variable: "
				<< variableId.c_str() << std::endl;
			return -2;
		}
		return 0;
	}
	return -1;
}

std::vector<std::string> CellmlSimulator::getModelVariables()
{
	return std::vector<std::string>(mVariableIds);
}

std::vector<double> CellmlSimulator::getModelOutputs()
{
	std::vector<double> outputs;
	// get the current time?
	double voi = mExecutableModel->bound[0];
	// make sure the output array is current
	mExecutableModel->getOutputs(voi);
	// and populate the vector
	for (int i=0; i<mExecutableModel->nOutputs; ++i)
	{
		outputs.push_back(mExecutableModel->outputs[i]);
	}
	return outputs;
}

std::string CellmlSimulator::simulateModel(double initialTime, double startTime, double endTime,
		double numSteps)
{
	std::string results;
	if (mExecutableModel && mSimulation && simulationIsValidDescription(mSimulation))
	{
		struct Integrator* integrator = CreateIntegrator(mSimulation, mExecutableModel);
		if (integrator)
		{
			double t = initialTime;
			// FIXME: need error checking?
			// integrate till startTime if needed
			if (fabs(initialTime-startTime) > 1.0e-10)
			{
				integrate(integrator, startTime, &t);
			}
			// grab the initial outputs
			std::vector<double> outputs = getModelOutputs();
			results += formatOutputValues(outputs);
			results += "\n";
			// and now integrate from startTime to endTime
			double tabT = (endTime - startTime) / numSteps;
			double tout = startTime + tabT;
			if (tout > endTime)
				tout = endTime;
			while (1)
			{
				integrate(integrator, tout, &t);
				outputs = getModelOutputs();
				results += formatOutputValues(outputs);
				results += "\n";
				/* have we reached endTime? */
				if (fabs(endTime - t) < 1.0e-10)
					break;
				/* if not, increase tout */
				tout += tabT;
				/* and make sure we don't go past tEnd */
				if (tout > endTime)
					tout = endTime;
			}
			DestroyIntegrator(&integrator);
		}
		else
		{
			std::cerr << "CellmlSimulator::simulateModel: Error creating integrator." << std::endl;
		}
	}
	else
	{
		std::cerr << "CellmlSimulator::simulateModel: Error, invalid arguments." << std::endl;
	}

	return results;
}
