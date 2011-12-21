/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CPPINTERFACE_HPP_
#define CPPINTERFACE_HPP_

/**
 * Preliminary version of an interface to easily create vocs, theories and structures from c++
 *
 * It allows for example the following declaration:
 auto s = sort("x", -2, 2);
 auto p = pred("p", {s});
 auto q = pred("q", {s});
 auto r = pred("r", {s});

 auto xt = varterm(s);
 auto x = xt->var();

 auto vocabulary = new Vocabulary("V");
 add(vocabulary, {p.p(), q.p(), r.p()});

 auto theory = new Theory("T", vocabulary, ParseInfo());
 Formula& formula = all(x, (not p({x})) | q({x})) & all(x, (not q({x})) | r({x}));
 */

#include <string>
#include <set>
#include <vector>

class Sort;
class DomainElement;
class DomainTerm;
class Variable;
class VarTerm;
class SetExpr;
class Term;
class AggTerm;
class Function;
class Formula;
class Predicate;
class Vocabulary;
class PFSymbol;

namespace Tests {

Sort* sort(const std::string& name, int min, int max);

const DomainElement* domainelement(int value);
DomainTerm* domainterm(Sort*, int value);

Variable* var(Sort*);
VarTerm* varterm(Sort*);

SetExpr* qset(const std::set<Variable*>&, Formula&, Term*);
AggTerm* sum(SetExpr*);

Term& functerm(Function*, const std::vector<Variable*>&);
Term& functerm(Function*, const std::vector<Term*>&);

Formula& operator==(Term& left, Term& right);
Formula& operator<(Term& left, Term& right);

Formula& operator&(Formula& left, Formula& right);
Formula& operator|(Formula& left, Formula& right);
Formula& operator not(Formula&);

Formula& all(Variable*, Formula&);
Formula& atom(Predicate*, const std::vector<Variable*>&);

void add(Vocabulary*, const std::vector<PFSymbol*>);

class PredWrapper {
private:
	Predicate* _p;
public:
	PredWrapper(Predicate* p) : _p(p) { }
	Formula& operator()(const std::vector<Variable*>& vars) {
		return atom(_p, vars);
	}
	Predicate* p() const {
		return _p;
	}
};

class FuncWrapper {
private:
	Function* _f;
public:
	FuncWrapper(Function* f) : _f(f) { }
	Term& operator()(const std::vector<Variable*>& vars) {
		return functerm(_f, vars);
	}
	Term& operator()(const std::vector<Term*>& terms) {
		return functerm(_f, terms);
	}
	Function* f() const {
		return _f;
	}
};

PredWrapper pred(const std::string& name, const std::vector<Sort*>& sorts);
FuncWrapper func(const std::string& name, const std::vector<Sort*>& insorts, Sort* outsort);

} /* namespace Tests */

#endif /* CPPINTERFACE_HPP_ */