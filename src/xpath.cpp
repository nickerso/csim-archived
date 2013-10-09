/*
 * xpath.c
 *
 *  Created on: Dec 21, 2011
 *      Author: dnic019
 */
#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "xpath.hpp"
#include "xmldoc.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
#include "simulation.h"
#include "utils.h"
#include "outputVariables.h"
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

/*
 * Need to be able to load in the XML source and create a simulation
 */

#define CSIM_SIMULATION_NS "http://cellml.sourceforge.net/csim/simulation/0.1#"

struct Simulation* getSimulation(const char* uri)
{
	struct Simulation* simulation;
    std::string value;
    XmlDoc doc;
	double number;
	void* variableList;

	DEBUG(99, "getSimulation", "Starting to get the simulation for uri: %s\n", uri);

    if (doc.parseDocument(uri) != 0)
	{
		ERROR("getSimulation", "Error: unable to parse URI \"%s\"\n", uri);
		return(NULL);
	}

	simulation = CreateSimulation();
	DEBUG(0, "getSimulation", "created a blank simulation\n");
	// these are not yet used, but need to be set.
	simulationSetURI(simulation, uri);
	simulationSetModelURI(simulation, uri);
	simulationSetBvarURI(simulation, "bob");

	DEBUG(99, "getSimulation", "set base fields for new simulation\n");

	// TODO: need to get algorithm and parameters...
	// default to BDF with Newton iterations and dense solver
	simulationSetMultistepMethod(simulation, BDF);
	simulationSetIterationMethod(simulation, NEWTON);
	simulationSetLinearSolver(simulation, DENSE);

    value = doc.getTextContent("//csim:simulation/@id");
    if (!value.empty())
	{
		DEBUG(99, "getSimulation", "got the simulation ID from the input document.\n");
        simulationSetID(simulation, value.c_str());
	}
	else WARNING("getSimulation", "Missing simulation ID\n");
    if (doc.getDoubleContent("//csim:simulation/csim:boundVariable/@start", &number))
	{
		DEBUG(99, "getSimulation", "Got a double value for BvarStart...\n");
		simulationSetBvarStart(simulation, number);
	}
	else WARNING("getSimulation", "Missing simulation bvar start\n");
    if (doc.getDoubleContent("//csim:simulation/csim:boundVariable/@end", &number))
		simulationSetBvarEnd(simulation, number);
	else WARNING("getSimulation", "Missing simulation bvar end\n");
    if (doc.getDoubleContent("//csim:simulation/csim:boundVariable/@maxStep", &number))
		simulationSetBvarMaxStep(simulation, number);
	else WARNING("getSimulation", "Missing simulation bvar max step\n");
    if (doc.getDoubleContent("//csim:simulation/csim:boundVariable/@tabulationStep", &number))
		simulationSetBvarTabStep(simulation, number);
	else WARNING("getSimulation", "Missing simulation bvar tab step\n");
	DEBUG(99, "getSimulation", "Got the double type input parameters for the simulation.\n");

    variableList = doc.getCSimOutputVariables();
	if (variableList)
	{
		simulationSetOutputVariables(simulation, variableList);
		outputVariablesDestroy(variableList);
	}
	else WARNING("getSimulation", "Missing simulation output variables\n");

	DEBUG(99, "getSimulation", "got the output variables for the simulation.\n");

	return simulation;
}
