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
#include <math.h>

/* Header files with a description of contents used in cvsdenx.c */
#include <cvodes/cvodes.h>         /* prototypes for CVODES fcts. and consts. */
#include <nvector/nvector_serial.h> /* serial N_Vector types, fcts., and macros */
#include <cvodes/cvodes_dense.h>   /* prototype for CVDense */
#include <cvodes/cvodes_band.h>    /* prototype for CVBand */
#include <cvodes/cvodes_diag.h>    /* prototype for CVDiag */
#include <cvodes/cvodes_spgmr.h>   /* prototypes & consts. for CVSPGMR solver */
#include <cvodes/cvodes_spbcgs.h>  /* prototypes & consts. for CVSPBCG solver */
#include <cvodes/cvodes_sptfqmr.h> /* prototypes & consts. for CVSPTFQMR solver */
#include <sundials/sundials_dense.h> /* definitions DenseMat and DENSE_ELEM */

#include "common.h"
#include "utils.h"
#include "integrator.h"
#include "simulation.h"
#include "integrator_user_data.h"
#include "cellml_methods.h"

#include "ccgs_required_functions.h"

/* FIXME: Temporary? */
#ifndef SUNDIALS_DOUBLE_PRECISION
# error "Sorry, can only handle double precision versions of Sundials"
#endif

/* Private type */
struct Integrator
{
  N_Vector y;
  void* cvode_mem;
  struct Simulation* simulation;
};

/* Function called by the Solver (CVODES only) */
static int f(realtype t,N_Vector y,N_Vector ydot,void *f_data);

static int check_flag(void *flagvalue,char *funcname,int opt);

/*
 * Functions to use CVODES
 *
 */
struct Integrator* CreateIntegrator(struct Simulation* sim,
  struct IntegratorUserData* ud)
{
  if (!(sim && ud && ud->methods))
  {
    ERROR("CreateIntegrator","Invalid arguments when creating integrator");
    return((struct Integrator*)NULL);
  }
  int flag,mlmm,miter;
  struct Integrator* integrator =
    (struct Integrator*)malloc(sizeof(struct Integrator));
  integrator->y = NULL;
  integrator->cvode_mem = NULL;
  integrator->simulation = simulationClone(sim);

  /* Check for errors in the simulation */
  if (simulationGetATolLength(integrator->simulation) < 1)
  {
    ERROR("CreateIntegrator","Absolute tolerance(s) not set\n");
    DestroyIntegrator(&integrator);
    return(NULL);
  }
  /* Would be good if we didn't require the specification of the
     tabulation step size, but for now it is required so we should
     check that it has a sensible value */
  if (!simulationIsBvarTabStepSet(integrator->simulation))
  {
    ERROR("CreateIntegrator","Tabulation step size must be set\n");
    DestroyIntegrator(&integrator);
    return(NULL);
  }
  /* start and end also needs to be set */
  if (!simulationIsBvarStartSet(integrator->simulation))
  {
    ERROR("CreateIntegrator","Bound variable interval start must be set\n");
    DestroyIntegrator(&integrator);
    return(NULL);
  }
  if (!simulationIsBvarEndSet(integrator->simulation))
  {
    ERROR("CreateIntegrator","Bound variable interval end must be set\n");
    DestroyIntegrator(&integrator);
    return(NULL);
  }

  /* Create serial vector of length NR for I.C. */
  integrator->y = N_VNew_Serial(ud->NR);
  if (check_flag((void *)(integrator->y),"N_VNew_Serial",0))
  {
    DestroyIntegrator(&integrator);
    return(NULL);
  }
  /* Initialize y */
  realtype* yD = NV_DATA_S(integrator->y);
  int i;
  for (i=0;i<(ud->NR);i++) yD[i] = (realtype)(ud->STATES[i]);

  /* adjust parameters accordingly */
  switch (simulationGetMultistepMethod(integrator->simulation))
  {
    case ADAMS:
    {
      mlmm = CV_ADAMS;
      break;
    }
    case BDF:
    {
      mlmm = CV_BDF;
      break;
    }
    default:
    {
      ERROR("CreateIntegrator","Invalid multistep method choice\n");
      DestroyIntegrator(&integrator);
      return(NULL);
    }
  }
  switch (simulationGetIterationMethod(integrator->simulation))
  {
    case FUNCTIONAL:
    {
      miter = CV_FUNCTIONAL;
      break;
    }
    case NEWTON:
    {
      miter = CV_NEWTON;
      break;
    }
    default:
    {
      ERROR("CreateIntegrator","Invalid iteration method choice\n");
      DestroyIntegrator(&integrator);
      return(NULL);
    }
  }
  /* 
     Call CVodeCreate to create the solver memory:     
     A pointer to the integrator problem memory is returned and
     stored in cvode_mem.
  */
  integrator->cvode_mem = CVodeCreate(mlmm,miter);
  if (check_flag((void *)(integrator->cvode_mem),"CVodeCreate",0))
  {
    DestroyIntegrator(&integrator);
    return(NULL);
  }
  
