/*
 * outputVariables.h
 *
 *  Created on: Dec 22, 2011
 *      Author: dnic019
 */

#ifndef OUTPUTVARIABLES_H_
#define OUTPUTVARIABLES_H_

// The array the output variable is stored in
enum VariableCodeArray
{
	VOI_ARRAY,
	CONSTANT_ARRAY,
	ALGEBRAIC_ARRAY,
	STATE_ARRAY,
	UNKNOWN_ARRAY
};

void* outputVariablesCreate();

void* outputVariablesClone(void* src);

void outputVariablesDestroy(void* outputVariables);

void outputVariablesAppendVariable(void* outputVariables, const char* component, const char* variable, int column);

void outputVariablesPrint(void* outputVariables, FILE* f, const char* indent);

int outputVariablesGetLength(void *outputVariables);
int outputVariablesGetColumn(void* outputVariables, int index);
const char* outputVariablesGetVariable(void* outputVariables, int index);
const char* outputVariablesGetComponent(void* outputVariables, int index);

enum VariableCodeArray outputVariablesGetCodeArray(void* outputVariables, int index);
void outputVariablesSetCodeArray(void* outputVariables, int index, enum VariableCodeArray array);
int outputVariablesGetCodeIndex(void* outputVariables, int index);
void outputVariablesSetCodeIndex(void* outputVariables, int index, int codeIndex);

#endif /* OUTPUTVARIABLES_H_ */
