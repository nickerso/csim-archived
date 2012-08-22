
#ifndef _CELLML_H_
#define _CELLML_H_

#ifdef __cplusplus
extern "C"
{
#endif

  /* A basic object to provide a C interface to the CellML API */
  struct CellMLModel;
  struct CellMLModel* CreateCellMLModel(const char* url);
  struct CellMLModel* CreateCellMLModelFromString(const char* modelString);
  int DestroyCellMLModel(struct CellMLModel** model);
  struct CellMLModel* cellMLModelClone(const struct CellMLModel* src);
  char* getCellMLModelId(const struct CellMLModel* model);
  char* getCellMLModelURI(const struct CellMLModel* model);
  char* getCellMLModelAsCCode(struct CellMLModel* model, void* outputVariables, int debugCode);
  void annotateCellMLModelOutputs(struct CellMLModel* model, void* outputVariables);

#if defined (OLD_CODE)
  int cellmlModelSetVariableInitialValue(struct CellMLModel* model,
    const char* component,const char* variable,const char* value);
#endif /* defined (OLD_CODE) */

  /* used to iterate through imports in the reference_description ModelList */
  int cellMLModelIterateImports(struct CellMLModel* model,
    int (*func)(const char*,void*),void* user_data);

  /* a method to be able to grab the actual CellML model object outside
     of here
     FIXME: being lazy, shouldn't pass through two user data pointers.
  */
  int cellMLModelPassThroughModel(struct CellMLModel* model,
    int (*func)(void*,void*,void*),void* ptr1,void* ptr2);

  /* Methods that are in here so all the C++ code is kept together */

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _CELLML_H_ */
