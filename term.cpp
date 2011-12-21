/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include <sstream>
#include "common.hpp"
#include "vocabulary.hpp"
#include "structure.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "error.hpp"
#include "visitors/TheoryVisitor.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

using namespace std;

/********************
 *  Abstract terms
 *******************/

IMPLACCEPTBOTH(VarTerm, Term)
IMPLACCEPTBOTH(FuncTerm, Term)
IMPLACCEPTBOTH(AggTerm, Term)
IMPLACCEPTBOTH(DomainTerm, Term)
IMPLACCEPTBOTH(QuantSetExpr, SetExpr)
IMPLACCEPTBOTH(EnumSetExpr, SetExpr)

void Term::setFreeVars() {
	_freevars.clear();
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(), (*it)->freeVars().cend());
	}
	for (auto it = _subsets.cbegin(); it != _subsets.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(), (*it)->freeVars().cend());
	}
}

void Term::recursiveDelete() {
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	for (auto it = _subsets.cbegin(); it != _subsets.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	delete (this);
}

bool Term::contains(const Variable* v) const {
	for (auto it = _freevars.cbegin(); it != _freevars.cend(); ++it) {
		if (*it == v) {
			return true;
		}
	}
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		if ((*it)->contains(v)) {
			return true;
		}
	}
	for (auto it = _subsets.cbegin(); it != _subsets.cend(); ++it) {
		if ((*it)->contains(v)) {
			return true;
		}
	}
	return false;
}


ostream& operator<<(ostream& output, const Term& t) {
	return t.put(output);
}

/*************
 *  VarTerm
 ************/

void VarTerm::setFreeVars() {
	_freevars.clear();
	_freevars.insert(_var);
}

void VarTerm::sort(Sort* s) {
	_var->sort(s);
}

VarTerm::VarTerm(Variable* v, const TermParseInfo& pi) :
		Term(pi), _var(v) {
	setFreeVars();
}

VarTerm* VarTerm::clone() const {
	return new VarTerm(_var, _pi.clone());
}

VarTerm* VarTerm::cloneKeepVars() const {
	return new VarTerm(_var, _pi.clone());
}

VarTerm* VarTerm::clone(const map<Variable*, Variable*>& mvv) const {
	map<Variable*, Variable*>::const_iterator it = mvv.find(_var);
	if (it != mvv.cend()) {
		return new VarTerm(it->second, _pi);
	} else {
		return new VarTerm(_var, _pi.clone(mvv));
	}
}

inline Sort* VarTerm::sort() const {
	return _var->sort();
}

ostream& VarTerm::put(std::ostream& output) const {
	var()->put(output);
	return output;
}

/**************
 *  FuncTerm
 *************/

FuncTerm::FuncTerm(Function* func, const vector<Term*>& args, const TermParseInfo& pi) 
		: Term(pi), _function(func) {
	subterms(args);
}

FuncTerm* FuncTerm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

FuncTerm* FuncTerm::cloneKeepVars() const {
	vector<Term*> newargs;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		newargs.push_back((*it)->cloneKeepVars());
	}
	return new FuncTerm(_function, newargs, _pi.clone());
}

FuncTerm* FuncTerm::clone(const map<Variable*, Variable*>& mvv) const {
	vector<Term*> newargs;
	for (auto it = subterms().cbegin(); it != subterms().cend(); ++it) {
		newargs.push_back((*it)->clone(mvv));
	}
	return new FuncTerm(_function, newargs, _pi.clone(mvv));
}

Sort* FuncTerm::sort() const {
	return _function->outsort();
}

ostream& FuncTerm::put(ostream& output) const {
	function()->put(output);
	if (not subterms().empty()) {
		output << '(';
		subterms()[0]->put(output);
		for (size_t n = 1; n < subterms().size(); ++n) {
			output << ',';
			subterms()[n]->put(output);
		}
		output << ')';
	}
	return output;
}

/****************
 *  DomainTerm
 ***************/

DomainTerm::DomainTerm(Sort* sort, const DomainElement* value, const TermParseInfo& pi) 
		: Term(pi), _sort(sort), _value(value) {
	Assert(_sort!=NULL);
}

void DomainTerm::sort(Sort* s) {
	Assert(_sort!=NULL);
	_sort = s;
}

DomainTerm* DomainTerm::clone() const {
	return new DomainTerm(_sort, _value, _pi.clone());
}

DomainTerm* DomainTerm::cloneKeepVars() const {
	return clone();
}

DomainTerm* DomainTerm::clone(const map<Variable*, Variable*>& mvv) const {
	return new DomainTerm(_sort, _value, _pi.clone(mvv));
}

