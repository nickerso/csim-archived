#include <iostream>
#include <SBW/SBW.h>
#include <cellmlsbw.h>

#include <CellmlSimulator.hpp>

// public methods ... replace with actual calls to the CellML API

CellmlSbw::CellmlSbw()
{
	csim = new CellmlSimulator();
}

CellmlSbw::~CellmlSbw()
{
	if (csim) delete csim;
}

/*
 * FIXME: just adding this method for testing and demonstration - not sure if its appropriate here.
 */
// this method serialises the model from the given URL into a string and ensures the the xml:base is set appropriately
std::string CellmlSbw::serialiseCellmlFromUrl(const std::string & url)
{
	std::cout << "Serialising the CellML model: \"" << url.c_str() << "\"" << std::endl;
	std::string model = csim->serialiseCellmlFromUrl(url);
	return model;
}

// this method loads the given model into the cellml simulator
/* FIXME: We assume here that the xml:base of the serialised document has been set correctly
 * to ensure that imports can be resolved appropriately.
 */
void CellmlSbw::loadCellml(const std::string & cellmlModelString)
{
	//std::cout << "Loading CellML model: \"" << cellmlModelString.c_str() << "\"" << std::endl;
	// load the CellML model
	if (csim->loadModelString(cellmlModelString) != 0)
	{
		std::cerr << "CelllmlSbw::loadCellml: Error loading model string." << std::endl;
		return;
	}
	// make the (dummy) simulation object, which will have all variables in the top-level model
	// as outputs.
	if (csim->createSimulationDefinition() != 0)
	{
		std::cerr << "CellmlSbw::loadCellml: Error creating the simulation definition." << std::endl;
		return;
	}
	// generate the code
	if (csim->generateCode() != 0)
	{
		std::cerr << "CellmlSbw::loadCellml: Error generating code." << std::endl;
		return;
	}
	// create output variables for all variables in the top-level model
	// use the getOutputs method to get the values to return in getValues
	std::cout << "Loaded the model and did stuff successfully :)" << std::endl;
}

// this method resets the simulator back to initial conditions
void CellmlSbw::reset()
{

}
// this method sets the value of the given component id (component.variable) to the given value
void CellmlSbw::setValue(const std::string& componentId, double value)
{

}

// this method returns all variables as vector with elements of format component.variable
std::vector<std::string> CellmlSbw::getVariables()
{
	std::vector < std::string > listOfVariables;

	listOfVariables.push_back("component.time");
	listOfVariables.push_back("component.A");

	return listOfVariables;
}

// this method return the values of all variables at the last timepoint
std::vector<double> CellmlSbw::getValues()
{
	std::vector<double> listOfLastResults;

	listOfLastResults.push_back(1.1);
	listOfLastResults.push_back(2.1);

	return listOfLastResults;
}

// this method simulates the loaded model returning the result as string
std::string CellmlSbw::simulate(double initialTime, double startTime,
		double endTime, int numSteps)
{
	return "the same results as obtained / written to file before";
}

// this method brings the model to the next output point
void CellmlSbw::oneStep(double stepSize)
{

}

// this method brings the model to the next steady state
void CellmlSbw::steadyState()
{

}

// protected methods, the actual sbw calls

DataBlockWriter CellmlSbw::loadCellmlImpl(Module from, DataBlockReader reader)
{
	std::string cellmlModelString;
	reader >> cellmlModelString;
	loadCellml(cellmlModelString);
	return DataBlockWriter();
}

DataBlockWriter CellmlSbw::resetImpl(Module from, DataBlockReader reader)
{
	reset();
	return DataBlockWriter();
}

DataBlockWriter CellmlSbw::setValueImpl(Module from, DataBlockReader reader)
{
	std::string component;
	double value;
	reader >> component >> value;
	setValue(component, value);
	return DataBlockWriter();
}

DataBlockWriter CellmlSbw::getVariablesImpl(Module from, DataBlockReader reader)
{
	return DataBlockWriter() << getVariables();
}

DataBlockWriter CellmlSbw::getValuesImpl(Module from, DataBlockReader reader)
{
	return DataBlockWriter() << getValues();
}

DataBlockWriter CellmlSbw::simulateImpl(Module from, DataBlockReader reader)
{
	double initialTime, startTime, endTime;
	int numPoints;
	reader >> initialTime >> startTime >> endTime >> numPoints;
	return DataBlockWriter()
			<< simulate(initialTime, startTime, endTime, numPoints);
}

DataBlockWriter CellmlSbw::oneStepImpl(Module from, DataBlockReader reader)
{
	double stepSize;
	reader >> stepSize;

	oneStep(stepSize);

	return DataBlockWriter();
}

DataBlockWriter CellmlSbw::steadyStateImpl(Module from, DataBlockReader reader)
{
	steadyState();
	return DataBlockWriter();
}

DataBlockWriter CellmlSbw::serialiseCellmlFromUrlImpl(Module from, DataBlockReader reader)
{
	std::string url;
	reader >> url;
	return DataBlockWriter() << serialiseCellmlFromUrl(url);
}

int main(int argc, char* argv[])
{
	try
	{
		ModuleImpl modImpl("edu.caltech.cellmlsbw", // module identification
				"CellML Simulator (CSim) Wrapper", // humanly readable name
				UniqueModule); // management scheme
		modImpl.addServiceObject("cellmlsbw", // service identification
				"cellmlsbw written in C++", // humanly readable name
				"cellml", // category
				new CellmlSbw()); // service implementation
		// connect to broker providing services
		modImpl.run(argc, argv);
	}
	catch (SBWException *e)
	{
		return -1;
	}
	return 0;
}
