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
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "version.h"
#include "common.h"
#include "utils.h"
#include "cellml.h"
#include "cellml_code_manager.h"
#include "simulation.h"
#include "timer.h"
#include "integrator_user_data.h"
#include "integrator.h"
#include "xpath.h"

/* Just for convenience */
#define PRE_EXIT_FREE                                         \
		if (inputURI) free(inputURI);                               \
		if (cCompiler) free(cCompiler);                             \
		if (simulation) DestroySimulation(&simulation);       \
		if (codeManager) DestroyCellMLCodeManager(&codeManager);    \
/* and for use when program is interrupted */
struct SignalHandlerData
{
	struct CellMLCodeManager* codeManager;
	void (*handler)(int);
};
static struct SignalHandlerData signalData;
/* make sure we tidy up created files */
void signalHandler(int s)
{
	if (signalData.codeManager)
		DestroyCellMLCodeManager(&(signalData.codeManager));
	ERROR("signalHandler", "Caught an interrupt signal. Exiting...\n");
	if (signalData.handler)
	{
		ERROR("signalHandler", "Passing through signal.\n");
		signalData.handler(s);
	}
	exit(1);
}

static int runSimulation(struct Simulation* simulation,
		struct CellMLCodeManager* codeManager);

static void printVersion()
{
	char* version = getVersion((char*) NULL, (char*) NULL);
	printf("%s\n", version);
	printf(
			"Copyright (C) 2007-2008 David Nickerson.\n"
					"This is free software; see the source for copying conditions. There \n"
					"is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A \n"
					"PARTICULAR PURPOSE.\n");
	free(version);
}

static void usage(char* prog)
{
	printf("Examines given input CellML and executes the\n"
			"simulation that is found, writing the simulation outputs to the terminal.\n\n");
	printf("Usage: %s [options] <input file>\n\n", prog);
	printf(
			"Available options:\n"
					"  --help\n\tDisplay help and exit.\n"
					"  --version\n\tDisplay version information and exit.\n"
					"  --quiet\n\tTurn off all printing to terminal except the simulation outputs.\n"
					"  --save-temp-files\n"
					"\tSave the temporary files generated from CellML.\n"
					"  --debug\n"
					"\tMainly for development, more occurrences more output.\n"
					"  --CC\n"
					"\tDefine the C compiler (default: 'gcc -fPIC -O3 -shared -x c -o')\n"
					"  --generate-debug-code\n"
					"\tGenerate code with debug bits included, useful for finding errors in "
					"models.\n"
					"\n");
}

static void help(char* prog)
{
	usage(prog);
	printf("\n\n");
	const char const * sundials_version = getSundialsVersion();
	printf("Useful links:\n"
			"http://cellml.sourceforge.net\n"
			"http://www.cellml.org\n"
			"http://www.cellml.org/specifications\n"
			"\n"
			"%s uses:\n"
			"- The CellML API (version 1.6)\n"
			"  http://www.cellml.org\n"
			"- The CVODES integrator from sundials (version %s)\n"
			"  http://www.llnl.gov/CASC/sundials/\n", prog, sundials_version);
	printf("\n");
	printf("Report bugs to nickerso@users.sourceforge.net\n");
}
/*
 *-------------------------------
 * Main Program
 *-------------------------------
 */
