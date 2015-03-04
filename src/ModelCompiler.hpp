/*
 * ModelCompiler.h
 *
 *  Created on: Jan 26, 2012
 *      Author: dnic019
 */

#ifndef MODELCOMPILER_HPP_
#define MODELCOMPILER_HPP_

// forward declare from LLVM
namespace llvm
{
	class Module;
}

class ModelCompiler
{
public:
    ModelCompiler(const char* executable, bool verbose = false, bool debug = true);
	virtual ~ModelCompiler();

    std::unique_ptr<llvm::Module> compileModel(const char* filename);

private:
	bool mVerbose;
	bool mDebug;
	std::string mExecutable;
};

#endif /* MODELCOMPILER_HPP_ */
