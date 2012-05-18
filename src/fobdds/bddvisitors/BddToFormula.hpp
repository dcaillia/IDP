/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef BDDTOFORMULA_HPP_
#define BDDTOFORMULA_HPP_

#include "IncludeComponents.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFuncTerm.hpp"
#include "fobdds/FoBddAggTerm.hpp"
#include "fobdds/FoBddAggKernel.hpp"
#include "fobdds/FoBddDomainTerm.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "fobdds/FoBddIndex.hpp"
#include "fobdds/FoBddAtomKernel.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddUtils.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddSetExpr.hpp"
#include "theory/TheoryUtils.hpp"

#include <vector>

/**
 * Given a bdd or a kernel, creates the associated formula.
 * Given a bddterm, creates the associated term.
 */
class BDDToFO: public FOBDDVisitor {
protected:
	Formula* _currformula;
	Term* _currterm;
	SetExpr* _currset;
	std::map<const FOBDDDeBruijnIndex*, Variable*> _dbrmapping;

	void reset() {
		_currformula = NULL;
		_currterm = NULL;
		_currset = NULL;
		_dbrmapping.clear();
	}

public:
	BDDToFO(FOBDDManager* m)
			: FOBDDVisitor(m), _currformula(NULL), _currterm(NULL), _currset(NULL) {
		_dbrmapping.clear();
	}

	template<typename BddNode>
	Formula* createFormula(const BddNode* object) {
		reset();
		object->accept(this);
		return _currformula;
	}

	template<typename BddTerm>
	Term* createTerm(const BddTerm* arg) {
		reset();
		arg->accept(this);
		return _currterm;
	}

	template<typename BddSet>
	SetExpr* createSet(const BddSet* arg) {
		reset();
		arg->accept(this);
		return _currset;
	}

protected:
	virtual void visit(const FOBDDDeBruijnIndex* index) {
		auto it = _dbrmapping.find(index);
		Variable* v;
		Assert(it != _dbrmapping.cend());
		if (it == _dbrmapping.cend()) {
			v = new Variable(index->sort());
			_dbrmapping[index] = v;
		} else {
			v = it->second;
		}
		_currterm = new VarTerm(v, TermParseInfo());
	}

	virtual void visit(const FOBDDVariable* var) {
		_currterm = new VarTerm(var->variable(), TermParseInfo());
	}

	virtual void visit(const FOBDDDomainTerm* dt) {
		_currterm = new DomainTerm(dt->sort(), dt->value(), TermParseInfo());
	}

	virtual void visit(const FOBDDFuncTerm* ft) {
		std::vector<Term*> args;
		for (auto it = ft->args().cbegin(); it != ft->args().cend(); ++it) {
			(*it)->accept(this);
			args.push_back(_currterm);
		}
		_currterm = new FuncTerm(ft->func(), args, TermParseInfo());
	}

	virtual void visit(const FOBDDAggTerm* aggterm) {
		aggterm->setexpr()->accept(this);
		auto set = _currset;
		_currterm = new AggTerm(set, aggterm->aggfunction(), TermParseInfo());
	}

	virtual void visit(const FOBDDEnumSetExpr* set) {
		std::vector<Formula*> formulas(set->size());
		std::vector<Term*> terms(set->size());
		for (int i = 0; i < set->size(); i++) {
			set->subformula(i)->accept(this);
			formulas[i] = _currformula;
			set->subterm(i)->accept(this);
			terms[i] = _currterm;
		}
		_currset = new EnumSetExpr(formulas, terms, SetParseInfo());
	}

	virtual void visit(const FOBDDQuantSetExpr* set) {
		std::map<const FOBDDDeBruijnIndex*, Variable*> savedmapping = _dbrmapping;
		_dbrmapping.clear();
		std::set<Variable*> vars;
		int i = 0;
		for (auto it = set->quantvarsorts().rbegin(); it != set->quantvarsorts().rend(); it++, i++) {
			auto v = new Variable(*it);
			_dbrmapping[_manager->getDeBruijnIndex(*it, i)] = v;
			vars.insert(v);
		}
		for (auto it = savedmapping.cbegin(); it != savedmapping.cend(); ++it) {
			_dbrmapping[_manager->getDeBruijnIndex(it->first->sort(), it->first->index() + set->quantvarsorts().size())] = it->second;
		}
		set->subformula(0)->accept(this);
		auto subform = _currformula;
		set->subterm(0)->accept(this);
		auto subterm = _currterm;
		_dbrmapping = savedmapping;
		_currset = new QuantSetExpr(vars, subform, subterm, SetParseInfo());
	}