int main(int argc, char* argv[])
{
	char* inputURI = (char*) NULL;
	struct Simulation* simulation = (struct Simulation*) NULL;
	struct CellMLCodeManager* codeManager = (struct CellMLCodeManager*) NULL;
	signalData.codeManager = (struct CellMLCodeManager*) NULL;
	/* default values */
	char* cCompiler = strcopy("gcc -fPIC -O3 -shared -x c -o");

	/* Parse command line arguments */
	int c, invalidargs = 0;
	extern int PauseForCodeChanges;
	static int helpRequest = 0;
	static int versionRequest = 0;
	static int saveTempFiles = 0;
	static int generateDebugCode = 0;
	while (1)
	{
		static struct option long_options[] =
		{
		{ "help", no_argument, &helpRequest, 1 },
		{ "version", no_argument, &versionRequest, 1 },
		{ "save-temp-files", no_argument, &saveTempFiles, 1 },
		{ "generate-debug-code", no_argument, &generateDebugCode, 1 },
		{ "pause-for-changes", no_argument, &PauseForCodeChanges, 1 },
		{ "quiet", no_argument, NULL, 13 },
		{ "debug", no_argument, NULL, 14 },
		{ "CC", required_argument, NULL, 17 },
		{ 0, 0, 0, 0 } };
		int option_index;
		c = getopt_long(argc, argv, "", long_options, &option_index);
		/* check if we're finished the options */
		if (c == -1)
			break;
		/* check what option we got */
		switch (c)
		{
		case 0:
		{
			/* do nothing ?? */
			/*printf ("option %s", long_options[option_index].name);
			 if (optarg) printf (" with arg %s", optarg);
			 printf ("\n");*/
		}
			break;
		case 13:
		{
			/* quiet - to stop all non-error output */
			setQuiet();
		}
			break;
		case 14:
		{
			/* debug level, the more times the more debug output */
			setDebugLevel();
		}
			break;
		case 17:
		{
			if (cCompiler)
				free(cCompiler);
			cCompiler = strcopy(optarg);
		}
			break;
		case '?':
		{
			/* unknown option/missing argument found */
			invalidargs = 1;
		}
			break;
		default:
		{
			/* ?? */
			invalidargs = 1;
		}
			break;
		}
	}

	/* CellML model should be the only other entry on the command line */
	if (optind < argc)
		inputURI = getAbsoluteURI(argv[optind]);

	if (helpRequest)
	{
		help(argv[0]);
		PRE_EXIT_FREE;
		return (0);
	}
	else if (versionRequest)
	{
		printVersion();
		PRE_EXIT_FREE;
		return (0);
	}

	/* Check required arguments */
	if (inputURI == NULL)
	{
		ERROR("main", "Missing input file URI\n");
		invalidargs = 1;
	}
	if (invalidargs)
	{
		usage(argv[0]);
		PRE_EXIT_FREE;
		return (1);
	}

	/* Create the CellML Code Manager */
	codeManager = CreateCellMLCodeManager(saveTempFiles, cCompiler,
			generateDebugCode);
	signalData.codeManager = codeManager;

	/* set up the signal handler to ensure we clean up temporary files when
	 interrupt signal is received, and save any existing handler? */
	signalData.handler = signal(SIGINT, &signalHandler);

	/* no graphs, so just look for simulations */
	simulation = getSimulation(inputURI);

	int code = OK;
	if (simulation)
	{
		if (simulationIsValidDescription(simulation))
		{
			char* simulationName = simulationGetID(simulation);
			MESSAGE("Running the simulation: %s\n", simulationName);
			//simulationPrint(simulation, stdout, "###");
			DEBUG(0, "main", "Running the simulation: %s\n", simulationName);
			if (runSimulation(simulation, codeManager) == OK)
			{
				DEBUG(
						0,
						"main",
						"Ran the simulation (%s) successfully\n", simulationName);
			}
			else
			{
				ERROR("main",
						"Unable to run the simulation: %s\n", simulationName);
				code = ERR;
			}
			if (simulationName)
				free(simulationName);
		}
		else
		{
			ERROR("main", "Invalid simulation description\n");
			code = ERR;
		}
		DestroySimulation(&simulation);
	}
	else DEBUG(
			0,
			"main",
			"No simulations (or not exactly one simulation found), probably something missing\n");

	if (code == OK)
	{
	}
	else DEBUG(0, "main", "Something went wrong getting all the "
	"simulation results?\n");

	PRE_EXIT_FREE;
	return (0);
}

