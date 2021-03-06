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

#ifdef __cplusplus
extern "C"
{
#endif
#include "outputVariables.h"
#ifdef __cplusplus
}
#endif

class OutputVariable
{
public:
	OutputVariable()
	{
		column = -1;
		codeArray = UNKNOWN_ARRAY;
		codeIndex = -1;
	}
	// the name of the component in the top level model to output
	std::string component;
	// the name of the variable in that component to output
	std::string variable;
	// the column to store this variable in the output data
	int column;
	// the array for this variable in the generated code
	enum VariableCodeArray codeArray;
	// the index of this variable in the codeArray
	int codeIndex;
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

enum VariableCodeArray outputVariablesGetCodeArray(void* outputVariables, int index)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	return (*list)[index].codeArray;
}

void outputVariablesSetCodeArray(void* outputVariables, int index, enum VariableCodeArray array)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	(*list)[index].codeArray = array;
}

int outputVariablesGetCodeIndex(void* outputVariables, int index)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	return (*list)[index].codeIndex;
}

void outputVariablesSetCodeIndex(void* outputVariables, int index, int codeIndex)
{
	OutputVariables* list = static_cast<OutputVariables*>(outputVariables);
	(*list)[index].codeIndex = codeIndex;
}
