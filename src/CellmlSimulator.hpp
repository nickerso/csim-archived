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

private:
	std::string mUrl;
	struct CellMLModel* mModel;
	struct Simulation* mSimulation;
};

#endif /* CELLMLSIMULATOR_HPP_ */
