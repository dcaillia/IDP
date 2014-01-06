/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once
#include <vector>
#include <set>
#include "vocabulary/vocabulary.hpp"

class Structure;
class Theory;
class Definition;

/*
 * The resulting struct for a definition calculation. It contains:
 *
 *  - A bool to indicate whether the total definition calculation resulted
 *    in a two-valued structure.
 *  - The resulting structure.
 *  - The calculated definitions. The user is responsible for deleting these
 *    properly, as they will no longer be present in the theory.
 *
 *  If not all definitions could be calculated (one of them resulted in an
 *  inconsistent result), the structure in this struct is the one used for
 *  the unsuccessful definition calculation and the set of definitions
 *  contains all (up to that point) successfully calculated definitions.
 */
struct DefinitionCalculationResult {
	bool _hasModel;
	Structure* _calculated_model;
	std::vector<Definition*> _calculated_definitions;

	DefinitionCalculationResult() :
		_calculated_definitions()
	{
		_hasModel = false;
		_calculated_model = NULL;
	}


};

class CalculateDefinitions {
public:
	/*
	 * !Removes calculated definitions from the theory.
	 * Also modifies the structure. Clone your theory and structure before doing this!
	 *
	 * parameter satdelay:
	 *		If true: allow further code to not calculate the definition if it is too big
	 *
	 * parameter symbolsToQuery:
	 * 		A subset of the defined symbols that you are interested in.
	 * 		Defined symbols not in this set will not be calculated.
	 */
	static DefinitionCalculationResult doCalculateDefinitions(Theory* theory, Structure* structure,
			bool satdelay = false, std::set<PFSymbol*> symbolsToQuery = std::set<PFSymbol*>()) {
		CalculateDefinitions c;
		return c.calculateKnownDefinitions(theory, structure, satdelay, symbolsToQuery);
	}
	static DefinitionCalculationResult doCalculateDefinitions(
			Definition* definition, Structure* structure, bool satdelay = false,
			std::set<PFSymbol*> symbolsToQuery = std::set<PFSymbol*>()) {
		CalculateDefinitions c;
		return c.calculateKnownDefinition(definition, structure, satdelay, symbolsToQuery);
	}

private:
	DefinitionCalculationResult calculateKnownDefinitions(Theory* theory, Structure* structure,
			bool satdelay, std::set<PFSymbol*> symbolsToQuery) const;

	DefinitionCalculationResult calculateKnownDefinition(Definition* definition, Structure* structure,
			bool satdelay, std::set<PFSymbol*> symbolsToQuery);

	DefinitionCalculationResult calculateDefinition(Definition* definition, Structure* structure,
			bool satdelay, bool& tooExpensive, bool withxsb, std::set<PFSymbol*> symbolsToQuery) const;

	DefinitionCalculationResult processXSBResult(const bool success, Structure* structure,
			Structure* oldStructure) const;
};
