/*
 * outputVariables.h
 *
 *  Created on: Dec 22, 2011
 *      Author: dnic019
 */

#ifndef OUTPUTVARIABLES_H_
#define OUTPUTVARIABLES_H_

void* outputVariablesCreate();

void* outputVariablesClone(void* src);

void outputVariablesDestroy(void* outputVariables);

void outputVariablesAppendVariable(void* outputVariables, const char* component, const char* variable, int column);

void outputVariablesPrint(void* outputVariables, FILE* f, const char* indent);

int outputVariablesGetLength(void *outputVariables);
int outputVariablesGetColumn(void* outputVariables, int index);
const char* outputVariablesGetVariable(void* outputVariables, int index);
const char* outputVariablesGetComponent(void* outputVariables, int index);

#endif /* OUTPUTVARIABLES_H_ */
