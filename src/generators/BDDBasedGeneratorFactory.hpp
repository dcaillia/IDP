/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef BDDBASEDGENERATORFACTORY_HPP_
#define BDDBASEDGENERATORFACTORY_HPP_

#include "commontypes.hpp"
#include "GeneratorFactory.hpp"

class FOBDDManager;
class FOBDDVariable;
class FOBDD;
class FOBDDKernel;
class GeneratorNode;
class Term;
class PredForm;
class Variable;
class InstGenerator;
class DomElemContainer;
class AbstractStructure;
class Universe;

struct BddGeneratorData {
	const FOBDD* bdd;
	std::vector<Pattern> pattern;
	std::vector<const DomElemContainer*> vars;
	std::vector<const FOBDDVariable*> bddvars;
	const AbstractStructure* structure;
	Universe universe;

	bool check() const {
		return pattern.size() == vars.size() && pattern.size() == bddvars.size() && pattern.size() == universe.tables().size();
	}
};

enum class BRANCH {
	FALSEBRANCH, TRUEBRANCH
};

/**
 * Class to convert a bdd into a generator
 */
class BDDToGenerator {
private:
	FOBDDManager* _manager;

	/*
	 * Help-method for creating from predform
	 */
	PredForm* smartGraphFunction(PredForm* atom, const std::vector<Pattern>& pattern, const std::vector<Variable*>& atomvars);

	/*
	 * Creates an instance generator from a predform (i.e.~an atom kernel).
	 * Can only be used on atoms that contain only var and domain terms (no functions and aggregates)
	 */
	InstGenerator *createFromSimplePredForm(PredForm *atom, const std::vector<Pattern>& pattern, const std::vector<const DomElemContainer*> & vars,
			const std::vector<Variable*> & atomvars, const AbstractStructure *structure, BRANCH branchToGenerate, const Universe & universe);

	/*
	 * Creates an instance generator from a predform (i.e.~an atom kernel).
	 * branchToGenerate determines whether all instances for which the predform evaluates to true
	 * or all instances for which the predform evaluates to false are generated
	 */
	InstGenerator* createFromPredForm(PredForm*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<Variable*>&,
			const AbstractStructure*, BRANCH branchToGenerate, const Universe&);

	/*
	 * Creates an instance generator from a aggform (i.e.~an aggkernel).
	 * branchToGenerate determines whether all instances for which the predform evaluates to true
	 * or all instances for which the aggform evaluates to false are generated
	 */
	InstGenerator* createFromAggForm(AggForm*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<Variable*>&,
			const AbstractStructure*, BRANCH branchToGenerate, const Universe&);

	/*
	 * Creates an instance generator from a formula
	 * PRECONDITION: f is either a predform or an aggform (i.e.~is the direct translation of an atomkernel or an aggkernel)
	 */
	InstGenerator* createFromFormula(Formula* f, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&, const std::vector<Variable*>&,
			const AbstractStructure*, BRANCH branchToGenerate, const Universe&);
	/*
	 * Creates an instance generator from a kernel.  branchToGenerate determines whether all instances for which the kernel evaluates to true
	 * or all instances for which the kernel evaluates to false are generated
	 */
	InstGenerator* createFromKernel(const FOBDDKernel*, const std::vector<Pattern>&, const std::vector<const DomElemContainer*>&,
			const std::vector<const FOBDDVariable*>&, const AbstractStructure* structure, BRANCH branchToGenerate, const Universe&);

	std::vector<InstGenerator*> turnConjunctionIntoGenerators(const std::vector<Pattern> & pattern, const std::vector<const DomElemContainer*> & vars,
			const std::vector<Variable*> & atomvars, const Universe & universe, QuantForm *quantform, const AbstractStructure *structure,
			std::vector<Formula*> conjunction, Formula *origatom, BRANCH branchToGenerate);

	GeneratorNode* createnode(const BddGeneratorData& data);

public:
	BDDToGenerator(FOBDDManager* manager);

	/**
	 * Create a generator which generates all instances for which the formula is CERTAINLY TRUE in the given structure.
	 */
	InstGenerator* create(const BddGeneratorData& data);
};

#endif /* BDDBASEDGENERATORFACTORY_HPP_ */
