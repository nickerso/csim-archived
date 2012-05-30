
#ifndef _METADATA_SIMULATION_H_
#define _METADATA_SIMULATION_H_

/* The simulation metadata */

/*
 * The user of the CVODES package specifies whether to use
 * the ADAMS or BDF (backward differentiation formula)
 * linear multistep method. The BDF method is recommended
 * for stiff problems, and the ADAMS method is recommended
 * for nonstiff problems.
 */
enum MultistepMethod
{
  /*EULER,IMPROVED_EULER,RK4?? is it worth it? */
  ADAMS,
  BDF
};

/*
 * At each internal time step, a nonlinear equation must
 * be solved. The user can specify either FUNCTIONAL
 * iteration, which does not require linear algebra, or a
 * NEWTON iteration, which requires the solution of linear
 * systems. In the NEWTON case, the user also specifies a
 * linear solver. NEWTON is recommended in case of
 * stiff problems.
 */
enum IterationMethod
{
  FUNCTIONAL,
  NEWTON
};

/*
 * Linear solver options for use with NEWTON iterations. Currently
 * these all use the internal (difference quotient) jacobian approximation
 * from CVODES
 * but could potentially try out using a user defined jacobian
 * function. Some are probably more appropriate than others for a
 * given problem. Using BDF stepping with NEWTON iterations and one of
 * the preconditioned Kryolv solvers is recommended for large stiff
 * systems.
 */
enum LinearSolver
{
  DENSE,   /* Direct solvers only compatible with the serial version of     */
  BAND,    /*   the NVector implementation provided with SUNDIALS. They use */
  DIAG,    /*   either a dense, banded, or diagonal approximation of the
            *   Jacobian matrix.*/
  SPGMR,   /* Krylov iterative solvers, which use scaled preconditioned     */
  SPBCG,   /*   GMRES, scaled preconditioned Bi-CGStab, and scaled          */
  SPTFQMR, /*   preconditioned TFQMR, respectively.*/
  NONE     /* The default for when a linear solver is not needed */
};

/* private type */
struct Simulation;
struct IntegratorUserData;
struct CellMLCodeManager;

/* convenience methods */
const char* multistepMethodToString(enum MultistepMethod lmm);
const char* iterationMethodToString(enum IterationMethod iter);
const char* linearSolverToString(enum LinearSolver solver);
enum MultistepMethod multistepMethodFromString(const char* lmm);
enum IterationMethod iterationMethodFromString(const char* iter);
enum LinearSolver linearSolverFromString(const char* solver);

int simulationPrint(struct Simulation* sim,FILE* file,const char* indent);

struct Simulation* CreateSimulation();
int DestroySimulation(struct Simulation** sim);

struct Simulation* simulationClone(const struct Simulation* sim);

int simulationSetModelURI(struct Simulation* sim,const char* s);
char* simulationGetModelURI(struct Simulation* sim);

int simulationSetURI(struct Simulation* sim,const char* s);
char* simulationGetURI(struct Simulation* sim);

int simulationSetID(struct Simulation* sim,const char* s);
char* simulationGetID(struct Simulation* sim);

int simulationSetName(struct Simulation* sim,const char* s);
char* simulationGetName(struct Simulation* sim);

int simulationSetBvarURI(struct Simulation* sim,const char* s);
char* simulationGetBvarURI(struct Simulation* sim);

int simulationSetBvarStart(struct Simulation* sim,double value);
int simulationSetBvarEnd(struct Simulation* sim,double value);
int simulationSetBvarTabStep(struct Simulation* sim,double value);
int simulationSetBvarMaxStep(struct Simulation* sim,double value);

int simulationSetMultistepMethod(struct Simulation* sim,
  enum MultistepMethod lmm);
int simulationSetIterationMethod(struct Simulation* sim,
  enum IterationMethod iter);
int simulationSetLinearSolver(struct Simulation* sim,enum LinearSolver solver);

/* Absolute tolerance(s) */
int simulationSetATol(struct Simulation* sim,int n,double* tol);
/* Relative tolerance */
int simulationSetRTol(struct Simulation* sim,double tol);

double simulationGetBvarStart(struct Simulation* sim);
double simulationGetBvarEnd(struct Simulation* sim);
double simulationGetBvarTabStep(struct Simulation* sim);
double simulationGetBvarMaxStep(struct Simulation* sim);

enum MultistepMethod simulationGetMultistepMethod(struct Simulation* sim);
enum IterationMethod simulationGetIterationMethod(struct Simulation* sim);
enum LinearSolver simulationGetLinearSolver(struct Simulation* sim);

double* simulationGetATol(struct Simulation* sim);
int simulationGetATolLength(struct Simulation* sim);
double simulationGetRTol(struct Simulation* sim);

int simulationIsBvarStartSet(struct Simulation* sim);
int simulationIsBvarEndSet(struct Simulation* sim);
int simulationIsBvarMaxStepSet(struct Simulation* sim);
int simulationIsBvarTabStepSet(struct Simulation* sim);
int simulationIsRTolSet(struct Simulation* sim);

/* Is the simulation a valid description? Returns non-zero if valid,
   0 otherwise */
int simulationIsValidDescription(struct Simulation* simulation);

int simulationSetOutputVariables(struct Simulation* simulation, void* outputVariables);
void* simulationGetOutputVariables(struct Simulation* simulation);

#endif /* _METADATA_SIMULATION_H_ */
