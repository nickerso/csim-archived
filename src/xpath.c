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
static void* getOutputVariables(xmlDocPtr doc);

/*
 * Need to be able to load in the XML source and create a simulation
 */

#define CSIM_SIMULATION_NS "http://cellml.sourceforge.net/csim/simulation/0.1#"

struct Simulation* getSimulation(const char* uri)
{
	struct Simulation* simulation;
	char* value;
	xmlDocPtr doc;
	double number;
	void* variableList;
	/* Init libxml */
	xmlInitParser();
	LIBXML_TEST_VERSION

	DEBUG(99, "getSimulation", "Starting to get the simulation for uri: %s\n", uri);

	doc = xmlParseFile(uri);
	if (doc == NULL)
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

	value = getTextContent(doc, BAD_CAST "//csim:simulation/@id");
	if (value)
	{
		DEBUG(99, "getSimulation", "got the simulation ID from the input document.\n");
		simulationSetID(simulation, value);
		DEBUG(99, "getSimulation", "free(value) go\n");
		free(value);
		DEBUG(99, "getSimulation", "free(value) gone\n");
	}
	else WARNING("getSimulation", "Missing simulation ID\n");
	if (getDoubleContent(doc, BAD_CAST "//csim:simulation/csim:boundVariable/@start", &number))
	{
		DEBUG(99, "getSimulation", "Got a double value for BvarStart...\n");
		simulationSetBvarStart(simulation, number);
	}
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
	DEBUG(99, "getSimulation", "Got the double type input parameters for the simulation.\n");

	variableList = getOutputVariables(doc);
	if (variableList)
	{
		simulationSetOutputVariables(simulation, variableList);
		outputVariablesDestroy(variableList);
	}
	else WARNING("getSimulation", "Missing simulation output variables\n");

	DEBUG(99, "getSimulation", "got the output variables for the simulation.\n");

	xmlFreeDoc(doc);
	DEBUG(99, "getSimulation", "xmlFreeDoc worked.\n");
	/* Shutdown libxml */
	xmlCleanupParser();
	DEBUG(99, "getSimulation", "xmlCleanupParser worked.\n");

	return simulation;
}

static xmlNodeSetPtr executeXPath(xmlDocPtr doc, const xmlChar* xpathExpr)
{
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	xmlNodeSetPtr results = NULL;
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

	if (xmlXPathNodeSetGetLength(xpathObj->nodesetval) > 0)
	{
		int i;
		results = xmlXPathNodeSetCreate(NULL);
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
			text = strcopy((char*)s);
			xmlFree(s);
		}
		xmlXPathFreeNodeSet(results);
	}
	return text;
}

static int getDoubleContent(xmlDocPtr doc, const xmlChar* xpathExpr, double* value)
{
	int returnCode = 0;
	xmlNodeSetPtr results = NULL;
	DEBUG(99, "getDoubleContent", "Executing the XPath: %s\n", xpathExpr);
	results = executeXPath(doc, xpathExpr);
	if (results)
	{
		if (xmlXPathNodeSetGetLength(results) == 1)
		{
			xmlNodePtr n = xmlXPathNodeSetItem(results, 0);
			xmlChar* s = xmlNodeGetContent(n);
			if (sscanf((char*)s, "%lf", value) == 1) returnCode = 1;
			else ERROR("getDoubleContent", "found a value for xpath expression, but its not a number: \"%s\"\n", (char*)s);
			DEBUG(99, "getSimulation", "free(s) go\n");
			xmlFree(s);
			DEBUG(99, "getSimulation", "free(s) gone\n");
		}
		xmlXPathFreeNodeSet(results);
	}
	return returnCode;
}

static void* getOutputVariables(xmlDocPtr doc)
{
	void* list = 0;
	xmlNodeSetPtr results = executeXPath(doc, BAD_CAST "//csim:simulation/csim:outputVariable");
	if (results)
	{
		int i, n = xmlXPathNodeSetGetLength(results);
		list = outputVariablesCreate();
		for (i = 0; i < n; ++i)
		{
			xmlNodePtr node = xmlXPathNodeSetItem(results, i);
			if (node->type == XML_ELEMENT_NODE)
			{
				char* component = (char*)xmlGetProp(node, BAD_CAST "component");
				char* variable = (char*)xmlGetProp(node, BAD_CAST "variable");
				char* column = (char*)xmlGetProp(node, BAD_CAST "column");
				int c;
				if (sscanf((char*)column, "%d", &c) != 1)
				{
					c = -1;
					ERROR("getOutputVariables", "found a value for a column, but its not a integer: \"%s\"\n", (char*)column);
				}
				xmlFree(column);
				outputVariablesAppendVariable(list, component, variable, c);
				xmlFree(component);
				xmlFree(variable);
			}
		}
	}
	return list;
}
