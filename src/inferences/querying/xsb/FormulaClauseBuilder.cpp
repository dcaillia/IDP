/*
 * FormulaClauseBuilder.cpp
 *
 *  Created on: Oct 28, 2013
 *      Author: joachim
 */

#include "FormulaClauseBuilder.hpp"
#include "FormulaClause.hpp"
#include "XSBToIDPTranslator.hpp"
#include "PrologProgram.hpp"
#include "vocabulary/vocabulary.hpp"
#include "structure/StructureComponents.hpp"
#include "theory/theory.hpp"
#include "theory/Sets.hpp"
#include "theory/term.hpp"
#include "theory/TheoryUtils.hpp"

using std::string;

void FormulaClauseBuilder::enter(FormulaClause* f) {
	f->parent(_parent);
	_parent = f;
}

void FormulaClauseBuilder::leave() {
	_parent->close();
	_parent = _parent->parent();
}

string FormulaClauseBuilder::generateNewAndClauseName() {
	std::stringstream ss;
	ss << _translator->new_pred_name_with_prefix("andClause");
	return ss.str();
}

string FormulaClauseBuilder::generateNewOrClauseName() {
	std::stringstream ss;
	ss << _translator->new_pred_name_with_prefix("orClause");
	return ss.str();
}

string FormulaClauseBuilder::generateNewExistsClauseName() {
	std::stringstream ss;
	ss << _translator->new_pred_name_with_prefix("existsClause");
	return ss.str();
}

string FormulaClauseBuilder::generateNewForallClauseName() {
	std::stringstream ss;
	ss << _translator->new_pred_name_with_prefix("forallClause");
	return ss.str();
}

string FormulaClauseBuilder::generateNewAggregateClauseName() {
	std::stringstream ss;
	ss << _translator->new_pred_name_with_prefix("aggClause");
	return ss.str();
}

string FormulaClauseBuilder::generateNewAggregateTermName() {
	std::stringstream ss;
	ss << _translator->new_pred_name_with_prefix("aggTerm");
	return ss.str();
}

string FormulaClauseBuilder::generateNewEnumSetExprName() {
	std::stringstream ss;
	ss << _translator->new_pred_name_with_prefix("enumSetExpr");
	return ss.str();
}

string FormulaClauseBuilder::generateNewQuantSetExprName() {
	std::stringstream ss;
	ss << _translator->new_pred_name_with_prefix("quantSetExpr");
	return ss.str();
}

PrologVariable* FormulaClauseBuilder::createPrologVar(const Variable* var) {
	auto prologVarName = _translator->to_prolog_varname(var->name());
	auto prologVarSortName = _translator->to_prolog_sortname(var->sort());
	return _translator->create(prologVarName, prologVarSortName);
}

PrologConstant* FormulaClauseBuilder::createPrologConstant(const DomainElement* domelem) {
	return new PrologConstant(_translator->to_prolog_term(domelem));
}

PrologTerm* FormulaClauseBuilder::createPrologTerm(const PFSymbol* pfsymbol) {
	return new PrologTerm(_translator->to_prolog_term(pfsymbol));
}

void FormulaClauseBuilder::visit(const Theory* t) {
	auto defs = t->definitions();
	for (auto def = defs.begin(); def != defs.end(); ++def) {
		(*def)->accept(this);
	}
}

void FormulaClauseBuilder::visit(const Definition* d) {
	auto rules = d->rules();
	for (auto rule = rules.begin(); rule != rules.end(); ++rule) {
		(*rule)->accept(this);
	}
}

void FormulaClauseBuilder::visit(const Rule* r) {
//	The rule of a definition results in a clause with an exists quantor
	ExistsClause* ec = new ExistsClause(_translator->to_prolog_term(r->head()->symbol()));
	ec->quantifiedVariables(_translator->prologVars(r->head()->freeVars()));
	enter(ec);
	r->body()->accept(this);
	leave();
	for (auto it1 = r->head()->subterms().begin(); it1 != r->head()->subterms().end(); ++it1) {
		if ( (*it1)->type() == TermType::DOM) {
			// Domain elements can occur in the head of a definition (ints and strings), these
			// must be treated specially
			DomainTerm* domterm = (DomainTerm*) (*it1);
			ec->addArgument(createPrologConstant(domterm->value()));
		} else if ((*it1)->type() == TermType::VAR) {
			auto varterm = (VarTerm*) (*it1);
			auto prologvar = createPrologVar(varterm->var());
			ec->addArgument(prologvar);
			ec->addVariable(prologvar);
			_pp->addDomainDeclarationFor(varterm->var()->sort());
		} else if ((*it1)->type() == TermType::FUNC) {
			enter(ec);
			FuncTerm* functerm = (FuncTerm*) (*it1);
			functerm->accept(this);
			leave();
		}
	}
	_ruleClauses.push_back(ec);
}

