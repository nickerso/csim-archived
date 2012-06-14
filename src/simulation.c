
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "utils.h"
#include "timer.h"
#include "simulation.h"
#include "outputVariables.h"

#ifdef _MSC_VER
#  define strcasecmp _stricmp
#endif

#define INVALID_MM_STRING "invalid Multistep Method"
#define INVALID_IM_STRING "invalid Iteration Method"
#define INVALID_LS_STRING "invalid Linear Solver"

struct Simulation
{
  char* uri;
  char* id; /* this is the modified uri to be compatible as a HDF identifier */
  char* name;
  char* modelURI; /* might be useful? */
  /* Bound variable information -
     FIXME: need to be able to handle multiple
     boundIntervals as given in the simulation metadata specification */
  char* bVarURI;
  double start;   /* start value */
  int startSet;
  double end;     /* end value for the integration */
  int endSet;
  double tabStep; /* tabulation step size */
  int tabStepSet;
  double maxStep; /* maximum step size */
  int maxStepSet;
  /* Numerical solution methods to use */
  enum MultistepMethod lmm;
  enum IterationMethod iter;
  enum LinearSolver solver;
  /* Numerical tolerances to use */
  double* aTol;
  int aTolLength;
  double rTol;
  int rTolSet;
  /* the output variables for this simulation */
  void* outputVariables;
};

struct Simulation* CreateSimulation()
{
  struct Simulation* sim = (struct Simulation*)
    malloc(sizeof(struct Simulation));
  sim->uri = (char*)NULL;
  sim->id = (char*)NULL;
  sim->name = (char*)NULL;
  sim->modelURI = (char*)NULL;
  sim->bVarURI = (char*)NULL;
  sim->start = 0;
  sim->startSet = 0;
  sim->end = 0;
  sim->endSet = 0;
  sim->tabStep = 0;
  sim->tabStepSet = 0;
  sim->maxStep = 0;
  sim->maxStepSet = 0;
  /* useful default? */
  sim->lmm = ADAMS;
  sim->iter = FUNCTIONAL;
  sim->solver = NONE;
  sim->aTol = (double*)NULL;
  sim->aTolLength = 0;
  sim->rTol = 0;
  sim->rTolSet = 0;

  /* Until this gets added to the metadata set the default tolerances here */
  double rtol = 1.0e-6;
  double atol = 1.0e-8;
  simulationSetRTol(sim,rtol);
  simulationSetATol(sim,1,&atol);
  sim->outputVariables = NULL;
  return(sim);
}

struct Simulation* simulationClone(const struct Simulation* src)
{
  if (src)
  {
    struct Simulation* sim = CreateSimulation();
    if (src->uri) simulationSetURI(sim,src->uri);
    if (src->id) simulationSetID(sim,src->id);
    if (src->name) simulationSetName(sim,src->name);
    if (src->modelURI) simulationSetModelURI(sim,src->modelURI);
    if (src->bVarURI) simulationSetBvarURI(sim,src->bVarURI);
    sim->start = src->start;
    sim->end = src->end;
    sim->tabStep = src->tabStep;
    sim->maxStep = src->maxStep;
    sim->startSet = src->startSet;
    sim->endSet = src->endSet;
    sim->tabStepSet = src->tabStepSet;
    sim->maxStepSet = src->maxStepSet;
    sim->lmm = src->lmm;
    sim->iter = src->iter;
    sim->solver = src->solver;
    if (src->aTolLength > 0)
    {
      simulationSetATol(sim,src->aTolLength,src->aTol);
    }
    sim->rTol = src->rTol;
    sim->outputVariables = outputVariablesClone(src->outputVariables);
    return(sim);
  }
  return((struct Simulation*)NULL);
}

int DestroySimulation(struct Simulation** sim_ptr)
{
  struct Simulation* sim = *sim_ptr;
  if (sim)
  {
    if (sim->uri) free(sim->uri);
    if (sim->id) free(sim->id);
    if (sim->name) free(sim->name);
    if (sim->modelURI) free(sim->modelURI);
    if (sim->bVarURI) free(sim->bVarURI);
    if (sim->aTol) free(sim->aTol);
    outputVariablesDestroy(sim->outputVariables);
    free(sim);
    *sim_ptr = (struct Simulation*)NULL;
    return(OK);
  }
  return(ERR);
}

