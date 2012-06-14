/*
 * outputVariables.cpp
 *
 *  Created on: Dec 23, 2011
 *      Author: dnic019
 */
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

#ifndef _MSC_VER
extern "C"
{
#endif
#include "outputVariables.h"
#ifndef _MSC_VER
}
#endif

enum VariableType
{
	UNKNOWN_VARIABLE_TYPE = 0,
	BOUND_VARIABLE,
	STATE_VARIABLE,
	ALGEBRAIC_VARIABLE
};

class OutputVariable
{
public:
	OutputVariable()
	{
		column = -1;
		type = UNKNOWN_VARIABLE_TYPE;
		arrayIndex = -1;
	}
	// the name of the component in the top level model to output
	std::string component;
	// the name of the variable in that component to output
	std::string variable;
	// the column to store this variable in the output data
	int column;
	// the type array the variable is in
	enum VariableType type;
	// the index of the variable in the simulation data array for that type
	int arrayIndex;
};

typedef std::vector<OutputVariable> OutputVariables;

void* outputVariablesCreate()
{
	OutputVariables* list = new OutputVariables();
	return static_cast<void*>(list);
}

void* outputVariablesClone(void* src)
{
	OutputVariables* list = static_cast<OutputVariables*>(src);
	OutputVariables* newList = new OutputVariables(*list);
	return static_cast<void*>(newList);
}

void outputVariablesDestroy(void* outputVariables)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	delete list;
}

void outputVariablesAppendVariable(void* outputVariables, const char* component, const char* variable, int column)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	OutputVariable ov;
	ov.component = std::string(component);
	ov.variable = std::string(variable);
	ov.column = column;
	list->push_back(ov);
}

void outputVariablesPrint(void* outputVariables, FILE* f, const char* indent)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	OutputVariables::const_iterator iter = list->begin();
	fprintf(f, "%s  OutputVariables:\n", indent);
	while (iter < list->end())
	{
		fprintf(f, "%s    %d) %s/%s\n", indent, iter->column, iter->component.c_str(), iter->variable.c_str());
		++iter;
	}
}

int outputVariablesGetLength(void *outputVariables)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	return list->size();
}

const char* outputVariablesGetVariable(void* outputVariables, int index)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	return (*list)[index].variable.c_str();
}

const char* outputVariablesGetComponent(void* outputVariables, int index)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	return (*list)[index].component.c_str();
}

int outputVariablesGetColumn(void* outputVariables, int index)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	return (*list)[index].column;
}
