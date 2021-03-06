/*
 * CellmlCode.hpp
 *
 *  Created on: May 31, 2012
 *      Author: dnic019
 */

#ifndef CELLMLCODE_HPP_
#define CELLMLCODE_HPP_

struct Simulation;
struct CellMLModel;

class CellmlCode
{
public:
	CellmlCode();
	CellmlCode(bool saveGeneratedCode);
	~CellmlCode();

	int createCodeForSimulation(struct Simulation* simulation, bool generateDebugCode = false);
	int createCodeForSimulation(struct CellMLModel* model, struct Simulation* simulation,
			bool generateDebugCode = false);

	const char* const codeFileName()
	{
		return mCodeFileName.c_str();
	}

private:

	bool mSaveGeneratedCode;
	std::string mCodeFileName;
	std::string mModelUri;
	bool mCodeFileExists;

};

#endif /* CELLMLCODE_HPP_ */
