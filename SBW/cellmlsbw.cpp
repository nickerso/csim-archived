#include <iostream>
#include <vector>
#include <SBW/SBW.h>

#include <cellmlsbw.h>
#include <CellmlSimulator.hpp>

#include <string.h> // for memset

using namespace std;

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
	// generate the code and make it executable
	if (csim->compileModel() != 0)
	{
		std::cerr << "CellmlSbw::loadCellml: Error compiling model." << std::endl;
		return;
	}
	// checkpoint the model at the initial values to provide the initial "reset" point.
	if (csim->checkpointModelValues() != 0)
	{
		std::cerr << "CellmlSbw::loadCellml: Error checkpointing initial values." << std::endl;
		return;
	}
	std::cout << "Loaded the model and did stuff successfully :)" << std::endl;
}

// this method resets the simulator back to initial conditions
void CellmlSbw::reset()
{
	if (csim->updateModelFromCheckpoint() != 0)
	{
		std::cerr << "CellmlSbw::reset: Error with reset." << std::endl;
		return;
	}
	/*
	 * FIXME: for now, assume that when the model is reset the integration also needs to be reset.
	 */
	csim->resetIntegrator();
	std::cout << "Success in resetting the model." << std::endl;
}
// this method sets the value of the given variable id (component.variable) to the given value
void CellmlSbw::setValue(const std::string& variableId, double value)
{
	if (csim->setVariableValue(variableId, value) != 0)
	{
		std::cerr << "CellmlSbw::setValue: Error setting the value of the variable: "
				<< variableId.c_str() << std::endl;
		return;
	}
	std::cout << "Success in setting the value of the variable: " << variableId.c_str() << std::endl;
}

// this method returns all variables as vector with elements of format component.variable
std::vector<std::string> CellmlSbw::getVariables()
{
	std::vector < std::string > listOfVariables = csim->getModelVariables();
	return listOfVariables;
}

// this method return the values of all variables at the last timepoint
std::vector<double> CellmlSbw::getValues()
{
	std::vector<double> listOfLastResults = csim->getModelOutputs();
	return listOfLastResults;
}

// this method simulates the loaded model returning the result as string
std::vector<std::vector<double> > CellmlSbw::simulate(double initialTime, double startTime,
		double endTime, int numSteps)
{
	/**
	 * FIXME: I'm presuming this is supposed to return the simulation results for the entire
	 * simulation run as a single string? This may not always be a viable operation to perform?
	 * But I vaguely recall Frank mentioning that most things are done via the oneStep method
	 * nowadays, so this might not be a problem?
	 */
	return csim->simulateModel(initialTime, startTime, endTime, numSteps);
}

// this method brings the model to the next output point
void CellmlSbw::oneStep(double stepSize)
{
	csim->simulateModelOneStep(stepSize);
}

// this method brings the model to the next steady state
void CellmlSbw::steadyState()
{
	std::cerr << "CellmlSbw::steadyState: Not implemented." << std::endl;
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

void addData(DataBlockWriter &writer, const vector<vector<double>> &data)
{
	int numRows = data.size();	
	int numCols = numRows > 0 ? data[0].size() : 0;
	
	// allocate
	double ** rawData = (double**)malloc(sizeof(double*)*numRows); 	
	memset(rawData, 0, sizeof(double*)*numRows);
	
	// copy
	for (size_t i = 0; i < data.size(); ++i)
	{
		rawData [i] = (double*)malloc(sizeof(double)*numCols);
		memset(rawData[i], 0, sizeof(double*)*numRows);
		for (size_t j = 0; j < data[j].size(); ++j)
		{
			rawData [i][j] = data[i][j];
#ifdef DEBUG
			cout << rawData [i][j] << '\t';
#endif
		}
#ifdef DEBUG
		cout << endl;
#endif
	}
	
	// add to SBW
	writer.add(numRows, numCols, rawData);
	
	// cleanup
	for (size_t i = 0; i < data.size(); ++i)
		free (rawData[i]);
	free (rawData);
	
}

DataBlockWriter CellmlSbw::simulateImpl(Module from, DataBlockReader reader)
{
	double initialTime, startTime, endTime;
	int numPoints;
	reader >> initialTime >> startTime >> endTime >> numPoints;
	const vector<vector<double>> &data = simulate(initialTime, startTime, endTime, numPoints);
	DataBlockWriter result; 
	addData(result, data);
	return result;			
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
