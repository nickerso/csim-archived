/*
 * ModelCompiler.cpp
 *
 *  Created on: Jan 26, 2012
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
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ExecutionEngine/JIT.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
using namespace clang;
using namespace clang::driver;

#include "ModelCompiler.hpp"

// This function isn't referenced outside its translation unit, but it
// can't use the "static" keyword because its address is used for
// GetMainExecutable (since some platforms don't support taking the
// address of main, and some platforms can't implement GetMainExecutable
// without being given the address of a function in the main executable).
std::string GetExecutablePath(const char *Argv0)
{
	// This just needs to be some symbol in the binary; C++ doesn't
	// allow taking the address of ::main however.
	void *MainAddr = (void*) (intptr_t) GetExecutablePath;
	return llvm::sys::fs::getMainExecutable(Argv0, MainAddr);
}

ModelCompiler::ModelCompiler(const char* executable, bool verbose, bool debug) :
		mVerbose(verbose), mDebug(debug), mExecutable(executable)
{
    llvm::InitializeNativeTarget();
}

ModelCompiler::~ModelCompiler()
{
	// Shutdown.
    //llvm::llvm_shutdown();
}

llvm::Module* ModelCompiler::compileModel(const char* filename)
{
	void *MainAddr = (void*) (intptr_t) GetExecutablePath;
	std::string Path = GetExecutablePath(mExecutable.c_str());
    DiagnosticOptions diagOpts;
//	TextDiagnosticPrinter *DiagClient = new TextDiagnosticPrinter(llvm::errs(), &diagOpts
//			);

	llvm::IntrusiveRefCntPtr < DiagnosticIDs > DiagID(new DiagnosticIDs());
	DiagnosticsEngine Diags(DiagID, &diagOpts);
	Driver TheDriver(Path, llvm::sys::getDefaultTargetTriple(), "a.out", /*IsProduction=*/ //false, 
	             Diags);
//xx    Driver TheDriver(Path, llvm::sys::getHostTriple(), "a.out", /*IsProduction=*/
//xx            false, Diags);
    TheDriver.setTitle("clang compiler");

    llvm::SmallVector<const char *, 16> Args;//(argv, argv + argc);
    std::cout << "executable = '" << mExecutable.c_str() << "'" << std::endl;
    std::cout << "filename = '" << filename << "'" << std::endl;
    Args.push_back(mExecutable.c_str());
	Args.push_back("-fsyntax-only");
	Args.push_back("-x");
	Args.push_back("c");
	if (mDebug) Args.push_back("-g");
	else Args.push_back("-O3");
	if (mVerbose) Args.push_back("-v");
	Args.push_back(filename);
	llvm::OwningPtr < Compilation > C(TheDriver.BuildCompilation(Args));
	if (!C)
		return 0;

	// We expect to get back exactly one command job, if we didn't something
	// failed. Extract that job from the compilation.
	const driver::JobList &Jobs = C->getJobs();
	if (Jobs.size() != 1 || !isa < driver::Command > (*Jobs.begin()))
	{
		llvm::SmallString < 256 > Msg;
		llvm::raw_svector_ostream OS(Msg);
        C->getJobs().Print(OS, ";", true);
        //C->PrintJob(OS, C->getJobs(), "; ", true);
		Diags.Report(diag::err_fe_expected_compiler_job) << OS.str();
		return 0;
	}

	const driver::Command *Cmd = cast < driver::Command > (*Jobs.begin());
	if (llvm::StringRef(Cmd->getCreator().getName()) != "clang")
	{
		Diags.Report(diag::err_fe_expected_clang_command);
		return 0;
	}

	// Initialize a compiler invocation object from the clang (-cc1) arguments.
	const driver::ArgStringList &CCArgs = Cmd->getArguments();
	llvm::OwningPtr < CompilerInvocation > CI(new CompilerInvocation);
	CompilerInvocation::CreateFromArgs(*CI,
			const_cast<const char **>(CCArgs.data()),
			const_cast<const char **>(CCArgs.data()) + CCArgs.size(), Diags);

	// Show the invocation, with -v.
	if (CI->getHeaderSearchOpts().Verbose)
	{
        llvm::errs() << "clang invocation:\n";
        C->getJobs().Print(llvm::errs(), "\n", true);
        //C->PrintJob(llvm::errs(), C->getJobs(), "\n", true);
		llvm::errs() << "\n";
	}

	// Create a compiler instance to handle the actual work.
	CompilerInstance Clang;
	Clang.setInvocation(CI.take());

	// Create the compilers actual diagnostics engine.
	//Clang.createDiagnostics(int(CCArgs.size()), const_cast<char**>(CCArgs.data()));
	Clang.createDiagnostics();
	if (!Clang.hasDiagnostics())
		return 0;

	// Infer the builtin include path if unspecified.
	if (Clang.getHeaderSearchOpts().UseBuiltinIncludes
			&& Clang.getHeaderSearchOpts().ResourceDir.empty())
		Clang.getHeaderSearchOpts().ResourceDir =
				CompilerInvocation::GetResourcesPath(mExecutable.c_str(), MainAddr);

