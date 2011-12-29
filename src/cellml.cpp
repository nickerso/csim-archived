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
#include <inttypes.h>
#include <exception>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string.h>
#include <cwchar>
#include <vector>
#include <list>
#include <algorithm>

#include <IfaceCellML_APISPEC.hxx>
#include <IfaceCCGS.hxx>
#include <AnnoToolsBootstrap.hpp>
#include <CeVASBootstrap.hpp>
#include <MaLaESBootstrap.hpp>
#include <CCGSBootstrap.hpp>
#include <CellMLBootstrap.hpp>

#include "cellml.h"
#include "cellml-utils.h"
#include "cellml.hpp"
#include "utils.hxx"
extern "C"
{
#include "outputVariables.h"
}

#ifdef __cplusplus
extern "C"
{
#endif
#include "utils.h"
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

struct CellMLModel
{
  iface::cellml_api::Model* model;
  iface::cellml_services::AnnotationSet* annotationSet;
  char* uri;
};

static std::wstring
formatNumber(const int value)
{
  wchar_t valueString[100];
  swprintf(valueString,100,L"%d",value);
  return std::wstring(valueString);
}

static std::wstring
formatNumber(const uint32_t value)
{
  wchar_t valueString[100];
  swprintf(valueString,100,L"%u",value);
  return std::wstring(valueString);
}

static const wchar_t*
TypeToString(iface::cellml_services::VariableEvaluationType vet)
{
  switch (vet)
  {
  case iface::cellml_services::VARIABLE_OF_INTEGRATION:
    return L"variable of integration";
  case iface::cellml_services::CONSTANT:
    return L"constant";
  case iface::cellml_services::STATE_VARIABLE:
    return L"state variable";
  case iface::cellml_services::ALGEBRAIC:
    return L"algebraic variable";
  case iface::cellml_services::FLOATING:
    return L"uncomputed";
  }

  return L"invalid type";
}

static int
checkCodeInformation(iface::cellml_services::CodeInformation* cci,
  int debugCode)
{
  iface::cellml_services::ModelConstraintLevel mcl =
    cci->constraintLevel();
  if (mcl == iface::cellml_services::UNDERCONSTRAINED)
  {
    fprintf(stderr,"Model is underconstrained.\n"
      "List of undefined variables follows...\n");
    iface::cellml_services::ComputationTargetIterator* cti =
      cci->iterateTargets();
    iface::cellml_services::ComputationTarget* ct;
    std::vector<std::wstring> messages;
    while (true)
    {
      ct = cti->nextComputationTarget();
      if (ct == NULL)
        break;
      if (ct->type() != iface::cellml_services::FLOATING)
      {
        ct->release_ref();
        continue;
      }
      iface::cellml_api::CellMLVariable* v = ct->variable();
      wchar_t* n = v->name();
      wchar_t* c = v->componentName();
      std::wstring str = L" * ";
      uint32_t deg = ct->degree();
      if (deg != 0)
      {
        str += L"d^";
        wchar_t buf[20];
        swprintf(buf, 20, L"%u", deg);
        str += buf;
        str += L"/dt^";
        str += buf;
        str += L" ";
      }
      str += n;
      str += L" (in ";
      str += c;
      str += L")\n";
      free(n);
      free(c);
      messages.push_back(str);
      v->release_ref();
      ct->release_ref();
    }
    cti->release_ref();
    // Sort the messages...
    std::sort(messages.begin(), messages.end());
    std::vector<std::wstring>::iterator msgi;
    for (msgi = messages.begin(); msgi != messages.end(); msgi++)
      fprintf(stderr,"%S", (*msgi).c_str());
    fprintf(stderr,"\n");
    return(-1);
  }
  else if (mcl == iface::cellml_services::OVERCONSTRAINED)
  {
    fprintf(stderr,"Model is overconstrained.\n"
      "List variables defined at time of error follows...\n");
    iface::cellml_services::ComputationTargetIterator* cti =
      cci->iterateTargets();
    iface::cellml_services::ComputationTarget* ct;
    std::vector<std::wstring> messages;
    while (true)
    {
      ct = cti->nextComputationTarget();
      if (ct == NULL)
        break;
      if (ct->type() == iface::cellml_services::FLOATING)
      {
        ct->release_ref();
        continue;
      }
      iface::cellml_api::CellMLVariable* v = ct->variable();
      wchar_t* n = v->name();
      std::wstring str = L" * ";
      uint32_t deg = ct->degree();
      if (deg != 0)
      {
        str += L"d^";
        wchar_t buf[20];
        swprintf(buf, 20, L"%u", deg);
        str += buf;
        str += L"/dt^";
        str += buf;
        str += L" ";
      }
      str += n;
      free(n);
      str += L"\n";
      messages.push_back(str);
      v->release_ref();
      ct->release_ref();
    }
    cti->release_ref();

    // Sort the messages...
    std::sort(messages.begin(), messages.end());
    std::vector<std::wstring>::iterator msgi;
    for (msgi = messages.begin(); msgi != messages.end(); msgi++)
      fprintf(stderr,"%S", (*msgi).c_str());

    // Get flagged equations...
    iface::mathml_dom::MathMLNodeList* mnl = cci->flaggedEquations();
    fprintf(stderr," * Extraneous equation was:\n");
    iface::dom::Node* n = mnl->item(0);
    mnl->release_ref();
    iface::dom::Element* el =
      reinterpret_cast<iface::dom::Element*>(n->query_interface(
                                               "dom::Element"));
    n->release_ref();

    wchar_t* cmeta =
      el->getAttributeNS(L"http://www.cellml.org/metadata/1.0#",L"id");
    if (!wcscmp(cmeta, L"")) fprintf(stderr,
      " *   <equation with no cmeta ID>\n");
    else fprintf(stderr," *   %S\n", cmeta);
    free(cmeta);

    n = el->parentNode();
    el->release_ref();

    el = reinterpret_cast<iface::dom::Element*>
      (n->query_interface("dom::Element"));
    n->release_ref();

    cmeta = el->getAttributeNS(L"http://www.cellml.org/metadata/1.0#", L"id");
    if (!wcscmp(cmeta, L""))
      fprintf(stderr," *   in <math with no cmeta ID>\n");
    else
      fprintf(stderr," *   in math with cmeta:id %S\n", cmeta);
    free(cmeta);
    el->release_ref();

    fprintf(stderr,"\n");
    return 1;
  }
  else if (mcl == iface::cellml_services::UNSUITABLY_CONSTRAINED)
  {
    printf("Model is unsuitably constrained (i.e. would need capabilities\n"
           "beyond those of the CCGS to solve).\n"
           "The status of variables at time of error follows...\n");
    iface::cellml_services::ComputationTargetIterator* cti =
      cci->iterateTargets();
    iface::cellml_services::ComputationTarget* ct;
    std::vector<std::wstring> messages;
    while (true)
    {
      ct = cti->nextComputationTarget();
      if (ct == NULL)
        break;
      std::wstring str = L" * ";
      if (ct->type() == iface::cellml_services::FLOATING)
        str += L" Undefined: ";
      else
        str += L" Defined: ";

      uint32_t deg = ct->degree();
      if (deg != 0)
      {
        str += L"d^";
        wchar_t buf[20];
        swprintf(buf, 20, L"%u", deg);
        str += buf;
        str += L"/dt^";
        str += buf;
        str += L" ";
      }
      iface::cellml_api::CellMLVariable* v = ct->variable();
      wchar_t* n = v->name();
      str += n;
      free(n);
      str += L"\n";
      messages.push_back(str);
      v->release_ref();
      ct->release_ref();
    }
    cti->release_ref();

    // Sort the messages...
    std::sort(messages.begin(), messages.end());
    std::vector<std::wstring>::iterator msgi;
    for (msgi = messages.begin(); msgi != messages.end(); msgi++)
      fprintf(stderr,"%S", (*msgi).c_str());

    fprintf(stderr,"\n");
    
    return(2);
  }

  if (debugLevel() > 2)
  {
    printf("Model is correctly constrained.\n");
    iface::mathml_dom::MathMLNodeList* mnl = cci->flaggedEquations();
    uint32_t i, l = mnl->length();
    if (l == 0)
      printf(" * No equations needed Newton-Raphson evaluation.\n");
    else
      printf(" * The following equations needed Newton-Raphson evaluation:\n");
    for (i = 0; i < l; i++)
    {
      iface::dom::Node* n = mnl->item(i);
      DECLARE_QUERY_INTERFACE(el,n,dom::Element);
      /*iface::dom::Element* el =
        dynamic_cast<iface::dom::Element*>
        (n->query_interface("dom::Element"));*/
      n->release_ref();
      
      wchar_t* cmeta = el->getAttribute(L"id");
      if (!wcscmp(cmeta, L""))
        printf(" *   <equation with no ID>\n");
      else
        printf(" *   %S\n", cmeta);
      free(cmeta);
      
      n = el->parentNode();
      el->release_ref();
      
      QUERY_INTERFACE(el,n,dom::Element);
      /*el = dynamic_cast<iface::dom::Element*>
        (n->query_interface("dom::Element"));*/
      n->release_ref();
      
      cmeta = el->getAttributeNS(L"http://www.cellml.org/metadata/1.0#",
                                 L"id");
      if (!wcscmp(cmeta, L""))
        printf(" *   in <math with no cmeta ID>\n");
      else
        printf(" *   in math with cmeta:id %S\n", cmeta);
      free(cmeta);
      el->release_ref();
    }
    mnl->release_ref();

    printf(" * The rate and state arrays need %u entries.\n",
      cci->rateIndexCount());
    printf(" * The algebraic variables array needs %u entries.\n",
      cci->algebraicIndexCount());
    printf(" * The constant array needs %u entries.\n",
      cci->constantIndexCount());
  }
  if (debugLevel() > 3)
  {
    printf(" * Variable storage is as follows:\n");
    std::vector<std::wstring> messages;
    iface::cellml_services::ComputationTargetIterator* cti =
      cci->iterateTargets();
    while (true)
    {
      iface::cellml_services::ComputationTarget* ct =
        cti->nextComputationTarget();
      if (ct == NULL) break;
      iface::cellml_api::CellMLVariable* v = ct->variable();
      iface::cellml_api::CellMLElement* el = v->parentElement();
      iface::cellml_api::CellMLComponent* c =
        reinterpret_cast<iface::cellml_api::CellMLComponent*>
        (el->query_interface("cellml_api::CellMLComponent"));
      el->release_ref();

      std::wstring str;
      wchar_t* vn = v->name(), * cn = c->name();
      str += L" * * Target ";
      uint32_t deg = ct->degree();
      if (deg != 0)
      {
        str += L"d^";
        wchar_t buf[20];
        swprintf(buf, 20, L"%u", deg);
        str += buf;
        str += L"/dt^";
        str += buf;
        str += L" ";
      }
      str += vn;
      str += L" in component ";
      str += cn;
      str += L"\n";
      free(vn);
      free(cn);
      
      c->release_ref();
      v->release_ref();

      str += L" * * * Variable type: ";
      str += TypeToString(ct->type());
      str += L"\n * * * Variable index: ";
      wchar_t buf[40];
      swprintf(buf, 40, L"%u\n", ct->assignedIndex());
      str += buf;

      str += L" * * * Variable storage: ";
      wchar_t * vsn;
      vsn = ct->name();
      str += vsn;
      free(vsn);
      str += '\n';

      ct->release_ref();

      messages.push_back(str);
    }
    cti->release_ref();

    // Sort the messages...
    std::sort(messages.begin(), messages.end());
    std::vector<std::wstring>::iterator msgi;
    for (msgi = messages.begin(); msgi != messages.end(); msgi++)
      printf("%S", (*msgi).c_str());

    printf(" */\n");
  }
  
  if (debugCode)
  {
    std::vector<std::wstring> voi(1);
    std::vector<std::wstring> constant(cci->constantIndexCount());
    std::vector<std::wstring> state(cci->rateIndexCount());
    std::vector<std::wstring> alg(cci->algebraicIndexCount());
    std::vector<std::wstring> rates(cci->rateIndexCount());    
    iface::cellml_services::ComputationTargetIterator* cti =
      cci->iterateTargets();
    while (true)
    {
      iface::cellml_services::ComputationTarget* ct =
        cti->nextComputationTarget();
      if (ct == NULL) break;
      iface::cellml_api::CellMLVariable* v = ct->variable();
      iface::cellml_api::CellMLElement* el = v->parentElement();
      iface::cellml_api::CellMLComponent* c =
        reinterpret_cast<iface::cellml_api::CellMLComponent*>
        (el->query_interface("cellml_api::CellMLComponent"));
      el->release_ref();

      std::wstring str;
      wchar_t* vn = v->name(), * cn = c->name();
      uint32_t deg = ct->degree();
      if (deg != 0)
      {
        str += L"d^";
        wchar_t buf[20];
        swprintf(buf, 20, L"%u", deg);
        str += buf;
        str += L"/dt^";
        str += buf;
        str += L" ";
      }
      str += vn;
      str += L" in component ";
      str += cn;
      free(vn);
      free(cn);
      
      c->release_ref();
      v->release_ref();

      switch (ct->type())
      {
        case iface::cellml_services::VARIABLE_OF_INTEGRATION:
        {
          voi[ct->assignedIndex()] = str;
        } break;
        case iface::cellml_services::CONSTANT:
        {
          constant[ct->assignedIndex()] = str;
        } break;
        case iface::cellml_services::STATE_VARIABLE:
        {
          state[ct->assignedIndex()] = str;
        } break;
        case iface::cellml_services::ALGEBRAIC:
        {
          if (deg != 0) rates[ct->assignedIndex()] = str;
          else alg[ct->assignedIndex()] = str;
        } break;
        default:
        {
          fprintf(stderr,"WARNING: Unhandled CT type in %s at line %d",
            __FILE__,__LINE__);
        }
      }
      ct->release_ref();
    }
    cti->release_ref();

    // dump out the array storage
    int j;
    std::vector<std::wstring>::const_iterator i = voi.begin();
    for (j=0;i!=voi.end();++i,++j) printf("VOI[%d] = %S\n",j,i->c_str());
    i = rates.begin();
    for (j=0;i!=rates.end();++i,++j) printf("RATES[%d] = %S\n",j,i->c_str());
    i = state.begin();
    for (j=0;i!=state.end();++i,++j) printf("STATES[%d] = %S\n",j,i->c_str());
    i = alg.begin();
    for (j=0;i!=alg.end();++i,++j) printf("ALGEBRAIC[%d] = %S\n",j,i->c_str());
    i = constant.begin();
    for (j=0;i!=constant.end();++i,++j) printf("CONSTANTS[%d] = %S\n",j,
      i->c_str());
  }
  return 0;
} // checkCodeInformation

/*
 * Ideally these should be the variable id's rather than the names since
 * that is what will be referenced by the plot metadata. Just fall back on
 * the names when no cmeta:id is found, cause in that case the variable can't
 * be referenced by a plot anyway.
 */

/* This needs to be redone by iterating through connected variable sets as
 * obtained from the CeVAS object from the code generator...
 */
#if 0
static char*
writeVariableNameCases(iface::cellml_services::CodeInformation* cci)
{
  iface::cellml_services::ComputationTargetIterator* cti =
    cci->iterateTargets();
  std::string code;
  while (true)
  {
    iface::cellml_services::ComputationTarget* ct =
      cti->nextComputationTarget();
    if (ct == NULL)
      break;
    if ((v->type() == iface::cellml_services::DIFFERENTIAL) ||
      (v->type() == iface::cellml_services::COMPUTED) ||
      (v->type() == iface::cellml_services::COMPUTED_CONSTANT))
    {
      char* name = (char*)NULL;
      int index = v->variableIndex();
      name = v->ids();
      iface::cellml_api::CellMLVariable* s = v->source();
#if defined OLD_CODE
      wchar_t* ID = s->cmetaId();
      /* ID may come back an empty string rather than NULL if no
         cmeta:id defined */
      if (ID && ((name = wstring2string(ID)) != NULL))
      {
        free(ID);
        s->release_ref();
      }
      else if (ID) free(ID);
#endif
      if (name == NULL)
      {
        iface::cellml_api::CellMLElement* el = s->parentElement();
        DECLARE_QUERY_INTERFACE(c,el,cellml_api::CellMLComponent);
        el->release_ref();
        wchar_t* vn = s->name();
        wchar_t* cn = c->name();
        char* vname = wstring2string(vn);
        char* cname = wstring2string(cn);
        free(vn);
        free(cn);
        s->release_ref();
        c->release_ref();
        name = (char*)malloc(strlen(vname)+strlen(cname)+2);
        sprintf(name,"%s/%s",cname,vname);
        free(vname);
        free(cname);
      }
      if (name)
      {
        code += "case ";
        code += formatNumber(index);
        code += ": return \"";
        code += name;
        code += "\";\n";
        free(name);
      }
    }
    v->release_ref();
  }
  cvi->release_ref();
  const int len = strlen(code.c_str());
  char* string = new char[len+1];
  strcpy(string,code.c_str());
  return(string);
} // writeVariableNameCases
#endif


/* given a computation target and a code generator, grab the generator's
 * CeVAS and use it to assemble a string listing the cmeta:id's of all
 * variables connected to the computation target's variable.
 */
static std::wstring
getConnectedVariableIDString(iface::cellml_services::CodeGenerator* cg,
  iface::cellml_services::ComputationTarget* ct)
{
  std::wstring wstr = L"";
  // retrieve the CeVAS from the code generator (we set this earlier)
  iface::cellml_services::CeVAS* cevas = cg->useCeVAS();
  if (cevas)
  {
    // the computation target's variable
    iface::cellml_api::CellMLVariable* ctv = ct->variable();
    // and the set of variables connected to it
    iface::cellml_services::ConnectedVariableSet* cvs =
      cevas->findVariableSet(ctv);
    ctv->release_ref();
    cevas->release_ref();
    int i,j,l=(int)(cvs->length());
    for (i=0,j=0;i<l;i++)
    {
      iface::cellml_api::CellMLVariable* v = cvs->getVariable(i);
      wchar_t* id = v->cmetaId();
      if (id)
      {
        // cmetaId() may return an empty string 
        if (wcscmp(id,L""))
        {
          // grab the model's base_uri to build the actual URI for
          // the given variable
          iface::cellml_api::Model* model = v->modelElement();
          iface::cellml_api::URI* uri = model->base_uri();
          model->release_ref();
          RETURN_INTO_WSTRING(uriS,uri->asText());
          uri->release_ref();
          uriS.append(L"#");
          uriS.append(id);
          if (j>0) wstr += L",";
          wstr += uriS;
          j++;
        }
        free(id);
      }
      v->release_ref();
    }
    cvs->release_ref();
  }
  else
  {
    ERROR("getConnectedVariableIDString","Unable to retrieve CeVAS object");
  }
  return(wstr);
}

static void setOutputVariableIndices(iface::cellml_services::CodeInformation* cci,
		iface::cellml_services::CodeGenerator* cg, void* outputVariables)
{
	RETURN_INTO_OBJREF(as, iface::cellml_services::AnnotationSet, cg->useAnnoSet());
	RETURN_INTO_OBJREF(cti, iface::cellml_services::ComputationTargetIterator, cci->iterateTargets());
	while (true)
	{
		RETURN_INTO_OBJREF(ct, iface::cellml_services::ComputationTarget, cti->nextComputationTarget());
		if (ct == NULL) break;
		RETURN_INTO_WSTRING(column, as->getStringAnnotation(ct->variable(), L"CSim::OutputColumn"));
		if (column.length() > 0)
		{
			RETURN_INTO_STRING(col, wstring2string(column.c_str()));
			int c = strtol(col.c_str(), NULL, /*base 10*/10);
			// FIXME: assume this always works since we set the annotation...
			MESSAGE("Setting output variable column: %d\n", c);
		}
	}
}

/* write out all the cmeta:id's for each variable of the given <vet> type
 */
static std::wstring
writeVariableCases(iface::cellml_services::CodeInformation* cci,
  iface::cellml_services::CodeGenerator* cg,
  iface::cellml_services::VariableEvaluationType vet)
{
  std::wstring wstr = L"";
  iface::cellml_services::ComputationTargetIterator* cti =
    cci->iterateTargets();
  while (true)
  {
    iface::cellml_services::ComputationTarget* ct =
      cti->nextComputationTarget();
    if (ct == NULL) break;
    /* FIXME: is it correct to simply ignore all computation targets with
     *        a degree != 0?
     */
    if ((ct->type() == vet) && (ct->degree() == 0))
    {
      std::wstring ids = getConnectedVariableIDString(cg,ct);
      if (!wcscmp(ids.c_str(),L""))
      {
        // no cmeta:id's so use component_name/variable_name
        RETURN_INTO_OBJREF(v,iface::cellml_api::CellMLVariable,ct->variable());
        RETURN_INTO_WSTRING(vn,v->name());
        RETURN_INTO_WSTRING(cn,v->componentName());
        ids = cn;
        ids += L"/";
        ids += vn;
      }
      wstr += L"case ";
      wstr += formatNumber(ct->assignedIndex());
      wstr += L": return \"";
      wstr += ids;
      wstr += L"\";\n";
    }
    ct->release_ref();
  }
  cti->release_ref();
  return(wstr);
}

static std::wstring
writeCode(iface::cellml_services::CodeInformation* cci,
  iface::cellml_services::CodeGenerator* cg,void* outputVariables,int debugCode)
{
  // Assuming here that the code information has been checked using the
  // checkCodeInformation function.
  std::wstring code;
  // The required headers
  code += L"#include <math.h>\n";
  code += L"#include <stdio.h>\n";
  /* required functions */
  code += L"extern double fabs(double x);";
  code += L"extern double acos(double x);";
  code += L"extern double acosh(double x);";
  code += L"extern double atan(double x);";
  code += L"extern double atanh(double x);";
  code += L"extern double asin(double x);";
  code += L"extern double asinh(double x);";
  code += L"extern double acos(double x);";
  code += L"extern double acosh(double x);";
  code += L"extern double asin(double x);";
  code += L"extern double asinh(double x);";
  code += L"extern double atan(double x);";
  code += L"extern double atanh(double x);";
  code += L"extern double ceil(double x);";
  code += L"extern double cos(double x);";
  code += L"extern double cosh(double x);";
  code += L"extern double tan(double x);";
  code += L"extern double tanh(double x);";
  code += L"extern double sin(double x);";
  code += L"extern double sinh(double x);";
  code += L"extern double exp(double x);";
  code += L"extern double floor(double x);";
  code += L"extern double pow(double x, double y);";    
  code += L"extern double factorial(double x);";
  code += L"extern double log(double x);";
  code += L"extern double arbitrary_log(double x, double base);";
  code += L"extern double gcd_pair(double a, double b);";
  code += L"extern double lcm_pair(double a, double b);";
  code += L"extern double gcd_multi(unsigned int size, ...);";
  code += L"extern double lcm_multi(unsigned int size, ...);";
  code += L"extern double multi_min(unsigned int size, ...);";
  code += L"extern double multi_max(unsigned int size, ...);";
  code += L"extern void NR_MINIMISE(double(*func)(double VOI, double *C, double *R, double *S, double *A),"
    L"double VOI, double *C, double *R, double *S, double *A, double *V);";

  /*
  code += L"double arbitrary_log(double value, double logbase)\n"
    L"{\n"
    L"return log(value) / log(logbase);\n"
    L"}\n";
  */

  wchar_t* frag = cci->functionsString();
  code += frag;
  free(frag);

  /* if generating debug code we need some extra methods */
  if (debugCode)
  {
    code += L"double CHECK_DENOMINATOR(double value,int line)\n"
      L"{\n"
      L"  if (fabs(value) < 1.0e-8) {\n"
      L"    printf(\"MODEL ERROR: divide by zero (%s: line %d)\\n\",__FILE__,line);\n"
      L"  }\n"
      L"  return(value);\n"
      L"}\n";
  }
  
  // Some helper functions for the simulator
  code += L"int getNbound() { return ";
  /* FIXME: is there something better for this? */
  code += formatNumber(1);
  code += L"; }\n";
  code += L"int getNrates() { return ";
  code += formatNumber(cci->rateIndexCount());
  code += L"; }\n";
  code += L"int getNalgebraic() { return ";
  code += formatNumber(cci->algebraicIndexCount());
  code += L"; }\n";
  code += L"int getNconstants() { return ";
  code += formatNumber(cci->constantIndexCount());
  code += L"; }\n";
  
  // TODO: rather than these different cases, need to only write out the variables that have been annotated with the annotations for the output variables.
  setOutputVariableIndices(cci, cg, outputVariables);

  code += L"const char* getStateVariableIDs(int index)\n{\n"
    L"switch (index)\n{\n";
  code += writeVariableCases(cci,cg,
    iface::cellml_services::STATE_VARIABLE);
  code += L"default: return \"UNDEF\";\n}\n}\n";
  
  code += L"const char* getConstantVariableIDs(int index)\n{\n"
    L"switch (index)\n{\n";
  code += writeVariableCases(cci,cg,
    iface::cellml_services::CONSTANT);
  code += L"default: return \"UNDEF\";\n}\n}\n";
  
  code += L"const char* getAlgebraicVariableIDs(int index)\n{\n"
    L"switch (index)\n{\n";
  code += writeVariableCases(cci,cg,
    iface::cellml_services::ALGEBRAIC);
  code += L"default: return \"UNDEF\";\n}\n}\n";
  
  code += L"const char* getVariableOfIntegrationIDs()\n{\n";
  code += L"return(\"";
  iface::cellml_services::ComputationTargetIterator* cti =
    cci->iterateTargets();
  iface::cellml_services::ComputationTarget* ct =
    cti->nextComputationTarget();
  while (ct && (ct->type() != iface::cellml_services::VARIABLE_OF_INTEGRATION))
  {
    ct->release_ref();
    ct = cti->nextComputationTarget();
  }
  if (ct)
  {
    code += getConnectedVariableIDString(cg,ct);
    ct->release_ref();
  }
  else code += L"UNDEF";
  cti->release_ref();
  code += L"\");\n";
  /* FIXME: do this bit
  frag = writeBoundVariableNameCases(cci);
  code += frag;
  delete [] frag;
  */
  code += L"}\n";

  // Now start the model code...
  
  /* https://svn.physiomeproject.org/svn/physiome/CellML_DOM_API/trunk/interfaces/CCGS.idl for full description */
  
  /* initConsts - all variables which aren't state variables but have
   *              an initial_value attribute, and any variables & rates
   *              which follow.
   */
  frag = cci->initConstsString();
  code += L"void SetupFixedConstants(double* CONSTANTS,double* RATES,"
    "double* STATES)\n{\n";
  code += frag;
  code += L"}\n";
  free(frag);

  /* rates      - All rates which are not static.
   */
  frag = cci->ratesString();
  code += L"void ComputeRates(double VOI,double* STATES,double* RATES,"
    L"double* CONSTANTS,double* ALGEBRAIC)\n{\n";
  code += frag;
  code += L"}\n";
  free(frag);

  /* variables  - All variables not computed by initConsts or rates
   *  (i.e., these are not required for the integration of the model and
   *   thus only need to be called for output or presentation or similar
   *   purposes)
   */
  frag = cci->variablesString();
  code += L"void EvaluateVariables(double VOI,double* CONSTANTS,"
    L"double* RATES, double* STATES, double* ALGEBRAIC)\n{\n";
  code += frag;
  code += L"}\n";
  free(frag);
  
  return(code);
} // writeCode

struct CellMLModel*
CreateCellMLModel(const char* mbrurl)
{
  // Get the URL from which to load the model
  wchar_t* URL = string2wstring(mbrurl);

  if (!URL) return((struct CellMLModel*)NULL);
  DEBUG(2,"CreateCellMLModel","model URI: %S\n",URL);

  struct CellMLModel* model =
    (struct CellMLModel*)malloc(sizeof(struct CellMLModel));

  model->uri = (char*)malloc(strlen(mbrurl)+1);
  strcpy(model->uri,mbrurl);
  
  // Need a CellML bootstrap to create the model loader
  iface::cellml_api::CellMLBootstrap* cb =
    CreateCellMLBootstrap();
  iface::cellml_api::ModelLoader* ml =
    cb->modelLoader();
  cb->release_ref();

  // Try and load the CellML model from the URL
  try
  {
    model->model = ml->loadFromURL(URL);
    /* and make sure it is fully instantiated */
    model->model->fullyInstantiateImports();
  }
  catch (...)
  {
    fprintf(stderr,"Error loading model URL: %S\n",URL);
    free(URL);
    ml->release_ref();
    free(model->uri);
    free(model);
    return((struct CellMLModel*)NULL);
  }
  // finished with the model loader now
  ml->release_ref();
  model->annotationSet = 0;
  /* create an AnnotationSet for use in keeping links between math and variable objects */
  RETURN_INTO_OBJREF(ats, iface::cellml_services::AnnotationToolService,
		  CreateAnnotationToolService());
  model->annotationSet = ats->createAnnotationSet();
  DEBUG(0, "CreateCellMLModel", "Created the annotation set\n");
  free(URL);
  return(model);
}

int DestroyCellMLModel(struct CellMLModel** model)
{
  if (*model)
  {
    (*model)->model->release_ref();
    if ((*model)->uri) free((*model)->uri);
    if ((*model)->annotationSet) (*model)->annotationSet->release_ref();
    free(*model);
    *model = (struct CellMLModel*)NULL;
    return(0);
  }
  return(1);
}

struct CellMLModel* cellMLModelClone(const struct CellMLModel* model)
{
  struct CellMLModel* c = (struct CellMLModel*)NULL;
  if (model && model->model)
  {
    c = (struct CellMLModel*)malloc(sizeof(struct CellMLModel));
    c->uri = getCellMLModelURI(model);
    c->model = model->model;
    c->model->add_ref();
  }
  else INVALID_ARGS("cellmlModelClone");
  return(c);
}

char* getCellMLModelId(const struct CellMLModel* model)
{
  char* id = (char*)NULL;
  if (model)
  {
    wchar_t* ID = model->model->cmetaId();
    id = wstring2string(ID);
    free(ID);
  }
  return(id);
}

char* getCellMLModelURI(const struct CellMLModel* model)
{
  char* uri = (char*)NULL;
  if (model && model->uri)
  {
    uri = (char*)malloc(strlen(model->uri)+1);
    strcpy(uri,model->uri);
  }
  return(uri);
}

char* getCellMLModelAsCCode(struct CellMLModel* model, void* outputVariables, int debugCode)
{
  char* code = (char*)NULL;
  if (model && model->model)
  {
    // Create a code generator and try to create code from the
    // CellML model. (defaults for code generator are to generate C code)
    iface::cellml_services::CodeGeneratorBootstrap* cgb =
      CreateCodeGeneratorBootstrap();
    iface::cellml_services::CodeGenerator* cg =
      cgb->createCodeGenerator();
    cgb->release_ref();

    /* we want to create and assign our own CeVAS so that we can
       easily fetch it back from the code generator */
    iface::cellml_services::CeVASBootstrap* cbs = CreateCeVASBootstrap();
    iface::cellml_services::CeVAS* cevas = 
      cbs->createCeVASForModel(model->model);
    cbs->release_ref();
    /* check for errors in the CeVAS */
    wchar_t* m = cevas->modelError();
    if (wcscmp(m, L""))
    {
      ERROR("getCellMLModelAsCCode","Error creating CeVAS: %S\n",m);
      free(m);
      cevas->release_ref();
      cg->release_ref();
      return(code);
    }
    free(m);
    cg->useCeVAS(cevas);
    cevas->release_ref();

    // Want to use our annotation set so that we get the custom annotations inside the code generation
    cg->useAnnoSet(model->annotationSet);

    if (debugCode)
    {
      /* we need to define a special MaLaES with the debug code */
      iface::cellml_services::MaLaESBootstrap* mbs = CreateMaLaESBootstrap();
      iface::cellml_services::MaLaESTransform* mt = mbs->compileTransformer(
L"opengroup: (\r\n"
L"closegroup: )\r\n"
L"abs: #prec[H]fabs(#expr1)\r\n"
L"and: #prec[20]#exprs[&&]\r\n"
L"arccos: #prec[H]acos(#expr1)\r\n"
L"arccosh: #prec[H]acosh(#expr1)\r\n"
L"arccot: #prec[1000(900)]atan(1.0/#expr1)\r\n"
L"arccoth: #prec[1000(900)]atanh(1.0/#expr1)\r\n"
L"arccsc: #prec[1000(900)]asin(1/#expr1)\r\n"
L"arccsch: #prec[1000(900)]asinh(1/#expr1)\r\n"
L"arcsec: #prec[1000(900)]acos(1/#expr1)\r\n"
L"arcsech: #prec[1000(900)]acosh(1/#expr1)\r\n"
L"arcsin: #prec[H]asin(#expr1)\r\n"
L"arcsinh: #prec[H]asinh(#expr1)\r\n"
L"arctan: #prec[H]atan(#expr1)\r\n"
L"arctanh: #prec[H]atanh(#expr1)\r\n"
L"ceiling: #prec[H]ceil(#expr1)\r\n"
L"cos: #prec[H]cos(#expr1)\r\n"
L"cosh: #prec[H]cosh(#expr1)\r\n"
L"cot: #prec[900(0)]1.0/tan(#expr1)\r\n"
L"coth: #prec[900(0)]1.0/tanh(#expr1)\r\n"
L"csc: #prec[900(0)]1.0/sin(#expr1)\r\n"
L"csch: #prec[900(0)]1.0/sinh(#expr1)\r\n"
L"diff: #lookupDiffVariable\r\n"
L"divide: #prec[900]#expr1/CHECK_DENOMINATOR(#expr2,__LINE__)\r\n"
L"eq: #prec[30]#exprs[==]\r\n"
L"exp: #prec[H]exp(#expr1)\r\n"
L"factorial: #prec[H]factorial(#expr1)\r\n"
L"factorof: #prec[30(900)]#expr1 % #expr2 == 0\r\n"
L"floor: #prec[H]floor(#expr1)\r\n"
L"gcd: #prec[H]gcd_multi(#count, #exprs[, ])\r\n"
L"geq: #prec[30]#exprs[>=]\r\n"
L"gt: #prec[30]#exprs[>]\r\n"
L"implies: #prec[10(950)] !#expr1 || #expr2\r\n"
L"int: #prec[H]defint(func#unique1, BOUND, CONSTANTS, RATES, VARIABLES, "
L"#bvarIndex)#supplement double func#unique1(double* BOUND, "
L"double* CONSTANTS, double* RATES, double* VARIABLES) { return #expr1; }\r\n"
L"lcm: #prec[H]lcm_multi(#count, #exprs[, ])\r\n"
L"leq: #prec[30]#exprs[<=]\r\n"
L"ln: #prec[H]log(#expr1)\r\n"
L"log: #prec[H]arbitrary_log(#expr1, #logbase)\r\n"
L"lt: #prec[30]#exprs[<]\r\n"
L"max: #prec[H]multi_max(#count, #exprs[, ])\r\n"
L"min: #prec[H]multi_min(#count, #exprs[, ])\r\n"
L"minus: #prec[500]#expr1 - #expr2\r\n"
L"neq: #prec[30]#expr1 != #expr2\r\n"
L"not: #prec[950]!#expr1\r\n"
L"or: #prec[10]#exprs[||]\r\n"
L"plus: #prec[500]#exprs[+]\r\n"
L"power: #prec[H]pow(#expr1, #expr2)\r\n"
L"quotient: #prec[900(0)] (int)(#expr1) / (int)(#expr2)\r\n"
L"rem: #prec[900(0)] (int)(#expr1) % (int)(#expr2)\r\n"
L"root: #prec[1000(900)] pow(#expr1, 1.0 / #degree)\r\n"
L"sec: #prec[900(0)]1.0 / cos(#expr1)\r\n"
L"sech: #prec[900(0)]1.0 / cosh(#expr1)\r\n"
L"sin: #prec[H] sin(#expr1)\r\n"
L"sinh: #prec[H] sinh(#expr1)\r\n"
L"tan: #prec[H] tan(#expr1)\r\n"
L"tanh: #prec[H] tanh(#expr1)\r\n"
L"times: #prec[900] #exprs[*]\r\n"
L"unary_minus: #prec[950]- #expr1\r\n"
L"units_conversion: #prec[500(900)]#expr1*#expr2 + #expr3\r\n"
L"units_conversion_factor: #prec[900]#expr1*#expr2\r\n"
L"units_conversion_offset: #prec[500]#expr1+#expr2\r\n"
L"xor: #prec[25(30)] (#expr1 != 0) ^ (#expr2 != 0)\r\n"
L"piecewise_first_case: #prec[5]#expr1 ? #expr2 : \r\n"
L"piecewise_extra_case: #prec[5]#expr1 ? #expr2 : \r\n"
L"piecewise_otherwise: #prec[5]#expr1\r\n"
L"piecewise_no_otherwise: #prec[5]0.0/0.0\r\n"
L"pi: #prec[999] 3.14159265358979\r\n"
L"eulergamma: #prec[999]0.577215664901533\r\n"
L"infinity: #prec[900]1.0/0.0\r\n"
        );
      cg->transform(mt);
      mt->release_ref();
    }
    else
    {
      /* keep this for now... */
#if defined (USE_LOCAL_MALAES)
    /* assign our own temporary MaLaES transform for testing */
    iface::cellml_services::MaLaESBootstrap* mbs = CreateMaLaESBootstrap();
    iface::cellml_services::MaLaESTransform* mt = mbs->compileTransformer(
L"opengroup: (\r\n"
L"closegroup: )\r\n"
L"abs: #prec[H]fabs(#expr1)\r\n"
L"and: #prec[20]#exprs[&&]\r\n"
L"arccos: #prec[H]acos(#expr1)\r\n"
L"arccosh: #prec[H]acosh(#expr1)\r\n"
L"arccot: #prec[1000(900)]atan(1.0/#expr1)\r\n"
L"arccoth: #prec[1000(900)]atanh(1.0/#expr1)\r\n"
L"arccsc: #prec[1000(900)]asin(1/#expr1)\r\n"
L"arccsch: #prec[1000(900)]asinh(1/#expr1)\r\n"
L"arcsec: #prec[1000(900)]acos(1/#expr1)\r\n"
L"arcsech: #prec[1000(900)]acosh(1/#expr1)\r\n"
L"arcsin: #prec[H]asin(#expr1)\r\n"
L"arcsinh: #prec[H]asinh(#expr1)\r\n"
L"arctan: #prec[H]atan(#expr1)\r\n"
L"arctanh: #prec[H]atanh(#expr1)\r\n"
L"ceiling: #prec[H]ceil(#expr1)\r\n"
L"cos: #prec[H]cos(#expr1)\r\n"
L"cosh: #prec[H]cosh(#expr1)\r\n"
L"cot: #prec[900(0)]1.0/tan(#expr1)\r\n"
L"coth: #prec[900(0)]1.0/tanh(#expr1)\r\n"
L"csc: #prec[900(0)]1.0/sin(#expr1)\r\n"
L"csch: #prec[900(0)]1.0/sinh(#expr1)\r\n"
L"diff: #lookupDiffVariable\r\n"
L"divide: #prec[900]#expr1/#expr2\r\n"
L"eq: #prec[30]#exprs[==]\r\n"
L"exp: #prec[H]exp(#expr1)\r\n"
L"factorial: #prec[H]factorial(#expr1)\r\n"
L"factorof: #prec[30(900)]#expr1 % #expr2 == 0\r\n"
L"floor: #prec[H]floor(#expr1)\r\n"
L"gcd: #prec[H]gcd_multi(#count, #exprs[, ])\r\n"
L"geq: #prec[30]#exprs[>=]\r\n"
L"gt: #prec[30]#exprs[>]\r\n"
L"implies: #prec[10(950)]!#expr1 || #expr2\r\n"
L"int: #prec[H]defint(func#unique1, BOUND, CONSTANTS, RATES, VARIABLES, #bvarIndex)#supplement double func#unique1(double* BOUND, double* CONSTANTS, double* RATES, double* VARIABLES) { return #expr1; }\r\n"
L"lcm: #prec[H]lcm_multi(#count, #exprs[, ])\r\n"
L"leq: #prec[30]#exprs[<=]\r\n"
L"ln: #prec[H]log(#expr1)\r\n"
L"log: #prec[H]arbitrary_log(#expr1, #logbase)\r\n"
L"lt: #prec[30]#exprs[<]\r\n"
L"max: #prec[H]multi_max(#count, #exprs[, ])\r\n"
L"min: #prec[H]multi_min(#count, #exprs[, ])\r\n"
L"minus: #prec[500]#expr1 - #expr2\r\n"
L"neq: #prec[30]#expr1 != #expr2\r\n"
L"not: #prec[950]!#expr1\r\n"
L"or: #prec[10]#exprs[||]\r\n"
L"plus: #prec[500]#exprs[+]\r\n"
L"power: #prec[H]pow(#expr1, #expr2)\r\n"
L"quotient: #prec[900(0)](int)(#expr1) / (int)(#expr2)\r\n"
L"rem: #prec[900(0)](int)(#expr1) % (int)(#expr2)\r\n"
L"root: #prec[1000(900)]pow(#expr1, 1.0 / #degree)\r\n"
L"sec: #prec[900(0)]1.0 / cos(#expr1)\r\n"
L"sech: #prec[900(0)]1.0 / cosh(#expr1)\r\n"
L"sin: #prec[H]sin(#expr1)\r\n"
L"sinh: #prec[H]sinh(#expr1)\r\n"
L"tan: #prec[H]tan(#expr1)\r\n"
L"tanh: #prec[H]tanh(#expr1)\r\n"
L"times: #prec[900]#exprs[*]\r\n"
L"unary_minus: #prec[950]-#expr1\r\n"
L"units_conversion: #prec[500(900)](___#expr1*#expr2 + #expr3___)\r\n"
L"units_conversion_offset: #prec[500](___#expr1+#expr2___)\r\n"
L"units_conversion_factor: #prec[900](___#expr1*#expr2___)\r\n"
L"xor: #prec[25(30)](#expr1 != 0) ^ (#expr2 != 0)\r\n"
L"piecewise_first_case: #prec[5]#expr1 ? #expr2 : \r\n"
L"piecewise_extra_case: #prec[5]#expr1 ? #expr2 : \r\n"
L"piecewise_otherwise: #prec[5]#expr1\r\n"
L"piecewise_no_otherwise: #prec[5]0.0/0.0\r\n"
L"pi: #prec[999] 3.14159265358979\r\n"
L"eulergamma: #prec[999]0.577215664901533\r\n"
L"infinity: #prec[900]1.0/0.0\r\n"
        );
    cg->transform(mt);
    mt->release_ref();
#endif /* defined (USE_LOCAL_MALAES) */
    }
    
    /* generate the code */
    iface::cellml_services::CodeInformation* cci = NULL;
    try
    {
      cci = cg->generateCode(model->model);
    }
    catch (iface::cellml_api::CellMLException& ce)
    {
      ERROR("getCellMLModelAsCCode",
        "Caught a CellMLException while generating code\n");
      cg->release_ref();
      return(code);
    }
    catch (...)
    {
      ERROR("getCellMLModelAsCCode",
        "Unexpected exception calling generateCode\n");
      cg->release_ref();
      return(code);
    }
    /* check for errors in generating the code */
    m = cci->errorMessage();
    if (wcscmp(m, L""))
    {
      ERROR("getCellMLModelAsCCode","Error generating code: %S\n",m);
      cci->release_ref();
      free(m);
      cg->release_ref();
      return(code);
    }
    free(m);
    
    DEBUG(2,"getCellMLModelAsCCode","Generated code\n");
    // We now have the code information, so check it
    if (checkCodeInformation(cci,debugCode) == 0)
    {
      DEBUG(2,"getCellMLModelAsCCode","Generated code looks ok\n");
      // create the C code with the generated code
      std::wstring Code = writeCode(cci,cg,outputVariables,debugCode);
      code = wstring2string(Code.c_str());
      /* and finished with this */
      cci->release_ref();
      cg->release_ref();
      DEBUG(1,"getCellMLModelAsCCode","Wrote the generated code\n");
    }
    else
    {
      ERROR("getCellMLModelAsCCode",
        "Something is wrong with the model code\n");
      cci->release_ref();
      cg->release_ref();
      return(code);
    }
  }
  return(code);
}

int cellMLModelIterateImports(struct CellMLModel* model,
  int (*func)(const char*,void*),void* user_data)
{
  int code = 0;
  if (model && model->model && func && user_data)
  {
    RETURN_INTO_OBJREF(cis,iface::cellml_api::CellMLImportSet,
      model->model->imports());
    RETURN_INTO_OBJREF(cii,iface::cellml_api::CellMLImportIterator,
      cis->iterateImports());
    while(true)
    {
      code = 1;
      RETURN_INTO_OBJREF(ci,iface::cellml_api::CellMLImport,
        cii->nextImport());
      if (ci == NULL) break;
      std::wstring importHref;
      GET_SET_URI(ci->xlinkHref(),importHref);
      std::wstring absURL = resolveAbsoluteURL(model->model,importHref);
      char* url = wstring2string(absURL.c_str());
      func(url,user_data);
      free(url);
    }
  }
  else INVALID_ARGS("cellMLModelIterateImports");
  return(code);
}

int cellMLModelPassThroughModel(struct CellMLModel* model,
  int (*func)(void*,void*,void*),void* user1,void* user2)
{
  int code = 0;
  if (model && model->model && func)
  {
    code = func((void*)(model->model),user1,user2);
  }
  else INVALID_ARGS("cellMLModelPassThroughModel");
  return(code);
}

#if defined (OLD_CODE)
int cellmlModelSetVariableInitialValue(struct CellMLModel* model,
  const char* cName,const char* vName,const char* val)
{
  int code = 0;
  if (model && model->model && cName && vName && val)
  {
    wchar_t* component = string2wstring(cName);
    wchar_t* variable = string2wstring(vName);
    wchar_t* value = string2wstring(val);
    
    iface::cellml_api::CellMLComponentSet* cs = model->model->allComponents();
    iface::cellml_api::CellMLComponent* c = cs->getComponent(component);
    cs->release_ref();
    if (c != NULL)
    {
      iface::cellml_api::CellMLVariableSet* vs = c->variables();
      c->release_ref();
      iface::cellml_api::CellMLVariable* v = vs->getVariable(variable);
      vs->release_ref();
      if (v != NULL)
      {
        v->initialValue(value);
        v->release_ref();
        code = 1;
        /*printf("Set the variable \"%S/%S\" to the value \"%S\"\n",component,
          variable,value);*/
      }
    }
    free(value);
    free(variable);
    free(component);
  }
  return(code);
}
#endif /* defined (OLD_CODE) */

#if defined (OLD_CODE)
/* Useful test to work out if we can handle a given file - returns non-zero
   if the document element in the given XML file at <uri> is in the namespace
   <ns> with the local name <name>. Returns zero otherwise. */
int xmlFileIs(const char* uri,const char* ns,const char* name)
{
  int code = 0;
  if (uri && ns && name)
  {
    wchar_t* URI = string2wstring(uri);
    wchar_t* NS = string2wstring(ns);
    wchar_t* NAME = string2wstring(name);
    iface::cellml_api::CellMLBootstrap* cb = CreateCellMLBootstrap();
    iface::cellml_api::DOMURLLoader* loader = cb->localURLLoader();
    iface::dom::Document* d = (iface::dom::Document*)NULL;
    try
    {
      d = loader->loadDocument(URI);
    }
    catch (...)
    {
      DEBUG(0,"xmlFileIs","unable to load the URI: %S\n",URI);
    }
    loader->release_ref();
    cb->release_ref();
    if (d)
    {
      iface::dom::Element* de = (iface::dom::Element*)NULL;
      try
      {
        de = d->documentElement();
      }
      catch (...)
      {
        DEBUG(0,"xmlFileIs","unable to get the document element\n");
      }
      d->release_ref();
      if (de)
      {
        wchar_t* n = (wchar_t*)NULL;
        wchar_t* s = (wchar_t*)NULL;
        try
        {
          n = de->localName();
          s = de->namespaceURI();
          DEBUG(3,"xmlFileIs","local name = \"%S\"; namespace URI = \"%S\"\n",
            n,s);
        }
        catch (...)
        {
          DEBUG(0,"xmlFileIs","unable to get the local name and namespace "
            "URI of the document element\n");          
        }
        de->release_ref();
        if (n && !wcscmp(n,NAME) && s && !wcscmp(s,NS)) code = 1;
        if (n) free(n);
        if (s) free(s);
      }
    }
    free(NAME);
    free(NS);
    free(URI);
  }
  return(code);
}
#endif /* defined (OLD_CODE) */

#if defined (OLD_CODE)
/* Get the contents of the given XML file as a standard C string */
char* getXMLFileContentsAsString(const char* uri)
{
  char* string = (char*)NULL;
  if (uri)
  {
    wchar_t* URI = string2wstring(uri);
    iface::cellml_api::CellMLBootstrap* cb = CreateCellMLBootstrap();
    iface::cellml_api::DOMURLLoader* loader = cb->localURLLoader();
    iface::dom::Document* d = (iface::dom::Document*)NULL;
    try
    {
      d = loader->loadDocument(URI);
    }
    catch (...)
    {
      DEBUG(0,"getXMLFileContentsAsString","unable to load the URI: %S\n",URI);
    }
    loader->release_ref();
    free(URI);
    if (d)
    {
      iface::dom::Element* de = (iface::dom::Element*)NULL;
      try
      {
        de = d->documentElement();
      }
      catch (...)
      {
        DEBUG(0,"xmlFileIs","unable to get the document element\n");
      }
      d->release_ref();
      if (de)
      {
        iface::dom::Node* node = dynamic_cast<iface::dom::Node*>(de);
        node->add_ref();
        de->release_ref();
        wchar_t* str = cb->serialiseNode(node);
        string = wstring2string(str);
        free(str);
        node->release_ref();
      }
#ifdef OLD_CODE
      DOMWriter dw;
      std::wstring str;
      dw.writeDocument(NULL,d,str);
      string = wstring2string(str.c_str());
      d->release_ref();
#endif
    }
    cb->release_ref();
  }
  return(string);
}
#endif /* defined (OLD_CODE) */

/* modified from CellML_DOM_API/sources/cellml/CellMLImplementation.cpp */
std::wstring
resolveAbsoluteURL(iface::cellml_api::Model* aModel,std::wstring aURL)
{
  // It may already be an absolute URL...
  if (aURL.find(L"://") != std::wstring::npos) return(aURL);

  // See if we can get an xml:base...
  std::wstring base;
  GET_SET_URI(aModel->base_uri(),base);

  // See if it is a '/' type URL...
  if (aURL[0] == L'/')
  {
    size_t pos = base.find(L"://");
    // Bail if we are trying to resolve relative to a relative URL...
    if (pos == std::wstring::npos) return(L"");

    // Assume protocol://host/path, where host may be zero length e.g. file:///
    pos = base.find(L"/", pos + 3);
    std::wstring absURL;
    if (pos == std::wstring::npos)
      absURL = base;
    else
      // Don't include the slash, only everything up to it...
      absURL = base.substr(0, pos);
    absURL += aURL;
    return(absURL);
  }

  // It is a completely relative URL.
  // See if base ends in a /...
  if (base[base.length() - 1] != L'/')
  {
    // aURL last component needs to be removed...
    size_t pos = base.rfind(L"/");
    if (pos == std::wstring::npos)
      base += L"/";
    else
      base = base.substr(0, pos + 1);
  }
  base += aURL;

  // Substitute [^/]*/../ => / and /./ => /
  size_t pos = base.find(L"://");
  aURL.assign(base.substr(0, pos + 3));

  std::list<std::wstring> pathComponents;
  size_t pos2;
  bool last = false;
  do
  {
    pos2 = base.find(L"/", pos + 1);
    if (pos2 == std::wstring::npos)
    {
      last = true;
      pos2 = base.length();
    }

    // Don't bother putting an empty path component for //
    if (pos2 != pos + 1)
    {
      std::wstring str = base.substr(pos + 1, pos2 - pos - 1);
      if (str == L"..")
      {
        if (!pathComponents.empty())
          pathComponents.pop_back();
      }
      else if (str == L".")
        ;
      else
        pathComponents.push_back(str);
    }
    pos = pos2;
  }
  while (!last);

  // Now go through the path components and make a path...
  std::list<std::wstring>::iterator i;
  for (i = pathComponents.begin(); i != pathComponents.end(); i++)
  {
    // only add in the '/' if required - i.e., http URL's should have a
    // host name before the path components really begin
    if (!((i == pathComponents.begin()) && (aURL.find(L"http://") == 0)))
      aURL += L'/';
    aURL += *i;
  }
  if (base[base.length() - 1] == '/')
    aURL += L'/';
  return(aURL);
}

// annotate the output variable's sources so that we can find them in the list of computation targets later on...
void annotateCellMLModelOutputs(struct CellMLModel* model, void* outputVariables)
{
	RETURN_INTO_OBJREF(localComponents, iface::cellml_api::CellMLComponentSet,
			model->model->localComponents());
	DEBUG(0, "annotateCellMLModelOutputs", "Got the local components\n");
	int l = outputVariablesGetLength(outputVariables);
	for (int i = 0; i < l; ++i)
	{
		RETURN_INTO_WSTRING(cname, string2wstring(outputVariablesGetComponent(outputVariables, i)));
		RETURN_INTO_OBJREF(component, iface::cellml_api::CellMLComponent,
				localComponents->getComponent(cname.c_str()));
		if (component)
		{
			DEBUG(0, "annotateCellMLModelOutputs", "Got the local component: %s\n", outputVariablesGetComponent(outputVariables, i));
			RETURN_INTO_WSTRING(vname, string2wstring(outputVariablesGetVariable(outputVariables, i)));
			RETURN_INTO_OBJREF(variables, iface::cellml_api::CellMLVariableSet, component->variables());
			RETURN_INTO_OBJREF(variable, iface::cellml_api::CellMLVariable,
					variables->getVariable(vname.c_str()));
			if (variable)
			{
				DEBUG(0, "annotateCellMLModelOutputs", "Got the variable: %s\n", outputVariablesGetVariable(outputVariables, i));
				RETURN_INTO_OBJREF(src, iface::cellml_api::CellMLVariable, variable->sourceVariable());
				DEBUG(0, "annotateCellMLModelOutputs", "Got the source variable\n");
				wchar_t column[128];
				swprintf(column, 128, L"%d\0", outputVariablesGetColumn(outputVariables, i));
				model->annotationSet->setStringAnnotation(src, L"CSim::OutputColumn", column);
			}
		}
	}
}