ostream& DomainTerm::put(ostream& output) const {
	value()->put(output);
	return output;
}

/*************
 *  AggTerm
 ************/

AggTerm::AggTerm(SetExpr* set, AggFunction function, const TermParseInfo& pi) 
		: Term(pi), _function(function) {
	addSet(set);
}

AggTerm* AggTerm::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

AggTerm* AggTerm::cloneKeepVars() const {
	auto newset = subsets()[0]->cloneKeepVars();
	return new AggTerm(newset, _function, _pi.clone());
}

AggTerm* AggTerm::clone(const map<Variable*, Variable*>& mvv) const {
	SetExpr* newset = subsets()[0]->clone(mvv);
	return new AggTerm(newset, _function, _pi.clone(mvv));
}

Sort* AggTerm::sort() const {
	if (_function == AggFunction::CARD) {
		return VocabularyUtils::natsort();
	} else {
		return set()->sort();
	}
}

ostream& AggTerm::put(ostream& output) const {
	output << function();
	subsets()[0]->put(output);
	return output;
}

/*************
 *  SetExpr
 ************/

void SetExpr::setFreeVars() {
	_freevars.clear();
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(), (*it)->freeVars().cend());
	}
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		_freevars.insert((*it)->freeVars().cbegin(), (*it)->freeVars().cend());
	}
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		_freevars.erase(*it);
	}
}

void SetExpr::recursiveDelete() {
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		(*it)->recursiveDelete();
	}
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		delete (*it);
	}
	delete (this);
}

bool SetExpr::contains(const Variable* v) const {
	for (auto it = _freevars.cbegin(); it != _freevars.cend(); ++it) {
		if (*it == v) {
			return true;
		}
	}
	for (auto it = _quantvars.cbegin(); it != _quantvars.cend(); ++it) {
		if (*it == v) {
			return true;
		}
	}
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		if ((*it)->contains(v)) {
			return true;
		}
	}
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		if ((*it)->contains(v)) {
			return true;
		}
	}
	return false;
}

ostream& operator<<(ostream& output, const SetExpr& set) {
	return set.put(output);
}

/*****************
 *  EnumSetExpr
 ****************/

EnumSetExpr::EnumSetExpr(const vector<Formula*>& subforms, const vector<Term*>& weights, const SetParseInfo& pi) 
		: SetExpr(pi) {
	_subformulas = subforms;
	_subterms = weights;
	setFreeVars();
}

EnumSetExpr* EnumSetExpr::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

EnumSetExpr* EnumSetExpr::cloneKeepVars() const {
	vector<Formula*> newforms;
	vector<Term*> newweights;
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		newforms.push_back((*it)->cloneKeepVars());
	}
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		newweights.push_back((*it)->cloneKeepVars());
	}
	return new EnumSetExpr(newforms, newweights, _pi.clone());
}

EnumSetExpr* EnumSetExpr::clone(const map<Variable*, Variable*>& mvv) const {
	vector<Formula*> newforms;
	vector<Term*> newweights;
	for (auto it = _subformulas.cbegin(); it != _subformulas.cend(); ++it) {
		newforms.push_back((*it)->clone(mvv));
	}
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		newweights.push_back((*it)->clone(mvv));
	}
	return new EnumSetExpr(newforms, newweights, _pi.clone(mvv));
}
EnumSetExpr* EnumSetExpr::positiveSubset() const {
	std::vector<Formula*> newsubforms(0);
	std::vector<Term*> newsubterms(0);
	auto nul = DomainElementFactory::createGlobal()->create(0);
	auto form = _subformulas.cbegin();
	for (auto term = _subterms.cbegin(); form != _subformulas.cend(); ++term, ++form) {
		auto nulterm = new DomainTerm((*term)->sort(), nul, (*term)->pi());
		auto termpos = new EqChainForm(SIGN::POS, true, { (*term)->clone(), nulterm }, { CompType::GT }, (*form)->pi());
		newsubforms.push_back(new BoolForm(SIGN::POS, true, { (*form)->clone(), termpos }, (*form)->pi()));
		newsubterms.push_back((*term)->clone());
	}
	return new EnumSetExpr(newsubforms, newsubterms, _pi.clone());

}
EnumSetExpr* EnumSetExpr::negativeSubset() const {
	std::vector<Formula*> newsubforms(0);
	std::vector<Term*> newsubterms(0);
	auto nul = DomainElementFactory::createGlobal()->create(0);
	auto form = _subformulas.cbegin();
	for (auto term = _subterms.cbegin(); form != _subformulas.cend(); ++term, ++form) {
		auto nulterm = new DomainTerm((*term)->sort(), nul, (*term)->pi());
		auto minSymbol = (*((*term)->sort()->firstVocabulary()))->func("-/1");
		newsubterms.push_back(new FuncTerm(minSymbol, { (*term)->clone() }, (*term)->pi()));
		auto termneg = new EqChainForm(SIGN::POS, true, { (*term)->clone(), nulterm }, { CompType::LT }, (*form)->pi());
		newsubforms.push_back(new BoolForm(SIGN::POS, true, { (*form)->clone(), termneg }, (*form)->pi()));
	}
	return new EnumSetExpr(newsubforms, newsubterms, _pi.clone());
}

