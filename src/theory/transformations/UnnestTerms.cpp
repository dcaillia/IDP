/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "IncludeComponents.hpp"
#include "UnnestTerms.hpp"

#include "errorhandling/error.hpp"
#include "theory/TheoryUtils.hpp"
#include "theory/term.hpp"

#include <numeric> // for accumulate
#include <functional> // for multiplies
#include <algorithm> // for min_element and max_element
using namespace std;

UnnestTerms::UnnestTerms()
		: 	_structure(NULL),
			_vocabulary(NULL),
			_context(Context::POSITIVE),
			_allowedToUnnest(false),
			_chosenVarSort(NULL) {
}

void UnnestTerms::contextProblem(Term* t) {
	if (t->pi().userDefined()) {
		if (TermUtils::isPartial(t)) {
			Warning::ambigpartialterm(toString(t->pi().originalobject()), t->pi());
		}
	}
}

/**
 * Returns true is the term should be moved
 * (this is the most important method to overwrite in subclasses)
 */
bool UnnestTerms::shouldMove(Term* t) {
	return isAllowedToUnnest() && t->type() != TermType::VAR && t->type() != TermType::DOM;
}
/**
 * Tries to derive a sort for the term given a structure.
 */
Sort* UnnestTerms::deriveSort(Term* term) {
	auto sort = (_chosenVarSort != NULL) ? _chosenVarSort : term->sort();
	if (_structure != NULL && SortUtils::isSubsort(term->sort(), get(STDSORT::INTSORT), _vocabulary)) {
		sort = TermUtils::deriveSmallerSort(term, _structure);
	}
	return sort;
}

/**
 * Given a term t
 * 		Add a quantified variable v over sort(t)
 * 		Add an equality t =_sort(t) v
 * 		return v
 */
Term* UnnestTerms::move(Term* origterm) {
	if (getContext() == Context::BOTH) {
		contextProblem(origterm);
	}

	auto newsort = deriveSort(origterm);
	Assert(newsort != NULL);

	auto var = new Variable(newsort);
	_variables.insert(var);

	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 1) {
		Warning::introducedvar(var->name(), var->sort()->name(), toString(origterm));
	}

	auto varterm = new VarTerm(var, TermParseInfo(origterm->pi()));
	auto equalatom = new PredForm(SIGN::POS, get(STDPRED::EQ, origterm->sort()), { varterm, origterm }, FormulaParseInfo());
	_equalities.push_back(equalatom);
	return varterm->clone();
}

/**
 * Rewrite the given formula with the current equalities
 */
Formula* UnnestTerms::rewrite(Formula* formula) {
	const FormulaParseInfo& origpi = formula->pi();
	bool univ_and_disj = false;
	if (getContext() == Context::POSITIVE) {
		univ_and_disj = true;
		for (auto it = _equalities.cbegin(); it != _equalities.cend(); ++it) {
			(*it)->negate();
		}
	}
	if (not _equalities.empty()) {
		_equalities.push_back(formula);
		if (not _variables.empty()) {
			formula = new BoolForm(SIGN::POS, !univ_and_disj, _equalities, origpi);
		} else {
			formula = new BoolForm(SIGN::POS, !univ_and_disj, _equalities, FormulaParseInfo());
		}
		_equalities.clear();
	}
	if (not _variables.empty()) {
		formula = new QuantForm(SIGN::POS, (univ_and_disj ? QUANT::UNIV : QUANT::EXIST), _variables, formula, origpi);
		_variables.clear();
	}
	return formula;
}

/**
 *	Visit all parts of the theory, assuming positive context for sentences
 */
Theory* UnnestTerms::visit(Theory* theory) {
	for (auto it = theory->sentences().begin(); it != theory->sentences().end(); ++it) {
		setContext(Context::POSITIVE);
		setAllowedToUnnest(false);
		*it = (*it)->accept(this);
	}
	for (auto it = theory->definitions().begin(); it != theory->definitions().end(); ++it) {
		*it = (*it)->accept(this);
	}
	for (auto it = theory->fixpdefs().begin(); it != theory->fixpdefs().end(); ++it) {
		*it = (*it)->accept(this);
	}
	return theory;
}