static int runSimulation(struct Simulation* simulation,
		struct CellMLCodeManager* codeManager)
{
	int code = ERR;
	if (simulation && simulationIsValidDescription(simulation))
	{
		struct IntegratorUserData* userData = CreateIntegratorUserDataForSimulation(
				codeManager, simulation);
		if (userData)
		{
			DEBUG(0, "runSimulation",
					"Got the user data for the model:\n");
			integratorUserDataInitialise(userData,
					simulationGetBvarStart(simulation));
			struct Integrator* integrator = CreateIntegrator(simulation,
					userData);
			if (integrator)
			{
				DEBUG(0, "runSimulation", "Initialised the simulation data\n");
				double tStart = simulationGetBvarStart(simulation);
				double tEnd = simulationGetBvarEnd(simulation);
				double tabT = simulationGetBvarTabStep(simulation);
				int iout = 0;
				double tout = tStart + tabT;
				if (tout > tEnd)
					tout = tEnd;
				struct Timer* timer = CreateTimer();
				startTimer(timer);
				double integrationTimes[3] =
				{ 0.0, 0.0, 0.0 };
				double dataStoreTimes[3] =
				{ 0.0, 0.0, 0.0 };
				int i;
				for (i = 0; i < userData->NO; i++)
					printf("\t%15.10e", userData->OUTPUTS[i]);
				printf("\n");
				while (1)
				{
					DEBUG(5, "runSimulation", "tout = "REAL_FORMAT"\n", tout);
					double t;
					iout++;
					TIME_FUNCTION_CALL(integrationTimes, intTimer, code,
							integrate, integrator, userData, tout, &t);
					if (code == OK)
					{
						int i;
						for (i = 0; i < userData->NO; i++)
							printf("\t%15.10e", userData->OUTPUTS[i]);
						printf("\n");
						if (code == OK)
						{
							/* have we reached tEnd? */
							if (fabs(tEnd - t) < ZERO_TOL)
								break;
							/* if not, increase tout */
							tout += tabT;
							/* and make sure we don't go past tEnd */
							if (tout > tEnd)
								tout = tEnd;
						}
						else
						{
							DEBUG(0, "runSimulation",
									"Error appending integration results "
									"at time: "REAL_FORMAT"\n", tout);
							break;
						}
					}
					else
					{
						DEBUG(
								0,
								"runSimulation",
								"Error integrating at time: "REAL_FORMAT"\n", tout);
						break;
					}
				}
				stopTimer(timer);
				double user = getUserTime(timer);
				double system = getSystemTime(timer);
				double total = user + system;
				double wall = getWallTime(timer);
				DestroyTimer(&timer);
				MESSAGE("  Wall clock time : "REAL_FORMAT" s\n", wall);
				MESSAGE(
						"    (integration "REAL_FORMAT" s)\n", integrationTimes[2]);
				MESSAGE(
						"    (data store  "REAL_FORMAT" s)\n", dataStoreTimes[2]);
				MESSAGE(
						"  CPU time        : "REAL_FORMAT" s "
						"(user "REAL_FORMAT"/system "REAL_FORMAT")\n", total, user, system);
				MESSAGE(
						"    (integration "REAL_FORMAT" s)\n", integrationTimes[0]+integrationTimes[1]);
				MESSAGE(
						"    (data store  "REAL_FORMAT" s)\n", dataStoreTimes[0]+dataStoreTimes[1]);
				if (!quietSet())
				{
					printMemoryStats();
					/* Print some final statistics from the integrator */
					PrintFinalStats(integrator);
				}
				if (code == OK)
				{
					DEBUG(2, "runSimulation", "Simulation complete\n");
					//simulationFlagResultsComplete(simulation,1);
				}
				else
				{
					DEBUG(0, "runSimulation", "Something went wrong with the "
					"integration so flagging partial results\n");
					//simulationFlagResultsComplete(simulation,0);
				}
				DestroyIntegrator(&integrator);
			}
			else
				ERROR("runSimulation", "Error creating integrator\n");
			DestroyIntegratorUserData(&userData);
		}
		else
			ERROR("runSimulation", "Error getting the user data for the "
			"model:\n");
	}
	else DEBUG(0, "runSimulation", "Invalid arguments\n");
	return (code);
}
