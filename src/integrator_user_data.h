
#ifndef _INTEGRATOR_USER_DATA_H_
#define _INTEGRATOR_USER_DATA_H_

struct CellMLMethods;
struct CellMLCodeManager;
struct Simulation;

/* should hide this away? */
struct IntegratorUserData
{
  struct CellMLMethods* methods;
  int NA,NB,NC,NR,NO;
  double* BOUND;
  double* CONSTANTS;
  double* STATES;
  double* ALGEBRAIC;
  double* RATES; /* Need to keep track of current set of rates */
  double* OUTPUTS; /* The outputs as requested in the simulation description */
};

struct IntegratorUserData* CreateIntegratorUserDataForSimulation(
  struct CellMLCodeManager* codeManager, struct Simulation* simulation);

#if defined (OLD_CODE)
struct IntegratorUserData* CreateIntegratorUserData(const char* soFileName);
#endif /* defined (OLD_CODE) */

int DestroyIntegratorUserData(struct IntegratorUserData** userData);

int integratorUserDataInitialise(struct IntegratorUserData *userData,
  double value);

#endif /* _INTEGRATOR_USER_DATA_H_ */