EnumSetExpr* EnumSetExpr::zeroSubset() const {
	std::vector<Formula*> newsubforms(0);
	std::vector<Term*> newsubterms(0);
	auto nul = DomainElementFactory::createGlobal()->create(0);
	auto form = _subformulas.cbegin();
	for (auto term = _subterms.cbegin(); form != _subformulas.cend(); ++term, ++form) {
		auto nulterm = new DomainTerm((*term)->sort(), nul, (*term)->pi());
		newsubterms.push_back(nulterm);
		auto termisnul = new EqChainForm(SIGN::POS, true, { (*term)->clone(), nulterm }, { CompType::EQ }, (*form)->pi());
		newsubforms.push_back(new BoolForm(SIGN::POS, true, { (*form)->clone(), termisnul }, (*form)->pi()));
	}
	return new EnumSetExpr(newsubforms, newsubterms, _pi.clone());
}

Sort* EnumSetExpr::sort() const {
	Sort* currsort = VocabularyUtils::natsort();
	for (auto it = _subterms.cbegin(); it != _subterms.cend(); ++it) {
		if ((*it)->sort()) {
			currsort = SortUtils::resolve(currsort, (*it)->sort());
		} else {
			return NULL;
		}
	}
	if (currsort != NULL) {
		if (SortUtils::isSubsort(currsort, VocabularyUtils::natsort())) {
			return VocabularyUtils::natsort();
		} else if (SortUtils::isSubsort(currsort, VocabularyUtils::intsort())) {
			return VocabularyUtils::intsort();
		} else if (SortUtils::isSubsort(currsort, VocabularyUtils::floatsort())) {
			return VocabularyUtils::floatsort();
		} else {
			Error::notsubsort(currsort->name(),VocabularyUtils::floatsort()->name(),pi());
			return NULL;
		}
	} else {
		//TODO There should be some error or warning thrown here!
		return NULL;
	}
}

tablesize EnumSetExpr::maxSize(const AbstractStructure*) const {
	return tablesize(TST_EXACT,subformulas().size());
}

ostream& EnumSetExpr::put(ostream& output) const {
	output << "[ ";
	if (not subformulas().empty()) {
		for (size_t n = 0; n < subformulas().size(); ++n) {
			output << '(';
			subformulas()[n]->put(output);
			output << ',';
			subterms()[n]->put(output);
			output << ')';
			if (n < subformulas().size() - 1) {
				output << "; ";
			}
		}
	}
	output << " ]";
	return output;
}

/******************
 *  QuantSetExpr
 *****************/

QuantSetExpr::QuantSetExpr(const set<Variable*>& qvars, Formula* formula, Term* term, const SetParseInfo& pi) 
		: SetExpr(pi) {
	_quantvars = qvars;
	_subterms.push_back(term);
	_subformulas.push_back(formula);
	setFreeVars();
}

QuantSetExpr* QuantSetExpr::clone() const {
	map<Variable*, Variable*> mvv;
	return clone(mvv);
}

QuantSetExpr* QuantSetExpr::cloneKeepVars() const {
	auto newterm = subterms()[0]->cloneKeepVars();
	auto newform = subformulas()[0]->cloneKeepVars();
	return new QuantSetExpr(quantVars(), newform, newterm, _pi.clone());
}

QuantSetExpr* QuantSetExpr::clone(const map<Variable*, Variable*>& mvv) const {
	set<Variable*> newvars;
	map<Variable*, Variable*> nmvv = mvv;
	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
		Variable* nv = new Variable((*it)->name(), (*it)->sort(), (*it)->pi());
		newvars.insert(nv);
		nmvv[*it] = nv;
	}
	auto newterm = subterms()[0]->clone(nmvv);
	auto newform = subformulas()[0]->clone(nmvv);
	return new QuantSetExpr(newvars, newform, newterm, _pi.clone(mvv));
}

