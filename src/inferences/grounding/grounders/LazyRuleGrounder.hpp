/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef LAZYRULEGROUNDER_HPP_
#define LAZYRULEGROUNDER_HPP_

#include "DefinitionGrounders.hpp"
#include "LazyFormulaGrounders.hpp"

class LazyRuleGrounder: public RuleGrounder, public LazyUnknBoundGrounder {
private:
	std::vector<std::pair<int, int> > sameargs; // a list of indices into the head terms which are the same variables

public:
	LazyRuleGrounder(const Rule* rule, const std::vector<Term*>& vars, HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct);
	void run(DefId defid, GroundDefinition* grounddefinition) const;

private:
	enum class Substitutable { UNIFIABLE, NO_UNIFIER};
	Substitutable createInst(const ElementTuple& headargs, dominstlist& domlist);

	void doGround(const Lit& head, const ElementTuple& headargs);
};

#endif /* LAZYRULEGROUNDER_HPP_ */