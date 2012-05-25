/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "cppinterface.hpp"

#include "IncludeComponents.hpp"

namespace Gen {

Sort* sort(const std::string& name, int min, int max) {
	auto sorttable = new SortTable(new IntRangeInternalSortTable(min, max));
	auto sort = new Sort(name, sorttable);
	sort->addParent(get(STDSORT::INTSORT));
	return sort;
}

const DomainElement* domainelement(int v) {
	return DomainElementFactory::createGlobal()->create(v);
}

DomainTerm* domainterm(Sort* s, int v) {
	auto domainelement = DomainElementFactory::createGlobal()->create(v);
	return new DomainTerm(s,domainelement,TermParseInfo());
}

Variable* var(Sort* s) {
	return new Variable(s);
}

VarTerm* varterm(Sort* s) {
	auto variable = new Variable(s);
	return new VarTerm(variable, TermParseInfo());
}

EnumSetExpr* qset(const std::set<Variable*>& vars, Formula& subform, Term* subterm) {
	return new EnumSetExpr({new QuantSetExpr(vars, &subform, subterm, SetParseInfo())}, SetParseInfo());
}

AggTerm* sum(EnumSetExpr* set) {
	return new AggTerm(set, AggFunction::SUM, TermParseInfo());
}

Term& functerm(Function* f, const std::vector<Variable*>& vars) {
	std::vector<Term*> terms;
	for (auto i = vars.cbegin(); i < vars.cend(); ++i) {
		terms.push_back(new VarTerm(*i, TermParseInfo()));
	}
	return *new FuncTerm(f, terms, TermParseInfo());
}

Term& functerm(Function* f, const std::vector<Term*>& terms) {
	return *new FuncTerm(f, terms, TermParseInfo());
}

Formula& operator==(Term& left, Term& right) {
	auto sort = SortUtils::resolve(left.sort(),right.sort());
	Assert(sort != NULL);
	auto eq = get(STDPRED::EQ, sort);
	return *new PredForm(SIGN::POS, eq, { &left, &right }, FormulaParseInfo());
}

Formula& operator<(Term& left, Term& right) {
	auto sort = SortUtils::resolve(left.sort(),right.sort());
	Assert(sort != NULL);
	auto lt = get(STDPRED::LT, sort);
	return *new PredForm(SIGN::POS, lt, { &left, &right }, FormulaParseInfo());
}

Formula& operator&(Formula& left, Formula& right) {
	return *new BoolForm(SIGN::POS, true, &left, &right, FormulaParseInfo());
}

Formula& operator|(Formula& left, Formula& right) {
	return *new BoolForm(SIGN::POS, false, &left, &right, FormulaParseInfo());
}

Formula& operator not(Formula& f) {
	f.negate();
	return f;
}

Formula& disj(const std::vector<Formula*>& formulas){
	return *new BoolForm(SIGN::POS, false, formulas, FormulaParseInfo());
}
Formula& conj(const std::vector<Formula*>& formulas){
	return *new BoolForm(SIGN::POS, true, formulas, FormulaParseInfo());
}

Formula& forall(Variable* var, Formula& formula) {
	return forall(std::set<Variable*>{var}, formula);
}
Formula& exists(Variable* var, Formula& formula) {
	return exists(std::set<Variable*>{var}, formula);
}

Formula& quant(QUANT q, const std::set<Variable*>& vars, Formula& formula){
	return *new QuantForm(SIGN::POS, q, vars, &formula, FormulaParseInfo());
}

Formula& exists(const std::set<Variable*>& vars, Formula& formula) {
	return quant(QUANT::EXIST, vars, formula);
}

Formula& forall(const std::set<Variable*>& vars, Formula& formula) {
	return quant(QUANT::UNIV, vars, formula);
}

PredForm& atom(Predicate* p, const std::vector<Variable*>& vars) {
	std::vector<Term*> terms;
	for (auto i = vars.cbegin(); i < vars.cend(); ++i) {
		terms.push_back(new VarTerm(*i, TermParseInfo()));
	}
	return *new PredForm(SIGN::POS, p, terms, FormulaParseInfo());
}

void add(Vocabulary* v, const std::vector<Sort*> symbols) {
	for (auto i = symbols.cbegin(); i < symbols.cend(); ++i) {
		v->add(*i);
	}
}

void add(Vocabulary* v, const std::vector<PFSymbol*> symbols) {
	for (auto i = symbols.cbegin(); i < symbols.cend(); ++i) {
		v->add(*i);
	}
}

PredWrapper pred(const std::string& name, const std::vector<Sort*>& sorts) {
	return PredWrapper(new Predicate(name, sorts));
}

FuncWrapper func(const std::string& name, const std::vector<Sort*>& insorts, Sort* outsort) {
	return FuncWrapper(new Function(name, insorts, outsort)); 
}

} /* namespace Tests */
