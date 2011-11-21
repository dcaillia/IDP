/************************************
	CombineConstsOfMults.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef TERMADDER_HPP_
#define TERMADDER_HPP_

#include <vector>
#include <cassert>
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddUtils.hpp"

#include "fobdds/bddvisitors/FirstConstTermInFunc.hpp"
#include "fobdds/bddvisitors/FirstNonConstMultTerm.hpp"

/**
 * Given the root is an addition
 *
 * TODO should only be called on certain parse trees, document this!
 */
class CombineConstsOfMults: public FOBDDVisitor {
public:
	CombineConstsOfMults(FOBDDManager* m) :
			FOBDDVisitor(m) {
	}

	const FOBDDArgument* change(const FOBDDFuncTerm* functerm) {
		if (not isAddition(functerm)) {
			return FOBDDVisitor::change(functerm);
		}

		FirstNonConstMultTerm ncte;
		auto leftncte = ncte.run(functerm->args(0));

		if (isBddFuncTerm(functerm->args(1))) {
			auto rightterm = getBddFuncTerm(functerm->args(1));
			if (isAddition(rightterm)) {
				auto rightncte = ncte.run(rightterm->args(0));
				if(leftncte != rightncte){
					return FOBDDVisitor::change(functerm);
				}
				FirstConstMultTerm cte(_manager);
				auto leftconst = cte.run(functerm->args(0));
				auto rightconst = cte.run(rightterm->args(0));
				auto addterm = add(_manager, leftconst, rightconst);
				auto mult = Vocabulary::std()->func("*/2");
				auto multsort = SortUtils::resolve(addterm->sort(), leftncte->sort());
				mult = mult->disambiguate(std::vector<Sort*>(3, multsort), NULL);
				assert(mult!=NULL);
				auto newterm = _manager->getFuncTerm(mult, {addterm, leftncte});
				auto plus = Vocabulary::std()->func("+/2");
				auto plussort = SortUtils::resolve(newterm->sort(), rightterm->args(1)->sort());
				plus = plus->disambiguate(std::vector<Sort*>(3, plussort), NULL);
				assert(plus!=NULL);
				auto addbddterm = _manager->getFuncTerm(plus, {newterm,rightterm->args(1)});
				return addbddterm->acceptchange(this);
			}
		}

		auto rightncte = ncte.run(functerm->args(1));
		if (leftncte == rightncte) {
			FirstConstMultTerm cte(_manager);
			auto leftconst = cte.run(functerm->args(0));
			auto rightconst = cte.run(functerm->args(1));
			auto addterm = add(_manager, leftconst, rightconst);
			auto mult = Vocabulary::std()->func("*/2");
			auto multsort = SortUtils::resolve(addterm->sort(), leftncte->sort());
			mult = mult->disambiguate(std::vector<Sort*>(3, multsort), NULL);
			assert(mult!=NULL);
			return _manager->getFuncTerm(mult, {addterm, leftncte});
		}

		return FOBDDVisitor::change(functerm);
	}
};

#endif /* TERMADDER_HPP_ */