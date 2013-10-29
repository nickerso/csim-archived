
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#ifndef _MSC_VER
#  include <getopt.h>
#endif
#include <time.h>
#include <signal.h>
#include <string>
#include <iostream>

#ifdef __cplusplus
extern "C"
{
#endif
#include "version.h"
#include "common.h"
#include "utils.h"
#include "cellml.h"
#include "simulation.h"
#include "timer.h"
#ifdef __cplusplus
}
#endif

#include "xpath.hpp"
#include "integrator.hpp"
#include "CellmlCode.hpp"
#include "ModelCompiler.hpp"
#include "ExecutableModel.hpp"

/* Just for convenience */
#define PRE_EXIT_FREE                                         \
		if (inputURI) free(inputURI);                         \
		if (simulation) DestroySimulation(&simulation);       \
		if (cellmlCode) delete cellmlCode;
/* and for use when program is interrupted */
struct SignalHandlerData
{
	CellmlCode* code;
	void (*handler)(int);
};
static struct SignalHandlerData signalData;
/* make sure we tidy up created files */
void signalHandler(int s)
{
	if (signalData.code)
		delete signalData.code;
	ERROR("signalHandler", "Caught an interrupt signal. Exiting...\n");
	if (signalData.handler)
	{
		ERROR("signalHandler", "Passing through signal.\n");
		signalData.handler(s);
	}
	exit(1);
}

static int runSimulation(struct Simulation* simulation, ExecutableModel* em);

static void printVersion()
{
	char* version = getVersion((char*) NULL, (char*) NULL);
	printf("%s\n", version);
	printf(
			"Copyright (C) 2007-2012 David Nickerson.\n"
					"This is free software; see the source for copying conditions. There \n"
					"is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A \n"
					"PARTICULAR PURPOSE.\n");
	free(version);
}

static void usage(char* prog)
{
	printf("Examines given input CellML and executes the\n"
			"simulation that is found, writing the simulation outputs to the terminal.\n\n");
#ifdef _MSC_VER
	printf("Usage: %s <input file> [XXX]\n\n", prog);
	printf("No options for windows version yet, but put random characters after the <input file>\n"
			"to get some help.\n\n");
#else
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
					"  --generate-debug-code\n"
					"\tGenerate code with debug bits included, useful for finding errors in "
					"models.\n"
					"\n");
#endif // _MSC_VER
}

