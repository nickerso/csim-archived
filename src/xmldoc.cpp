#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "xmldoc.hpp"
#ifdef __cplusplus
extern "C"
{
#endif
#include "utils.h"
#include "outputVariables.h"
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#define CSIM_SIMULATION_NS "http://cellml.sourceforge.net/csim/simulation/0.1#"

typedef std::map<std::string, std::string> NamespaceMap;

static xmlNodeSetPtr executeXPath(xmlDocPtr doc, const xmlChar* xpathExpr, const NamespaceMap &namespaces);

class LibXMLWrapper
{
public:
    LibXMLWrapper()
    {
        std::cout << "initialise libxml\n";
        /* Init libxml */
        xmlInitParser();
        LIBXML_TEST_VERSION
    }
    ~LibXMLWrapper()
    {
        std::cout << "terminate libxml\n";
        /* Shutdown libxml */
        xmlCleanupParser();
    }
};

static LibXMLWrapper dummyWrapper;

XmlDoc::XmlDoc() : mXmlDocPtr(0)
{
}

XmlDoc::~XmlDoc()
{
    if (mXmlDocPtr)
    {
        xmlFreeDoc(static_cast<xmlDocPtr>(mXmlDocPtr));
    }
}

int XmlDoc::parseString(const std::string &data)
{
    xmlDocPtr doc = xmlParseMemory(data.c_str(), data.size());
    if (doc == NULL)
    {
        std::cerr << "Error parsing data string: **" << data.c_str() << "**\n";
        return -1;
    }
    mXmlDocPtr = static_cast<void*>(doc);
    return 0;
}

int XmlDoc::parseDocument(const char* url)
{
    xmlDocPtr doc = xmlParseFile(url);
    if (doc == NULL)
    {
        std::cerr << "Error parsing document from URL: **" << url << "**\n";
        return -1;
    }
    mXmlDocPtr = static_cast<void*>(doc);
    return 0;
}

std::string XmlDoc::serialise(int format)
{
    if (mXmlDocPtr == 0)
    {
        std::cerr << L"Trying to serialise nothing?\n";
        return "";
    }
    xmlDocPtr doc = static_cast<xmlDocPtr>(mXmlDocPtr);
    xmlChar* data;
    int size = -1;
    xmlDocDumpFormatMemory(doc, &data, &size, format);
    std::string xmlString((char*)data);
    xmlFree(data);
    return xmlString;
}

std::string XmlDoc::getTextContent(const char* xpathExpr)
{
    std::string text;
    xmlDocPtr doc = static_cast<xmlDocPtr>(mXmlDocPtr);
    xmlNodeSetPtr results = executeXPath(doc, BAD_CAST xpathExpr, NamespaceMap());
    if (results)
    {
        if (xmlXPathNodeSetGetLength(results) == 1)
        {
            xmlNodePtr n = xmlXPathNodeSetItem(results, 0);
            xmlChar* s = xmlNodeGetContent(n);
            text = std::string((char*)s);
            xmlFree(s);
        }
        xmlXPathFreeNodeSet(results);
    }
    return text;
}

int XmlDoc::getDoubleContent(const char* xpathExpr, double* value)
{
    int returnCode = 0;
    xmlDocPtr doc = static_cast<xmlDocPtr>(mXmlDocPtr);
    xmlNodeSetPtr results = NULL;
    DEBUG(99, "getDoubleContent", "Executing the XPath: %s\n", xpathExpr);
    results = executeXPath(doc, BAD_CAST xpathExpr, NamespaceMap());
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

void* XmlDoc::getCSimOutputVariables()
{
    void* list = 0;
    xmlDocPtr doc = static_cast<xmlDocPtr>(mXmlDocPtr);
    xmlNodeSetPtr results = executeXPath(doc, BAD_CAST "//csim:simulation/csim:outputVariable", NamespaceMap());
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

std::string XmlDoc::getVariableId(const char* xpathExpr, std::map<std::string, std::string>& namespaces)
{
    std::string id;
    xmlDocPtr doc = static_cast<xmlDocPtr>(mXmlDocPtr);
    xmlNodeSetPtr results = executeXPath(doc, BAD_CAST xpathExpr, namespaces);
    if (results)
    {
        if (xmlXPathNodeSetGetLength(results) == 1)
        {
            xmlNodePtr node = xmlXPathNodeSetItem(results, 0);
            // we expect all XPath's to resolve into a single variable element.
            if ((node->type == XML_ELEMENT_NODE) && node->parent->name &&
                    (!strcmp((char*)(node->parent->name), "component")))
            {
                char* variableName = (char*)xmlGetProp(node, BAD_CAST "name");
                char* componentName = (char*)xmlGetProp(node->parent, BAD_CAST "name");
                id = std::string(componentName);
                id += ".";
                id += std::string(variableName);
                xmlFree(variableName);
                xmlFree(componentName);
            }
        }
        xmlXPathFreeNodeSet(results);
    }
    return id;
}

static xmlNodeSetPtr executeXPath(xmlDocPtr doc, const xmlChar* xpathExpr, const NamespaceMap& namespaces)
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
    /* Register namespaces - always CSIM by default */
    if (!(xmlXPathRegisterNs(xpathCtx, BAD_CAST "csim", BAD_CAST CSIM_SIMULATION_NS) == 0))
    {
        fprintf(stderr, "Error: unable to register default CSim namespace\n");
        return NULL;
    }
    for (auto it=namespaces.begin(); it!=namespaces.end(); ++it)
    {
        if (!(xmlXPathRegisterNs(xpathCtx, BAD_CAST it->first.c_str(), BAD_CAST it->second.c_str()) == 0))
        {
            fprintf(stderr, "Error: unable to register default namespace\n");
            return NULL;
        }
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
