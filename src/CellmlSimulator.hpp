/*
 * CellmlSimulator.hpp
 *
 *  Created on: Aug 15, 2012
 *      Author: dnic019
 */

#ifndef CELLMLSIMULATOR_HPP_
#define CELLMLSIMULATOR_HPP_

#include <vector>
#include <string>

struct CellMLModel;
struct Simulation;
class CellmlCode;
class ExecutableModel;

class CellmlSimulator
{
public:
	CellmlSimulator();
	~CellmlSimulator();

	/**
	 * Serialise the model from the given URL into a string, making sure the
	 * xml:base is set appropriately for future resolution of imports.
	 */
	std::string serialiseCellmlFromUrl(const std::string& url);

	/**
	 * Load the CellML model from the given string. Returns 0 on success.
	 */
	int loadModelString(const std::string& modelString);

	/**
	 * Create a (dummy) simulation definition for this model, setting all variables
	 * in the top-level model as output variables. Returns 0 on success.
	 */
	int createSimulationDefinition();

	/**
	 * Generate code and compile the model into an executable object. Return 0 on success.
	 */
	int compileModel();

	/**
	 * Checkpoint (cache) the current model values for future reference/resetting. Return 0 on success.
	 */
	int checkpointModelValues();

	/**
	 * Update the executable model's values from the current checkpoint data. Return 0 on success.
	 */
	int updateModelFromCheckpoint();

	/**
	 * Set the value of the given variable (component.variable) to the specified value. Return 0 on success.
	 */
	int setVariableValue(const std::string& variableId, double value);

private:
	std::string mUrl;
	std::vector<std::string> mVariableIds;
	struct CellMLModel* mModel;
	struct Simulation* mSimulation;
	class CellmlCode* mCode;
	class ExecutableModel* mExecutableModel;
	double* mBoundCache;
	double* mRatesCache;
	double* mStatesCache;
	double* mConstantsCache;
	double* mAlgebraicCache;
	double* mOutputsCache;
};

#endif /* CELLMLSIMULATOR_HPP_ */
