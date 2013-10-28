/*
 * CellmlCode.cpp
 *
 *  Created on: May 31, 2012
 *      Author: dnic019
 */
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <iostream>

#ifndef _MSC_VER
#  include <unistd.h>
#endif

#ifdef _MSC_VER
#  include <io.h>
#  include <fcntl.h>
#  include <windows.h>
#  define unlink _unlink
#  define fdopen _fdopen
#  define mkstemp win32_mkstemp
#endif

#include "CellmlCode.hpp"
#ifdef __cplusplus
extern "C"
{
#endif
#include "utils.h"
#include "simulation.h"
#include "cellml.h"
#ifdef __cplusplus
}
#endif

// from http://gitorious.org/git-win32/mainline/blobs/8cfc8e4414bbb568075a948562ebb357cb84b6c3/win32/mkstemp.c
#ifdef _MSC_VER
static int win32_mkstemp(char * t)
{
	DWORD pathSize;
	char pathBuffer[1000];
	char tempFilename[MAX_PATH];
	UINT uniqueNum;
	pathSize = GetTempPath( 1000, pathBuffer);
	if (pathSize < 1000)
		pathBuffer[pathSize] = 0;
	else
		pathBuffer[0] = 0;

	uniqueNum = GetTempFileName(pathBuffer, "tmp", FILE_FLAG_DELETE_ON_CLOSE , tempFilename);
	strcpy(t, tempFilename);
	return _open(tempFilename, _O_RDWR|_O_BINARY);
}
#endif

CellmlCode::CellmlCode() : mSaveGeneratedCode(false)
{
}

CellmlCode::CellmlCode(bool save) : mSaveGeneratedCode(save)
{
}

CellmlCode::~CellmlCode()
{
	if (mSaveGeneratedCode)
		std::cout << "Leaving generated code for model: " << mModelUri.c_str() << std::endl;
	if (mCodeFileExists)
	{
		if (!mSaveGeneratedCode) unlink(mCodeFileName.c_str());
		else std::cout << "  leaving generated code file: " << mCodeFileName.c_str() << std::endl;
	}
}

int CellmlCode::createCodeForSimulation(struct CellMLModel* model, struct Simulation* simulation,
		bool generateDebugCode)
{
	int returnCode = 0;
	if (model && simulation)
	{
		mCodeFileExists = false;
		annotateCellMLModelOutputs(model,
				simulationGetOutputVariables(simulation));
		/* generate some code */
		char* code_string = getCellMLModelAsCCode(model,
				simulationGetOutputVariables(simulation), generateDebugCode);
		if (code_string)
		{
			DEBUG(1, "CellmlCode::createCodeForSimulation(model,simulation)",
					"Successfully generated code from CellML model\n");
			/* We have code, so dump it out to a temporary file in a temporary
			 directory so we can have the compiled object nice and handy to
			 delete */
			char fileTemplate[64] = "tmp.cellml2code.XXXXXX";
			int tmpFileDes = mkstemp(fileTemplate);
			mCodeFileName = std::string(fileTemplate);
			mCodeFileExists = true;
			FILE* cFile = fdopen(tmpFileDes, "w");
			fprintf(cFile, "%s", code_string);
			fclose(cFile);
			free(code_string);
		}
		else
		{
			ERROR("CellmlCode::createCodeForSimulation(model,simulation)",
					"Error generating code from the CellML model\n");
			returnCode = -1;
		}
	}
	else DEBUG(0, "CellmlCode::createCodeForSimulation(model,simulation)",
			"Invalid arguments\n");
	return (returnCode);
}

int CellmlCode::createCodeForSimulation(struct Simulation* simulation, bool generateDebugCode)
{
	int returnCode = 0;
	char* modelUri = simulationGetModelURI(simulation);
	if (modelUri)
	{
		mCodeFileExists = false;
		/* parse the model */
		struct CellMLModel* cellmlModel = CreateCellMLModel(modelUri);
		if (cellmlModel)
		{
			createCodeForSimulation(cellmlModel, simulation, generateDebugCode);
			DestroyCellMLModel(&cellmlModel);
		}
		else
		{
			ERROR("CellmlCode::createCodeForSimulation(simulation)","Unable to parse model.\n");
			returnCode = -1;
		}
	}
	else DEBUG(0,"CellmlCode::createCodeForSimulation(simulation)","Invalid arguments\n");
	return(returnCode);
}
