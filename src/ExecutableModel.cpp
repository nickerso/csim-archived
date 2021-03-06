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
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/FrontendDiagnostic.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ExecutionEngine/MCJIT.h"
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

static llvm::ExecutionEngine *
createExecutionEngine(std::unique_ptr<llvm::Module> M, std::string *ErrorStr)
{
  return llvm::EngineBuilder(std::move(M))
      .setEngineKind(llvm::EngineKind::Either)
      .setErrorStr(ErrorStr)
      .create();
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

    std::unique_ptr<llvm::Module> compiledModel(compiler->compileModel(filename));

    if (!compiledModel)
    {
        std::cerr << "Error compiling model" << std::endl;
        return -3;
    }

    // FIXME: learn how to use std::unique_ptr - no idea why this works, but its what the clang-interpreter does :)
    llvm::Module& M = *compiledModel;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    std::string Error;

	// This takes over managing the compiledModel object.
    mEE = createExecutionEngine(std::move(compiledModel), &Error);

    mEE->finalizeObject();

    llvm::Function* getNbound = M.getFunction("getNbound");
    llvm::Function* getNrates = M.getFunction("getNrates");
    llvm::Function* getNalgebraic = M.getFunction("getNalgebraic");
    llvm::Function* getNconstants = M.getFunction("getNconstants");
    llvm::Function* getNoutputs = M.getFunction("getNoutputs");
	if (!(getNalgebraic && getNbound && getNconstants && getNoutputs && getNrates))
	{
		llvm::errs() << "'getN*' function not found in module.\n";
        return -3;
	}
    mSetupFixedConstants = (SetupFixedConstantsFunction)(mEE->getPointerToFunction(M.getFunction("SetupFixedConstants")));
    mComputeRates = (ComputeRatesFunction)(mEE->getPointerToFunction(M.getFunction("ComputeRates")));
    mEvaluateVariables = (EvaluateVariablesFunction)(mEE->getPointerToFunction(M.getFunction("EvaluateVariables")));
    mGetOutputs = (GetOutputsFunction)(mEE->getPointerToFunction(M.getFunction("GetOutputs")));
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
	if (bound) free(bound);
	if (constants) free(constants);
	if (rates) free(rates);
	if (states) free(states);
	if (algebraic) free(algebraic);
	if (outputs) free(outputs);
}

int ExecutableModel::setupFixedConstants()
{
/*	std::vector<llvm::GenericValue> args(3);
	args[0].PointerVal = (void*)constants;
	args[1].PointerVal = (void*)rates;
	args[2].PointerVal = (void*)states;
	llvm::GenericValue gv = mEE->runFunction(mSetupFixedConstants, args);
*/
	(*mSetupFixedConstants)(constants, rates, states);
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
/*    std::vector<llvm::GenericValue> args(5);
    args[0].DoubleVal = voi;
    args[1].PointerVal = constants;
    args[2].PointerVal = rates;
    args[3].PointerVal = states;
    args[4].PointerVal = algebraic;
    llvm::GenericValue gv = mEE->runFunction(mEvaluateVariables, args);
*/
	(*mEvaluateVariables)(voi, constants, rates, states, algebraic);
	return 0;
}

int ExecutableModel::getOutputs(double voi)
{
/*    std::vector<llvm::GenericValue> args(5);
    args[0].DoubleVal = voi;
	args[1].PointerVal = (void*)constants;
	args[2].PointerVal = (void*)states;
	args[3].PointerVal = (void*)algebraic;
	args[4].PointerVal = (void*)outputs;
	llvm::GenericValue gv = mEE->runFunction(mGetOutputs, args);
*/
	(*mGetOutputs)(voi, constants, states, algebraic, outputs);
	return 0;
}

