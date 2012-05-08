/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CONTAINSAGGCTERMS_HPP_
#define CONTAINSAGGCTERMS_HPP_

#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBdd.hpp"

/**
 * Checks whether the given term contains Aggregates
 */
class ContainsAggTerms: public FOBDDVisitor {
private:
	bool _result;
	void visit(const FOBDDAggTerm*) {
		_result = true;
		return;
	}

public:
	ContainsAggTerms(FOBDDManager* m)
			: FOBDDVisitor(m) {
	}
	template<typename Tree>
	bool check(const Tree* arg) {
		_result = false;
		arg->accept(this);
		return _result;
	}
};

#endif /* CONTAINSAGGCTERMS_HPP_ */
