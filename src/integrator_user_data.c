
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "utils.h"
#include "integrator_user_data.h"
#include "cellml_methods.h"
#include "cellml_code_manager.h"
#include "simulation.h"

struct IntegratorUserData* CreateIntegratorUserDataForSimulation(
		struct CellMLCodeManager* codeManager, struct Simulation* simulation)
{
	struct IntegratorUserData* ud = (struct IntegratorUserData*) NULL;
	if (codeManager && simulation)
	{
		ud = (struct IntegratorUserData*) malloc(
				sizeof(struct IntegratorUserData));
		if (ud)
		{
			/* initialise */
			ud->methods = (struct CellMLMethods*) NULL;
			ud->BOUND = (double*) NULL;
			ud->CONSTANTS = (double*) NULL;
			ud->STATES = (double*) NULL;
			ud->ALGEBRAIC = (double*) NULL;
			ud->RATES = (double*) NULL;
			ud->OUTPUTS = (double*) NULL;
			/* try to load the cellml model methods */
			if ((ud->methods = cellmlCodeManagerGetMethodsForSimulation(codeManager,
					simulation)))
			{
				/* allocate memory */
				ud->NB = ud->methods->getNbound();
				ud->BOUND = (double*) calloc(ud->NB, sizeof(double));
				ud->NC = ud->methods->getNconstants();
				ud->CONSTANTS = (double*) calloc(ud->NC, sizeof(double));
				ud->NR = ud->methods->getNrates();
				ud->RATES = (double*) calloc(ud->NR, sizeof(double));
				ud->STATES = (double*) calloc(ud->NR, sizeof(double));
				ud->NA = ud->methods->getNalgebraic();
				ud->ALGEBRAIC = (double*) calloc(ud->NA, sizeof(double));
				ud->NO = ud->methods->getNoutputs();
				ud->OUTPUTS = (double*) calloc(ud->NO, sizeof(double));
			}
			else
			{
				ERROR("CreateIntegratorUserDataForModel",
						"Error loading the CellML model methods.\n");
				free(ud);
				ud = (struct IntegratorUserData*) NULL;
			}
		}
	}
	else DEBUG(0, "CreateIntegratorUserDataForSimulation", "Invalid argument(s)\n");
	return (ud);
}

#if defined (OLD_CODE)
struct IntegratorUserData* CreateIntegratorUserData(const char* soFileName)
{
	struct IntegratorUserData* ud =
			(struct IntegratorUserData*)malloc(sizeof(struct IntegratorUserData));
	if (ud)
	{
		/* initialise */
		ud->methods = (struct CellMLMethods*)NULL;
		ud->BOUND = (double*)NULL;
		ud->CONSTANTS = (double*)NULL;
		ud->VARIABLES = (double*)NULL;
		ud->RATES = (double*)NULL;
		/* try to load the cellml model methods */
		if ((ud->methods = CreateCellMLMethods(soFileName)))
		{
			/* allocate memory */
			ud->NB = ud->methods->getNbound();
			ud->BOUND = (double*)calloc(ud->NB,sizeof(double));
			ud->NC = ud->methods->getNconstants();
			ud->CONSTANTS = (double*)calloc(ud->NC,sizeof(double));
			ud->NY = ud->methods->getNvariables();
			ud->VARIABLES = (double*)calloc(ud->NY,sizeof(double));
			ud->N = ud->methods->getNrates();
			ud->RATES = (double*)calloc(ud->N,sizeof(double));
		}
		else
		{
			fprintf(stderr,"Error loading the CellML model methods.\n");
			free(ud);
			ud = (struct IntegratorUserData*)NULL;
		}
	}
	return(ud);
}
#endif /* defined (OLD_CODE) */

int DestroyIntegratorUserData(struct IntegratorUserData** ud_ptr)
{
	int code = ERR;
	struct IntegratorUserData* ud = *ud_ptr;
	if (ud)
	{
		if (ud->methods)
			DestroyCellMLMethods(&(ud->methods));
		if (ud->BOUND)
			free(ud->BOUND);
		if (ud->CONSTANTS)
			free(ud->CONSTANTS);
		if (ud->STATES)
			free(ud->STATES);
		if (ud->ALGEBRAIC)
			free(ud->ALGEBRAIC);
		if (ud->RATES)
			free(ud->RATES);
		if (ud->OUTPUTS)
			free(ud->OUTPUTS);
		free(ud);
		*ud_ptr = (struct IntegratorUserData*) NULL;
		code = OK;
	}
	return (code);
}

int integratorUserDataInitialise(struct IntegratorUserData *ud, double value)
{
	int code = ERR;
	if (ud && ud->methods)
	{
		ud->BOUND[0] = value;
		/* initialise the values in the arrays */
		ud->methods->SetupFixedConstants(ud->CONSTANTS, ud->RATES, ud->STATES);
		ud->methods->ComputeRates(*(ud->BOUND), ud->STATES, ud->RATES,
				ud->CONSTANTS, ud->ALGEBRAIC);
		ud->methods->EvaluateVariables(*(ud->BOUND), ud->CONSTANTS, ud->RATES,
				ud->STATES, ud->ALGEBRAIC);
		ud->methods->GetOutputs(*(ud->BOUND), ud->CONSTANTS, ud->STATES, ud->ALGEBRAIC, ud->OUTPUTS);
		code = OK;
	}
	return (code);
}