	virtual void visit(const FOBDDAtomKernel* atom) {
		std::vector<Term*> args;
		for (auto it = atom->args().cbegin(); it != atom->args().cend(); ++it) {
			(*it)->accept(this);
			args.push_back(_currterm);
		}
		switch (atom->type()) {
		case AtomKernelType::AKT_TWOVALUED:
			_currformula = new PredForm(SIGN::POS, atom->symbol(), args, FormulaParseInfo());
			break;
		case AtomKernelType::AKT_CT:
			_currformula = new PredForm(SIGN::POS, atom->symbol()->derivedSymbol(ST_CT), args, FormulaParseInfo());
			break;
		case AtomKernelType::AKT_CF:
			_currformula = new PredForm(SIGN::POS, atom->symbol()->derivedSymbol(ST_CF), args, FormulaParseInfo());
			break;
		}

	}

	virtual void visit(const FOBDDQuantKernel* quantkernel) {
		std::map<const FOBDDDeBruijnIndex*, Variable*> savedmapping = _dbrmapping;
		_dbrmapping.clear();
		for (auto it = savedmapping.cbegin(); it != savedmapping.cend(); ++it) {
			_dbrmapping[_manager->getDeBruijnIndex(it->first->sort(), it->first->index() + 1)] = it->second;
		}
		auto index = _manager->getDeBruijnIndex(quantkernel->sort(), 0);
		auto quantvar = new Variable(index->sort());
		_dbrmapping[index] = quantvar;
		quantkernel->bdd()->accept(this);

		_dbrmapping = savedmapping;
		_currformula = new QuantForm(SIGN::POS, QUANT::EXIST, { quantvar }, _currformula, FormulaParseInfo());
	}

	virtual void visit(const FOBDDAggKernel* fak) {
		fak->left()->accept(this);
		auto left = _currterm;
		fak->right()->accept(this);
		Assert(isa<AggTerm>(*_currterm));
		auto right = dynamic_cast<AggTerm*>(_currterm);
		_currformula = new AggForm(SIGN::POS, left, fak->comp(), right, FormulaParseInfo());
	}

	virtual void visit(const FOBDD* bdd) {
		if (_manager->isTruebdd(bdd)) {
			_currformula = FormulaUtils::trueFormula();
			return;
		}

		if (_manager->isFalsebdd(bdd)) {
			_currformula = FormulaUtils::falseFormula();
			return;
		}

		bdd->kernel()->accept(this);
		// NOTE afterwards, _currformula is a formula representing the kernel

		if (_manager->isFalsebdd(bdd->falsebranch())) {
			if (_manager->isTruebdd(bdd->truebranch())) {
				return; // kernel is the whole formula
			}
			auto kernelformula = _currformula;
			bdd->truebranch()->accept(this);
			auto branchformula = _currformula;
			_currformula = new BoolForm(SIGN::POS, true, kernelformula, branchformula, FormulaParseInfo());
		} else if (_manager->isFalsebdd(bdd->truebranch())) {
			_currformula->negate();

			if (_manager->isTruebdd(bdd->falsebranch())) {
				return; // \lnot kernel is the whole formula
			}

			auto kernelformula = _currformula;
			bdd->falsebranch()->accept(this);
			auto branchformula = _currformula;
			_currformula = new BoolForm(SIGN::POS, true, kernelformula, branchformula, FormulaParseInfo());
		} else { // No branch is false
			auto kernelformula = _currformula;
			auto negkernelformula = kernelformula->clone();
			negkernelformula->negate();

			if (_manager->isTruebdd(bdd->falsebranch())) {
				bdd->truebranch()->accept(this);
				auto bf = new BoolForm(SIGN::POS, true, kernelformula, _currformula, FormulaParseInfo());
				_currformula = new BoolForm(SIGN::POS, false, negkernelformula, bf, FormulaParseInfo());
			} else if (_manager->isTruebdd(bdd->truebranch())) {
				bdd->falsebranch()->accept(this);
				auto bf = new BoolForm(SIGN::POS, true, negkernelformula, _currformula, FormulaParseInfo());
				_currformula = new BoolForm(SIGN::POS, false, kernelformula, bf, FormulaParseInfo());
			} else {
				bdd->truebranch()->accept(this);
				auto trueform = _currformula;
				bdd->falsebranch()->accept(this);
				auto falseform = _currformula;
				auto bf1 = new BoolForm(SIGN::POS, true, kernelformula, trueform, FormulaParseInfo());
				auto bf2 = new BoolForm(SIGN::POS, true, negkernelformula, falseform, FormulaParseInfo());
				_currformula = new BoolForm(SIGN::POS, false, bf1, bf2, FormulaParseInfo());
			}
		}
	}
};

#endif /* BDDTOFORMULA_HPP_ */