  /* 
     Call CVodeMalloc to initialize the integrator memory: 
     
     cvode_mem is the pointer to the integrator memory returned by CVodeCreate
     f         is the user's right hand side function in y'=f(t,y)
     tStart    is the initial time
     y         is the initial dependent variable vector
     CV_SS     specifies scalar relative and absolute tolerances
     reltol    the scalar relative tolerance
     &abstol   is the absolute tolerance vector
  */
  flag = CVodeInit(integrator->cvode_mem,f,
    simulationGetBvarStart(integrator->simulation),integrator->y);
  if (check_flag(&flag,"CVodeMalloc",1))
  {
	  DestroyIntegrator(&integrator);
      return(NULL);
  }

  double* atol = simulationGetATol(integrator->simulation);
  if (simulationGetATolLength(integrator->simulation) == 1)
  {
	  flag = CVodeSStolerances(integrator->cvode_mem,simulationGetRTol(integrator->simulation),atol[0]);
	  if (check_flag(&flag,"CVodeSStolerances",1))
	  {
		  DestroyIntegrator(&integrator);
		  return(NULL);
	  }
  }
  else
  {
	  ERROR("CreateIntegrator", "array abstol not supported yet.");
	  DestroyIntegrator(&integrator);
	  return(NULL);
  }
  free(atol);

  /* if using Newton iteration need a linear solver */
  if (simulationGetIterationMethod(integrator->simulation) == NEWTON)
  {
    switch (simulationGetLinearSolver(integrator->simulation))
    {
      case DENSE:
      {
        /* Call CVDense to specify the CVDENSE dense linear solver */
        flag = CVDense(integrator->cvode_mem,ud->NR);
        if (check_flag(&flag,"CVDense",1))
        {
          DestroyIntegrator(&integrator);
          return(NULL);
        }
      } break;
      case BAND:
      {
        /* Call CVBand to specify the CVBAND linear solver */
        long int upperBW = ud->NR - 1; /* FIXME: This probably doesn't make */
        long int lowerBW = ud->NR - 1; /* any sense, but should do until I */
                                       /* fix it */
        flag = CVBand(integrator->cvode_mem,ud->NR,upperBW,lowerBW);
        if (check_flag(&flag,"CVBand",1))
        {
          DestroyIntegrator(&integrator);
          return(NULL);
        }
      } break;
      case DIAG:
      {
        /* Call CVDiag to specify the CVDIAG linear solver */
        flag = CVDiag(integrator->cvode_mem);
        if (check_flag(&flag,"CVDiag",1))
        {
          DestroyIntegrator(&integrator);
          return(NULL);
        }
      } break;
      case SPGMR:
      {
        /* Call CVSpgmr to specify the linear solver CVSPGMR 
           with no preconditioning and the maximum Krylov dimension maxl */
        flag = CVSpgmr(integrator->cvode_mem,PREC_NONE,0);
        if(check_flag(&flag,"CVSpgmr",1))
        {
          DestroyIntegrator(&integrator);
          return(NULL);
        }

      } break;
      case SPBCG:
      {
        /* Call CVSpbcg to specify the linear solver CVSPBCG 
           with no preconditioning and the maximum Krylov dimension maxl */
        flag = CVSpbcg(integrator->cvode_mem,PREC_NONE,0);
        if(check_flag(&flag,"CVSpbcg",1))
        {
          DestroyIntegrator(&integrator);
          return(NULL);
        }

      } break;
      case SPTFQMR:
      {
        /* Call CVSptfqmr to specify the linear solver CVSPTFQMR 
           with no preconditioning and the maximum Krylov dimension maxl */
        flag = CVSptfqmr(integrator->cvode_mem,PREC_NONE,0);
        if(check_flag(&flag,"CVSptfqmr",1))
        {
          DestroyIntegrator(&integrator);
          return(NULL);
        }

      } break;
      default:
      {
        ERROR("CreateIntegrator",
          "Must specify a valid linear solver when using "
          "Newton iteration\n");
        DestroyIntegrator(&integrator);
        return(NULL);
      }
    }
  }

