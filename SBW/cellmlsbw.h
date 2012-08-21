#ifndef CELLML_SBW
#define CELLML_SBW

#include <SBW/SBW.h>

class CellmlSimulator;

class CellmlSbw
{

public:
	CellmlSbw();
	~CellmlSbw();

private:
	CellmlSimulator* csim;

public:
	// this method loads the given model into the cellml simulator
	void loadCellml(const std::string & cellmlModelString);
	// this method resets the simulator back to initial conditions
	void reset();
	// this method sets the value of the given variable id (component.variable) to the given value
	void setValue(const std::string& variableId, double value);
	// this method returns all variables as vector with elements of format component.variable
	std::vector<std::string> getVariables();
	// this method return the values of all variables at the last timepoint
	std::vector<double> getValues();
	// this method simulates the loaded model returning the result as string
	std::string simulate(double initialTime, double startTime, double endTime, int numSteps);
	// this method brings the model to the next output point
	void oneStep(double stepSize);
	// this method brings the model to the next steady state
	void steadyState();

	/*
	 * FIXME: just adding this method for testing and demonstration - not sure if its appropriate here.
	 */
	// this method serialises the model from the given URL into a string and ensures the the xml:base is set appropriately
	std::string serialiseCellmlFromUrl(const std::string & url);

protected :
	// the sbw implementation of loadCellml
	DataBlockWriter loadCellmlImpl(Module from, DataBlockReader reader);
	// the sbw implementation of the reset call
	DataBlockWriter resetImpl(Module from, DataBlockReader reader);
	// the sbw implementation of the setValue(string, double) function
	DataBlockWriter setValueImpl(Module from, DataBlockReader reader);
	// the sbw implementation of the getVariablesCall
	DataBlockWriter getVariablesImpl(Module from, DataBlockReader reader);
	// the sbw implementation of the getVariablesCall
	DataBlockWriter getValuesImpl(Module from, DataBlockReader reader);
	// the sbw implementation of the simulateCall
	DataBlockWriter simulateImpl(Module from, DataBlockReader reader);
	// the sbw implementation of the oneStep method
	DataBlockWriter oneStepImpl(Module from, DataBlockReader reader);
	// the sbw implementation of the steady state call
	DataBlockWriter steadyStateImpl(Module from, DataBlockReader reader);
	// the sbw implementation of the serialise CellML model method
	DataBlockWriter serialiseCellmlFromUrlImpl(Module from, DataBlockReader reader);

public:
	// the registration of methods with sbw
	static void registerMethods(MethodTable<CellmlSbw> &table)
	{
		table.addMethod(&CellmlSbw::loadCellmlImpl, "void loadCellml(string)");
		table.addMethod(&CellmlSbw::resetImpl, "void reset()");
		table.addMethod(&CellmlSbw::setValueImpl, "void setValue(string, double)");
		table.addMethod(&CellmlSbw::getVariablesImpl, "string[] getVariables()");
		table.addMethod(&CellmlSbw::getValuesImpl, "double[] getValues()");
		table.addMethod(&CellmlSbw::simulateImpl, "string simulate(double,double,double,int)");
		table.addMethod(&CellmlSbw::oneStepImpl, "void oneStep(double)");
		table.addMethod(&CellmlSbw::steadyStateImpl, "void steadyState()");
		table.addMethod(&CellmlSbw::serialiseCellmlFromUrlImpl, "string serialiseCellmlFromUrl(string)");
	}

};

#endif //CELLML_SBW
