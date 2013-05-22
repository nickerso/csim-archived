/*
 * CellmlSimulator.hpp
 *
 *  Created on: Aug 15, 2012
 *      Author: dnic019
 */

#ifndef CELLMLSIMULATOR_HPP_
#define CELLMLSIMULATOR_HPP_

#ifdef WIN32
#define CSIM_API __declspec(dllexport)
#else
#define CSIM_API
#endif

#include <vector>
#include <string>

struct CellMLModel;
struct Simulation;
struct Integrator;
class CellmlCode;
class ExecutableModel;

class CSIM_API CellmlSimulator
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

	/**
	 * Returns a copy of the current list of variable IDs from the model.
	 */
	std::vector<std::string> getModelVariables();

	/**
	 * Returns the current output array as a vector of doubles.
	 */
	std::vector<double> getModelOutputs();

	/**
	 * Simulates the model from @initialTime to @endTime. The results of stepping between @startTime
	 * and @endTime in @numSteps is returned as a string.
	 *
	 * FIXME: need to be able to control the actual integrator used underneath this...
	 */
	std::vector<std::vector<double> > simulateModel(double initialTime, double startTime, double endTime, double numSteps);

	/**
	 * this method brings the model to the next output point. Returns 0 on success.
	 */
	int simulateModelOneStep(double stepSize);

	/**
	 * Reset the integrator. If an integrator exists, simply destroy it so that it will be re-created
	 * next time the oneStep method is called.
	 */
	int resetIntegrator();

    /**
      * Set the tolerances and maximum number of steps in the integtator.
      */
    void setTolerances(double aTol, double rTol, int maxSteps);

private:
	std::string mUrl;
	std::vector<std::string> mVariableIds;
	struct CellMLModel* mModel;
	struct Simulation* mSimulation;
	class CellmlCode* mCode;
	class ExecutableModel* mExecutableModel;
	struct Integrator* mIntegrator;
	double* mBoundCache;
	double* mRatesCache;
	double* mStatesCache;
	double* mConstantsCache;
	double* mAlgebraicCache;
	double* mOutputsCache;
};

#endif /* CELLMLSIMULATOR_HPP_ */