  /* Pass through fData to f */
  flag = CVodeSetUserData(integrator->cvode_mem,(void*)(ud));
  if (check_flag(&flag,"CVodeSetUserData",1))
  {
    DestroyIntegrator(&integrator);
    return(NULL);
  }

  /* Set the maximum time step size if it looks valid */
  double mxD = simulationGetBvarMaxStep(integrator->simulation);
  double tbD = simulationGetBvarTabStep(integrator->simulation);
  double maxStep;
  if (simulationIsBvarMaxStepSet(integrator->simulation))
  {
    flag = CVodeSetMaxStep(integrator->cvode_mem,
      (realtype)mxD);
    maxStep = mxD;
    if (check_flag(&flag,"CVodeSetMaxStep (max)",1))
    {
      DestroyIntegrator(&integrator);
      return(NULL);
    }
  }
  else
  {
    flag = CVodeSetMaxStep(integrator->cvode_mem,
      (realtype)tbD);
    maxStep = tbD;
    if (check_flag(&flag,"CVodeSetMaxStep (tab)",1))
    {
      DestroyIntegrator(&integrator);
      return(NULL);
    }
  }
  simulationSetBvarMaxStep(integrator->simulation,maxStep);
  simulationSetBvarMaxStep(sim,maxStep);
  /* try and make a sensible guess at the maximal number of steps to
     get to tout to take into account case where we simply want to
     integrate over a large time period in small steps (why?) */
  double tout = simulationGetBvarStart(integrator->simulation)+
    simulationGetBvarTabStep(integrator->simulation);
  if (simulationIsBvarEndSet(integrator->simulation))
  {
    double end = simulationGetBvarEnd(integrator->simulation);
    if (tout > end) tout = end;
  }
  long int maxsteps = (long int)ceil(tout/maxStep) * 100;
  if (maxsteps < 500) maxsteps = 500; /* default value */
  flag = CVodeSetMaxNumSteps(integrator->cvode_mem,maxsteps);
  if (check_flag(&flag,"CVodeSetMaxNumSteps",1))
  {
    DestroyIntegrator(&integrator);
    return(NULL);
  }
  return(integrator);
}

int DestroyIntegrator(struct Integrator** integrator)
{
  struct Integrator* intg = *integrator;
  if (intg)
  {
    if (intg->y) N_VDestroy_Serial(intg->y);
    if (intg->cvode_mem) CVodeFree(&(intg->cvode_mem));
    if (intg->simulation) DestroySimulation(&(intg->simulation));
    free(intg);
  }
  *integrator = NULL;
  return(OK);
}

int integratorInitialise(struct Integrator* integrator,
  struct IntegratorUserData* ud)
{
  int i,code = ERR;
  if (integrator && ud)
  {
    /* Initialize y */
    realtype* yD = NV_DATA_S(integrator->y);
    for (i=0;i<(ud->NR);i++) yD[i] = (realtype)(ud->STATES[i]);
    code = OK;
  }
  return(code);
}

int integrate(struct Integrator* integrator,struct IntegratorUserData* ud,
  double tout,double* t)
{
  if (ud->NR > 0)
  {
    /* need to integrate if we have any differential equations */
    int flag;
    /* Make sure we don't go past the specified end time - could run into
       trouble if we're almost reaching a threshold */
    flag = CVodeSetStopTime(integrator->cvode_mem,(realtype)tout);
    if (check_flag(&flag,"CVode",1)) return(ERR);
    flag = CVode(integrator->cvode_mem,tout,integrator->y,t,CV_NORMAL);
    if (check_flag(&flag,"CVode",1)) return(ERR);
    /* we also need to evaluate all the other variables that are not required
       to be updated during integration */
    ud->methods->EvaluateVariables(tout,ud->CONSTANTS,ud->RATES,
      ud->STATES,ud->ALGEBRAIC);
  }
  else
  {
    *t = tout;
    ud->BOUND[0] = *t;
    /* no differential equations so just evaluate once */
    ud->methods->ComputeRates(*(ud->BOUND),ud->STATES,ud->RATES,ud->CONSTANTS,
      ud->ALGEBRAIC);
    ud->methods->EvaluateVariables(*(ud->BOUND),ud->CONSTANTS,ud->RATES,
      ud->STATES,ud->ALGEBRAIC);
  }

  /*
   * Now that using CV_NORMAL_TSTOP this is no longer required?
   */
#ifdef OLD_CODE
  /* only the y array is gonna be at the desired tout, so we need to also
     update the full variables array */
  ud->BOUND[0] = *t;
  ud->methods->ComputeVariables(ud->BOUND,ud->RATES,ud->CONSTANTS,
    ud->VARIABLES);
#endif
  
  /* Make sure the outputs are up-to-date */
  ud->methods->GetOutputs(*(ud->BOUND), ud->CONSTANTS, ud->STATES, ud->ALGEBRAIC, ud->OUTPUTS);
  return(OK);
}

