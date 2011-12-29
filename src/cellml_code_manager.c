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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "utils.h"
#include "cellml.h"
#include "cellml_code_manager.h"
#include "cellml_methods.h"
#include "simulation.h"

int PauseForCodeChanges = 0;

struct Code
{
  int saveFiles;
  char* modelURI;
  char* codeFileName;
  int codeFileExists;
  char* dsoFileName;
  int dsoFileExists;
  char* tmpDirName;
  int tmpDirExists;
};

int DestroyCode(struct Code** _ptr)
{
  int c = ERR;
  struct Code* code = *_ptr;
  if (code)
  {
    DEBUG(1,"DestroyCode","Destroying code for model: %s\n",code->modelURI);
    if (code->saveFiles) fprintf(stderr,
      "Leaving generated code for model: %s\n",code->modelURI);
    if (code->codeFileName)
    {
      if (code->codeFileExists && !code->saveFiles) unlink(code->codeFileName);
      else if (code->codeFileExists) fprintf(stderr,
        "Leaving generated code file: %s\n",code->codeFileName);
      free(code->codeFileName);
    }
    if (code->dsoFileName)
    {
      if (code->dsoFileExists && !code->saveFiles) unlink(code->dsoFileName);
      else if (code->dsoFileExists) fprintf(stderr,
        "Leaving generated shared object file: %s\n",code->dsoFileName);
      free(code->dsoFileName);
    }
    if (code->tmpDirName)
    {
      if (code->tmpDirExists && !code->saveFiles) rmdir(code->tmpDirName);
      else if (code->tmpDirExists) fprintf(stderr,
        "Leaving generated temporary directory: %s\n",code->tmpDirName);
      free(code->tmpDirName);
    }
    if (code->modelURI) free(code->modelURI);
    free(code);
    *_ptr = (struct Code*)NULL;
    c = OK;
  }
  else DEBUG(0,"DestroyCode","Invalid argument(s)\n");
  return(c);
}

struct Code* CreateCode(struct Simulation* simulation,const char* modelURI,int save,const char* gcc,
  int generateDebugCode)
{
  struct Code* code = (struct Code*)NULL;
  if (modelURI)
  {
    code = (struct Code*)malloc(sizeof(struct Code));
    /* initialise */
    code->saveFiles = save;
    code->modelURI = strcopy(modelURI);
    code->codeFileName = (char*)NULL;
    code->dsoFileName = (char*)NULL;
    code->tmpDirName = (char*)NULL;
    code->codeFileExists = 0;
    code->dsoFileExists = 0;
    code->tmpDirExists = 0;
    /* parse the model */
    struct CellMLModel* cellmlModel = CreateCellMLModel(modelURI);
    if (cellmlModel)
    {
    	annotateCellMLModelOutputs(cellmlModel, simulationGetOutputVariables(simulation));
      /* generate some code */
      char* code_string = getCellMLModelAsCCode(cellmlModel,simulationGetOutputVariables(simulation),
    		  generateDebugCode);
      if (code_string)
      {
        DEBUG(1,"CreateCode",
          "Successfully generated code from CellML model\n");
        /* We have code, so dump it out to a temporary file in a temporary
           directory so we can have the compiled object nice and handy to
           delete */
        char template[64] = "tmp.cellml2code.XXXXXX";
        if (!mkdtemp(template))
	{
	  /* error creating temp directory - picked up by Jaunty compilers */
	  /* FIXME: all the following should be dependent on success */
	}
	code->tmpDirName = strcopy(template);
        code->tmpDirExists = 1;
        sprintf(template,"%s/cellml2code.XXXXXX",code->tmpDirName);
        int tmpFileDes = mkstemp(template);
        code->codeFileName = strcopy(template);
        code->codeFileExists = 1;
        FILE* cFile = fdopen(tmpFileDes,"w");
        fprintf(cFile,"%s",code_string);
        fclose(cFile);
        code->dsoFileName = (char*)malloc(strlen(code->codeFileName)+10);
        /* need to make the dso name right so that it'll load */
        sprintf(code->dsoFileName,"%s%s.so",(template[0]=='/'?"":"./"),
          code->codeFileName);
        /* compile the code into a shared object */
        /*const char* gcc = "gcc -O3 -shared -x c -o ";*/
        char* compileCommand =
          (char*)malloc(strlen(gcc)+strlen(code->dsoFileName)+
            strlen(code->codeFileName)+3);
        sprintf(compileCommand,"%s %s %s",gcc,code->dsoFileName,
          code->codeFileName);
        DEBUG(1,"CreateCode","Compile command: \"%s\"\n",compileCommand);
        if (PauseForCodeChanges)
        {
          printf("=> Pausing to allow for changes to be made to the"
            " generated source file:\n\t%s\n   Press enter to continue...",
            code->codeFileName);
          getchar();
        }
        if (system(compileCommand) == 0)
        {
          code->dsoFileExists = 1;
        }
        else
        {
          ERROR("CreateCode","Error compiling the code into a shared object");
          DestroyCode(&code);
        }
        free(compileCommand);
        free(code_string);
      }
      else
      {
        ERROR("CreateCode","Error generating code from the CellML model\n");
        DestroyCode(&code);
      }
      DestroyCellMLModel(&cellmlModel);
    }
  }
  else DEBUG(0,"CreateCode","Invalid arguments\n");
  return(code);
}

