/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is CellMLSimulator.
 *
 * The Initial Developer of the Original Code is
 * David Nickerson <nickerso@users.sourceforge.net>.
 * Portions created by the Initial Developer are Copyright (C) 2007-2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
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
		code = OK;
	}
	return (code);
}