void FormulaClauseBuilder::visit(const VarTerm* v) {
	_pp->addDomainDeclarationFor(v->var()->sort());
	auto var = createPrologVar(v->var());
	_parent->addArgument(var);
	_parent->addVariable(var);
}

void FormulaClauseBuilder::visit(const DomainTerm* d) {
	_parent->addArgument(createPrologConstant(d->value()));
}

void FormulaClauseBuilder::visit(const EnumSetExpr* e) {
	auto term = new EnumSetExpression(generateNewEnumSetExprName(), _translator, SetUtils::approxTwoValued(e,_pp->structure()));
	enter(term);
	for (uint i = 0; i < e->getSubSets().size(); ++i) {
		e->getSets()[i]->accept(this);
	}
	term->arguments(PrologTerm::vars2terms(term->variables()));
	leave();
}

void FormulaClauseBuilder::visit(const QuantSetExpr* q) {
	q->getCondition()->accept(this);
	q->getTerm()->accept(this);
	for (auto var : q->quantVars()) {
		_parent->addVariable(createPrologVar(var));
	}
}

//
void FormulaClauseBuilder::visit(const EqChainForm* ecf) {
	if(FormulaUtils::isRange(ecf)) {
		// Use the builtin between(lowerBound,upperBound,VAR) to be more efficient
		auto term = new PrologTerm("between");
		enter(term);
		term->addArgument(new PrologConstant(toString(FormulaUtils::getLowerBound(ecf))));
		term->addArgument(new PrologConstant(toString(FormulaUtils::getUpperBound(ecf))));
		auto idpvar = FormulaUtils::getVariable(ecf);
		auto var2 = createPrologVar(idpvar);
		term->addArgument(var2);
		term->addVariable(var2);
		term->addOutputvarToCheck(var2);
		leave();
		_parent->addVariables(term->variables());
	} else {
		// Standard case: treat as conjunction / disjunction of comparisons
		auto boolform = FormulaUtils::splitComparisonChains((Formula*)ecf->cloneKeepVars());
		boolform->accept(this);
	}
}

void FormulaClauseBuilder::visit(const AggForm* a) {
	auto term = new AggregateClause(generateNewAggregateClauseName());
	term->comparison_type(_translator->to_prolog_term(a->comp()));
	enter(term);
	a->getBound()->accept(this);
	a->getAggTerm()->accept(this);
	leave();

	list<PrologVariable*> args = list<PrologVariable*>();
	for (auto var : term->variables()) {
		bool add = true;
		for (auto quantset : a->getAggTerm()->set()->getSets()) {
			for (auto quantvar : quantset->quantVars()) {
				auto compvar = createPrologVar(quantvar);
				// TODO: a better check is possible (maintain mapping of Variable => PrologVariable)
				if (compvar == var) {
					add = false;
				}
			}
		}
		if (add) {
			args.push_back(var);
		}
	}
	term->arguments(PrologTerm::vars2terms(args));
	_parent->addVariables(term->variables());
}

void FormulaClauseBuilder::visit(const AggTerm* a) {
	auto term = new AggregateTerm(generateNewAggregateTermName(), _translator);
	term->agg_type(_translator->to_prolog_term(a->function()));
	enter(term);
	a->set()->accept(this);
	for (auto it = a->freeVars().begin(); it != a->freeVars().end(); ++it) {
		auto var = createPrologVar(*it);
		term->instantiatedVariables().insert(var);
		term->addVariable(var);
	}
	term->arguments(PrologTerm::vars2terms(term->variables()));

	leave();
	_parent->addVariables(term->variables());
}

void FormulaClauseBuilder::visit(const FuncTerm* f) {
	if (f->function()->isConstructorFunction()) {
		auto term = new PrologTerm(_translator->to_prolog_term(f->function()));
		term->sign(true);
		term->tabled(false);
		term->fact(true);
		term->constructor(true);
		enter(term);
		for (auto it = f->subterms().begin(); it != f->subterms().end(); ++it) {
			(*it)->accept(this);
		}
		leave();
		_parent->addArgument(term);
		_parent->addVariables(term->variables());
	} else {
		_parent->numeric(true); //TODO: what does this do here? Affects printing, but how?
		if (f->function()->arity() == 0) {
			// The "function" is a constant and needs to be made into a PrologConstant instead of a PrologTerm
			auto interpretation = f->function()->interpretation(_pp->structure()); 	// retrieve the interpretation
			auto elementTuple = *(interpretation->funcTable()->begin());			// extract the first (and only) element from the interpretation
			auto domelem = elementTuple[0];											// Constants only have one element in their elementTuple
			_parent->addArgument(createPrologConstant(domelem));
		} else {
			auto term = new PrologTerm(_translator->to_prolog_term(f->function()));
			term->infix(true);
			term->numeric(true);
			enter(term);
			for (auto it = f->subterms().begin(); it != f->subterms().end(); ++it) {
				(*it)->accept(this);
			}
			leave();
			_parent->addVariables(term->variables());
			auto tmp = set<PrologVariable*>(term->variables().begin(), term->variables().end());
			_parent->addInputvarsToCheck(tmp);
		}
	}
}