QuantSetExpr* QuantSetExpr::positiveSubset() const {
	auto newset = clone();
	auto form = newset->subformulas()[0];
	auto term = newset->subterms()[0];
	auto nul = DomainElementFactory::createGlobal()->create(0);
	auto nulterm = new DomainTerm(term->sort(), nul, TermParseInfo());
	auto termpos = new EqChainForm(SIGN::POS, true, { term->clone(), nulterm }, { CompType::GT }, form->pi());
	auto newform = new BoolForm(SIGN::POS, true, { form, termpos }, form->pi());
	newset->subformula(0,newform);
	return newset;
}

QuantSetExpr* QuantSetExpr::negativeSubset() const {
	auto newset = clone();
	auto form = newset->subformulas()[0];
	auto term = newset->subterms()[0];
	auto nul = DomainElementFactory::createGlobal()->create(0);
	auto nulterm = new DomainTerm(term->sort(), nul, TermParseInfo());
	auto termneg = new EqChainForm(SIGN::POS, true, { term->clone(), nulterm }, { CompType::LT }, form->pi());
	auto newform = new BoolForm(SIGN::POS, true, { form, termneg }, form->pi());
	newset->subformula(0,newform);
	auto minSymbol = (*(term->sort()->firstVocabulary()))->func("-/1");
	auto newterm = new FuncTerm(minSymbol, { term }, term->pi());
	newset->subterm(0,newterm);
	return newset;
}

QuantSetExpr* QuantSetExpr::zeroSubset() const {
	auto newset = clone();
	auto form = newset->subformulas()[0];
	auto term = newset->subterms()[0];
	auto nul = DomainElementFactory::createGlobal()->create(0);
	auto nulterm = new DomainTerm(term->sort(), nul, TermParseInfo());
	auto termisnul = new EqChainForm(SIGN::POS, true, { term->clone(), nulterm }, { CompType::EQ }, form->pi());
	auto newform = new BoolForm(SIGN::POS, true, { form, termisnul }, form->pi());
	newset->subformula(0,newform);
	newset->subterm(0,nulterm);
	delete term;
	return newset;
}

Sort* QuantSetExpr::sort() const {
	Sort* termsort = (*_subterms.cbegin())->sort();
	if (termsort != NULL) {
		if (SortUtils::isSubsort(termsort, VocabularyUtils::natsort())) {
			return VocabularyUtils::natsort();
		} else if (SortUtils::isSubsort(termsort, VocabularyUtils::intsort())) {
			return VocabularyUtils::intsort();
		} else if (SortUtils::isSubsort(termsort, VocabularyUtils::floatsort())) {
			return VocabularyUtils::floatsort();
		} else {
			Error::notsubsort(termsort->name(),VocabularyUtils::floatsort()->name(),pi());
			return NULL;
		}
	} else {
		//TODO There should be some error or warning thrown here!
		return NULL;
	}
}

tablesize QuantSetExpr::maxSize(const AbstractStructure* structure) const {
	if (structure == NULL) {
		return tablesize(TST_UNKNOWN,0);
	}
	size_t currentsize = 1;
	TableSizeType tst = TST_EXACT;
	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
		auto qvardom = structure->inter((*it)->sort());
		Assert(qvardom != NULL);
		tablesize qvardomsize = qvardom->size();
		switch (qvardomsize._type) {
		case TST_UNKNOWN:
			return tablesize(TST_UNKNOWN,0);
		case TST_INFINITE:
			return tablesize(TST_INFINITE,0);
		case TST_APPROXIMATED:
			currentsize *= qvardomsize._size;
			tst = TST_APPROXIMATED;
			break;
		case TST_EXACT:
			currentsize *= qvardomsize._size;
			break;
		}
	}
	return tablesize(tst,currentsize);
}

ostream& QuantSetExpr::put(ostream& output) const {
	output << "{";
	for (auto it = quantVars().cbegin(); it != quantVars().cend(); ++it) {
		output << ' ';
		(*it)->put(output);
	}
	output << " : ";
	subformulas()[0]->put(output);
	output << " : ";
	subterms()[0]->put(output);
	output << " }";
	return output;
}

/***************
 *  Utilities
 **************/

namespace TermUtils {

vector<Term*> makeNewVarTerms(const vector<Variable*>& vars) {
	vector<Term*> terms;
	for (auto it = vars.cbegin(); it != vars.cend(); ++it) {
		terms.push_back(new VarTerm(*it, TermParseInfo()));
	}
	return terms;
}

}

