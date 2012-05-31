/*
 * CellmlCode.cpp
 *
 *  Created on: May 31, 2012
 *      Author: dnic019
 */
#include <string>
#include <unistd.h>
#include <cstdlib>
#include <iostream>

#include "CellmlCode.hpp"
extern "C"
{
#include "utils.h"
#include "simulation.h"
#include "cellml.h"
}

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
			annotateCellMLModelOutputs(cellmlModel, simulationGetOutputVariables(simulation));
			/* generate some code */
			char* code_string = getCellMLModelAsCCode(cellmlModel,
					simulationGetOutputVariables(simulation),
					generateDebugCode);
			if (code_string)
			{
				DEBUG(1,"CreateCode",
						"Successfully generated code from CellML model\n");
				/* We have code, so dump it out to a temporary file in a temporary
           	   	   directory so we can have the compiled object nice and handy to
           	   	   delete */
				char fileTemplate[64] = "tmp.cellml2code.XXXXXX";
				int tmpFileDes = mkstemp(fileTemplate);
				mCodeFileName = std::string(fileTemplate);
				mCodeFileExists = true;
				FILE* cFile = fdopen(tmpFileDes,"w");
				fprintf(cFile,"%s",code_string);
				fclose(cFile);
				free(code_string);
			}
			else
			{
				ERROR("CreateCode","Error generating code from the CellML model\n");
				returnCode = -1;
			}
			DestroyCellMLModel(&cellmlModel);
		}
	}
	else DEBUG(0,"CreateCode","Invalid arguments\n");
	return(returnCode);
}
