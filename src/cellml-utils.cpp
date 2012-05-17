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
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <IfaceCellML_APISPEC.hxx>
#include <CellMLBootstrap.hpp>
#include <cellml-api-cxx-support.hpp>

#include "cellml-utils.h"
#include "utils.hxx"

#ifdef __cplusplus
extern "C"
{
#endif
#include "utils.h"
#include "common.h"
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

/*
 * Prototypes for local methods
 */
static int updateVariableInitialValue(iface::cellml_api::Model* model,
  const char* variableURI,double value);

/*
 * Global methods
 */
char* wstring2string(const wchar_t* str)
{
  if (str)
  {
    size_t len = wcsrtombs(NULL,&str,0,NULL);
    if (len > 0)
    {
      len++;
      char* s = (char*)malloc(len);
      wcsrtombs(s,&str,len,NULL);
      return(s);
    }
  }
  return((char*)NULL);
}

wchar_t* string2wstring(const char* str)
{
  if (str)
  {
    wchar_t* s;
    size_t l = strlen(str);
    s = (wchar_t*)malloc(sizeof(wchar_t)*(l+1));
    memset(s,0,(l+1)*sizeof(wchar_t));
    mbsrtowcs(s,&str,l,NULL);
    return(s);
  }
  return((wchar_t*)NULL);
}

char* getCellMLMetadataAsRDFXMLString(const char* mbrurl)
{
  char *string = (char*)NULL;

  // Get the URL from which to load the model
  wchar_t* URL = string2wstring(mbrurl);

  if (!URL) return(string);
  DEBUG(2,"getCellMLMetadataAsRDFXMLString","model URI: %S\n",URL);

  RETURN_INTO_OBJREF(cb,iface::cellml_api::CellMLBootstrap,
    CreateCellMLBootstrap());
  RETURN_INTO_OBJREF(ml,iface::cellml_api::ModelLoader,cb->modelLoader());

  iface::cellml_api::Model* model = (iface::cellml_api::Model*)NULL;
  // Try and load the CellML model from the URL
  try
  {
    model = ml->loadFromURL(URL);
  }
  catch (...)
  {
    ERROR("getCellMLMetadataAsRDFXMLString",
      "Error loading model URL: %S\n",URL);
    free(URL);
    return((char*)NULL);
  }

  free(URL);

  RETURN_INTO_OBJREF(rr,iface::cellml_api::RDFRepresentation,
    model->getRDFRepresentation(L"http://www.cellml.org/RDFXML/string"));
  if (rr)
  {
    DECLARE_QUERY_INTERFACE(rrs,rr,cellml_api::RDFXMLStringRepresentation);
    std::wstring rdf = rrs->serialisedData();
    string = wstring2string(rdf.c_str());
  }
  model->release_ref();
  return(string);
}

int updateModelInitialValues(const char* inputURI,const char* outputFile,
  char** variables,double* values,int N)
{
  int code = ERR;
  if (inputURI && variables && values && (N > 0))
  {
    // Get the URL from which to load the model
    RETURN_INTO_WSTRING(URL,string2wstring(inputURI));
    DEBUG(2,"updateModelInitialValues","model URI: %S\n",URL.c_str());
    RETURN_INTO_OBJREF(cb,iface::cellml_api::CellMLBootstrap,
      CreateCellMLBootstrap());
    RETURN_INTO_OBJREF(ml,iface::cellml_api::ModelLoader,cb->modelLoader());
    iface::cellml_api::Model* model = (iface::cellml_api::Model*)NULL;
    // Try and load the CellML model from the URL
    try
    {
      model = ml->loadFromURL(URL.c_str());
    }
    catch (...)
    {
      ERROR("getCellMLMetadataAsRDFXMLString",
        "Error loading model URL: %S\n",URL.c_str());
      return(ERR);
    }
    // update all the initial value variables
    int i;
    for (i=0;i<N;i++)
    {
      updateVariableInitialValue(model,variables[i],values[i]);
    }
    // and write out the updated model
    DECLARE_QUERY_INTERFACE(modelDE,model,cellml_api::CellMLDOMElement);
    RETURN_INTO_OBJREF(element,iface::dom::Element,modelDE->domElement());
    // FIXME: want to get rid of the xml:base attribute as it generally
    //        confuses things
    element->removeAttributeNS(L""XML_NS,L"base");
    DECLARE_QUERY_INTERFACE(node,element,dom::Node);
    RETURN_INTO_WSTRING(str,cb->serialiseNode(node));
    if (outputFile)
    {
      std::wofstream file(outputFile);
      if (file.is_open())
      {
        file << str.c_str();
        file.close();
        code = OK;
      }
      else ERROR("updateModelInitialValues","Unable to open output file: %s\n",
        outputFile);
    }
    else
    {
      fprintf(stdout,"%S\n",str.c_str());
      code = OK;
    }
    //node->release_ref();
    model->release_ref();
  }
  else INVALID_ARGS("updateModelInitialValues");
  return(code);
}

/*
 * Local methods
 */
static int updateVariableInitialValue(iface::cellml_api::Model* model,
  const char* variableURI,double value)
{
  int code = ERR;
  if (model && variableURI && (strlen(variableURI) > 1))
  {
    /* first we need to find the variable matching the given URI */
    // need the model's URI to use when making the full URI for each variable
    RETURN_INTO_OBJREF(uri,iface::cellml_api::URI,model->base_uri());
    RETURN_INTO_WSTRING(modelURI,uri->asText());
    RETURN_INTO_WSTRING(vURI,string2wstring(variableURI));
    // iterate over all local components
    RETURN_INTO_OBJREF(components,iface::cellml_api::CellMLComponentSet,
      model->localComponents());
    RETURN_INTO_OBJREF(ci,iface::cellml_api::CellMLComponentIterator,
      components->iterateComponents());
    bool variableFound = false;
    while (!variableFound)
    {
      RETURN_INTO_OBJREF(c,iface::cellml_api::CellMLComponent,
        ci->nextComponent());
      if (c == NULL) break;
      // iterate over all variables
      RETURN_INTO_OBJREF(variables,iface::cellml_api::CellMLVariableSet,
        c->variables());
      RETURN_INTO_OBJREF(vi,iface::cellml_api::CellMLVariableIterator,
        variables->iterateVariables());
      while (!variableFound)
      {
        RETURN_INTO_OBJREF(v,iface::cellml_api::CellMLVariable,
          vi->nextVariable());
        if (v == NULL) break;
        RETURN_INTO_WSTRING(cmetaID,v->cmetaId());
        // cmetaId() may return an empty string 
        if (!cmetaID.empty())
        {
          std::wstring uri = modelURI + L"#" + cmetaID;
          DEBUG(2,"updateVariableInitialValue",
            "Found a variable with the uri: %S\n",uri.c_str());
          if (uri == vURI)
          {
            DEBUG(2,"updateVariableInitialValue","Found a matching variable, "
              "so updating initial_value attribute\n");
            wchar_t str[256];
            swprintf(str,256,REAL_FORMAT_W,value);
            v->initialValue(str);
            variableFound = true;
          }
        }
      }
    }
  }
  else INVALID_ARGS("updateVariableInitialValue");
  return(code);
}