/*
 *-------------------------------
 * Functions called by the solver
 *-------------------------------
 */

/*
 * f routine. Compute function f(t,y). 
 */

static int f(realtype t,N_Vector y,N_Vector ydot,void *f_data)
{
  struct IntegratorUserData* ud = (struct IntegratorUserData*)f_data;
  realtype* yD = NV_DATA_S(y);
  realtype* ydotD = NV_DATA_S(ydot);
  long int len = NV_LENGTH_S(y);
  long int i;

  ud->BOUND[0] = (double)t;
  for (i=0;i<len;i++) ud->STATES[i]=(double)yD[i];
  /* while integrating we only need to compute the rates */
  ud->methods->ComputeRates(*(ud->BOUND),ud->STATES,ud->RATES,ud->CONSTANTS,
    ud->ALGEBRAIC);
  for (i=0;i<len;i++) ydotD[i] = (realtype)(ud->RATES[i]);
  return(0);
}

/*
 * Check function return value...
 *   opt == 0 means SUNDIALS function allocates memory so check if
 *            returned NULL pointer
 *   opt == 1 means SUNDIALS function returns a flag so check if
 *            flag >= 0
 *   opt == 2 means function allocates memory so check if returned
 *            NULL pointer
 *
 * FIXME: non-standard error code used, should change?
 */
static int check_flag(void *flagvalue, char *funcname, int opt)
{
  int *errflag;

  /* Check if SUNDIALS function returned NULL pointer - no memory allocated */
  if (opt == 0 && flagvalue == NULL)
  {
    ERROR("check_flag",
      "\nSUNDIALS_ERROR: %s() failed - returned NULL pointer\n\n",
	    funcname);
    return(1);
  }
  /* Check if flag < 0 */
  else if (opt == 1)
  {
    errflag = (int *) flagvalue;
    if (*errflag < 0)
    {
      ERROR("check_flag","\nSUNDIALS_ERROR: %s() failed with flag = %d\n\n",
	      funcname,*errflag);
      return(1);
    }
  }
  /* Check if function returned NULL pointer - no memory allocated */
  else if (opt == 2 && flagvalue == NULL)
  {
    ERROR("check_flag",
      "\nMEMORY_ERROR: %s() failed - returned NULL pointer\n\n",
	    funcname);
    return(1);
  }
  return(0);
}

/* 
 * Get and print some final statistics
 */
