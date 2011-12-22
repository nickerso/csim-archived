/*
 * xpath.c
 *
 *  Created on: Dec 21, 2011
 *      Author: dnic019
 */
#include <stdio.h>
#include <stdlib.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "xpath.h"
#include "simulation.h"
#include "utils.h"
#include "outputVariables.h"

static xmlNodeSetPtr executeXPath(xmlDocPtr doc, const xmlChar* xpathExpr);
static char* getTextContent(xmlDocPtr doc, const xmlChar* xpathExpr);
static int getDoubleContent(xmlDocPtr doc, const xmlChar* xpathExpr, double* value);

/*
 * Need to be able to load in the XML source and create a simulation
 */

#define CSIM_SIMULATION_NS "http://cellml.sourceforge.net/csim/simulation/0.1#"

struct Simulation* getSimulation(const char* uri)
{
	/* Init libxml */
	xmlInitParser();
	LIBXML_TEST_VERSION

	xmlDocPtr doc = xmlParseFile(uri);
	if (doc == NULL)
	{
		ERROR("getSimulation", "Error: unable to parse URI \"%s\"\n", uri);
		return(NULL);
	}

	struct Simulation* simulation = CreateSimulation();
	DEBUG(0, "getSimulation", "created a blank simulation\n");
	// these are not yet used, but need to be set.
	simulationSetURI(simulation, uri);
	simulationSetModelURI(simulation, uri);
	simulationSetBvarURI(simulation, "bob");

	char* value = getTextContent(doc, BAD_CAST "//csim:simulation/@id");
	if (value)
	{
		simulationSetID(simulation, value);
		free(value);
	}
	else WARNING("getSimulation", "Missing simulation ID\n");
	double number;
	if (getDoubleContent(doc, BAD_CAST "//csim:simulation/csim:boundVariable/@start", &number))
		simulationSetBvarStart(simulation, number);
	else WARNING("getSimulation", "Missing simulation bvar start\n");
	if (getDoubleContent(doc, BAD_CAST "//csim:simulation/csim:boundVariable/@end", &number))
		simulationSetBvarEnd(simulation, number);
	else WARNING("getSimulation", "Missing simulation bvar end\n");
	if (getDoubleContent(doc, BAD_CAST "//csim:simulation/csim:boundVariable/@maxStep", &number))
		simulationSetBvarMaxStep(simulation, number);
	else WARNING("getSimulation", "Missing simulation bvar max step\n");
	if (getDoubleContent(doc, BAD_CAST "//csim:simulation/csim:boundVariable/@tabulationStep", &number))
		simulationSetBvarTabStep(simulation, number);
	else WARNING("getSimulation", "Missing simulation bvar tab step\n");

	void* variableList = outputVariablesCreate();

	simulationSetOutputVariables(simulation, variableList);
	outputVariablesDestroy(variableList);

	xmlFreeDoc(doc);
	/* Shutdown libxml */
	xmlCleanupParser();

	return simulation;
}

static xmlNodeSetPtr executeXPath(xmlDocPtr doc, const xmlChar* xpathExpr)
{
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if (xpathCtx == NULL)
	{
		fprintf(stderr, "Error: unable to create new XPath context\n");
		return NULL;
	}
	/* Register namespaces */
	if (!(xmlXPathRegisterNs(xpathCtx, BAD_CAST "csim", BAD_CAST CSIM_SIMULATION_NS) == 0))
	{
		fprintf(stderr, "Error: unable to register namespaces\n");
		return NULL;
	}
	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if (xpathObj == NULL)
	{
		fprintf(stderr, "Error: unable to evaluate xpath expression \"%s\"\n",
				xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		return NULL;
	}

	xmlNodeSetPtr results = NULL;
	if (xmlXPathNodeSetGetLength(xpathObj->nodesetval) > 0)
	{
		results = xmlXPathNodeSetCreate(NULL);
		int i;
		for (i=0; i< xmlXPathNodeSetGetLength(xpathObj->nodesetval); ++i)
			xmlXPathNodeSetAdd(results, xmlXPathNodeSetItem(xpathObj->nodesetval, i));
	}
	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	return results;
}

static char* getTextContent(xmlDocPtr doc, const xmlChar* xpathExpr)
{
	char* text = NULL;
	xmlNodeSetPtr results = executeXPath(doc, xpathExpr);
	if (results)
	{
		if (xmlXPathNodeSetGetLength(results) == 1)
		{
			xmlNodePtr n = xmlXPathNodeSetItem(results, 0);
			xmlChar* s = xmlNodeGetContent(n);
			text = (char*)s;
		}
		xmlXPathFreeNodeSet(results);
	}
	return text;
}

static int getDoubleContent(xmlDocPtr doc, const xmlChar* xpathExpr, double* value)
{
	int returnCode = 0;
	xmlNodeSetPtr results = executeXPath(doc, xpathExpr);
	if (results)
	{
		if (xmlXPathNodeSetGetLength(results) == 1)
		{
			xmlNodePtr n = xmlXPathNodeSetItem(results, 0);
			xmlChar* s = xmlNodeGetContent(n);
			if (sscanf((char*)s, "%lf", value) == 1) returnCode = 1;
			else ERROR("getDoubleContent", "found a value for xpath expression, but its not a number: \"%s\"\n", (char*)s);
			free(s);
		}
		xmlXPathFreeNodeSet(results);
	}
	return returnCode;
}