static void help(char* prog)
{
	usage(prog);
	printf("\n\n");
	const char * sundials_version = getSundialsVersion();
	printf("Useful links:\n"
			"http://cellml.sourceforge.net\n"
			"http://code.google.com/p/cellml-simulator/\n"
			"http://www.cellml.org\n"
			"http://www.cellml.org/specifications\n"
			"\n"
			"%s uses:\n"
			"- The CellML API (version 1.12)\n"
			"  http://www.cellml.org\n"
			"- LLVM/Clang (version 3.1)\n"  // FIXME: need to pull in verson better?
			"  http://www.llvm.org\n"
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
	CellmlCode* cellmlCode = NULL;
	signalData.code = NULL;

	/* Parse command line arguments */
	int invalidargs = 0;
	static int helpRequest = 0;
	static int versionRequest = 0;
	static int saveTempFiles = 0;
	static int generateDebugCode = 0;
#ifdef _MSC_VER
	// no standard getopt_long for windows, so default some decent options
	setQuiet();
	int optind;
	if (argc == 2)
	{
		optind = 1;
	}
	else if (argc == 3)
	{
		printVersion();
		helpRequest = 1;
	}
	else
	{
		invalidargs = 1;
		optind = argc + 1;
	}
#else
	while (1)
	{
		static struct option long_options[] =
		{
		{ "help", no_argument, &helpRequest, 1 },
		{ "version", no_argument, &versionRequest, 1 },
		{ "save-temp-files", no_argument, &saveTempFiles, 1 },
		{ "generate-debug-code", no_argument, &generateDebugCode, 1 },
		{ "quiet", no_argument, NULL, 13 },
		{ "debug", no_argument, NULL, 14 },
		{ 0, 0, 0, 0 } };
		int option_index;
		int c = getopt_long(argc, argv, "", long_options, &option_index);
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
#endif

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

	/* CellML model should be the only other entry on the command line */
	if (optind < argc)
		inputURI = getAbsoluteURI(argv[optind]);

	if (debugLevel() > 98)
	{
		if (inputURI)
		{
			DEBUG(99, "main", "inputURI found and is: %s\n", inputURI);
		}
		else
		{
			DEBUG(99, "main", "inputURI not found\n");
		}
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

	/* Create the CellML Code */
	cellmlCode = new CellmlCode(saveTempFiles == 1);
	signalData.code = cellmlCode;

	/* set up the signal handler to ensure we clean up temporary files when
	 interrupt signal is received, and save any existing handler? */
	signalData.handler = signal(SIGINT, &signalHandler);

	DEBUG(99, "main", "About to try and get the simulation from: %s\n", inputURI);

	/* look for a single simulation */
	simulation = getSimulation(inputURI);

	DEBUG(99, "main", "Got the simulation from: %s\n", inputURI);

	int code = OK;
	if (simulation)
	{
		if (simulationIsValidDescription(simulation))
		{
			// create the code from the cellml model
			cellmlCode->createCodeForSimulation(simulation, generateDebugCode == 1);
			// create the LLVM/Clang model compiler
			ModelCompiler mc(argv[0], quietSet() == 0, generateDebugCode == 1);
			// and the executable model
			ExecutableModel em;
			if (em.initialise(&mc, cellmlCode->codeFileName(), simulationGetBvarStart(simulation)) != 0)
			{
				ERROR("main", "Unable to create the executable model from '%s'\n",
						cellmlCode->codeFileName());
				PRE_EXIT_FREE;
				return -1;
			}
			char* simulationName = simulationGetID(simulation);
			MESSAGE("Running the simulation: %s\n", simulationName);
			//simulationPrint(simulation, stdout, "###");
			DEBUG(0, "main", "Running the simulation: %s\n", simulationName);
			if (runSimulation(simulation, &em) == OK)
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

static int runSimulation(struct Simulation* simulation, ExecutableModel* em)
{
	int code = ERR;
	if (em && simulation && simulationIsValidDescription(simulation))
	{
		struct Integrator* integrator = CreateIntegrator(simulation, em);
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
			for (i = 0; i < em->nOutputs; i++)
				printf("\t%15.10e", em->outputs[i]);
			printf("\n");
			while (1)
			{
                DEBUG(5, "runSimulation", "tout = " REAL_FORMAT "\n", tout);
				double t;
				iout++;
				TIME_FUNCTION_CALL(integrationTimes, intTimer, code,
						integrate, integrator, tout, &t);
				if (code == OK)
				{
					int i;
					for (i = 0; i < em->nOutputs; i++)
						printf("\t%15.10e", em->outputs[i]);
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
                                "at time: " REAL_FORMAT "\n", tout);
						break;
					}
				}
				else
				{
					DEBUG(
							0,
							"runSimulation",
                            "Error integrating at time: " REAL_FORMAT "\n", tout);
					break;
				}
			}
			stopTimer(timer);
			double user = getUserTime(timer);
			double system = getSystemTime(timer);
			double total = user + system;
			double wall = getWallTime(timer);
			DestroyTimer(&timer);
            MESSAGE("  Wall clock time : " REAL_FORMAT " s\n", wall);
			MESSAGE(
                    "    (integration " REAL_FORMAT " s)\n", integrationTimes[2]);
			MESSAGE(
                    "    (data store  " REAL_FORMAT " s)\n", dataStoreTimes[2]);
			MESSAGE(
                    "  CPU time        : " REAL_FORMAT " s "
                    "(user " REAL_FORMAT "/system " REAL_FORMAT ")\n", total, user, system);
			MESSAGE(
                    "    (integration " REAL_FORMAT " s)\n", integrationTimes[0]+integrationTimes[1]);
			MESSAGE(
                    "    (data store  " REAL_FORMAT " s)\n", dataStoreTimes[0]+dataStoreTimes[1]);
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
	}
	else DEBUG(0, "runSimulation", "Invalid arguments\n");
	return (code);
}
