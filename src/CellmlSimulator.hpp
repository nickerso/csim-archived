/*
 * CellmlSimulator.hpp
 *
 *  Created on: Aug 15, 2012
 *      Author: dnic019
 */

#ifndef CELLMLSIMULATOR_HPP_
#define CELLMLSIMULATOR_HPP_

struct CellMLModel;
struct Simulation;
class CellmlCode;

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
	 * Generate the code ready for executing the model description. Return 0 on success.
	 * FIXME: could propagate debug switch and saving generated file?
	 */
	int generateCode();

private:
	std::string mUrl;
	struct CellMLModel* mModel;
	struct Simulation* mSimulation;
	class CellmlCode* mCode;
};

#endif /* CELLMLSIMULATOR_HPP_ */
