/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef GROUNDING_GROUNDTHEORY_HPP_
#define GROUNDING_GROUNDTHEORY_HPP_

#include "IncludeComponents.hpp"
#include "AbstractGroundTheory.hpp"

template<class Policy>
class GroundTheory: public AbstractGroundTheory, public Policy {
	std::set<int> _printedtseitins; //!< Tseitin atoms produced by the translator that occur in the theory.
	std::set<int> _printedsets; //!< Set numbers produced by the translator that occur in the theory.
	std::set<int> _printedconstraints; //!< Atoms for which a connection to CP constraints are added.
	std::set<CPTerm*> _foldedterms;
	std::map<PFSymbol*, std::set<int> > _defined; //!< List of defined symbols and the heads which have a rule.

public:
	GroundTheory(AbstractStructure* str);
	GroundTheory(Vocabulary* voc, AbstractStructure* str);

	virtual ~GroundTheory() {
	}

	void add(Formula*) {
		Assert(false);
	}
	void add(Definition*) {
		Assert(false);
	}
	void add(FixpDef*) {
		Assert(false);
	}

	virtual void recursiveDelete();

	void closeTheory();

	void add(const GroundClause& cl, bool skipfirst = false);
	void add(const GroundDefinition& def);
	void add(int defid, PCGroundRule* rule);
	void add(GroundFixpDef*);
	void add(int tseitin, CPTsBody* body);
	void add(int setnr, unsigned int defnr, bool weighted);
	void add(int head, AggTsBody* body);
	void add(const Lit& head, TsType tstype, const std::vector<Lit>& clause, bool conj, int defnr);

	std::ostream& put(std::ostream& s) const;

	void accept(TheoryVisitor* v) const;
	AbstractTheory* accept(TheoryMutatingVisitor* v);

protected:
	void transformForAdd(const std::vector<int>& vi, VIType /*vit*/, int defnr, bool skipfirst = false);

	CPTerm* foldCPTerm(CPTerm* cpterm);

	/**
	 *	Adds constraints to the theory that state that each of the functions that occur in the theory is indeed a function.
	 *	This method should be called before running the SAT solver and after grounding.
	 */
	void addFuncConstraints();

	void addFalseDefineds();

private:
	void notifyDefined(int inputatom);
	void addRangeConstraint(Function* f, const litlist& set, SortTable* outSortTable);
};

#endif /* GROUNDING_GROUNDTHEORY_HPP_ */