void FormulaClauseBuilder::visit(const PredForm* p) {
	if (p->symbol()->nameNoArity() == "-" && p->args().size() == 2) {
		// Special case for the "-(x,y)" predicate, rewrite it to (x = -1*y)
		DomainTerm domterm(get(STDSORT::INTSORT), GlobalData::getGlobalDomElemFactory()->create(-1), {});
		FuncTerm fterm(get(STDFUNC::PRODUCT), {&domterm, p->subterms()[0]}, {});
		PredForm eqPredForm(SIGN::POS, get(STDPRED::EQ, p->subterms()[1]->sort()), {p->subterms()[1], &fterm}, {});
		eqPredForm.accept(this);
	} else {
		auto term = new PrologTerm(_translator->to_prolog_term(p->symbol()));
		term->sign(p->sign() == SIGN::POS);
		term->tabled(_pp->isTabling(p->symbol()));
		term->fact(true);
		enter(term);
		for (auto it = p->subterms().begin(); it != p->subterms().end(); ++it) {
			(*it)->accept(this);
		}
		if (_translator->isoperator(p->symbol()->name().at(0))) {
			if (p->args().size() == 3) {
				term->numeric(true);
				term->infix(true);
				auto inputvars = set<PrologVariable*>(term->variables().begin(), term->variables().end());
				try {
					auto tmp = (PrologVariable*) term->arguments().back();
					if (not isa<DomainTerm>(*(p->subterms()[2]))) {
						term->addOutputvarToCheck(tmp);
					}
					inputvars.erase(tmp);
				} catch (char *str) {
					std::cout << str << std::endl;
				}
				term->addInputvarsToCheck(inputvars);
			} else {
				auto toNumericalOperation = true;
				for(auto arg : p->args()) {
					if(not (SortUtils::isSubsort(arg->sort(),get(STDSORT::FLOATSORT)) ||
							SortUtils::isSubsort(arg->sort(),get(STDSORT::INTSORT)) ||
							SortUtils::isSubsort(arg->sort(),get(STDSORT::NATSORT))) ) {
						toNumericalOperation = false;
					}
				}
				term->numeric(true);
				term->numericalOperation(toNumericalOperation);
				term->addInputvarsToCheck(set<PrologVariable*>(term->variables().begin(), term->variables().end()));
			}
		} else if (is(p->symbol(),STDFUNC::ABS)) {
			term->numeric(false);
			auto output = term->variables().back();
			auto input = term->variables().front();
			term->addInputvarToCheck(input);
			term->addOutputvarToCheck(output);

		} else {
			term->numeric(false);
		}
		leave();
		_parent->addVariables(term->variables());
	}
}

void FormulaClauseBuilder::visit(const BoolForm* p) {
	if (p->trueFormula()) {
		PrologTerm* term = new PrologConstant("true");
		term->parent(_parent);
	} else if (p->falseFormula()) {
		PrologTerm* term = new PrologConstant("false");
		term->parent(_parent);
	} else {
		CompositeFormulaClause* term;
		if (p->conj()) {
			term = new AndClause(generateNewAndClauseName());
		} else {
			term = new OrClause(generateNewOrClauseName());
		}
		enter(term);
		auto subs = p->subformulas();
		for (auto sub = subs.begin(); sub != subs.end(); ++sub) {
			(*sub)->accept(this);
		}
		leave();
		term->arguments(PrologTerm::vars2terms(term->variables()));
		_parent->addVariables(term->variables());
	}

}

void FormulaClauseBuilder::visit(const QuantForm* p) {
	QuantifiedFormulaClause* term;
	if (p->isUniv()) {
		term = new ForallClause(generateNewForallClauseName());
	} else {
		term = new ExistsClause(generateNewExistsClauseName());
	}
	auto quantvars = p->quantVars();
	for (auto quantvar = quantvars.begin(); quantvar != quantvars.end(); ++quantvar) {
		auto var = createPrologVar(*quantvar);
		term->addQuantifiedVar(var);
	}
	enter(term);
	p->subformula()->accept(this);
	leave();

	auto vars = list<PrologVariable*>(term->variables().begin(), term->variables().end());

	for (auto it = term->quantifiedVariables().begin(); it != term->quantifiedVariables().end(); ++it) {
		vars.remove(*it);
	}
	term->variables(vars);
	term->arguments(PrologTerm::vars2terms(vars));
	_parent->addVariables(vars);
}