int simulationSetModelURI(struct Simulation* sim,const char* s)
{
  if (sim)
  {
    if (((sim->modelURI)=(char*)realloc(sim->modelURI,strlen(s)+1)))
    {
      strcpy(sim->modelURI,s);
      return(OK);
    }
  }
  return(ERR);
}

char* simulationGetModelURI(struct Simulation* sim)
{
  char* s = (char*)NULL;
  if (sim && sim->modelURI)
  {
    s = (char*)malloc(strlen(sim->modelURI)+1);
    strcpy(s,sim->modelURI);
  }
  return(s);
}

int simulationSetURI(struct Simulation* sim,const char* s)
{
  if (sim)
  {
    if (sim->uri) free(sim->uri);
    if ((sim->uri = strcopy(s))) return(OK);
  }
  return(ERR);
}

char* simulationGetURI(struct Simulation* sim)
{
  char* s = (char*)NULL;
  if (sim && sim->uri)
  {
    s = strcopy(sim->uri);
  }
  return(s);
}

int simulationSetID(struct Simulation* sim,const char* s)
{
  if (sim)
  {
    if (sim->id) free(sim->id);
    if ((sim->id = strcopy(s))) return(OK);
  }
  return(ERR);
}

char* simulationGetID(struct Simulation* sim)
{
  char* s = (char*)NULL;
  if (sim && sim->id)
  {
    s = strcopy(sim->id);
  }
  return(s);
}

int simulationSetName(struct Simulation* sim,const char* s)
{
  if (sim)
  {
    if (sim->name) free(sim->name);
    if (s)
    {
      sim->name = strcopy(s);
      return(OK);
    }
  }
  return(ERR);
}

char* simulationGetName(struct Simulation* sim)
{
  char* s = (char*)NULL;
  if (sim && sim->name)
  {
    s = (char*)malloc(strlen(sim->name)+1);
    strcpy(s,sim->name);
  }
  return(s);
}

int simulationSetBvarURI(struct Simulation* sim,const char* s)
{
  if (sim)
  {
    if (sim->bVarURI) free(sim->bVarURI);
    if ((sim->bVarURI = strcopy(s))) return(OK);
  }
  return(ERR);
}

char* simulationGetBvarURI(struct Simulation* sim)
{
  char* s = (char*)NULL;
  if (sim && sim->bVarURI)
  {
    s = strcopy(sim->bVarURI);
  }
  return(s);
}

int simulationSetBvarStart(struct Simulation* sim,double value)
{
  if (sim)
  {
    sim->start = value;
    sim->startSet = 1;
    return(OK);
  }
  return(ERR);
}

int simulationSetBvarEnd(struct Simulation* sim,double value)
{
  if (sim)
  {
    sim->end = value;
    sim->endSet = 1;
    return(OK);
  }
  return(ERR);
}

int simulationSetBvarTabStep(struct Simulation* sim,double value)
{
  if (sim)
  {
    sim->tabStep = value;
    sim->tabStepSet = 1;
    return(OK);
  }
  return(ERR);
}

int simulationSetBvarMaxStep(struct Simulation* sim,double value)
{
  if (sim)
  {
    sim->maxStep = value;
    sim->maxStepSet = 1;
    return(OK);
  }
  return(ERR);
}

int simulationSetMultistepMethod(struct Simulation* sim,
  enum MultistepMethod lmm)
{
  if (sim)
  {
    sim->lmm = lmm;
    return(OK);
  }
  return(ERR);
}

int simulationSetIterationMethod(struct Simulation* sim,
  enum IterationMethod iter)
{
  if (sim)
  {
    sim->iter = iter;
    return(OK);
  }
  return(ERR);
}

int simulationSetLinearSolver(struct Simulation* sim,enum LinearSolver solver)
{
  if (sim)
  {
    sim->solver = solver;
    return(OK);
  }
  return(ERR);
}

int simulationSetATol(struct Simulation* sim,int n,double* tol)
{
  if (sim && (n > 0))
  {
    if (sim->aTol) free(sim->aTol);
    sim->aTol = (double*)malloc(sizeof(double)*n);
    memcpy(sim->aTol,tol,sizeof(double)*n);
    sim->aTolLength = n;
    return(OK);
  }
  return(ERR);
}

int simulationSetRTol(struct Simulation* sim,double tol)
{
  if (sim)
  {
    sim->rTol = tol;
    sim->rTolSet = 1;
    return(OK);
  }
  return(ERR);
}

