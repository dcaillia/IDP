/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "LazyQuantGrounder.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"
#include "generators/InstGenerator.hpp"
#include "GroundUtils.hpp"
#include "inferences/grounding/GroundTranslator.hpp"

#include "IncludeComponents.hpp"

using namespace std;

unsigned int LazyQuantGrounder::maxid = 1;

LazyQuantGrounder::LazyQuantGrounder(const std::set<Variable*>& freevars, SolverTheory* groundtheory, FormulaGrounder* sub, SIGN sign, QUANT q,
		InstGenerator* gen, InstChecker* checker, const GroundingContext& ct)
		: QuantGrounder(groundtheory, sub, sign, q, gen, checker, ct), id_(maxid++), groundtheory_(groundtheory), _negatedformula(false),
			currentlyGrounding(false), freevars(freevars) {
	Assert(not conjunctive());
	// TODO: currently, can only lazy ground existential quants
	Assert(ct._tseitin != TsType::RULE);
	// TODO: currently only lazy ground formulas outside of definitions
}

void LazyQuantGrounder::requestGroundMore(ResidualAndFreeInst * instance) {
	notifyTheoryOccurence(instance);
}

// TODO toplevelquants?
// TODO lazy disjunctions and conjunctions?
// TODO delete free var ref and generator when at end
// TODO use the checker
// NOTE: generators are CLONED, SAVED and REUSED!

void LazyQuantGrounder::groundMore() const {
	if (verbosity() > 2) {
		printorig();
	}

	// if value is decided, allow to erase the formula

	currentlyGrounding = true;
	while (queuedtseitinstoground.size() > 0) {
		ResidualAndFreeInst* instance = queuedtseitinstoground.front();
		queuedtseitinstoground.pop();

		CHECKTERMINATION

		vector<const DomainElement*> originstantiation;
		overwriteVars(originstantiation, instance->freevarinst);

		Lit groundedlit = _false;
		while(groundedlit == _false && not instance->generator->isAtEnd()){
			ConjOrDisj formula;
			formula = ConjOrDisj();
			formula.setType(conn_);

			runSubGrounder(_subgrounder, context()._conjunctivePathFromRoot, formula);
			groundedlit = getReification(formula);
			instance->generator->operator ++();
		}

		restoreOrigVars(originstantiation, instance->freevarinst);

		Lit oldtseitin = instance->residual;

		if(groundedlit==_true){
			GroundClause c = {oldtseitin};
			groundtheory_->add(c);
			continue; // Formula is true, so do not need to continue grounding
		}else if(groundedlit == _false && instance->generator->isAtEnd()){
			GroundClause c = { -oldtseitin };
			groundtheory_->add(c);
			continue; // Formula is false, so do not need to continue grounding
		}

		GroundClause clause;
		clause.push_back(groundedlit);

		// TODO notify lazy should check whether the tseitin already has a value and request more grounding immediately!
		if (not instance->generator->isAtEnd()) {
			Lit newresidual = translator()->createNewUninterpretedNumber();
			clause.push_back(newresidual);
			instance->residual = newresidual;
			groundtheory_->notifyLazyResidual(instance, this); // set on not-decide and add to watchlist
		} else {
			// TODO deletion
		}

		groundtheory_->add(oldtseitin, context()._tseitin, clause);
	}

	currentlyGrounding = false;
}

void LazyQuantGrounder::internalRun(ConjOrDisj& formula) const {
	Assert(not conjunctive());
	formula.setType(Conn::DISJ);
	if (verbosity() > 2) {
		printorig();
	}

	_generator->begin();
	if (_generator->isAtEnd()) {
		return;
	}

	// Save the current instantiation of the free variables
	ResidualAndFreeInst* inst = new ResidualAndFreeInst();
	for (auto var = freevars.cbegin(); var != freevars.cend(); ++var) {
		auto tuple = varmap().at(*var);
		inst->freevarinst.push_back(dominst { tuple, tuple->get() });
	}
	inst->generator = _generator->clone();

	translator()->translate(this, inst, context()._tseitin);
	if (isNegative()) {
		inst->residual = -inst->residual;
	}
	formula.literals.push_back(inst->residual);
}

// restructured code to prevent recursion
void LazyQuantGrounder::notifyTheoryOccurence(ResidualAndFreeInst* instance) const {
	// FIXME duplication and const issues!
	queuedtseitinstoground.push(instance);
	if (not currentlyGrounding) {
		groundMore();
	}
}
