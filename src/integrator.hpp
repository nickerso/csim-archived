
#ifndef _INTEGRATOR_HPP_
#define _INTEGRATOR_HPP_

/* Private structure */
struct Simulation;
struct Integrator;
class ExecutableModel;

/* The main functions to create, destroy, and use an integrator */
struct Integrator* CreateIntegrator(struct Simulation* simulation,
  class ExecutableModel* em);
int DestroyIntegrator(struct Integrator** integrator);

int integratorInitialise(struct Integrator* integrator);

/* advance in the bound variable */
int integrate(struct Integrator* integrator, double tout, double* t);

/* function to print final statistics */
void PrintFinalStats(struct Integrator* integrator);

#endif /* _INTEGRATOR_H_ */
