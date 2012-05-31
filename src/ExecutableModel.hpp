/*
 * ExecutableModel.hpp
 *
 *  Created on: Jan 31, 2012
 *      Author: dnic019
 */

#ifndef EXECUTABLEMODEL_HPP_
#define EXECUTABLEMODEL_HPP_

// forward declare from LLVM
namespace llvm
{
	class Module;
	class Function;
	class ExecutionEngine;
}

class ExecutableModel
{
public:
    ExecutableModel();
	~ExecutableModel();

    /* Initialise the executable model for the given compiler and file.
      */
    int initialise(ModelCompiler* compiler, const char* filename);

	/* Set up the output array ready for writing.
	 */
	int getOutputs(double voi);

	/* Initialise all variables which aren't state variables but have an
	 * initial_value attribute, and any variables & rates which follow.
	 */
	int setupFixedConstants();

	/* Compute all rates which are not static
	 */
	int computeRates(double voi);

	/* Compute all variables not computed by initConsts or rates
	 *  (i.e., these are not required for the integration of the model and
	 *   thus only need to be called for output or presentation or similar
	 *   purposes)
	 */
	int evaluateVariables(double voi);

	int nBound;
	double* bound;
	int nRates;
	double* rates;
	double* states;
	int nConstants;
	double* constants;
	int nAlgebraic;
	double* algebraic;
	int nOutputs;
	double* outputs;

private:
	llvm::Function* mSetupFixedConstants;
	llvm::Function* mComputeRates;
	llvm::Function* mEvaluateVariables;
	llvm::Function* mGetOutputs;
	llvm::ExecutionEngine* mEE;
};

#endif /* EXECUTABLEMODEL_HPP_ */