double simulationGetBvarStart(struct Simulation* sim)
{
  if (sim) return(sim->start);
  return(0.0);
}

double simulationGetBvarEnd(struct Simulation* sim)
{
  if (sim) return(sim->end);
  return(0.0);
}

double simulationGetBvarTabStep(struct Simulation* sim)
{
  if (sim) return(sim->tabStep);
  return(0.0);
}

double simulationGetBvarMaxStep(struct Simulation* sim)
{
  if (sim) return(sim->maxStep);
  return(0.0);
}

enum MultistepMethod simulationGetMultistepMethod(struct Simulation* sim)
{
  if (sim) return(sim->lmm);
  return(INVALID_MM);
}

enum IterationMethod simulationGetIterationMethod(struct Simulation* sim)
{
  if (sim) return(sim->iter);
  return(INVALID_IM);
}

enum LinearSolver simulationGetLinearSolver(struct Simulation* sim)
{
  if (sim) return(sim->solver);
  return(INVALID_LS);
}

double* simulationGetATol(struct Simulation* sim)
{
  if (sim && sim->aTol && ((sim->aTolLength)>0))
  {
    size_t l = sizeof(double)*(sim->aTolLength);
    double* tol = (double*)malloc(l);
    memcpy(tol,sim->aTol,l);
    return(tol);
  }
  return((double*)NULL);
}

int simulationGetATolLength(struct Simulation* sim)
{
  if (sim) return(sim->aTolLength);
  return(-1);
}

double simulationGetRTol(struct Simulation* sim)
{
  if (sim) return(sim->rTol);
  return(0.0);
}

/* convenience methods */
const char* multistepMethodToString(enum MultistepMethod lmm)
{
  switch (lmm)
  {
    /*case EULER: return "Euler";*/
    case ADAMS: return "Adams";
    case BDF: return "BDF";
    default: return INVALID_MM_STRING;
  }
}

const char* iterationMethodToString(enum IterationMethod iter)
{
  switch (iter)
  {
    case FUNCTIONAL: return "Functional";
    case NEWTON: return "Newton";
    default: return INVALID_IM_STRING;
  }
}

const char* linearSolverToString(enum LinearSolver solver)
{
  switch (solver)
  {
    case DENSE: return "Dense";
    case BAND: return "Band";
    case DIAG: return "Diagonal";
    case SPGMR: return "SPGMR";
    case SPBCG: return "SPBCG";
    case SPTFQMR: return "SPTFQMR";
    case NONE: return "None";
    default: return INVALID_LS_STRING;
  }
}

enum MultistepMethod multistepMethodFromString(const char* lmm)
{
  if (strcasecmp(lmm,"Adams") == 0) return(ADAMS);
  else if (strcasecmp(lmm,"BDF") == 0) return(BDF);
  return(INVALID_MM);
}

enum IterationMethod iterationMethodFromString(const char* iter)
{
  if (strcasecmp(iter,"Functional") == 0) return(FUNCTIONAL);
  else if (strcasecmp(iter,"Newton") == 0) return(NEWTON);
  return(INVALID_IM);
}

enum LinearSolver linearSolverFromString(const char* solver)
{
  if (strcasecmp(solver,"Dense") == 0) return(DENSE);
  else if (strcasecmp(solver,"Band") == 0) return(BAND);
  else if (strcasecmp(solver,"Diagonal") == 0) return(DIAG);
  else if (strcasecmp(solver,"SPGMR") == 0) return(SPGMR);
  else if (strcasecmp(solver,"SPBCG") == 0) return(SPBCG);
  else if (strcasecmp(solver,"SPTFQMR") == 0) return(SPTFQMR);
  else if (strcasecmp(solver,"NONE") == 0) return(NONE);
  return(INVALID_LS);
}

