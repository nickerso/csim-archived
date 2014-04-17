
#include <iostream>
#include <string>
#include <cstdlib>
#include <CellmlSimulator.hpp>

static void usage(const char* progname)
{
    std::cerr << "Usage: " << progname << " <model URL> <stop time> <dt>"
              << std::endl;
}

int main(int argc, const char* argv[])
{
    if (argc != 4)
    {
        usage(argv[0]);
        return -1;
    }
    std::string url = argv[1];
    double tEnd = atof(argv[2]);
    double dt = atof(argv[3]);
    std::cout << "Simulating model: " << url << "; over the interval 0.0 -> " << tEnd << " with time step: "
              << dt << std::endl;
    // create a simulator
    CellmlSimulator csim;
    // load a CellML model into a single string (this will flatten any imports)
    std::string modelString = csim.serialiseCellmlFromUrl(url);
    // and then load the CellML model string into CSim
    csim.loadModelString(modelString);
    // we can now create a dummy simulation defintion (a hangover from the original code-base)
    csim.createSimulationDefinition();
    // for now, simply flag all variables in the model for output
    csim.setAllVariablesOutput();
    // and now we can compile the model into executable code
    csim.compileModel();
    // pull out the ID for all the variables in the model (which are all the outputs since we flagged them above)
    std::vector<std::string> names = csim.getModelVariables();
    // save the state of the model for future use
    csim.checkpointModelValues();
    // dump out the variable names to the terminal.
    for (const auto& s: names) std::cout << s << "\t";
    std::cout << std::endl;
    // run the full simulation in one hit
    std::vector<std::vector<double> > values = csim.simulateModel(0.0, 0.0, tEnd, tEnd / dt);
    // and dump the results to the terminal
    for (const auto& vec: values)
    {
        for (const auto& v: vec) std::cout << v << "\t";
        std::cout << std::endl;
    }
    // reset the model to the initial state
    csim.updateModelFromCheckpoint();
    // and reinitialise the integrator
    csim.resetIntegrator();
    // dump out the variable names to the terminal
    for (const auto& s: names) std::cout << s << "\t";
    std::cout << std::endl;
    // along with the initial conditions
    std::vector<double> vec = csim.getModelOutputs();
    for (const auto& v: vec) std::cout << v << "\t";
    std::cout << std::endl;
    // now integrate the model using the one-step method
    for (double time = 0.0; time <= tEnd; time += dt)
    {
        csim.simulateModelOneStep(dt);
        // dump out the variable values to the terminal
        std::vector<double> vec = csim.getModelOutputs();
        for (const auto& v: vec) std::cout << v << "\t";
        std::cout << std::endl;
    }
    return 0;
}