#if 0 // Not needed as long as generated CellML code doesn't #include anything...
	// Need to add the clang system path, is there a way to get this? Need to include in executable?
	Clang.getHeaderSearchOpts().AddPath("/data/std-libs/llvm-opt/lib/clang/3.1/include/",
			clang::frontend::System, /*IsUserSupplied*/false, /*IsFramework*/false,
			/*IgnoreSysRoot*/false);
#endif

	// Create and execute the frontend to generate an LLVM bitcode module.
	CodeGenAction* Act(new EmitLLVMOnlyAction()); // <== FIXME: memory leak??
	if (!Clang.ExecuteAction(*Act))
		return 0;

	llvm::Module* compiledModel = Act->takeModule();

	return compiledModel;
}

#if 0
int ModelCompiler::Compile(const char* executable, const char* filename, bool verbose)
{
	void *MainAddr = (void*) (intptr_t) GetExecutablePath;
	std::string Path = GetExecutablePath(executable);
	TextDiagnosticPrinter *DiagClient = new TextDiagnosticPrinter(llvm::errs(),
			DiagnosticOptions());

	llvm::IntrusiveRefCntPtr < DiagnosticIDs > DiagID(new DiagnosticIDs());
	DiagnosticsEngine Diags(DiagID, DiagClient);
	Driver TheDriver(Path, llvm::sys::getDefaultTargetTriple(), "a.out", /*IsProduction=*/
			false, Diags);
	TheDriver.setTitle("clang compiler");

	// FIXME: This is a hack to try to force the driver to do something we can
	// recognize. We need to extend the driver library to support this use model
	// (basically, exactly one input, and the operation mode is hard wired).
	llvm::SmallVector<const char *, 16> Args;//(argv, argv + argc);
	Args.push_back(executable);
	Args.push_back("-fsyntax-only");
	if (verbose) Args.push_back("-v");
	Args.push_back(filename);
	llvm::OwningPtr < Compilation > C(TheDriver.BuildCompilation(Args));
	if (!C)
		return 0;

	// FIXME: This is copied from ASTUnit.cpp; simplify and eliminate.

	// We expect to get back exactly one command job, if we didn't something
	// failed. Extract that job from the compilation.
	const driver::JobList &Jobs = C->getJobs();
	if (Jobs.size() != 1 || !isa < driver::Command > (*Jobs.begin()))
	{
		llvm::SmallString < 256 > Msg;
		llvm::raw_svector_ostream OS(Msg);
		C->PrintJob(OS, C->getJobs(), "; ", true);
		Diags.Report(diag::err_fe_expected_compiler_job) << OS.str();
		return 1;
	}

	const driver::Command *Cmd = cast < driver::Command > (*Jobs.begin());
	if (llvm::StringRef(Cmd->getCreator().getName()) != "clang")
	{
		Diags.Report(diag::err_fe_expected_clang_command);
		return 1;
	}

	// Initialize a compiler invocation object from the clang (-cc1) arguments.
	const driver::ArgStringList &CCArgs = Cmd->getArguments();
	llvm::OwningPtr < CompilerInvocation > CI(new CompilerInvocation);
	CompilerInvocation::CreateFromArgs(*CI,
			const_cast<const char **>(CCArgs.data()),
			const_cast<const char **>(CCArgs.data()) + CCArgs.size(), Diags);

	// Show the invocation, with -v.
	if (CI->getHeaderSearchOpts().Verbose)
	{
		llvm::errs() << "clang invocation:\n";
		C->PrintJob(llvm::errs(), C->getJobs(), "\n", true);
		llvm::errs() << "\n";
	}

	// FIXME: This is copied from cc1_main.cpp; simplify and eliminate.

	// Create a compiler instance to handle the actual work.
	CompilerInstance Clang;
	Clang.setInvocation(CI.take());

	// Create the compilers actual diagnostics engine.
	Clang.createDiagnostics(int(CCArgs.size()),
			const_cast<char**>(CCArgs.data()));
	if (!Clang.hasDiagnostics())
		return 1;

	// Infer the builtin include path if unspecified.
	if (Clang.getHeaderSearchOpts().UseBuiltinIncludes
			&& Clang.getHeaderSearchOpts().ResourceDir.empty())
		Clang.getHeaderSearchOpts().ResourceDir =
				CompilerInvocation::GetResourcesPath(executable, MainAddr);

#if 0 // Not needed as long as generated CellML code doesn't #include anything...
	// Need to add the clang system path, is there a way to get this? Need to include in executable?
	Clang.getHeaderSearchOpts().AddPath("/data/std-libs/llvm-opt/lib/clang/3.1/include/",
			clang::frontend::System, /*IsUserSupplied*/false, /*IsFramework*/false,
			/*IgnoreSysRoot*/false);
#endif

	// Create and execute the frontend to generate an LLVM bitcode module.
	llvm::OwningPtr < CodeGenAction > Act(new EmitLLVMOnlyAction());
	if (!Clang.ExecuteAction(*Act))
		return 1;

	int Res = 255;
	if (llvm::Module *Module = Act->takeModule())
		Res = this->Integrate(Module);

	// Shutdown.

	llvm::llvm_shutdown();

	//std::cout << "The result is: " << Res << std::endl;

	return Res;
}

#endif
