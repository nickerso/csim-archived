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
#include <string.h>

#include <libxml/xmlreader.h>

#include "common.h"
#include "cellml.h"
#include "utils.h"
#include "xml-utils.h"
#include "cellml-utils.h"

/*
 * Public functions
 */
char* getRDFXMLString(const char* inputURI)
{
  char* rdfxml = (char*)NULL;
  if (inputURI)
  {
    /* parse the input file */
    if (xmlFileIs(inputURI,RDF_NS,"RDF"))
    {
      /* we have a straight RDF/XML file */
      DEBUG(3,"getRDFXMLString","input file (%s) is RDF/XML\n",inputURI);
      rdfxml = getXMLFileContentsAsString(inputURI);
    }
    else if (xmlFileIs(inputURI,CELLML_1_1_NS,"model") ||
      xmlFileIs(inputURI,CELLML_1_0_NS,"model"))
    {
      /* we have a CellML model */
      DEBUG(3,"getRDFXMLString","input file (%s) is CellML\n",inputURI);
      rdfxml = getCellMLMetadataAsRDFXMLString(inputURI);
    }
    else
    {
      ERROR("getRDFXMLString","Input file unknown format: %s\n",inputURI);
    }
  }
  return(rdfxml);
}

/* Useful test to work out if we can handle a given file - returns non-zero
   if the document element in the given XML file at <uri> is in the namespace
   <ns> with the local name <name>. Returns zero otherwise. */
int xmlFileIs(const char* uri,const char* ns,const char* name)
{
  int code = 0;
  if (uri && ns && name)
  {
    /* startup libxml */
    xmlInitParser();
    LIBXML_TEST_VERSION;
    /* make an xml text reader for the given URI */
    xmlTextReaderPtr reader = xmlNewTextReaderFilename(uri);
    if (reader != NULL)
    {
      DEBUG(2,"xmlFileIs","Made the XML reader for the uri: %s\n",uri);
      /* iterate until we get the document element */
      int ret;
      while ((ret = xmlTextReaderRead(reader)) == 1)
      {
        /* FIXME: take the first element node as the document element */
        if (xmlTextReaderNodeType(reader) == XML_ELEMENT_NODE)
        {
          DEBUG(3,"xmlFileIs","Found the document node");
          const char* localName =
            (const char*)xmlTextReaderConstLocalName(reader);
          const char* namespace =
            (const char*)xmlTextReaderConstNamespaceUri(reader);
          DEBUG(3,"xmlFileIs","Document element is: \"%s\"; "
            "namespace: \"%s\"\n",localName,namespace);
          if (!strcmp(ns,namespace) && !strcmp(name,localName))
          {
            DEBUG(3,"xmlFileIs","Match found!\n");
            DEBUG(3,"xmlFileIs","%s == %s; %s == %s\n",name,localName,ns,
              namespace);
            code = 1;
          }
          else
          {
            DEBUG(3,"xmlFileIs","Not a match\n");
          }
          /* and we're done */
          break;
        }
      }
      /* free the reader */
      xmlFreeTextReader(reader);
    }
    else
    {
      ERROR("xmlFileIs","Unable to make the XML reader for the uri: %s\n",
        uri);
    }
    /* Shutdown libxml */
    //xmlCleanupParser();
  }
  else INVALID_ARGS("xmlFileIs");
  return(code);
}

/* Get the contents of the given XML file as a standard C string */
char* getXMLFileContentsAsString(const char* uri)
{
  char* string = (char*)NULL;
  if (uri)
  {
    /* ensure startup libxml */
    xmlInitParser();
    LIBXML_TEST_VERSION;
    /* make an xml text reader for the given URI */
    xmlTextReaderPtr reader = xmlNewTextReaderFilename(uri);
    if (reader != NULL)
    {
      DEBUG(2,"getXMLFileContentsAsString",
        "Made the XML reader for the uri: %s\n",uri);
      /* parse and preserve the entire document */
      int ret = xmlTextReaderRead(reader);
      xmlTextReaderPreservePattern(reader,BAD_CAST "*",NULL);
      while (ret == 1) ret = xmlTextReaderRead(reader);
      if (ret != 0)
      {
        ERROR("getXMLFileContentsAsString","%s: failed to parse\n",uri);
      }
      else
      {
        /* grab the resulting XML document */
        xmlDocPtr doc = xmlTextReaderCurrentDoc(reader);
        if (doc)
        {
          /* and dump it into the result string */
          xmlChar* s;
          int l;
          /*xmlDocDump(stdout,doc);*/
          xmlDocDumpMemory(doc,&s,&l);
          if (s)
          {
            string = strcopy((const char*)s);
            /*printf("string: **%s**\n",string);*/
            xmlFree(s);
          }
          else ERROR("getXMLFileContentsAsString",
            "%s: failed to dump to string\n",uri);
          /* free up the XML doc */
          xmlFreeDoc(doc);
        }
      }
      /* free the reader */
      xmlFreeTextReader(reader);
    }
    else
    {
      ERROR("getXMLFileContentsAsString",
        "Unable to make the XML reader for the uri: %s\n",uri);
    }
    /* Shutdown libxml */
    //xmlCleanupParser();    
  }
  else INVALID_ARGS("getXMLFileContentsAsString");
  return(string);
}
