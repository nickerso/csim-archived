/*
 * ExecutableModel.cpp
 *
 *  Created on: Jan 31, 2012
 *      Author: dnic019
 */

#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstdio>

#include "clang/CodeGen/CodeGenAction.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Tool.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/DiagnosticOptions.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "llvm/Module.h"
#include "llvm/DerivedTypes.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
using namespace clang;
using namespace clang::driver;

#ifdef __cplusplus
extern "C"
{
#endif
#include "utils.h"
#ifdef __cplusplus
}
#endif

#include "ModelCompiler.hpp"

#include "ExecutableModel.hpp"

ExecutableModel::ExecutableModel() :
        bound(0), rates(0), states(0), constants(0), algebraic(0), outputs(0)
{
}

int ExecutableModel::initialise(ModelCompiler *compiler, const char *filename, double voiInitialValue)
{
    if (!compiler)
    {
        std::cerr << "Invailid model compiler with which to initialise the model"
                  << std::endl;
        return -1;
    }
    if (!filename)
    {
        std::cerr << "Invailid filename with which to initialise the model"
                  << std::endl;
        return -2;
    }

    llvm::Module* compiledModel = compiler->compileModel(filename);
    if (!compiledModel)
    {
        std::cerr << "Error compiling model" << std::endl;
        return -3;
    }

    std::string Error;

	// This takes over managing the compiledModel object.
	mEE = llvm::ExecutionEngine::createJIT(compiledModel, &Error);
	if (!mEE)
	{
		llvm::errs() << "unable to make execution engine: " << Error << "\n";
        return -3;
	}

	llvm::Function* getNbound = compiledModel->getFunction("getNbound");
	llvm::Function* getNrates = compiledModel->getFunction("getNrates");
	llvm::Function* getNalgebraic = compiledModel->getFunction("getNalgebraic");
	llvm::Function* getNconstants = compiledModel->getFunction("getNconstants");
	llvm::Function* getNoutputs = compiledModel->getFunction("getNoutputs");
	if (!(getNalgebraic && getNbound && getNconstants && getNoutputs && getNrates))
	{
		llvm::errs() << "'getN*' function not found in module.\n";
        return -3;
	}
	mSetupFixedConstants = compiledModel->getFunction("SetupFixedConstants");
	mComputeRates = (ComputeRatesFunction)(mEE->getPointerToFunction(compiledModel->getFunction("ComputeRates")));
	mEvaluateVariables = compiledModel->getFunction("EvaluateVariables");
	mGetOutputs = compiledModel->getFunction("GetOutputs");
	if (!(mSetupFixedConstants && mComputeRates && mEvaluateVariables && mGetOutputs))
	{
		llvm::errs() << "'compute functions' function not found in module.\n";
        return -3;
	}

	std::vector<llvm::GenericValue> noargs;
	llvm::GenericValue gv = mEE->runFunction(getNbound, noargs);
	nBound = gv.IntVal.getLimitedValue();
	gv = mEE->runFunction(getNconstants, noargs);
	nConstants = gv.IntVal.getLimitedValue();
	gv = mEE->runFunction(getNrates, noargs);
	nRates = gv.IntVal.getLimitedValue();
	gv = mEE->runFunction(getNalgebraic, noargs);
	nAlgebraic = gv.IntVal.getLimitedValue();
	gv = mEE->runFunction(getNoutputs, noargs);
	nOutputs = gv.IntVal.getLimitedValue();

	/*
    llvm::Function* mult = compiledModel->getFunction("mult");
    std::vector<llvm::GenericValue> oneargs(1);
    oneargs[0].PointerVal = &nBound;
    gv = mEE->runFunction(mult, oneargs);
    */

	if (debugLevel() > 0)
	{
		std::cout << "nBound = " << nBound << std::endl;
		std::cout << "nConstants = " << nConstants << std::endl;
		std::cout << "nRates = " << nRates << std::endl;
		std::cout << "nAlgebraic = " << nAlgebraic << std::endl;
		std::cout << "nOutputs = " << nOutputs << std::endl;
	}

	bound = (double*) calloc(nBound, sizeof(double));
	constants = (double*) calloc(nConstants, sizeof(double));
	rates = (double*) calloc(nRates, sizeof(double));
	states = (double*) calloc(nRates, sizeof(double));
	algebraic = (double*) calloc(nAlgebraic, sizeof(double));
	outputs = (double*) calloc(nOutputs, sizeof(double));

	// intialise the arrays
	setupFixedConstants();
	computeRates(voiInitialValue);
    evaluateVariables(voiInitialValue);
    getOutputs(voiInitialValue);
    return 0;
}

ExecutableModel::~ExecutableModel()
{
	if (mEE) delete mEE;
	if (bound) free(bound);
	if (constants) free(constants);
	if (rates) free(rates);
	if (states) free(states);
	if (algebraic) free(algebraic);
	if (outputs) free(outputs);
}

int ExecutableModel::setupFixedConstants()
{
	std::vector<llvm::GenericValue> args(3);
	args[0].PointerVal = (void*)constants;
	args[1].PointerVal = (void*)rates;
	args[2].PointerVal = (void*)states;
	llvm::GenericValue gv = mEE->runFunction(mSetupFixedConstants, args);
	return 0;
}

int ExecutableModel::computeRates(double voi)
{
/*    std::vector<llvm::GenericValue> args(5);
    args[0].DoubleVal = voi;
    args[1].PointerVal = states;
    args[2].PointerVal = rates;
    args[3].PointerVal = constants;
    args[4].PointerVal = algebraic;
#if 0
    const llvm::FunctionType* FTy = mComputeRates->getFunctionType();
    const llvm::Type* ArgTy = FTy->getParamType(0);
    if (ArgTy->getTypeID() == llvm::Type::DoubleTyID)
        std::cout << "arg 0 is a double" << std::endl;
    else
        std::cout << "arg 0 is not a double - WTF?!" << std::endl;
#endif
	llvm::GenericValue gv = mEE->runFunction(mComputeRates, args);
*/
	(*mComputeRates)(voi, states, rates, constants, algebraic);
	return 0;
}

int ExecutableModel::evaluateVariables(double voi)
{
    std::vector<llvm::GenericValue> args(5);
    args[0].DoubleVal = voi;
    args[1].PointerVal = constants;
    args[2].PointerVal = rates;
    args[3].PointerVal = states;
    args[4].PointerVal = algebraic;
    llvm::GenericValue gv = mEE->runFunction(mEvaluateVariables, args);
	return 0;
}

int ExecutableModel::getOutputs(double voi)
{
    std::vector<llvm::GenericValue> args(5);
    args[0].DoubleVal = voi;
	args[1].PointerVal = (void*)constants;
	args[2].PointerVal = (void*)states;
	args[3].PointerVal = (void*)algebraic;
	args[4].PointerVal = (void*)outputs;
	llvm::GenericValue gv = mEE->runFunction(mGetOutputs, args);
	return 0;
}

