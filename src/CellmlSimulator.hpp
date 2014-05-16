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
#include <map>
#include <string>

struct CellMLModel;
struct Simulation;
struct Integrator;
class CellmlCode;
class ExecutableModel;
class XmlDoc;

class CSIM_API CellmlSimulator
{
public:
	CellmlSimulator();
	~CellmlSimulator();

	/**
     * Serialise the model from the given URL into a string. The model is flattened before serialising
     * to the string to ensure that the string contains the complete model without requiring future
     * resolution of imports. The xml:base of the flattened model is set to the given URL for future
     * reference.
     *
     * Note: Internally, we keep a copy of the original (un-flattened) XML document for use in resolving
     *      XPath expressions. So it is always best to use this method to get the model string.
	 */
	std::string serialiseCellmlFromUrl(const std::string& url);

	/**
	 * Load the CellML model from the given string. Returns 0 on success.
	 */
	int loadModelString(const std::string& modelString);

    /**
     * Map an XPath expression to a variable identifier suitable for use with the CellmlSimulator
     * methods. SED-ML specifies that all XPath expressions resolve to a single DOM node, so invalid
     * expressions will return an empty string.
     */
    std::string mapXpathToVariableId(const std::string& xpathExpr,
                                     const std::map<std::string, std::string>& namespaces);

	/**
     * Create a (dummy) simulation definition for this model. Returns 0 on success.
	 */
	int createSimulationDefinition();

    /**
     * Sets all variables in the top-level model as output variables. Since we work with the flattened model
     * this will be all variables?
     * @return zero on success.
     */
    int setAllVariablesOutput();

    /**
     * Add the given variable to the list of output variables for this simulation instance.
     * @param variableId The ID (componentName.variableName) for the variable to add.
     * @param columnIndex The index of this variable in the output array (first index = 1).
     * @return zero on success.
     */
    int addOutputVariable(const std::string& variableId, int columnIndex);

	/**
     * Generate code and compile the model into an executable object.
     * @param saveGeneratedCode If true, the generated code will be not be deleted (defaults to false).
     * @return zero on success.
	 */
    int compileModel(bool saveGeneratedCode = false);

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
    class XmlDoc* mXmlDoc;
	struct Integrator* mIntegrator;
	double* mBoundCache;
	double* mRatesCache;
	double* mStatesCache;
	double* mConstantsCache;
	double* mAlgebraicCache;
	double* mOutputsCache;
};

#endif /* CELLMLSIMULATOR_HPP_ */
