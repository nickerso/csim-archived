
#ifndef _CELLML_HPP_
#define _CELLML_HPP_

struct CellMLModel;
// forward declare from CellML API
namespace iface
{
	namespace cellml_api
	{
		class Model;
	}
}

/* methods shared for other C++ code to use? */

/* a method to generate an absolute URL based on a model and a possibly
   relative initial URL */
std::wstring
resolveAbsoluteURL(iface::cellml_api::Model* aModel,std::wstring aURL);

/**
 * Create an output variable list containing all variables in the top-level model - i.e., all
 * variables in the model which can be addressed using XPath (from SED-ML). Will clear and then
 * populate the variableIds vector with the variable's in the correct order so that we can be sure
 * to index the output array appropriately from the SBW module. Variable IDs are of the form
 * "componentName.variableName" in order to uniquely identify the variables in the model's top-level.
 */
void* createOutputVariablesForAllLocalComponents(struct CellMLModel* model,
		std::vector<std::string>& variableIds);

#endif /* _CELLML_HPP_ */