void PrintFinalStats(struct Integrator* integrator)
{
  void* cvode_mem = integrator->cvode_mem;
  long int lenrw, leniw, nst, nfe, nsetups, nni, ncfn, netf;
  long int lenrwLS, leniwLS, nje, nfeLS,npe,nps,ncfl,nli;
  int flag;

  flag = CVodeGetWorkSpace(cvode_mem, &lenrw, &leniw);
  check_flag(&flag, "CVodeGetWorkSpace", 1);
  flag = CVodeGetNumSteps(cvode_mem, &nst);
  check_flag(&flag, "CVodeGetNumSteps", 1);
  flag = CVodeGetNumRhsEvals(cvode_mem, &nfe);
  check_flag(&flag, "CVodeGetNumRhsEvals", 1);
  flag = CVodeGetNumLinSolvSetups(cvode_mem, &nsetups);
  check_flag(&flag, "CVodeGetNumLinSolvSetups", 1);
  flag = CVodeGetNumErrTestFails(cvode_mem, &netf);
  check_flag(&flag, "CVodeGetNumErrTestFails", 1);
  flag = CVodeGetNumNonlinSolvIters(cvode_mem, &nni);
  check_flag(&flag, "CVodeGetNumNonlinSolvIters", 1);
  flag = CVodeGetNumNonlinSolvConvFails(cvode_mem, &ncfn);
  check_flag(&flag, "CVodeGetNumNonlinSolvConvFails", 1);

  printf("\n Final integrator statistics for this run:\n");
  printf(" (MM: %s; IM: %s; LS: %s; max-step: %0.4le)\n",
    multistepMethodToString(
      simulationGetMultistepMethod(integrator->simulation)),
    iterationMethodToString(
      simulationGetIterationMethod(integrator->simulation)),
    linearSolverToString(simulationGetLinearSolver(integrator->simulation)),
    simulationGetBvarMaxStep(integrator->simulation));
  printf(" CVode real workspace length              = %4ld \n", lenrw);
  printf(" CVode integer workspace length           = %4ld \n", leniw);
  printf(" Number of steps                          = %4ld \n",  nst);
  printf(" Number of f-s                            = %4ld \n",  nfe);
  printf(" Number of setups                         = %4ld \n",  nsetups);
  printf(" Number of nonlinear iterations           = %4ld \n",  nni);
  printf(" Number of nonlinear convergence failures = %4ld \n",  ncfn);
  printf(" Number of error test failures            = %4ld \n\n",netf);

  if (simulationGetIterationMethod(integrator->simulation) == NEWTON)
  {
    enum LinearSolver solver =
      simulationGetLinearSolver(integrator->simulation);
    switch(solver)
    {
      case DENSE:
      {
        //flag = CVDenseGetNumJacEvals(cvode_mem, &nje);
        //check_flag(&flag, "CVDenseGetNumJacEvals", 1);
        //flag = CVDenseGetNumRhsEvals(cvode_mem, &nfeLS);
        //check_flag(&flag, "CVDenseGetNumRhsEvals", 1);
        //flag = CVDenseGetWorkSpace(cvode_mem, &lenrwLS, &leniwLS);
        //check_flag(&flag, "CVDenseGetWorkSpace", 1);
      } break;
      case BAND:
      {
        //flag = CVBandGetNumJacEvals(cvode_mem, &nje);
        //check_flag(&flag, "CVBandGetNumJacEvals", 1);
        //flag = CVBandGetNumRhsEvals(cvode_mem, &nfeLS);
        //check_flag(&flag, "CVBandGetNumRhsEvals", 1);
        //flag = CVBandGetWorkSpace(cvode_mem, &lenrwLS, &leniwLS);
        //check_flag(&flag, "CVBandGetWorkSpace", 1);
      } break;
      case DIAG:
      {
        nje = nsetups;
        flag = CVDiagGetNumRhsEvals(cvode_mem, &nfeLS);
        check_flag(&flag, "CVDiagGetNumRhsEvals", 1);
        flag = CVDiagGetWorkSpace(cvode_mem, &lenrwLS, &leniwLS);
        check_flag(&flag, "CVDiagGetWorkSpace", 1);
      } break;
      case SPGMR:
      case SPBCG:
      case SPTFQMR:
      {
        nje = nsetups;
        flag = CVSpilsGetWorkSpace(cvode_mem,&lenrwLS,&leniwLS);
        check_flag(&flag, "CVSpilsGetWorkSpace", 1);
        flag = CVSpilsGetNumRhsEvals(cvode_mem, &nfeLS);
        check_flag(&flag, "CVSpilsGetNumRhsEvals", 1);
        
        flag = CVSpilsGetNumLinIters(cvode_mem, &nli);
        check_flag(&flag, "CVSpilsGetNumLinIters", 1);
        flag = CVSpilsGetNumPrecEvals(cvode_mem, &npe);
        check_flag(&flag, "CVSpilsGetNumPrecEvals", 1);
        flag = CVSpilsGetNumPrecSolves(cvode_mem, &nps);
        check_flag(&flag, "CVSpilsGetNumPrecSolves", 1);
        flag = CVSpilsGetNumConvFails(cvode_mem, &ncfl);
        check_flag(&flag, "CVSpilsGetNumConvFails", 1);
      } break;
      default:
      {
        nje = -1;
        nfeLS = -1;
        lenrwLS = -1;
        leniwLS = -1;
      }
    }
    printf(" Linear solver real workspace length      = %4ld \n", lenrwLS);
    printf(" Linear solver integer workspace length   = %4ld \n", leniwLS);
    printf(" Number of Jacobian evaluations           = %4ld \n", nje);
    printf(" Number of f evals. in linear solver      = %4ld \n", nfeLS);
    if ((solver == SPGMR) || (solver == SPBCG) ||
      (solver == SPTFQMR))
    {
      printf(" Number of linear iterations              = %4ld \n",nli);
      printf(" Number of preconditioner evaluations     = %4ld \n",npe);
      printf(" Number of preconditioner solves          = %4ld \n",nps);
      printf(" Number of convergence failures           = %4ld \n",ncfl);
    }
    printf("\n");
  }
}
