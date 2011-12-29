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
#ifndef _CELLML_H_
#define _CELLML_H_

#ifdef __cplusplus
extern "C"
{
#endif

  /* A basic object to provide a C interface to the CellML API */
  struct CellMLModel;
  struct CellMLModel* CreateCellMLModel(const char* url);
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
