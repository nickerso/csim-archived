
#ifndef CELLMLMETHODS_H
#define CELLMLMETHODS_H

/* methods from the CellML shared object */
struct CellMLMethods
{
  void* soHandle;
  int (*getNbound)();
  int (*getNrates)();
  int (*getNalgebraic)();
  int (*getNconstants)();
  int (*getNoutputs)();
  /* Set up the output array ready for writing.
   */
  void (*GetOutputs)(double VOI,double* CONSTANTS,double* STATES,double* ALGEBRAIC,double* outputs);

#if 0
  const char* (*getStateVariableIDs)(int index);
  const char* (*getConstantVariableIDs)(int index);
  const char* (*getAlgebraicVariableIDs)(int index);
  const char* (*getVariableOfIntegrationIDs)();
#endif
  /* Initialise all variables which aren't state variables but have an
   * initial_value attribute, and any variables & rates which follow.
   */
  void (*SetupFixedConstants)(double* CONSTANTS,double* RATES,double* STATES);
  /* Compute all rates which are not static
   */
  void (*ComputeRates)(double VOI,double* STATES,double* RATES,
    double* CONSTANTS,double* ALGEBRAIC);
  /* Compute all variables not computed by initConsts or rates
   *  (i.e., these are not required for the integration of the model and
   *   thus only need to be called for output or presentation or similar
   *   purposes)
   */
  void (*EvaluateVariables)(double VOI,double* CONSTANTS,double* RATES,
    double* STATES,double* ALGEBRAIC);
};

/* Function to create a CellMLMethods struct */
struct CellMLMethods* CreateCellMLMethods(const char* soFileName);
int DestroyCellMLMethods(struct CellMLMethods** m);

#endif /* CELLMLMETHODS_H */
