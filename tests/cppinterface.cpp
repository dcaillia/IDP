#include "cppinterface.hpp"

#include "common.hpp"
#include "vocabulary.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "structure.hpp"

namespace Tests {

Sort* sort(const std::string& name, int min, int max) {
	auto sorttable = new SortTable(new IntRangeInternalSortTable(min, max));
	auto sort = new Sort(name, sorttable);
	sort->addParent(VocabularyUtils::intsort());
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

SetExpr* qset(const std::set<Variable*>& vars, Formula& subform, Term* subterm) {
	return new QuantSetExpr(vars, &subform, subterm, SetParseInfo());
}

AggTerm* sum(SetExpr* set) {
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
	auto eq = VocabularyUtils::equal(sort);
	return *new PredForm(SIGN::POS, eq, { &left, &right }, FormulaParseInfo());
}

Formula& operator<(Term& left, Term& right) {
	auto sort = SortUtils::resolve(left.sort(),right.sort());
	Assert(sort != NULL);
	auto lt = VocabularyUtils::lessThan(sort);
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

Formula& all(Variable* var, Formula& formula) {
	return *new QuantForm(SIGN::POS, QUANT::UNIV, { var }, &formula, FormulaParseInfo());
}

Formula& atom(Predicate* p, const std::vector<Variable*>& vars) {
	std::vector<Term*> terms;
	for (auto i = vars.cbegin(); i < vars.cend(); ++i) {
		terms.push_back(new VarTerm(*i, TermParseInfo()));
	}
	return *new PredForm(SIGN::POS, p, terms, FormulaParseInfo());
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