int simulationPrint(struct Simulation* s,FILE* f,const char* indent)
{
  int i,code = ERR;
  if (s && f)
  {
    fprintf(f,"%sSimulation start\n",indent);
    fprintf(f,"%s  URI: %s\n",indent,(s->uri)?s->uri:"UNSET");
    fprintf(f,"%s  ID: %s\n",indent,(s->id)?s->id:"UNSET");
    fprintf(f,"%s  name: %s\n",indent,(s->name)?s->name:"UNSET");
    fprintf(f,"%s  CellML model: %s\n",indent,
      (s->modelURI)?s->modelURI:"UNSET");
    fprintf(f,"%s  multistep method: %s\n",indent,
      multistepMethodToString(s->lmm));
    fprintf(f,"%s  iteration method: %s\n",indent,
      iterationMethodToString(s->iter));
    fprintf(f,"%s  linear solver: %s\n",indent,
      linearSolverToString(s->solver));
    fprintf(f,"%s  bound variable interval:\n",indent);
    fprintf(f,"%s    variable: %s\n",indent,
      (s->bVarURI)?s->bVarURI:"UNSET");
    fprintf(f,"%s    start: ",indent);
    if (s->startSet) fprintf(f,REAL_FORMAT,s->start);
    else fprintf(f,"UNSET");
    fprintf(f,"\n");
    fprintf(f,"%s    end: ",indent);
    if (s->endSet) fprintf(f,REAL_FORMAT,s->end);
    else fprintf(f,"UNSET");
    fprintf(f,"\n");
    fprintf(f,"%s    maximum step: ",indent);
    if (s->maxStepSet) fprintf(f,REAL_FORMAT,s->maxStep);
    else fprintf(f,"UNSET");
    fprintf(f,"\n");
    fprintf(f,"%s    tabulation step: ",indent);
    if (s->tabStepSet) fprintf(f,REAL_FORMAT,s->tabStep);
    else fprintf(f,"UNSET");
    fprintf(f,"\n");
    fprintf(f,"%s  numerical tolerances:\n",indent);
    fprintf(f,"%s    relative: ",indent);
    if (s->rTolSet) fprintf(f,REAL_FORMAT,s->rTol);
    else fprintf(f,"UNSET");
    fprintf(f,"\n");
    fprintf(f,"%s    absolute:",indent);
    if (s->aTol) for(i=0;i<s->aTolLength;i++) fprintf(f,
      " "REAL_FORMAT,s->aTol[i]);
    else fprintf(f,"UNSET");
    fprintf(f,"\n");
    if (s->outputVariables) outputVariablesPrint(s->outputVariables, f, indent);
    fprintf(f,"%sSimulation end\n",indent);
    code = OK;
  }
  return(code);
}

int simulationIsBvarStartSet(struct Simulation* sim)
{
  if (sim && sim->startSet) return(1);
  return(0);
}

int simulationIsBvarEndSet(struct Simulation* sim)
{
  if (sim && sim->endSet) return(1);
  return(0);
}

int simulationIsBvarMaxStepSet(struct Simulation* sim)
{
  if (sim && sim->maxStepSet) return(1);
  return(0);
}

int simulationIsBvarTabStepSet(struct Simulation* sim)
{
  if (sim && sim->tabStepSet) return(1);
  return(0);
}

int simulationIsRTolSet(struct Simulation* sim)
{
  if (sim && sim->rTolSet) return(1);
  return(0);
}

int simulationIsValidDescription(struct Simulation* simulation)
{
  if (simulation)
  {
    if (simulation->uri == NULL) return(0);
    if (simulation->id == NULL) return(0);
    if (simulation->modelURI == NULL) return(0);
    if (simulation->bVarURI == NULL) return(0);
    if (!simulationIsBvarStartSet(simulation)) return(0);
    if (!simulationIsBvarEndSet(simulation)) return(0);
    if (!simulationIsBvarMaxStepSet(simulation)) return(0);
    if (!simulationIsBvarTabStepSet(simulation)) return(0);
    if (!simulationIsRTolSet(simulation)) return(0);
    if (simulationGetATolLength(simulation) < 1) return(0);
    enum IterationMethod im = simulationGetIterationMethod(simulation);
    if (strcmp(
          multistepMethodToString(simulationGetMultistepMethod(simulation)),
          INVALID_MM_STRING) == 0) return(0);
    if (strcmp(iterationMethodToString(im),INVALID_IM_STRING) == 0) return(0);
    if (im == NEWTON)
    {
      /* linear solver required */
      if (strcmp(
            linearSolverToString(simulationGetLinearSolver(simulation)),
            INVALID_LS_STRING) == 0) return(0);
    }
    if (simulation->outputVariables == NULL) return 0;
    return(1);
  }
  return(0);
}

int simulationSetOutputVariables(struct Simulation* simulation, void* outputVariables)
{
	outputVariablesDestroy(simulation->outputVariables);
	simulation->outputVariables = outputVariablesClone(outputVariables);
	return 1;
}

void* simulationGetOutputVariables(struct Simulation* simulation)
{
	return simulation->outputVariables;
}
