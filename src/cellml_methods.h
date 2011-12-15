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
  const char* (*getStateVariableIDs)(int index);
  const char* (*getConstantVariableIDs)(int index);
  const char* (*getAlgebraicVariableIDs)(int index);
  const char* (*getVariableOfIntegrationIDs)();
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
