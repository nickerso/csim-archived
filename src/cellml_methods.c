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
#include <dlfcn.h>

#include "common.h"
#include "utils.h"
#include "cellml_methods.h"

/* Private function to check function return values */
static int check_method(void *method,char *funcname)
{
  if (method == NULL)
  {
    ERROR("check_method","Error getting the %s method\n",funcname);
    return(ERR);
  }
  return(OK);
}

/* Public methods */
struct CellMLMethods* CreateCellMLMethods(const char* soFileName)
{
  int err = OK;
  struct CellMLMethods* m =
    (struct CellMLMethods*)malloc(sizeof(struct CellMLMethods));
  if (soFileName == NULL)
  {
    ERROR("CreateCellMLMethods","Invalid shared object file name\n");
    DestroyCellMLMethods(&m);
    return((struct CellMLMethods*)NULL);
  }
  /* Open the shared object with the model */
  m->soHandle = dlopen(soFileName,RTLD_LOCAL|RTLD_LAZY);
  if (m->soHandle == NULL)
  {
    ERROR("CreateCellMLMethods","Error opening shared object (%s): %s\n",
      soFileName,dlerror());
    DestroyCellMLMethods(&m);
    return((struct CellMLMethods*)NULL);
  }
  /* find the required methods */
  m->getNbound = (int (*)())dlsym(m->soHandle,"getNbound");
  if (check_method(m->getNbound,"getNbound") == ERR) err = ERR;
  m->getNrates = (int (*)())dlsym(m->soHandle,"getNrates");
  if (check_method(m->getNrates,"getNrates") == ERR) err = ERR;
  m->getNalgebraic = (int (*)())dlsym(m->soHandle,"getNalgebraic");
  if (check_method(m->getNalgebraic,"getNalgebraic") == ERR) err = ERR;
  m->getNconstants = (int (*)())dlsym(m->soHandle,"getNconstants");
  if (check_method(m->getNconstants,"getNconstants") == ERR) err = ERR;
  m->getNoutputs = (int (*)())dlsym(m->soHandle,"getNoutputs");
  if (check_method(m->getNoutputs,"getNoutputs") == ERR) err = ERR;

#if 0
  m->getStateVariableIDs = (const char* (*)(int))
    dlsym(m->soHandle,"getStateVariableIDs");
  if (check_method(m->getStateVariableIDs,
      "getStateVariableIDs") == ERR) err = ERR;
  m->getConstantVariableIDs = (const char* (*)(int))
    dlsym(m->soHandle,"getConstantVariableIDs");
  if (check_method(m->getConstantVariableIDs,
      "getConstantVariableIDs") == ERR) err = ERR;
  m->getAlgebraicVariableIDs = (const char* (*)(int))
    dlsym(m->soHandle,"getAlgebraicVariableIDs");
  if (check_method(m->getAlgebraicVariableIDs,
      "getAlgebraicVariableIDs") == ERR) err = ERR;
  m->getVariableOfIntegrationIDs = (const char* (*)())
    dlsym(m->soHandle,"getVariableOfIntegrationIDs");
  if (check_method(m->getVariableOfIntegrationIDs,
      "getVariableOfIntegrationIDs") == ERR) err = ERR;
#endif

  m->SetupFixedConstants = (void (*)(double*,double*,double*))
    dlsym(m->soHandle,"SetupFixedConstants");
  if (check_method(m->SetupFixedConstants,
      "SetupFixedConstants") == ERR) err = ERR;
  m->ComputeRates = (void (*)(double,double*,double*,double*,double*))
    dlsym(m->soHandle,"ComputeRates");
  if (check_method(m->ComputeRates,"ComputeRates") == ERR) err = ERR;
  m->EvaluateVariables = (void (*)(double,double*,double*,double*,double*))
    dlsym(m->soHandle,"EvaluateVariables");
  if (check_method(m->EvaluateVariables,"EvaluateVariables") == ERR) err = ERR;
  m->GetOutputs = (void (*)(double,double*,double*,double*,double*))
      dlsym(m->soHandle,"GetOutputs");
  if (check_method(m->GetOutputs,"GetOutputs") == ERR) err = ERR;
  if (err == ERR)
  {
    DestroyCellMLMethods(&m);
    return((struct CellMLMethods*)NULL);
  }
  return(m);
}

int DestroyCellMLMethods(struct CellMLMethods** m_ptr)
{
  struct CellMLMethods* m = *m_ptr;
  if (m)
  {
    if (m->soHandle)
    {
      if (dlclose(m->soHandle) != 0)
      {
        ERROR("DestroyCellMLMethods",
          "Error closing shared object: %s\n",dlerror());
        return(ERR);
      }
    }
    free(m);
  }
  *m_ptr = (struct CellMLMethods*)NULL;
  return(OK);
}