struct CellMLCodeManager
{
  int generateDebugCode;
  int saveTempFiles;
  char* cCompiler;
  struct Code** codes;
  int N;
};

struct CellMLCodeManager* CreateCellMLCodeManager(int saveFiles,
  const char* cCompiler,int generateDebugCode)
{
  struct CellMLCodeManager* manager =
    (struct CellMLCodeManager*)malloc(
      sizeof(struct CellMLCodeManager));
  if (manager)
  {
    manager->generateDebugCode = generateDebugCode;
    manager->saveTempFiles = saveFiles;
    manager->cCompiler = strcopy(cCompiler);
    manager->codes = (struct Code**)NULL;
    manager->N = 0;
  }
  else ERROR("CreateCellMLManager","Error allocating memory\n");
  return(manager);
}

int DestroyCellMLCodeManager(struct CellMLCodeManager** _ptr)
{
  int i,c = ERR;
  struct CellMLCodeManager* manager = *_ptr;
  if (manager)
  {
    if (manager->codes)
    {
      for (i=0;i<manager->N;i++) DestroyCode(&(manager->codes[i]));
      free(manager->codes);
    }
    if (manager->cCompiler) free(manager->cCompiler);
    free(manager);
    *_ptr = (struct CellMLCodeManager*)NULL;
    c = OK;
  }
  else DEBUG(0,"DestroyCellMLCodeManager","Invalid argument(s)\n");
  return(c);
}

struct CellMLMethods* cellmlCodeManagerGetMethodsForSimulation(
  struct CellMLCodeManager* manager,struct Simulation* simulation)
{
  struct CellMLMethods* methods = (struct CellMLMethods*)NULL;
  if (manager && simulation)
  {
	char* modelURI = simulationGetModelURI(simulation);
	char* uri = getURIFromURIWithFragmentID(modelURI);
    /* first look for a code that matches the given model URI */
    if (manager->codes)
    {
      int i;
      for (i=0;i<manager->N;i++)
      {
        if (strcmp(uri,manager->codes[i]->modelURI) == 0)
        {
          if (manager->codes[i]->dsoFileExists)
          {
            methods = CreateCellMLMethods(manager->codes[i]->dsoFileName);
            DEBUG(1,"cellmlCodeManagerGetMethodsForModel","Found existing "
              "code for model: %s\n",uri);
          }
          else ERROR("cellmlCodeManagerGetMethodsForModel","Found code for "
            "model but shared object does not exist\n");
        }
      }
    }
    if (methods == NULL)
    {
      DEBUG(1,"cellmlCodeManagerGetMethodsForModel","Did not find existing "
        "model code, so making a new one for model: %s\n",uri);
      /* didn't find the code for the model */
      struct Code* code = CreateCode(simulation,uri,manager->saveTempFiles,
        manager->cCompiler,manager->generateDebugCode);
      if (code)
      {
        manager->N++;
        manager->codes = (struct Code**)
          realloc(manager->codes,sizeof(struct Code*)*(manager->N));
        manager->codes[manager->N-1] = code;
        methods = CreateCellMLMethods(code->dsoFileName);
        DEBUG(1,"cellmlCodeManagerGetMethodsForModel",
          "Created CellMLMethods for new code\n");
      }
      else ERROR("cellmlCodeManagerGetMethodsForModel","Error creating code "
        "for the model: %s\n",uri);
    }
  }
  else DEBUG(0,"cellmlCodeManagerGetMethodsForModel","Invalid argument(s)\n");
  return(methods);
}
