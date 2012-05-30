
#ifndef _CELLML_CODE_MANAGER_H_
#define _CELLML_CODE_MANAGER_H_

/* An object to manage code generation so we don't keep regenerating the
   same code */
struct CellMLCodeManager;
struct CellMLMethods;
struct Simulation;
  
struct CellMLCodeManager* CreateCellMLCodeManager(int saveFiles,
  const char* cCompiler,int generateDebugCode);
/* Only to be called after finished with everything - should probably do
   a reference count...maybe later */
int DestroyCellMLCodeManager(struct CellMLCodeManager** manager);

struct CellMLMethods* cellmlCodeManagerGetMethodsForSimulation(
  struct CellMLCodeManager* manager, struct Simulation* simulation);
  
#endif /* _CELLML_CODE_MANAGER_H_ */

