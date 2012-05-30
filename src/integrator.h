
#ifndef _INTEGRATOR_H_
#define _INTEGRATOR_H_

#include "integrator_user_data.h"

/* Private structure */
struct Simulation;
struct Integrator;

/* The main functions to create, destroy, and use an integrator */
struct Integrator* CreateIntegrator(struct Simulation* simulation,
  struct IntegratorUserData* userData);
int DestroyIntegrator(struct Integrator** integrator);

int integratorInitialise(struct Integrator* integrator,
  struct IntegratorUserData* userData);
/* advance in the bound variable */
int integrate(struct Integrator* integrator,struct IntegratorUserData* ud,
  double tout,double* t);

/* function to print final statistics */
void PrintFinalStats(struct Integrator* integrator);

#endif /* _INTEGRATOR_H_ */