void UnnestTerms::visitRuleHead(Rule* rule) {
	Assert(_equalities.empty());
	for (size_t termposition = 0; termposition < rule->head()->subterms().size(); ++termposition) {
		auto term = rule->head()->subterms()[termposition];
		if (shouldMove(term)) {
			auto new_head_term = move(term);
			rule->head()->subterm(termposition, new_head_term);
		}
	}
	if (not _equalities.empty()) {
		for (auto it = _variables.cbegin(); it != _variables.cend(); ++it) {
			rule->addvar(*it);
		}
		if (not rule->body()->trueFormula()) {
			_equalities.push_back(rule->body());
		}
		rule->body(new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo()));

		_equalities.clear();
		_variables.clear();
	}
}

/**
 *	Visit a rule, assuming negative context for body.
 *	May move terms from rule head.
 */
Rule* UnnestTerms::visit(Rule* rule) {
// Visit head
	auto saveallowed = isAllowedToUnnest();
	setAllowedToUnnest(true);
	visitRuleHead(rule);

// Visit body
	_context = Context::NEGATIVE;
	setAllowedToUnnest(false);
	rule->body(rule->body()->accept(this));
	setAllowedToUnnest(saveallowed);
	return rule;
}

Formula* UnnestTerms::traverse(Formula* f) {
	Context savecontext = _context;
	bool savemovecontext = isAllowedToUnnest();
	if (isNeg(f->sign())) {
		setContext(not _context);
	}
	for (size_t n = 0; n < f->subterms().size(); ++n) {
		f->subterm(n, f->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < f->subformulas().size(); ++n) {
		f->subformula(n, f->subformulas()[n]->accept(this));
	}
	setContext(savecontext);
	setAllowedToUnnest(savemovecontext);
	return f;
}

Formula* UnnestTerms::visit(EquivForm* ef) {
	Context savecontext = getContext();
	setContext(Context::BOTH);
	auto newef = traverse(ef);
	setContext(savecontext);
	return doRewrite(newef);
}

Formula* UnnestTerms::visit(AggForm* af) {
	auto newaf = traverse(af);
	return doRewrite(newaf);
}

Formula* UnnestTerms::visit(EqChainForm* ecf) {
	if (ecf->comps().size() == 1) { // Rewrite to a normal atom
		SIGN atomsign = ecf->sign();
		Sort* atomsort = SortUtils::resolve(ecf->subterms()[0]->sort(), ecf->subterms()[1]->sort(), _vocabulary);
		Predicate* comppred = NULL;
		switch (ecf->comps()[0]) {
		case CompType::EQ:
			comppred = get(STDPRED::EQ, atomsort);
			break;
		case CompType::LT:
			comppred = get(STDPRED::LT, atomsort);
			break;
		case CompType::GT:
			comppred = get(STDPRED::GT, atomsort);
			break;
		case CompType::NEQ:
			comppred = get(STDPRED::EQ, atomsort);
			atomsign = not atomsign;
			break;
		case CompType::LEQ:
			comppred = get(STDPRED::GT, atomsort);
			atomsign = not atomsign;
			break;
		case CompType::GEQ:
			comppred = get(STDPRED::LT, atomsort);
			atomsign = not atomsign;
			break;
		}
		Assert(comppred != NULL);
		vector<Term*> atomargs = { ecf->subterms()[0], ecf->subterms()[1] };
		PredForm* atom = new PredForm(atomsign, comppred, atomargs, ecf->pi());
		delete ecf;
		return atom->accept(this);
	} else { // Simple recursive call
		bool savemovecontext = isAllowedToUnnest();
		setAllowedToUnnest(true);
		auto newecf = traverse(ecf);
		setAllowedToUnnest(savemovecontext);
		return doRewrite(newecf);
	}
}

Formula* UnnestTerms::unnest(PredForm* predform) {
	// Special treatment for (in)equalities: possibly only one side needs to be moved
	bool savemovecontext = isAllowedToUnnest();
	bool moveonlyleft = false;
	bool moveonlyright = false;
	if (VocabularyUtils::isComparisonPredicate(predform->symbol())) {
		auto leftterm = predform->subterms()[0];
		auto rightterm = predform->subterms()[1];
		if (leftterm->type() == TermType::AGG) {
			moveonlyright = true;
		} else if (rightterm->type() == TermType::AGG) {
			moveonlyleft = true;
		} else if (is(predform->symbol(), STDPRED::EQ)) {
			moveonlyright = (leftterm->type() != TermType::VAR) && (rightterm->type() != TermType::VAR);
		} else {
			setAllowedToUnnest(true);
		}

		if (is(predform->symbol(), STDPRED::EQ)) {
			auto leftsort = leftterm->sort();
			auto rightsort = rightterm->sort();
			if (SortUtils::isSubsort(leftsort, rightsort)) {
				_chosenVarSort = leftsort;
			} else {
				_chosenVarSort = rightsort;
			}
		}
	} else {
		setAllowedToUnnest(true);
	}
	// Traverse the atom
	Formula* newf = predform;
	if (moveonlyleft) {
		predform->subterm(1, predform->subterms()[1]->accept(this));
		setAllowedToUnnest(true);
		predform->subterm(0, predform->subterms()[0]->accept(this));
	} else if (moveonlyright) {
		predform->subterm(0, predform->subterms()[0]->accept(this));
		setAllowedToUnnest(true);
		predform->subterm(1, predform->subterms()[1]->accept(this));
	} else {
		newf = traverse(predform);
	}
	_chosenVarSort = NULL;
	setAllowedToUnnest(savemovecontext);
	// Return result
	return newf;
}

Formula* UnnestTerms::visit(PredForm* predform) {
// Special treatment for (in)equalities: possibly only one side needs to be moved
	auto newf = unnest(predform);
	return doRewrite(newf);
}

Term* UnnestTerms::traverse(Term* term) {
	auto saveChosenVarSort = _chosenVarSort;
	_chosenVarSort = NULL;
	Context savecontext = getContext();
	bool savemovecontext = isAllowedToUnnest();
	for (size_t n = 0; n < term->subterms().size(); ++n) {
		term->subterm(n, term->subterms()[n]->accept(this));
	}
	for (size_t n = 0; n < term->subsets().size(); ++n) {
		term->subset(n, term->subsets()[n]->accept(this));
	}
	_chosenVarSort = saveChosenVarSort;
	setContext(savecontext);
	setAllowedToUnnest(savemovecontext);
	return term;
}

VarTerm* UnnestTerms::visit(VarTerm* t) {
	return t;
}

Term* UnnestTerms::visit(DomainTerm* t) {
	if (shouldMove(t)) {
		return move(t);
	}
	return t;
}

Term* UnnestTerms::visit(AggTerm* t) {
	auto savemovecontext = isAllowedToUnnest();

	auto result = traverse(t);

	setAllowedToUnnest(savemovecontext);

	if (shouldMove(result)) {
		return move(result);
	}
	return result;
}

Term* UnnestTerms::visit(FuncTerm* t) {
	bool savemovecontext = isAllowedToUnnest();
	setAllowedToUnnest(true);
	auto result = traverse(t);
	setAllowedToUnnest(savemovecontext);
	if (shouldMove(result)) {
		return move(result);
	}
	return result;
}

EnumSetExpr* UnnestTerms::visit(EnumSetExpr* s) {
	auto saveequalities = _equalities;
	_equalities.clear();
	auto savevars = _variables;
	_variables.clear();
	bool savemovecontext = isAllowedToUnnest();
	setAllowedToUnnest(true);
	Context savecontext = getContext();

	for (uint i = 0; i < s->getSets().size(); ++i) {
		s->setSet(i, s->getSets()[i]->accept(this));
		if (not _equalities.empty()) {
			//_equalities.push_back(s->subformulas()[n]);
			//s->subformula(n, new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo()));
			savevars.insert(_variables.cbegin(), _variables.cend());
			saveequalities.insert(saveequalities.end(), _equalities.cbegin(), _equalities.cend());
			_equalities.clear();
			_variables.clear();
		}
	}

	setContext(savecontext);
	setAllowedToUnnest(savemovecontext);
	_variables = savevars;
	_equalities = saveequalities;
	return s;
}

QuantSetExpr* UnnestTerms::visit(QuantSetExpr* s) {
	vector<Formula*> saveequalities = _equalities;
	_equalities.clear();
	set<Variable*> savevars = _variables;
	_variables.clear();
	bool savemovecontext = isAllowedToUnnest();
	setAllowedToUnnest(true);
	Context savecontext = getContext();
	setContext(Context::POSITIVE);

	s->setTerm(s->getTerm()->accept(this));
	if (not _equalities.empty()) {
		_equalities.push_back(s->getCondition());
		BoolForm* bf = new BoolForm(SIGN::POS, true, _equalities, FormulaParseInfo());
		s->setCondition(bf);
		for (auto it = _variables.cbegin(); it != _variables.cend(); ++it) {
			s->addQuantVar(*it);
		}
		_equalities.clear();
		_variables.clear();
	}

	setAllowedToUnnest(false);
	setContext(Context::POSITIVE);
	s->setCondition(s->getCondition()->accept(this));
	setContext(savecontext);
	setAllowedToUnnest(savemovecontext);
	_variables = savevars;
	_equalities = saveequalities;
	return s;
}
