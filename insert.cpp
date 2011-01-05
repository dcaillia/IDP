/************************************
	insert.cc	
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "data.hpp"
#include "namespace.hpp"
#include "execute.hpp"
#include "error.hpp"
#include "visitor.hpp"
#include "insert.hpp"
#include "parse.h"
#include "builtin.hpp"
#include <iostream>
#include <list>
#include <set>

/********************
	Sort checking
********************/

class SortChecker : public Visitor {

	private:
		Vocabulary* _vocab;

	public:
		SortChecker(Formula* f,Vocabulary* v)		: Visitor(), _vocab(v) { f->accept(this);	}
		SortChecker(Definition* d,Vocabulary* v)	: Visitor(), _vocab(v) { d->accept(this);	}
		SortChecker(FixpDef* d,Vocabulary* v)		: Visitor(), _vocab(v) { d->accept(this);	}

		void visit(PredForm*);
		void visit(EqChainForm*);
		void visit(FuncTerm*);
		void visit(AggTerm*);

};

void SortChecker::visit(PredForm* pf) {
	PFSymbol* s = pf->symb();
	for(unsigned int n = 0; n < s->nrsorts(); ++n) {
		Sort* s1 = s->sort(n);
		Sort* s2 = pf->subterm(n)->sort();
		if(s1 && s2) {
			if(!SortUtils::resolve(s1,s2,_vocab)) {
				Error::wrongsort(pf->subterm(n)->to_string(),s2->name(),s1->name(),pf->subterm(n)->pi());
			}
		}
	}
	traverse(pf);
}

void SortChecker::visit(FuncTerm* ft) {
	Function* f = ft->func();
	for(unsigned int n = 0; n < f->arity(); ++n) {
		Sort* s1 = f->insort(n);
		Sort* s2 = ft->subterm(n)->sort();
		if(s1 && s2) {
			if(!SortUtils::resolve(s1,s2,_vocab)) {
				Error::wrongsort(ft->subterm(n)->to_string(),s2->name(),s1->name(),ft->subterm(n)->pi());
			}
		}
	}
	traverse(ft);
}

void SortChecker::visit(EqChainForm* ef) {
	Sort* s = 0;
	unsigned int n = 0;
	while(!s && n < ef->nrSubterms()) {
		s = ef->subterm(n)->sort();
		++n;
	}
	for(; n < ef->nrSubterms(); ++n) {
		Sort* t = ef->subterm(n)->sort();
		if(t) {
			if(!SortUtils::resolve(s,t,_vocab)) {
				Error::wrongsort(ef->subterm(n)->to_string(),t->name(),s->name(),ef->subterm(n)->pi());
			}
		}
	}
	traverse(ef);
}

void SortChecker::visit(AggTerm* at) {
	if(at->type() != AGGCARD) {
		SetExpr* s = at->set();
		if(s->nrQvars() && s->qvar(0)->sort()) {
			if(!SortUtils::resolve(s->qvar(0)->sort(),*(StdBuiltin::instance()->sort("float")->begin()),_vocab)) {
				Error::wrongsort(s->qvar(0)->name(),s->qvar(0)->sort()->name(),"int or float",s->qvar(0)->pi());
			}
		}
		for(unsigned int n = 0; n < s->nrSubterms(); ++n) {
			if(s->subterm(n)->sort() && !SortUtils::resolve(s->subterm(n)->sort(),*(StdBuiltin::instance()->sort("float")->begin()),_vocab)) {
				Error::wrongsort(s->subterm(n)->to_string(),s->subterm(n)->sort()->name(),"int or float",s->subterm(n)->pi());
			}
		}
	}
	traverse(at);
}

/**********************
	Sort derivation
**********************/

class SortDeriver : public Visitor {

	private:
		map<Variable*,set<Sort*> >	_untyped;			// The untyped variables, with their possible types
		map<FuncTerm*,Sort*>		_overloadedterms;	// The terms with an overloaded function
		set<PredForm*>				_overloadedatoms;	// The atoms with an overloaded predicate
		set<DomainTerm*>			_domelements;		// The untyped domain elements
		bool						_changed;
		bool						_firstvisit;
		Sort*						_assertsort;
		Vocabulary*					_vocab;

	public:

		// Constructor
		SortDeriver(Formula* f,Vocabulary* v) : Visitor(), _vocab(v) { run(f); }
		SortDeriver(Rule* r,Vocabulary* v)	: Visitor(), _vocab(v) { run(r); }

		// Run sort derivation 
		void run(Formula*);
		void run(Rule*);

		// Visit 
		void visit(QuantForm*);
		void visit(PredForm*);
		void visit(EqChainForm*);

		void visit(Rule*);

		void visit(VarTerm*);
		void visit(DomainTerm*);
		void visit(FuncTerm*);

		void visit(QuantSetExpr*);

	private:

		// Auxiliary methods
		void derivesorts();		// derive the sorts of the variables, based on the sorts in _untyped
		void derivefuncs();		// disambiguate the overloaded functions
		void derivepreds();		// disambiguate the overloaded predicates

		// Check
		void check();
		
};

void SortDeriver::visit(QuantForm* qf) {
	if(_firstvisit) {
		for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
			if(!(qf->qvar(n)->sort())) _untyped[qf->qvar(n)] = set<Sort*>();
			_changed = true;
		}
	}
	traverse(qf);
}

void SortDeriver::visit(PredForm* pf) {
	PFSymbol* p = pf->symb();

	// At first visit, insert the atoms over overloaded predicates
	if(_firstvisit && p->overloaded()) {
		_overloadedatoms.insert(pf);
		_changed = true;
	}

	// Visit the children while asserting the sorts of the predicate
	for(unsigned int n = 0; n < p->nrsorts(); ++n) {
		_assertsort = p->sort(n);
		pf->subterm(n)->accept(this);
	}
}

void SortDeriver::visit(EqChainForm* ef) {
	Sort* s = 0;
	if(!_firstvisit) {
		for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
			Sort* temp = ef->subterm(n)->sort();
			if(temp && temp->nrParents() == 0 && temp->nrChildren() == 0) {
				s = temp;
				break;
			}
		}
	}
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		_assertsort = s;
		ef->subterm(n)->accept(this);
	}
}

void SortDeriver::visit(Rule* r) {
	if(_firstvisit) {
		for(unsigned int n = 0; n < r->nrQvars(); ++n) {
			if(!(r->qvar(n)->sort())) _untyped[r->qvar(n)] = set<Sort*>();
			_changed = true;
		}
	}
	traverse(r);
}

void SortDeriver::visit(VarTerm* vt) {
	if((!(vt->sort())) && _assertsort) {
		_untyped[vt->var()].insert(_assertsort);
	}
}

void SortDeriver::visit(DomainTerm* dt) {
	if(_firstvisit && (!(dt->sort()))) {
		_domelements.insert(dt);
	}

	if((!(dt->sort())) && _assertsort) {
		dt->sort(_assertsort);
		_changed = true;
		_domelements.erase(dt);
	}
}

void SortDeriver::visit(FuncTerm* ft) {
	Function* f = ft->func();

	// At first visit, insert the terms over overloaded functions
	if(f->overloaded()) {
		if(_firstvisit || _assertsort != _overloadedterms[ft]) {
			_changed = true;
			_overloadedterms[ft] = _assertsort;
		}
	}

	// Visit the children while asserting the sorts of the predicate
	for(unsigned int n = 0; n < f->arity(); ++n) {
		_assertsort = f->insort(n);
		ft->subterm(n)->accept(this);
	}
}

void SortDeriver::visit(QuantSetExpr* qs) {
	if(_firstvisit) {
		for(unsigned int n = 0; n < qs->nrQvars(); ++n) {
			if(!(qs->qvar(n)->sort())) {
				_untyped[qs->qvar(n)] = set<Sort*>();
				_changed = true;
			}
		}
	}
	traverse(qs);
}

void SortDeriver::derivesorts() {
	for(map<Variable*,set<Sort*> >::iterator it = _untyped.begin(); it != _untyped.end(); ) {
		map<Variable*,set<Sort*> >::iterator jt = it; ++jt;
		if(!((it->second).empty())) {
			set<Sort*>::iterator kt = (it->second).begin(); 
			Sort* s = *kt;
			++kt;
			for(; kt != (it->second).end(); ++kt) {
				s = SortUtils::resolve(s,*kt,_vocab);
				if(!s) { // In case of conflicting sorts, assign the first sort. 
						 // Error message will be given during final check.
					s = *((it->second).begin());
					break;
				}
			}
			assert(s);
			if((it->second).size() > 1 || s->builtin()) {	// Warning when the sort was resolved or builtin
				Warning::derivevarsort(it->first->name(),s->name(),it->first->pi());
			}
			it->first->sort(s);
			_untyped.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void SortDeriver::derivefuncs() {
	for(map<FuncTerm*,Sort*>::iterator it = _overloadedterms.begin(); it != _overloadedterms.end(); ) {
		map<FuncTerm*,Sort*>::iterator jt = it; ++jt;
		Function* f = it->first->func();
		vector<Sort*> vs(f->arity(),0);
		for(unsigned int n = 0; n < vs.size(); ++n) {
			vs[n] = it->first->subterm(n)->sort();
		}
		vs.push_back(it->second);
		Function* rf = f->disambiguate(vs,_vocab);
		if(rf) {
			it->first->func(rf);
			if(!rf->overloaded()) _overloadedterms.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void SortDeriver::derivepreds() {
	for(set<PredForm*>::iterator it = _overloadedatoms.begin(); it != _overloadedatoms.end(); ) {
		set<PredForm*>::iterator jt = it; ++jt;
		PFSymbol* p = (*it)->symb();
		vector<Sort*> vs(p->nrsorts(),0);
		for(unsigned int n = 0; n < vs.size(); ++n) {
			vs[n] = (*it)->subterm(n)->sort();
		}
		PFSymbol* rp = p->disambiguate(vs,_vocab);
		if(rp) {
			(*it)->symb(rp);
			if(!rp->overloaded()) _overloadedatoms.erase(it);
			_changed = true;
		}
		it = jt;
	}
}

void SortDeriver::run(Formula* f) {
	_changed = false;
	_firstvisit = true;
	f->accept(this);	// First visit: collect untyped symbols, set types of variables that occur in typed positions.
	_firstvisit = false;
	while(_changed) {
		_changed = false;
		derivesorts();
		derivefuncs();
		derivepreds();
		f->accept(this);	// Next visit: type derivation over overloaded predicates or functions.
	}
	check();
}

void SortDeriver::run(Rule* r) {
	// Set the sort of the variables in the head
	for(unsigned int n = 0; n < r->head()->nrSubterms(); ++n) 
		r->head()->subterm(n)->sort(r->head()->symb()->sort(n));
	// Rest of the algorithm
	_changed = false;
	_firstvisit = true;
	r->accept(this);
	_firstvisit = false;
	while(_changed) {
		_changed = false;
		derivesorts();
		derivefuncs();
		derivepreds();
		r->accept(this);
	}
	check();
}

void SortDeriver::check() {
	for(map<Variable*,set<Sort*> >::iterator it = _untyped.begin(); it != _untyped.end(); ++it) {
		assert((it->second).empty());
		Error::novarsort(it->first->name(),it->first->pi());
	}
	for(set<PredForm*>::iterator it = _overloadedatoms.begin(); it != _overloadedatoms.end(); ++it) {
		if((*it)->symb()->ispred()) Error::nopredsort((*it)->symb()->name(),(*it)->pi());
		else Error::nofuncsort((*it)->symb()->name(),(*it)->pi());
	}
	for(map<FuncTerm*,Sort*>::iterator it = _overloadedterms.begin(); it != _overloadedterms.end(); ++it) {
		Error::nofuncsort(it->first->func()->name(),it->first->pi());
	}
	for(set<DomainTerm*>::iterator it = _domelements.begin(); it != _domelements.end(); ++it) {
		Error::nodomsort((*it)->to_string(),(*it)->pi());
	}
}

/**************
	Parsing
**************/

string NSTuple::to_string() {
	assert(!_name.empty());
	string str = _name[0];
	for(unsigned int n = 1; n < _name.size(); ++n) str = str + "::" + _name[n];
	if(_sortsincluded) {
		if(_arityincluded) str = str.substr(0,str.find('/'));
		str = str + '[';
		if(!_sorts.empty()) {
			if(_func && _sorts.size() == 1) str = str + ':';
			str = str + _sorts[0]->name();
			for(unsigned int n = 1; n < _sorts.size()-1; ++n) {
				str = str + ',' + _sorts[n]->name();
			}
			if(_sorts.size() > 1) {
				if(_func) str = str + ':';
				else str = str + ',';
				str = str + _sorts[_sorts.size()-1]->name();
			}
		}
		str = str + ']';
	}
	return str;
}

namespace Insert {

	/***********
		Data
	***********/
	
	string*					_currfile = 0;	// The current file
	vector<string*>			_allfiles;		// All the parsed files
	Namespace*				_currspace;		// The current namespace
	Vocabulary*				_currvocab;		// The current vocabulary
	Theory*					_currtheory;	// The current theory
	Structure*				_currstructure;	// The current structure
	InfOptions*				_curroptions;	// The current options
	LuaProcedure*			_currproc;		// The current procedure

	vector<Vocabulary*>		_usingvocab;	// The vocabularies currently used to parse
	vector<unsigned int>	_nrvocabs;		// The number of using vocabulary statements in the current block

	vector<Namespace*>		_usingspace;	// The namespaces currently used to parse
	vector<unsigned int>	_nrspaces;		// The number of using namespace statements in the current block

	void closeblock() {
		for(unsigned int n = 0; n < _nrvocabs.back(); ++n) _usingvocab.pop_back();
		for(unsigned int n = 0; n < _nrspaces.back(); ++n) _usingspace.pop_back();
		_nrvocabs.pop_back();
		_nrspaces.pop_back();
		_currvocab = 0;
		_currtheory = 0;
		_currstructure = 0;
		_curroptions = 0;
		_currproc = 0;
	}

	map<string,vector<Inference*> >	_inferences;	// All inference methods

	string*		currfile()					{ return _currfile;	}
	void		currfile(const string& s)	{ _allfiles.push_back(_currfile); _currfile = new string(s);	}
	void		currfile(string* s)			{ _allfiles.push_back(_currfile); if(s) _currfile = new string(*s); else _currfile = 0;	}
	ParseInfo		parseinfo(YYLTYPE l)		{ return ParseInfo(l.first_line,l.first_column,_currfile);	}
	FormParseInfo	formparseinfo(Formula* f, YYLTYPE l)	{ return FormParseInfo(l.first_line,l.first_column,_currfile,f);	}
	FormParseInfo	formparseinfo(YYLTYPE l)	{ return FormParseInfo(l.first_line,l.first_column,_currfile,0);	}

	// Three-valued interpretations
	enum UTF { UTF_UNKNOWN, UTF_CT, UTF_CF, UTF_ERROR };
	string _utf2string[4] = { "u", "ct", "cf", "error" };

	map<Predicate*,FinitePredTable*>	_unknownpredtables;
	map<Function*,FinitePredTable*>		_unknownfunctables;

	
	/******************************************
		Convert vector of names to one name
	******************************************/

	string oneName(const vector<string>& vs) {
		assert(!vs.empty());
		string name = vs[0];
		for(unsigned int n = 1; n < vs.size(); ++n) name = name + "::" + vs[n];
		return name;
	}

	/*****************
		Find names
	*****************/

	bool belongsToVoc(Sort* s) {
		if(_currvocab->contains(s)) return true;
		return false;
	}

	bool belongsToVoc(Predicate* p) {
		if(_currvocab->contains(p)) return true;
		return false;
	}

	bool belongsToVoc(Function* f) {
		if(_currvocab->contains(f)) return true;
		return false;
	}

	Namespace* namespaceInScope(const string& name, const ParseInfo& pi) {
		Namespace* ns = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->name() == name) {
				if(ns) Error::overloadedspace(name,_usingspace[n]->pi(),ns->pi(),pi);
				else ns = _usingspace[n];
			}
		}
		return ns;
	}

	AbstractTheory* theoInScope(const string& name, const ParseInfo& pi) {
		AbstractTheory* th = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isTheory(name)) {
				if(th) Error::overloadedtheory(name,_usingspace[n]->theory(name)->pi(),th->pi(),pi);
				else th = _usingspace[n]->theory(name);
			}
		}
		return th;
	}

	Vocabulary* vocabInScope(const string& name, const ParseInfo& pi) {
		Vocabulary* v = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isVocab(name)) {
				if(v) Error::overloadedvocab(name,_usingspace[n]->vocabulary(name)->pi(),v->pi(),pi);
				else v = _usingspace[n]->vocabulary(name);
			}
		}
		return v;
	}

	AbstractStructure* structInScope(const string& name, const ParseInfo& pi) {
		AbstractStructure* s = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isStructure(name)) {
				if(s) Error::overloadedstructure(name,_usingspace[n]->structure(name)->pi(),s->pi(),pi);
				else s = _usingspace[n]->structure(name);
			}
		}
		return s;
	}

	InfOptions* optionsInScope(const string& name, const ParseInfo& pi) {
		InfOptions* opt = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isOption(name)) {
				if(opt) Error::overloadedopt(name,_usingspace[n]->option(name)->_pi,opt->_pi,pi);
				else opt = _usingspace[n]->option(name);
			}
		}
		return opt;
	}

	LuaProcedure* procInScope(const string& name, const ParseInfo& pi) {
		LuaProcedure* lp = 0;
		for(unsigned int n = 0; n < _usingspace.size(); ++n) {
			if(_usingspace[n]->isProc(name)) {
				if(lp) Error::overloadedproc(name,_usingspace[n]->procedure(name)->pi(),lp->pi(),pi);
				else lp = _usingspace[n]->procedure(name);
			}
		}
		return lp;
	}

	Namespace* namespaceInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return namespaceInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			for(unsigned int n = 1; n < vs.size(); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			return ns;
		}
	}

	Vocabulary* vocabInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return vocabInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			for(unsigned int n = 1; n < (vs.size()-1); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			if(ns->isVocab(vs.back())) {
				return ns->vocabulary(vs.back());
			}
			else return 0;
		}
	}

	AbstractStructure* structInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return structInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			for(unsigned int n = 1; n < (vs.size()-1); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			if(ns->isStructure(vs.back())) {
				return ns->structure(vs.back());
			}
			else return 0;
		}
	}

	InfOptions* optionsInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return optionsInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			for(unsigned int n = 1; n < (vs.size()-1); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			if(ns->isOption(vs.back())) {
				return ns->option(vs.back());
			}
			else return 0;
		}
	}

	LuaProcedure* procInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return procInScope(vs[0],pi);
		}
		else {
			Namespace* ns = namespaceInScope(vs[0],pi);
			for(unsigned int n = 1; n < (vs.size()-1); ++n) {
				if(ns->isSubspace(vs[n])) {
					ns = ns->subspace(vs[n]);
				}
				else return 0;
			}
			if(ns->isProc(vs.back())) {
				return ns->procedure(vs.back());
			}
			else return 0;
		}
	}

	Sort* sortInScope(const string& name, const ParseInfo& pi) {
		Sort* s = 0;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			const std::set<Sort*>* temp = _usingvocab[n]->sort(name);
			if(temp) {
				if(s) {
					Error::overloadedsort(s->name(),s->pi(),(*(temp->begin()))->pi(),pi);
				}
				else if(temp->size() > 1) {
					std::set<Sort*>::iterator it = temp->begin();
					Sort* s1 = *it; ++it; Sort* s2 = *it;
					Error::overloadedsort(s1->name(),s1->pi(),s2->pi(),pi);
				}
				else s = *(temp->begin());
			}
		}
		return s;
	}

	Sort* sortInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return sortInScope(vs[0],pi);
		}
		else { 
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) {
				const std::set<Sort*>* ss = v->sort(vs.back());
				if(ss) {
					if(ss->size() > 1) {
						std::set<Sort*>::iterator it = ss->begin();
						Sort* s1 = *it; ++it; Sort* s2 = *it;
						Error::overloadedsort(s1->name(),s1->pi(),s2->pi(),pi);
					}
					return *(ss->begin());
				}
				else return 0;
			}
			else return 0;
		}
	}

	Predicate* predInScope(const string& name) {
		vector<Predicate*> vp;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			Predicate* p = _usingvocab[n]->pred(name);
			if(p) vp.push_back(p);
		}
		if(vp.empty()) return 0;
		else return PredUtils::overload(vp);
	}

	Predicate* predInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return predInScope(vs[0]);
		}
		else { 
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) return v->pred(vs.back());
			else return 0;
		}
	}

	vector<Predicate*> noArPredInScope(const string& name) {
		vector<Predicate*> vp;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			vector<Predicate*> nvp = _usingvocab[n]->pred_no_arity(name);
			for(unsigned int m = 0; m < nvp.size(); ++m) vp.push_back(nvp[m]);
		}
		return vp;
	}

	vector<Predicate*> noArPredInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return noArPredInScope(vs[0]);
		}
		else {
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) return v->pred_no_arity(vs.back());
			else return vector<Predicate*>(0);
		}
	}

	Function* funcInScope(const string& name) {
		vector<Function*> vf;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			Function* f = _usingvocab[n]->func(name);
			if(f) vf.push_back(f);
		}
		if(vf.empty()) return 0;
		else return FuncUtils::overload(vf);
	}

	Function* funcInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return funcInScope(vs[0]);
		}
		else { 
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) return v->func(vs.back());
			else return 0;
		}
	}

	vector<Function*> noArFuncInScope(const string& name) {
		vector<Function*> vf;
		for(unsigned int n = 0; n < _usingvocab.size(); ++n) {
			vector<Function*> nvf = _usingvocab[n]->func_no_arity(name);
			for(unsigned int m = 0; m < nvf.size(); ++m) vf.push_back(nvf[m]);
		}
		return vf;
	}


	vector<Function*> noArFuncInScope(const vector<string>& vs, const ParseInfo& pi) {
		assert(!vs.empty());
		if(vs.size() == 1) {
			return noArFuncInScope(vs[0]);
		}
		else {
			vector<string> vv(vs.size()-1);
			for(unsigned int n = 0; n < vv.size(); ++n) vv[n] = vs[n];
			Vocabulary* v = vocabInScope(vv,pi);
			if(v) return v->func_no_arity(vs.back());
			else return vector<Function*>(0);
		}
	}


	/******************************************************************* 
		Data structures and methods to link variables during parsing 
	*******************************************************************/

	struct VarName {
		string		_name;
		Variable*	_var;
		VarName(const string& n, Variable* v) : _name(n), _var(v) { }
	};

	list<VarName>	curr_vars;

	Variable* getVar(const string& name) {
		for(list<VarName>::iterator i = curr_vars.begin(); i != curr_vars.end(); ++i) {
			if(name == i->_name) return i->_var;
		}
		return 0;
	}

	void remove_vars(const vector<Variable*>& v) {
		for(unsigned int n = 0; n < v.size(); ++n) {
			if(v[n]) {
				for(list<VarName>::iterator i = curr_vars.begin(); i != curr_vars.end(); ++i) {
					if(i->_name == v[n]->name()) {
						curr_vars.erase(i);
						break;
					}
				}
			}
		}
	}

	vector<Variable*> freevars(const ParseInfo& pi) {
		vector<Variable*> vv;
		string vs;
		for(list<VarName>::iterator i = curr_vars.begin(); i != curr_vars.end(); ++i) {
			vv.push_back(i->_var);
			vs = vs + ' ' + i->_name;
		}
		if(!vv.empty()) Warning::freevars(vs,pi);
		curr_vars.clear();
		return vv;
	}

	/***********************
		Global structure
	***********************/

	void initialize() {

		_currspace = Namespace::global();
		_nrspaces.push_back(1);
		_usingspace.push_back(_currspace);

		_inferences["print"].push_back(new PrintTheory());
		_inferences["print"].push_back(new PrintVocabulary());
		_inferences["print"].push_back(new PrintStructure());
		_inferences["print"].push_back(new PrintNamespace());
		_inferences["push_negations"].push_back(new PushNegations());
		_inferences["remove_equivalences"].push_back(new RemoveEquivalences());
		_inferences["remove_eqchains"].push_back(new RemoveEqchains());
		_inferences["flatten"].push_back(new FlattenFormulas());
		_inferences["ground"].push_back(new GroundingInference());
		_inferences["ground"].push_back(new GroundingWithResult());
		_inferences["convert_to_theory"].push_back(new StructToTheory());
		_inferences["move_quantifiers"].push_back(new MoveQuantifiers());
		_inferences["tseitin"].push_back(new ApplyTseitin());
		_inferences["reduce"].push_back(new GroundReduce());
		_inferences["move_functions"].push_back(new MoveFunctions());
		_inferences["model_expand"].push_back(new ModelExpansionInference());
	}

	void cleanup() {
		for(unsigned int n = 0; n < _allfiles.size(); ++n) {
			if(_allfiles[n]) delete(_allfiles[n]);
		}
		if(_currfile) delete(_currfile);
		for(map<string,vector<Inference*> >::iterator it = _inferences.begin(); it != _inferences.end(); ++it) {
			for(unsigned int n = 0; n < (it->second).size(); ++n) {
				delete((it->second)[n]);
			}
		}
	}

	void closespace() {
		closeblock();
		_currspace = _currspace->super();
		assert(_currspace);
	}

	void openspace(const string& sname, YYLTYPE l) {
		Info::print("Parsing namespace " + sname);
		ParseInfo pi = parseinfo(l);
		Namespace* ns = new Namespace(sname,_currspace,pi);
		_nrvocabs.push_back(0);
		_currspace = ns;
		_usingspace.push_back(ns);
		_nrvocabs.push_back(0);
		_nrspaces.push_back(1);
	}

	/*************
		Options
	*************/
	
	void openoptions(const string& name, YYLTYPE l) {
		Info::print("Parsing options " + name);
		ParseInfo pi = parseinfo(l);
		InfOptions* opt = optionsInScope(name,pi);
		if(opt) Error::multdeclopt(name,pi,opt->_pi);
		_curroptions = new InfOptions(name,pi);
		_currspace->add(_curroptions);
		_nrvocabs.push_back(0);
		_nrspaces.push_back(0);
	}

	void closeoptions() {
		closeblock();
	}

	void externoption(const vector<string>& name, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		InfOptions* opt = optionsInScope(name,pi);
		if(opt) _curroptions->set(opt);
		else Error::undeclopt(oneName(name),pi);
	}

	void option(const string& opt, const string& val, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		if(InfOptions::isoption(opt)) {
			Error::wrongvaluetype(opt,pi);
		}
		else Error::unknopt(opt,pi);
	}

	void option(const string& opt, double val, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		if(InfOptions::isoption(opt)) {
			Error::wrongvaluetype(opt,pi);
		}
		else Error::unknopt(opt,pi);
	}

	void option(const string& opt, int val, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		if(InfOptions::isoption(opt)) {
			if(opt == "nrmodels") {
				if(val >= 0) {
					_curroptions->_nrmodels = val;
				}
				else Error::posintexpected(opt,pi);
			}
			else Error::wrongvaluetype(opt,pi);
		}
		else Error::unknopt(opt,pi);
	}

	/*****************
		Procedures
	*****************/

	void openproc(const string& name, YYLTYPE l) {
		Info::print("Parsing procedure " + name);
		ParseInfo pi = parseinfo(l);
		LuaProcedure* lp = procInScope(name,pi);
		if(lp) Error::multdeclproc(name,pi,lp->pi());
		_currproc = new LuaProcedure(name,pi);
		_currspace->add(_currproc);
		_nrvocabs.push_back(0);
		_nrspaces.push_back(0);
	}

	void closeproc() {
		closeblock();
	}

	void procarg(const string& type, const string& name, YYLTYPE l) {
		InfArgType tp = IATUtils::to_iat(type);
		if(tp != IAT_ERROR) _currproc->addarg(tp,name);
		else {
			ParseInfo pi = parseinfo(l);
			Error::unkniat(type,pi);
		}
	}

	void procreturn(const string& type, YYLTYPE l) {
		InfArgType tp = IATUtils::to_iat(type);
		if(tp != IAT_ERROR) _currproc->outtype(tp);
		else {
			ParseInfo pi = parseinfo(l);
			Error::unkniat(type,pi);
		}
	}

	void luacode(char* s) {
		_currproc->add(s);
	}

	void luacode(const string& s) {
		_currproc->add(s);
	}

	void luacode(const vector<string>& vs) {
		assert(!vs.empty());
		_currproc->add(vs[0]);
		for(unsigned int n = 1; n < vs.size(); ++n) {
			_currproc->add(string("."));
			_currproc->add(vs[n]);
		}
	}

	/*******************
		Vocabularies
	*******************/

	/** Open and close vocabularies **/

	void openvocab(const string& vname, YYLTYPE l) {
		Info::print("Parsing vocabulary " + vname);
		ParseInfo pi = parseinfo(l);
		Vocabulary* v = new Vocabulary(vname,pi);
		_currspace->add(v);
		_currvocab = v;	
		_usingvocab.push_back(v);
		_nrvocabs.push_back(1);
		_nrspaces.push_back(0);
	}

	void closevocab() {
		closeblock();
	}

	void usingvocab(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Vocabulary* v = vocabInScope(vs,pi);
		if(v) {
			_usingvocab.push_back(v);
			_nrvocabs.back() = _nrvocabs.back()+1;
		}
		else Error::undeclvoc(oneName(vs),pi);
	}

	void usingspace(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Namespace* s = namespaceInScope(vs,pi);
		if(s) {
			_usingspace.push_back(s);
			_nrspaces.back() = _nrspaces.back() + 1;
		}
		else Error::undeclspace(oneName(vs),pi);
	}

	void setvocab(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Vocabulary* v = vocabInScope(vs,pi);
		if(v) {
			_usingvocab.push_back(v);
			_nrvocabs.back() = _nrvocabs.back()+1;
			if(_currstructure) _currstructure->vocabulary(v);
			else if(_currtheory) _currtheory->vocabulary(v);
			_currvocab = v;
		}
		else {
			Error::undeclvoc(oneName(vs),pi);
		}
	}

	/** Pointers to symbols **/

	Predicate* predpointer(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Predicate* p = predInScope(vs,pi);
		if(!p) Error::undeclpred(oneName(vs),pi);
		return p;
	}

	Predicate* predpointer(const vector<string>& vs, const vector<Sort*>& va, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		NSTuple nst(vs,va,false,pi);
		nst.includePredArity();
		Predicate* p = predpointer(nst._name,l);
		if(p) {
			p = p->resolve(va);
			if(!p) Error::undeclpred(nst.to_string(),pi);
		}
		return p;
	}

	Function* funcpointer(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Function* f = funcInScope(vs,pi);
		if(!f) Error::undeclfunc(oneName(vs),pi);
		return f;
	}

	Function* funcpointer(const vector<string>& vs, const vector<Sort*>& va, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		NSTuple nst(vs,va,false,pi);
		nst.func(true);
		nst.includeFuncArity();
		Function* f = funcpointer(nst._name,l);
		if(f) {
			f->resolve(va);
			if(!f) Error::undeclfunc(nst.to_string(),pi);
		}
		return f;
	}

	Sort* sortpointer(const vector<string>& vs, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Sort* s = sortInScope(vs,pi);
		if(!s) Error::undeclsort(oneName(vs),pi);
		return s;
	}

	Sort* theosortpointer(const vector<string>& vs, YYLTYPE l) {
		Sort* s = sortpointer(vs,l);
		if(s) {
			if(belongsToVoc(s)) {
				return s;
			}
			else {
				ParseInfo pi = parseinfo(l);
				string uname = oneName(vs);
				if(_currtheory) Error::sortnotintheovoc(uname,_currtheory->name(),pi);
				else if(_currstructure) Error::sortnotinstructvoc(uname,_currstructure->name(),pi);
				return 0;
			}
		}
		else return 0;
	}

	NSTuple* internpointer(const vector<string>& name, const vector<Sort*>& sorts, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		return new NSTuple(name,sorts,false,pi);
	}

	NSTuple* internpointer(const vector<string>& name, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		return new NSTuple(name,false,pi);
	}

	/** Add symbols to the current vocabulary **/

	void externvocab(const vector<string>& vname, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Vocabulary* v = vocabInScope(vname,pi);
		if(v) {
			for(map<string,std::set<Sort*> >::iterator it = v->firstsort(); it != v->lastsort(); ++it) {
				for(std::set<Sort*>::iterator jt = (it->second).begin(); jt != (it->second).end(); ++jt) {
					_currvocab->addSort(*jt);
				}
			}
			for(map<string,Predicate*>::iterator it = v->firstpred(); it != v->lastpred(); ++it) {
				_currvocab->addPred(it->second);
			}
			for(map<string,Function*>::iterator it = v->firstfunc(); it != v->lastfunc(); ++it) {
				_currvocab->addFunc(it->second);
			}
		}
		else Error::undeclvoc(oneName(vname),pi);
	}

	Sort* sort(Sort* s) {
		if(s) _currvocab->addSort(s);
		return s;
	}

	Sort* sort(const string& name, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		ParseInfo pip = parseinfo(l);
		Sort* s = new Sort(name,pi);
		_currvocab->addSort(s);
		Predicate* p = new Predicate(name + "/1",vector<Sort*>(1,s),pip);
		_currvocab->addPred(p);
		s->pred(p);
		return s;
	}

	Sort* sort(const string& name, const vector<Sort*> supbs, bool p, YYLTYPE l) {
		vector<Sort*> vs(0);
		if(p) return sort(name,supbs,vs,l);
		else return sort(name,vs,supbs,l);
	}

	Sort* sort(const string& name, const vector<Sort*> sups, const vector<Sort*> subs, YYLTYPE l) {
		Sort* s = sort(name,l);
		if(s) {
			vector<std::set<Sort*> > supsa(sups.size());
			vector<std::set<Sort*> > subsa(subs.size());
			for(unsigned int n = 0; n < sups.size(); ++n) {
				if(sups[n]) {
					supsa[n] = sups[n]->ancestors(0);
					supsa[n].insert(sups[n]);
				}
			}
			for(unsigned int n = 0; n < subs.size(); ++n) {
				if(subs[n]) {
					subsa[n] = subs[n]->ancestors(0);
					subsa[n].insert(subs[n]);
				}
			}
			for(unsigned int n = 0; n < sups.size(); ++n) {
				if(sups[n]) {
					for(unsigned int m = 0; m < subs.size(); ++m) {
						if(subs[m]) {
							if(subsa[m].find(sups[n]) == subsa[m].end()) {
								Error::notsubsort(subs[m]->name(),sups[n]->name(),parseinfo(l));
							}
						}
					}
					s->parent(sups[n]);
				}
			}
			for(unsigned int n = 0; n < subs.size(); ++n) {
				if(subs[n]) {
					for(unsigned int m = 0; m < sups.size(); ++m) {
						if(sups[m]) {
							if(supsa[m].find(subs[n]) != supsa[m].end()) {
								Error::cyclichierarchy(subs[n]->name(),sups[m]->name(),parseinfo(l));
							}
						}
					}
					s->child(subs[n]);
				}
			}
		}
		return s;
	}

	Predicate* predicate(Predicate* p) {
		if(p) _currvocab->addPred(p);
		return p;
	}

	Predicate* predicate(const string& name, const vector<Sort*>& sorts, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		string nar = string(name) + '/' + itos(sorts.size());
		for(unsigned int n = 0; n < sorts.size(); ++n) {
			if(!sorts[n]) return 0;
		}
		Predicate* p = new Predicate(nar,sorts,pi);
		_currvocab->addPred(p);
		return p;
	}

	Predicate* predicate(const string& name, YYLTYPE l) {
		vector<Sort*> vs(0);
		return predicate(name,vs,l);
	}

	Function* function(Function* f) {
		if(f) _currvocab->addFunc(f);
		return f;
	}

	Function* function(const string& name, const vector<Sort*>& insorts, Sort* outsort, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		string nar = string(name) + '/' + itos(insorts.size());
		for(unsigned int n = 0; n < insorts.size(); ++n) {
			if(!insorts[n]) return 0;
		}
		if(!outsort) return 0;
		Function* f = new Function(nar,insorts,outsort,pi);
		_currvocab->addFunc(f);
		return f;
	}

	Function* function(const string& name, const vector<Sort*>& sorts, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		string nar = string(name) + '/' + itos(sorts.size());
		for(unsigned int n = 0; n < sorts.size(); ++n) {
			if(!sorts[n]) return 0;
		}
		Function* f = new Function(nar,sorts,pi);
		_currvocab->addFunc(f);
		return f;
	}

	Function* function(const string& name, Sort* outsort, YYLTYPE l) {
		vector<Sort*> vs(0);
		return function(name,vs,outsort,l);
	}


	/*****************
		Structures
	*****************/

	void openstructure(const string& sname, YYLTYPE l) {
		Info::print("Parsing structure " + sname);
		ParseInfo pi = parseinfo(l);
		AbstractStructure* s = structInScope(sname,pi);
		if(s) {
			Error::multdeclstruct(sname,pi,s->pi());
		}
		_currstructure = new Structure(sname,pi);
		_currspace->add(_currstructure);
		_nrvocabs.push_back(0);
		_nrspaces.push_back(0);
	}

	void structinclusioncheck() {
		/*Vocabulary* v = _currstructure->vocabulary();
		for(unsigned int n = 0; n < v->nrSorts(); ++n) {
			Sort* s = v->sort(n);
			if(s->parent()) {
				SortTable* st = _currstructure->inter(s);
				SortTable* pt = _currstructure->inter(s->parent());
				if(st && pt) {
					for(unsigned int r = 0; r < st->size(); ++r) {
						if(!(pt->contains(st->element(r),st->type()))) {
							string str = ElementUtil::ElementToString(st->element(r),st->type());
							Error::sortelnotinsort(str,s->name(),s->parent()->name(),_currstructure->name());
						}
					}
				}
			}
		}
		for(unsigned int n = 0; n < v->nrPreds(); ++n) {
			Predicate* p = v->pred(n);
			PredInter* pt = _currstructure->inter(p);
			if(pt) {
				PredTable* ct = pt->ctpf();
				PredTable* cf = pt->cfpt();
				for(unsigned int c = 0; c < p->arity(); ++c) {
					SortTable* st = _currstructure->inter(p->sort(c));
					if(st) {
						if(ct) {
							for(unsigned int r = 0; r < ct->size(); ++r) {
								if(!(st->contains(ct->element(r,c),ct->type(c)))) {
									string str = ElementUtil::ElementToString(ct->element(r,c),ct->type(c));
									Error::predelnotinsort(str,p->name(),p->sort(c)->name(),_currstructure->name());
								}
							}
						}
						if(cf && ct != cf) {
							for(unsigned int r = 0; r < cf->size(); ++r) {
								if(!(st->contains(cf->element(r,c),cf->type(c)))) {
									string str = ElementUtil::ElementToString(cf->element(r,c),cf->type(c));
									Error::predelnotinsort(str,p->name(),p->sort(c)->name(),_currstructure->name());
								}
							}
						}
					}
				}
			}
		}
		for(unsigned int n = 0; n < v->nrFuncs(); ++n) {
			Function* f = v->func(n);
			PredInter* pt = 0;
			if(_currstructure->inter(f)) pt = _currstructure->inter(f)->predinter();
			if(pt) {
				PredTable* ct = pt->ctpf();
				PredTable* cf = pt->cfpt();
				for(unsigned int c = 0; c < f->nrsorts(); ++c) {
					SortTable* st = _currstructure->inter(f->sort(c));
					if(st) {
						if(ct) {
							for(unsigned int r = 0; r < ct->size(); ++r) {
								if(!(st->contains(ct->element(r,c),ct->type(c)))) {
									string str = ElementUtil::ElementToString(ct->element(r,c),ct->type(c));
									Error::predelnotinsort(str,f->name(),f->sort(c)->name(),_currstructure->name());
								}
							}
						}
						if(cf && ct != cf) {
							for(unsigned int r = 0; r < cf->size(); ++r) {
								if(!(st->contains(cf->element(r,c),cf->type(c)))) {
									string str = ElementUtil::ElementToString(cf->element(r,c),cf->type(c));
									Error::predelnotinsort(str,f->name(),f->sort(c)->name(),_currstructure->name());
								}
							}
						}
					}
				}
			}
		}
		*/
	}

	void functioncheck() {
		/*Vocabulary* v = _currstructure->vocabulary();
		for(unsigned int n = 0; n < v->nrFuncs(); ++n) {
			Function* f = v->func(n);
			FuncInter* ft = _currstructure->inter(f);
			if(ft) {
				PredInter* pt = ft->predinter();
				PredTable* ct = pt->ctpf();
				PredTable* cf = pt->cfpt();
				// Check if the interpretation is indeed a function
				bool isfunc = true;
				if(ct) {
					vector<ElementType> vet = ct->types(); vet.pop_back();
					ElementEquality eq(vet);
					for(unsigned int r = 1; r < ct->size(); ) {
						if(eq(ct->tuple(r-1),ct->tuple(r))) {
							vector<Element> vel = ct->tuple(r);
							vector<string> vstr(vel.size()-1);
							for(unsigned int c = 0; c < vel.size()-1; ++c) 
								vstr[c] = ElementUtil::ElementToString(vel[c],ct->type(c));
							Error::notfunction(f->name(),_currstructure->name(),vstr);
							while(eq(ct->tuple(r-1),ct->tuple(r))) ++r;
							isfunc = false;
						}
						else ++r;
					}
				}
				// Check if the interpretation is total
				if(isfunc && !(f->partial()) && ct && (ct == cf || (!(pt->cf()) && cf->size() == 0))) {
					unsigned int c = 0;
					unsigned int s = 1;
					for(; c < f->arity(); ++c) {
						if(_currstructure->inter(f->insort(c))) {
							s = s * _currstructure->inter(f->insort(c))->size();
						}
						else break;
					}
					if(c == f->arity()) {
						assert(ct->size() <= s);
						if(ct->size() < s) {
							Error::nottotal(f->name(),_currstructure->name());
						}
					}
				}
			}
		}
		*/
	}

	void closestructure() {

		// Complete the structure
		bool change = true;
		while(change) {
			change = false;

			// Assign the unknown predicate interpretations
			for(map<Predicate*,FinitePredTable*>::iterator it = _unknownpredtables.begin(); it != _unknownpredtables.end(); ) {
				map<Predicate*,FinitePredTable*>::iterator jt = it; ++it;
				PredInter* pt = _currstructure->inter(jt->first);
				if(pt) {
					if(pt->ctpf()) {
						if(pt->cfpt()) {
							Error::threethreepred(jt->first->name(),_currstructure->name());
							delete(jt->second);
						}
						else {
							assert(pt->ct()); 
							assert(pt->ctpf()->finite());
							FinitePredTable* fpt = jt->second;
							for(unsigned int n = 0; n < pt->ctpf()->size(); ++n) {
								fpt->addRow(pt->ctpf()->tuple(n),pt->ctpf()->types());
							}
							fpt->sortunique();
							pt->replace(fpt,false,false);
						}
					}
					else {
						assert(pt->cfpt()); 
						assert(pt->cf()); 
						assert(pt->cfpt()->finite());
						FinitePredTable* fpt = jt->second;
						for(unsigned int n = 0; n < pt->cfpt()->size(); ++n) {
							fpt->addRow(pt->cfpt()->tuple(n),pt->cfpt()->types());
						}
						fpt->sortunique();
						pt->replace(fpt,true,false);
					}
					_unknownpredtables.erase(jt);
					change = true;
				}
			}
			// Assign the unknown function interpretations
			for(map<Function*,FinitePredTable*>::iterator it = _unknownfunctables.begin(); it != _unknownfunctables.end(); ) {
				map<Function*,FinitePredTable*>::iterator jt = it; ++it;
				FuncInter* ft = _currstructure->inter(jt->first);
				PredInter* pt = 0;
				if(ft) pt = ft->predinter();
				if(pt) {
					if(pt->ctpf()) {
						if(pt->cfpt()) {
							Error::threethreefunc(jt->first->name(),_currstructure->name());
							delete(jt->second);
						}
						else {
							assert(pt->ct()); 
							assert(pt->ctpf()->finite());
							FinitePredTable* fpt = jt->second;
							for(unsigned int n = 0; n < pt->ctpf()->size(); ++n) {
								fpt->addRow(pt->ctpf()->tuple(n),pt->ctpf()->types());
							}
							fpt->sortunique();
							pt->replace(fpt,false,false);
						}
					}
					else {
						assert(pt->cfpt()); 
						assert(pt->cf()); 
						assert(pt->cfpt()->finite());
						FinitePredTable* fpt = jt->second;
						for(unsigned int n = 0; n < pt->cfpt()->size(); ++n) {
							fpt->addRow(pt->cfpt()->tuple(n),pt->cfpt()->types());
						}
						fpt->sortunique();
						pt->replace(fpt,true,false);
					}
					_unknownfunctables.erase(jt);
					change = true;
				}
			}
		}

		// Errors for non-assignable tables
		for(map<Predicate*,FinitePredTable*>::iterator it = _unknownpredtables.begin(); it != _unknownpredtables.end(); ++it) {
			Error::onethreepred(it->first->name(),_currstructure->name());
			delete(it->second);
		}
		for(map<Function*,FinitePredTable*>::iterator it = _unknownfunctables.begin(); it != _unknownfunctables.end(); ++it) {
			Error::onethreefunc(it->first->name(),_currstructure->name());
			delete(it->second);
		}
	
		// inclusion checks
		structinclusioncheck();

		// function check
		functioncheck();

		// close the structure
		_currstructure->close();

		// reset the auxiliary data structures
		_unknownpredtables.clear();
		_unknownfunctables.clear();

		// close block
		closeblock();
	}

	void closeaspstructure() {
		for(unsigned int n = 0; n < _currstructure->nrSortInters(); ++n) {
			SortTable* st = _currstructure->sortinter(n);
			if(st) st->sortunique();
		}
		for(unsigned int n = 0; n < _currstructure->nrPredInters(); ++n) {
			PredInter* pt = _currstructure->predinter(n);
			if(pt) pt->sortunique();
		}
		for(unsigned int n = 0; n < _currstructure->nrFuncInters(); ++n) {
			FuncInter* ft = _currstructure->funcinter(n);
			if(ft) ft->sortunique();
		}
		closestructure();
	}

	void closeaspbelief() {
		// TODO
	}

	/** Two-valued interpretations **/

	void sortinter(NSTuple* nst, FiniteSortTable* t) {
		ParseInfo pi = nst->_pi;
		Sort* s = sortInScope(nst->_name,pi);
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != 1) Error::incompatiblearity(nst->to_string(),pi);
			if(nst->_func) Error::prednameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + "/1";
		Predicate* p = predInScope(nst->_name,pi);
		if(p && nst->_sortsincluded && (nst->_sorts).size() == 1) p = p->resolve(nst->_sorts);
		if(s) {
			assert(p);
			t->sortunique();
			PredInter* i = new PredInter(t,true);
			_currstructure->inter(s,t);
			_currstructure->inter(p,i);
		}
		else if(p) {
			t->sortunique();
			PredInter* i = new PredInter(t,true);
			_currstructure->inter(p,i);
		}
		else Error::prednotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		delete(nst);
	}

	void predinter(NSTuple* nst, FinitePredTable* t) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->to_string(),pi);
			if(nst->_func) Error::prednameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(t->arity());
		Predicate* p = predInScope(nst->_name,pi);
		if(p && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) p = p->resolve(nst->_sorts);
		if(p) {
			if(belongsToVoc(p)) {
				if(_currstructure->inter(p)) Error::multpredinter(nst->to_string(),pi);
				else {
					t->sortunique();
					PredInter* pt = new PredInter(t,true);
					_currstructure->inter(p,pt);
				}
			}
			else Error::prednotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclpred(nst->to_string(),pi);
		delete(nst);
	}


	void funcinter(NSTuple* nst, FinitePredTable* t) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->to_string(),pi);
			if(!(nst->_func)) Error::funcnameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(t->arity()-1);
		Function* f = funcInScope(nst->_name,pi);
		if(f && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				if(_currstructure->inter(f)) Error::multfuncinter(f->name(),pi);
				else {
					t->sortunique();
					PredInter* pt = new PredInter(t,true);
					FiniteFuncTable* fft = new FiniteFuncTable(t);
					FuncInter* ft = new FuncInter(fft,pt);
					_currstructure->inter(f,ft);
				}
			}
			else Error::funcnotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclfunc(nst->to_string(),pi);
	}

	void truepredinter(NSTuple* nst) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		upt->addRow();
		predinter(nst,upt);
	}

	void falsepredinter(NSTuple* nst) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		predinter(nst,upt);
	}

	void emptyinter(NSTuple* nst) {
		if(nst->_sortsincluded) {
			FinitePredTable* upt = new FinitePredTable(vector<ElementType>((nst->_sorts).size(),ELINT));
			if(nst->_func) funcinter(nst,upt);
			else predinter(nst,upt);
		}
		else {
			ParseInfo pi = nst->_pi;
			vector<Predicate*> vp = noArPredInScope(nst->_name,pi);
			if(vp.empty()) Error::undeclpred(nst->to_string(),pi);
			else if(vp.size() > 1) {
				Error::overloadedpred(nst->to_string(),vp[0]->pi(),vp[1]->pi(),pi);
			}
			else {
				FinitePredTable* upt = new FinitePredTable(vector<ElementType>((vp[0]->arity(),ELINT)));
				predinter(nst,upt);
			}
		}
	}

	/** Three-valued interpretations **/

	UTF getUTF(const string& utf, const ParseInfo& pi) {
		if(utf == "u") return UTF_UNKNOWN;
		else if(utf == "ct") return UTF_CT;
		else if(utf == "cf") return UTF_CF;
		else {
			Error::expectedutf(utf,pi);
			return UTF_ERROR;
		}
	}

	void threepredinter(NSTuple* nst, const string& utf, FinitePredTable* t) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->to_string(),pi);
			if(nst->_func) Error::prednameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(t->arity());
		Predicate* p = predInScope(nst->_name,pi);
		if(p && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) p = p->resolve(nst->_sorts);
		if(p) {
			if(belongsToVoc(p)) {
				t->sortunique();
				switch(getUTF(utf,pi)) {
					case UTF_UNKNOWN:
						if(_unknownpredtables.find(p) == _unknownpredtables.end() && !(p->builtin()))  _unknownpredtables[p] = t;
						else Error::multunknpredinter(p->name(),pi);
						break;
					case UTF_CT:
					{	
						PredInter* pt = _currstructure->inter(p);
						if(pt) {
							if(pt->ctpf()) Error::multctpredinter(p->name(),pi);
							else pt->replace(t,true,true);
						}
						else {
							pt = new PredInter(t,0,true,true);
							_currstructure->inter(p,pt);
						}
						break;
					}
					case UTF_CF:
					{
						PredInter* pt = _currstructure->inter(p);
						if(pt) {
							if(pt->cfpt()) Error::multcfpredinter(p->name(),pi);
							else pt->replace(t,false,true);
						}
						else {
							pt = new PredInter(0,t,true,true);
							_currstructure->inter(p,pt);
						}
						break;
					}
					case UTF_ERROR:
						break;
					default:
						assert(false);
				}
			}
			else Error::prednotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclpred(nst->to_string(),pi);
	}

	void truethreepredinter(NSTuple* nst, const string& utf) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		upt->addRow();
		threepredinter(nst,utf,upt);
	}

	void falsethreepredinter(NSTuple* nst, const string& utf) {
		FinitePredTable* upt = new FinitePredTable(vector<ElementType>(0));
		threepredinter(nst,utf,upt);
	}

	void threefuncinter(NSTuple* nst, const string& utf, FinitePredTable* t) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != t->arity()) Error::incompatiblearity(nst->to_string(),pi);
			if(!(nst->_func)) Error::funcnameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(t->arity()-1);
		Function* f = funcInScope(nst->_name,pi);
		if(f && nst->_sortsincluded && (nst->_sorts).size() == t->arity()) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				t->sortunique();
				switch(getUTF(utf,pi)) {
					case UTF_UNKNOWN:
						if(_unknownfunctables.find(f) == _unknownfunctables.end() && !(f->builtin()))  _unknownfunctables[f] = t;
						else Error::multunknfuncinter(f->name(),pi);
						break;
					case UTF_CT:
					{	
						FuncInter* ft = _currstructure->inter(f);
						if(ft) {
							if(ft->predinter()->ctpf()) Error::multctfuncinter(f->name(),pi);
							else ft->predinter()->replace(t,true,true);
						}
						else {
							PredInter* pt = new PredInter(t,0,true,true);
							ft = new FuncInter(0,pt);
							_currstructure->inter(f,ft);
						}
						break;
					}
					case UTF_CF:
					{
						FuncInter* ft = _currstructure->inter(f);
						if(ft) {
							if(ft->predinter()->cfpt()) Error::multcffuncinter(f->name(),pi);
							else ft->predinter()->replace(t,false,true);
						}
						else {
							PredInter* pt = new PredInter(0,t,true,true);
							ft = new FuncInter(0,pt);
							_currstructure->inter(f,ft);
						}
						break;
					}
					case UTF_ERROR:
						break;
					default:
						assert(false);
				}
			}
			else Error::funcnotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclfunc(nst->to_string(),pi);
	}

	void threeinter(NSTuple* nst, const string& utf, FiniteSortTable* t) {
		FinitePredTable* spt = new FinitePredTable(*t);
		if(nst->_func) threefuncinter(nst,utf,spt);
		else threepredinter(nst,utf,spt);
	}

	void emptythreeinter(NSTuple* nst, const string& utf) {
		if(nst->_sortsincluded) {
			FinitePredTable* upt = new FinitePredTable(vector<ElementType>((nst->_sorts).size(),ELINT));
			if(nst->_func) threefuncinter(nst,utf,upt);
			else threepredinter(nst,utf,upt);
		}
		else {
			ParseInfo pi = nst->_pi;
			vector<Predicate*> vp = noArPredInScope(nst->_name,pi);
			if(vp.empty()) Error::undeclpred(nst->to_string(),pi);
			else if(vp.size() > 1) {
				Error::overloadedpred(nst->to_string(),vp[0]->pi(),vp[1]->pi(),pi);
			}
			else {
				FinitePredTable* upt = new FinitePredTable(vector<ElementType>((vp[0]->arity(),ELINT)));
				threepredinter(nst,utf,upt);
			}
		}
	}

	/** ASP atoms **/

	void predatom(Predicate* p, const vector<ElementType>& vet, const vector<Element>& ve, bool ct) {
		FinitePredTable* pt;
		if(ct) pt = dynamic_cast<FinitePredTable*>(_currstructure->inter(p)->ctpf());
		else pt = dynamic_cast<FinitePredTable*>(_currstructure->inter(p)->cfpt());
		pt->addRow(ve,vet);
	}

	void predatom(Predicate* p, ElementType t, Element e, bool ct) {
		PredInter* pt = _currstructure->inter(p);
		if(pt) {
			FiniteSortTable* spt;
			if(ct) spt = dynamic_cast<FiniteSortTable*>(pt->ctpf());
			else spt = dynamic_cast<FiniteSortTable*>(pt->cfpt());
			FiniteSortTable* ust = 0;
			switch(t) {
				case ELINT:
					ust = spt->add(e._int); break;
				case ELDOUBLE:
					ust = spt->add(e._double); break;
				case ELSTRING:
					ust = spt->add(e._string); break;
				case ELCOMPOUND:
					ust = spt->add(e._compound); break;
				default:
					assert(false);
			}
			if(ust != spt) {
				delete(spt);
				pt->replace(ust,ct,true);
			}
		}
		else {
			FiniteSortTable* ust = 0;
			FiniteSortTable* ost = new IntSortTable();
			switch(t) {
				case ELINT: ust = new IntSortTable(); ust->add(e._int); break;
				case ELDOUBLE: ust = new FloatSortTable(); ust->add(e._double); break;
				case ELSTRING: ust = new StrSortTable(); ust->add(e._string); break;
				case ELCOMPOUND: ust = new MixedSortTable(); ust->add(e._compound); break;
				default: assert(false);
			}
			if(ct) pt = new PredInter(ust,ost,true,true);
			else pt = new PredInter(ost,ust,true,true);
			_currstructure->inter(p,pt);
		}
	}

	void predatom(Predicate* p, FiniteSortTable* st, bool ct) {
		PredInter* pt = _currstructure->inter(p);
		if(!pt) {
			FiniteSortTable* ost = new IntSortTable();
			if(ct) pt = new PredInter(st,ost,true,true);
			else pt = new PredInter(ost,st,true,true);
			_currstructure->inter(p,pt);
		}
		else {
			switch(st->type()) {
				case ELINT: {
					RanSortTable* rst = dynamic_cast<RanSortTable*>(st);
					for(unsigned int n = 0; n < st->size(); ++n) {
						Element e; e._int = (*rst)[n];
						predatom(p,ELINT,e,ct);
					}
					break;
				}
				case ELSTRING: {
					StrSortTable* sst = dynamic_cast<StrSortTable*>(st);
					for(unsigned int n = 0; n < st->size(); ++n) {
						Element e; e._string = (*sst)[n];
						predatom(p,ELSTRING,e,ct);
					}
					break;
				}
				default: assert(false);
			}
			delete(st);
		}
	}

	void predatom(Predicate* p, const vector<ElementType>& vet, const vector<Element>& ve, const vector<FiniteSortTable*>& vst, const vector<bool>& vb, bool ct) {

		vector<ElementType> vet2(vb.size());
		unsigned int skipcounter = 0;
		for(unsigned int n = 0; n < vb.size(); ++n) {
			if(vb[n]) vet2[n] = vet[n-skipcounter];
			else {
				vet2[n] = vst[skipcounter]->type();
				++skipcounter;
			}
		}

		vector<Element> ve2(ve.size());
		vector<SortTable*> vst2; for(unsigned int n = 0; n < vst.size(); ++n) vst2.push_back(vst[n]);
		SortTableTupleIterator stti(vst2);
		if(!stti.empty()) {
			do {
				unsigned int skip = 0;
				for(unsigned int n = 0; n < vb.size(); ++n) {
					if(vb[n]) ve2[n] = ve[n-skip];
					else {
						ve2[n] = stti.value(skip);
						++skip;
					}
				}
				predatom(p,vet2,ve2,ct);
			} while(stti.nextvalue());
		}

		for(unsigned int n = 0; n < vst.size(); ++n) {
			delete(vst[n]);
		}
	}

	void predatom(NSTuple* nst, const vector<Element>& ve, const vector<FiniteSortTable*>& vt, const vector<ElementType>& vet, const vector<bool>& vb,bool ct) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != vb.size()) Error::incompatiblearity(nst->to_string(),pi);
			if(nst->_func) Error::prednameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(vb.size());
		Predicate* p = predInScope(nst->_name,pi);
		if(p && nst->_sortsincluded && (nst->_sorts).size() == vb.size()) p = p->resolve(nst->_sorts);
		if(p) {
			if(belongsToVoc(p)) {
				if(p->arity() == 1) {
					if(vt.empty()) predatom(p,vet[0],ve[0],ct);
					else predatom(p,vt[0],ct);
				}
				else {
					PredInter* pt = _currstructure->inter(p);
					if(!pt) {
						vector<ElementType> vet2(vb.size(),ELINT);
						FinitePredTable* ctt = new FinitePredTable(vet2);
						FinitePredTable* cft = new FinitePredTable(vet2);
						pt = new PredInter(ctt,cft,true,true);
						_currstructure->inter(p,pt);
					}
					if(vt.empty()) predatom(p,vet,ve,ct);
					else predatom(p,vet,ve,vt,vb,ct);
				}
			}
			else Error::prednotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclpred(nst->to_string(),pi);
		delete(nst);
	}

	void funcatom(Function* f, const vector<ElementType>& vet, const vector<Element>& ve, bool ct) {
		FuncInter* ft = _currstructure->inter(f);
		FinitePredTable* pt;
		if(ct) pt = dynamic_cast<FinitePredTable*>(ft->predinter()->ctpf());
		else pt = dynamic_cast<FinitePredTable*>(ft->predinter()->cfpt());
		pt->addRow(ve,vet);
	}

	void funcatom(Function* f, const vector<ElementType>& vet, const vector<Element>& ve, const vector<FiniteSortTable*>& vst, const vector<bool>& vb, bool ct) {
		vector<ElementType> vet2(vb.size());
		unsigned int skipcounter = 0;
		for(unsigned int n = 0; n < vb.size(); ++n) {
			if(vb[n]) vet2[n] = vet[n-skipcounter];
			else {
				vet2[n] = vst[skipcounter]->type();
				++skipcounter;
			}
		}

		vector<Element> ve2(ve.size());
		vector<SortTable*> vst2; for(unsigned int n = 0; n < vst.size(); ++n) vst2.push_back(vst[n]);
		SortTableTupleIterator stti(vst2);
		if(!stti.empty()) {
			do {
				unsigned int skip = 0;
				for(unsigned int n = 0; n < vb.size(); ++n) {
					if(vb[n]) ve2[n] = ve[n-skip];
					else {
						ve2[n] = stti.value(skip);
						++skip;
					}
				}
				funcatom(f,vet2,ve2,ct);
			} while(stti.nextvalue());
		}

		for(unsigned int n = 0; n < vst.size(); ++n) {
			delete(vst[n]);
		}
	}

	void funcatom(NSTuple* nst, const vector<Element>& ve, const vector<FiniteSortTable*>& vt, const vector<ElementType>& vet, const vector<bool>& vb,bool ct) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != vb.size()) Error::incompatiblearity(nst->to_string(),pi);
			if(!nst->_func) Error::funcnameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(vb.size()-1);
		Function* f = funcInScope(nst->_name,pi);
		if(f && nst->_sortsincluded && (nst->_sorts).size() == vb.size()) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				FuncInter* ft = _currstructure->inter(f);
				if(!ft) {
					vector<ElementType> vet2(vb.size(),ELINT);
					FinitePredTable* ctt = new FinitePredTable(vet2);
					FinitePredTable* cft = new FinitePredTable(vet2);
					PredInter* pt = new PredInter(ctt,cft,true,true);
					ft = new FuncInter(0,pt);
					_currstructure->inter(f,ft);
				}
				if(vt.empty()) funcatom(f,vet,ve,ct);
				else funcatom(f,vet,ve,vt,vb,ct);
			}
			else Error::funcnotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclfunc(nst->to_string(),pi);
		delete(nst);
	}


	/** Compound domain elements **/

	compound* makecompound(NSTuple* nst, const vector<TypedElement*>& vte) {
		ParseInfo pi = nst->_pi;
		if(nst->_sortsincluded) {
			if((nst->_sorts).size() != vte.size()+1) Error::incompatiblearity(nst->to_string(),pi);
			if(!(nst->_func)) Error::funcnameexpected(pi);
		}
		(nst->_name).back() = (nst->_name).back() + '/' + itos(vte.size());
		Function* f = funcInScope(nst->_name,pi);
		compound* c = 0;
		if(f && nst->_sortsincluded && (nst->_sorts).size() == vte.size()+1) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				if(f->constructor()) {
					vector<TypedElement> nvte;
					for(unsigned int n = 0; n < vte.size(); ++n) {
						if(vte[n]) {
							nvte.push_back(*(vte[n]));
							delete((vte[n]));
						}
						else return 0;
					}
					c = CPPointer(f,nvte);
				}
				else Error::funcnotconstr(nst->to_string(),pi);
			}
			else Error::funcnotinstructvoc(nst->to_string(),_currstructure->name(),pi);
		}
		else Error::undeclfunc(nst->to_string(),pi);
		return c;
	}

	compound* makecompound(NSTuple* nst) {
		vector<TypedElement*> vte(0);
		return makecompound(nst,vte);
	}
	
	/***************
		Theories
	***************/

	void opentheory(const string& tname, YYLTYPE l) {
		Info::print("Parsing theory "  + tname);
		ParseInfo pi = parseinfo(l);
		AbstractTheory* t = theoInScope(tname,pi);
		if(t) {
			Error::multdecltheo(tname,pi,t->pi());
		}
		_currtheory = new Theory(tname,pi);
		_currspace->add(_currtheory);
		_nrvocabs.push_back(0);
		_nrspaces.push_back(0);
	}

	void closetheory() {
		closeblock();
	}

	void definition(Definition* d) {
		if(d) _currtheory->add(d);
	}

	void sentence(Formula* f) {
		if(f) {
			// 1. Quantify the free variables universally
			vector<Variable*> vv = freevars(f->pi());
			if(!vv.empty()) f =  new QuantForm(true,true,vv,f,f->pi());
			// 2. Sort derivation & checking
			SortDeriver sd(f,_currvocab); 
			SortChecker sc(f,_currvocab);
			// Add the formula to the current theory
			_currtheory->add(f);
		}
		else curr_vars.clear();
	}

	void fixpdef(FixpDef* d) {
		if(d) _currtheory->add(d);
	}

	BoolForm* trueform() {
		vector<Formula*> vf(0);
		return new BoolForm(true,true,vf,FormParseInfo());
	}

	BoolForm* trueform(YYLTYPE l) {
		vector<Formula*> vf(0);
		FormParseInfo pi = formparseinfo(new BoolForm(true,true,vf,FormParseInfo()),l);
		return new BoolForm(true,true,vf,pi);
	}

	BoolForm* falseform() {
		vector<Formula*> vf(0);
		return new BoolForm(true,false,vf,FormParseInfo());
	}

	BoolForm* falseform(YYLTYPE l) {
		vector<Formula*> vf(0);
		FormParseInfo pi = formparseinfo(new BoolForm(true,false,vf,FormParseInfo()),l);
		return new BoolForm(true,false,vf,pi);
	}

	PredForm* predform(NSTuple* nst, const vector<Term*>& vt, YYLTYPE l) {
		if(nst->_sortsincluded) nst->includePredArity();
		else nst->includeArity(vt.size());
		FormParseInfo pi = formparseinfo(l);
		Predicate* p = predInScope(nst->_name,nst->_pi);
		PredForm* pf = 0;
		if(p && nst->_sortsincluded) p = p->resolve(nst->_sorts);
		if(p) {
			if(belongsToVoc(p)) {
				if(p->arity() == vt.size()) {
					unsigned int n = 0;
					for(; n < vt.size(); ++n) { if(!vt[n]) break; }
					if(n == vt.size()) pf = new PredForm(true,p,vt,pi);	// TODO change the formparseinfo
				}
				else Error::wrongpredarity(p->name(),nst->_pi);
			}
			else Error::prednotintheovoc(p->name(),_currtheory->name(),nst->_pi);
		}
		else Error::undeclpred(nst->to_string(),nst->_pi);
		
		// Cleanup
		if(!pf) {
			for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]);	}
		}
		delete(nst);

		return pf;
	}

	PredForm* predform(NSTuple* t, YYLTYPE l) {
		vector<Term*> vt(0);
		return predform(t,vt,l);
	}

	PredForm* funcgraphform(NSTuple* nst, const vector<Term*>& vt, Term* t, YYLTYPE l) {
		if(nst->_sortsincluded) nst->includeFuncArity();
		else nst->includeArity(vt.size());
		FormParseInfo pi = formparseinfo(l);
		Function* f = funcInScope(nst->_name,nst->_pi);
		PredForm* pf = 0;
		if(f && nst->_sortsincluded) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				if(f->arity() == vt.size()) {
					unsigned int n = 0;
					for(; n < vt.size(); ++n) { if(!vt[n]) break; }
					if(n == vt.size() && t) {
						vector<Term*> vt2(vt); vt2.push_back(t);
						pf = new PredForm(true,f,vt2,pi);	// TODO: change the formparseinfo
					}
				}
				else Error::wrongfuncarity(f->name(),nst->_pi);
			}
			else Error::funcnotintheovoc(f->name(),_currtheory->name(),nst->_pi);
		}
		else Error::undeclfunc(nst->to_string(),nst->_pi);

		// Cleanup
		if(!pf) {
			for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]); }
			if(t) delete(t);
		}
		delete(nst);

		return pf;
	}

	PredForm* funcgraphform(NSTuple* nst, Term* t, YYLTYPE l) {
		vector<Term*> vt;
		return funcgraphform(nst,vt,t,l);
	}

	EquivForm* equivform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			//Formula* fpf = new EquivForm(true,lf->pi().original(),rf->pi().original(),FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new EquivForm(true,lf,rf,pi);
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	BoolForm* disjform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			vector<Formula*> vf(2);
			vector<Formula*> pivf(2);
			vf[0] = lf; vf[1] = rf;
			//pivf[0] = lf->pi().original(); pivf[1] = rf->pi().original();
			//BoolForm* pibf = new BoolForm(true,false,pivf,FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new BoolForm(true,false,vf,pi);
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	BoolForm* conjform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			vector<Formula*> vf(2);
			vector<Formula*> pivf(2);
			vf[0] = lf; vf[1] = rf;
			//pivf[0] = lf->pi().original(); pivf[1] = rf->pi().original();
			//BoolForm* pibf = new BoolForm(true,true,pivf,FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new BoolForm(true,true,vf,pi);
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	BoolForm* implform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			lf->swapsign();
			BoolForm* bf = disjform(lf,rf,l);
			return bf;
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	BoolForm* revimplform(Formula* lf, Formula* rf, YYLTYPE l) {
		if(lf && rf) {
			rf->swapsign();
			BoolForm* bf = disjform(lf,rf,l);
			return bf;
		}
		else {
			if(lf) delete(lf);
			if(rf) delete(rf);
			return 0;
		}
	}

	QuantForm* univform(const vector<Variable*>& vv, Formula* f, YYLTYPE l) {
		remove_vars(vv);
		if(f) {
			//QuantForm* piqf = new QuantForm(true,true,vv,f->pi().original(),FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new QuantForm(true,true,vv,f,pi);
		}
		else {
			for(unsigned int n = 0; n < vv.size(); ++n) delete(vv[n]);
			delete(f);
			return 0;
		}
	}

	QuantForm* existform(const vector<Variable*>& vv, Formula* f, YYLTYPE l) {
		remove_vars(vv);
		if(f) {
			//QuantForm* piqf = new QuantForm(true,false,vv,f->pi().original(),FormParseInfo());
			FormParseInfo pi = formparseinfo(0,l);
			return new QuantForm(true,false,vv,f,pi);
		}
		else {
			for(unsigned int n = 0; n < vv.size(); ++n) delete(vv[n]);
			delete(f);
			return 0;
		}
	}

	PredForm* bexform(char c, bool b, int n, const vector<Variable*>& vv, Formula* f, YYLTYPE l) {
		remove_vars(vv);
		if(f) {
			FormParseInfo pi = formparseinfo(l);
			QuantSetExpr* qse = new QuantSetExpr(vv,f,pi);
			vector<Term*> vt(2);
			Element en; en._int = n;
			vt[0] = new DomainTerm(*(StdBuiltin::instance()->sort("int")->begin()),ELINT,en,pi);
			vt[1] = new AggTerm(qse,AGGCARD,pi);
			Predicate* p = StdBuiltin::instance()->pred(string(1,c) + "/2");
			return new PredForm(b,p,vt,pi);	// TODO adapt formparseinfo
		}
		else {
			for(unsigned int n = 0; n < vv.size(); ++n) delete(vv[n]);
			delete(f);
			return 0;
		}
	}

	EqChainForm* eqchain(char c, bool b, Term* lt, Term* rt, YYLTYPE l) {
		if(lt && rt) {
			FormParseInfo pi = formparseinfo(l);	// TODO adapt formparseinfo
			EqChainForm* ecf = new EqChainForm(true,true,lt,pi);
			ecf->add(c,b,rt);
			return ecf;
		}
		else {
			if(lt) delete(lt);
			if(rt) delete(rt);
			return 0;
		}
	}

	EqChainForm* eqchain(char c, bool b, EqChainForm* ecf, Term* rt, YYLTYPE) {
		if(ecf && rt) {
			ecf->add(c,b,rt);
			return ecf;
		}
		else {
			if(ecf) delete(ecf);
			if(rt) delete(rt);
			return 0;
		}
	}

	Rule* rule(const vector<Variable*>& qv, PredForm* head, Formula* body,YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		remove_vars(qv);
		if(head && body) {
			// Quantify the free variables
			vector<Variable*> vv = freevars(head->pi());
			remove_vars(vv);
			// Split quantified variables in head and body variables
			vector<Variable*> hv;
			vector<Variable*> bv;
			for(unsigned int n = 0; n < qv.size(); ++n) {
				if(head->contains(qv[n])) hv.push_back(qv[n]);
				else bv.push_back(qv[n]);
			}
			for(unsigned int n = 0; n < vv.size(); ++n) {
				if(head->contains(vv[n])) hv.push_back(vv[n]);
				else bv.push_back(vv[n]);
			}
			// Create a new rule
			if(!(bv.empty())) body = new QuantForm(true,false,bv,body,FormParseInfo((body->pi())));
			Rule* r = new Rule(hv,head,body,pi);
			// Sort derivation
			SortDeriver sd(r,_currvocab);
			// Return the rule
			return r;
		}
		else {
			curr_vars.clear();
			if(head) delete(head);
			if(body) delete(body);
			for(unsigned int n = 0; n < qv.size(); ++n) delete(qv[n]);
			return 0;
		}
	}

	Rule* rule(const vector<Variable*>& qv, PredForm* head, YYLTYPE l) {
		Formula* body = trueform();
		return rule(qv,head,body,l);
	}

	Rule* rule(PredForm* head, Formula* body, YYLTYPE l) {
		vector<Variable*> vv(0);
		return rule(vv,head,body,l);
	}

	Rule* rule(PredForm* head,YYLTYPE l) {
		Formula* body = trueform();
		return rule(head,body,l);
	}

	Definition* definition(const vector<Rule*>& rules) {
		Definition* d = new Definition();
		for(unsigned int n = 0; n < rules.size(); ++n) {
			if(rules[n]) d->add(rules[n]);
		}
		return d;
	}

	FixpDef* fixpdef(bool lfp, const vector<pair<Rule*,FixpDef*> >& vp) {
		FixpDef* d = new FixpDef(lfp);
		for(unsigned int n = 0; n < vp.size(); ++n) {
			if(vp[n].first) {
				d->add(vp[n].first);
			}
			else if(vp[n].second) {
				d->add(vp[n].second);
			}
		}
		// TODO: check if the fixpoint definition satisfies the restrictions of FO(FD)
		return d;
	}

	Variable* quantifiedvar(const string& name, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		Variable* v = new Variable(name,0,pi);
		curr_vars.push_front(VarName(name,v));
		return v;
	}

	Variable* quantifiedvar(const string& name, Sort* sort, YYLTYPE l) {
		Variable* v = quantifiedvar(name,l);
		if(sort) v->sort(sort);
		return v;
	}

	FuncTerm* functerm(NSTuple* nst, const vector<Term*>& vt) {
		if(nst->_sortsincluded) nst->includeFuncArity();
		else nst->includeArity(vt.size());
		Function* f = funcInScope(nst->_name,nst->_pi);
		FuncTerm* t = 0;
		if(f && nst->_sortsincluded) f = f->resolve(nst->_sorts);
		if(f) {
			if(belongsToVoc(f)) {
				if(f->arity() == vt.size()) {
					unsigned int n = 0;
					for(; n < vt.size(); ++n) { if(!vt[n]) break; }
					if(n == vt.size()) t = new FuncTerm(f,vt,nst->_pi);
				}
				else Error::wrongfuncarity(f->name(),nst->_pi);
			}
			else Error::funcnotintheovoc(f->name(),_currtheory->name(),nst->_pi);
		}
		else Error::undeclfunc(nst->to_string(),nst->_pi);

		// Cleanup
		if(!t) {
			for(unsigned int n = 0; n < vt.size(); ++n) { if(vt[n]) delete(vt[n]);	}
		}
		delete(nst);

		return t;
	}

	Term* functerm(NSTuple* nst) {
		if(nst->_sortsincluded || (nst->_name).size() != 1) {
			vector<Term*> vt = vector<Term*>(0);
			return functerm(nst,vt);
		}
		else {
			Term* t = 0;
			string name = (nst->_name)[0];
			Variable* v = getVar(name);
			nst->includeArity(0);
			Function* f = funcInScope(nst->_name,nst->_pi);
			if(v) {
				if(f) Warning::varcouldbeconst((nst->_name)[0],nst->_pi);
				t = new VarTerm(v,nst->_pi);
			}
			else if(f) {
				vector<Term*> vt(0);
				t = new FuncTerm(f,vt,nst->_pi);
			}
			else {
				YYLTYPE l; 
				l.first_line = (nst->_pi).line();
				l.first_column = (nst->_pi).col();
				v = quantifiedvar((nst->_name)[0],l);
				t = new VarTerm(v,nst->_pi);
			}
			delete(nst);
			return t;
		}
	}

	Term* arterm(char c, Term* lt, Term* rt, YYLTYPE l) {
		if(lt && rt) {
			Function* f = _currvocab->func(string(1,c) + "/2");
			assert(f);
			vector<Term*> vt(2); vt[0] = lt; vt[1] = rt;
			return new FuncTerm(f,vt,parseinfo(l));
		}
		else {
			if(lt) delete(lt);
			if(rt) delete(rt);
			return 0;
		}
	}

	Term* arterm(const string& s, Term* t, YYLTYPE l) {
		if(t) {
			Function* f = _currvocab->func(s + "/1");
			assert(f);
			vector<Term*> vt(1,t);
			return new FuncTerm(f,vt,parseinfo(l));
		}
		else {
			delete(t);
			return 0;
		}
	}

	FTTuple* fttuple(Formula* f, Term* t) {
		if(f && t) return new FTTuple(f,t);
		else {
			if(f) delete(f);
			if(t) delete(t);
			return 0;
		}
	}

	QuantSetExpr* set(const vector<Variable*>& vv, Formula* f, YYLTYPE l) {
		remove_vars(vv);
		if(f) {
			ParseInfo pi = parseinfo(l);
			return new QuantSetExpr(vv,f,pi);
		}
		else {
			for(unsigned int n = 0; n < vv.size(); ++n) delete(vv[n]);
			return 0;
		}
	}

	EnumSetExpr* set(const vector<FTTuple*>& vftt, YYLTYPE l) {
		vector<Formula*> vf;
		vector<Term*> vt;
		for(unsigned int n = 0; n < vftt.size(); ++n) {
			if(vftt[n]->_formula && vftt[n]->_term) {
				vf.push_back(vftt[n]->_formula);
				vt.push_back(vftt[n]->_term);
			}
			else {
				for(unsigned int m = 0; m < vftt.size(); ++m) {
					if(vftt[m]->_formula) delete(vftt[m]->_formula);
					if(vftt[m]->_term) delete(vftt[m]->_term);
				}
				return 0;
			}
		}
		ParseInfo pi = parseinfo(l);
		return new EnumSetExpr(vf,vt,pi);
	}

	EnumSetExpr* set(const vector<Formula*>& vf, YYLTYPE l) {
		vector<Term*> vt;
		for(unsigned int n = 0; n < vf.size(); ++n) {
			if(vf[n]) {
				Element one; one._int = 1;
				vt.push_back(new DomainTerm(*(StdBuiltin::instance()->sort("int")->begin()),ELINT,one,vf[n]->pi()));
			}
			else {
				for(unsigned int m = 0; m < vf.size(); ++m) {
					if(vf[m]) delete(vf[m]);
				}
				return 0;
			}
		}
		ParseInfo pi = parseinfo(l);
		return new EnumSetExpr(vf,vt,pi);
	}

	AggTerm* aggregate(AggType at, SetExpr* s, YYLTYPE l) {
		if(s) {
			ParseInfo pi = parseinfo(l);
			return new AggTerm(s,at,pi);
		}
		else return 0;
	}

	DomainTerm* domterm(int n, YYLTYPE l) {
		Element en; en._int = n;
		return new DomainTerm(*(StdBuiltin::instance()->sort("int")->begin()),ELINT,en,parseinfo(l));
	}

	DomainTerm* domterm(double d, YYLTYPE l) {
		Element ed; ed._double = d;
		return new DomainTerm(*(StdBuiltin::instance()->sort("float")->begin()),ELDOUBLE,ed,parseinfo(l));
	}

	DomainTerm* domterm(string* s, YYLTYPE l) {
		Element es; es._string = s;
		return new DomainTerm(*(StdBuiltin::instance()->sort("string")->begin()),ELSTRING,es,parseinfo(l));
	}

	DomainTerm* domterm(char c, YYLTYPE l) {
		Element es; es._string = new string(1,c);
		return new DomainTerm(*(StdBuiltin::instance()->sort("char")->begin()),ELSTRING,es,parseinfo(l));
	}

	DomainTerm* domterm(string* n, Sort* s, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		if(s) { 
			Element en; en._string = n;
			return new DomainTerm(s,ELSTRING,en,pi);
		}
		else return 0;
	}

	/*****************
		Statements
	*****************/

	bool checkarg(const string& arg, InfArgType t, const ParseInfo& pi) {
		switch(t) {
			case IAT_THEORY:
				if(theoInScope(arg,pi)) return true;
				break;
			case IAT_VOCABULARY:
				if(vocabInScope(arg,pi)) return true;
				break;
			case IAT_STRUCTURE:
				if(structInScope(arg,pi)) return true;
				break;
			case IAT_NAMESPACE:
				if(namespaceInScope(arg,pi)) return true;
				break;
			case IAT_VOID:
				if(arg.size() == 0) return true;
			default: assert(false); 
		}
		return false;
	}

	InfArg convertarg(const string& arg, InfArgType t, const ParseInfo& pi) {
		InfArg a;
		switch(t) {
			case IAT_THEORY:
				a._theory = theoInScope(arg,pi);
				break;
			case IAT_VOCABULARY:
				a._vocabulary = vocabInScope(arg,pi);
				break;
			case IAT_STRUCTURE:
				a._structure = structInScope(arg,pi);
				break;
			case IAT_NAMESPACE:
				a._namespace = namespaceInScope(arg,pi);
				break;
			case IAT_VOID:
				break;
			default:
				assert(false);
		}
		return a;
	}

	void command(InfArgType type, const string& cname, const vector<string>& args, const string& res, YYLTYPE l) {
		ParseInfo pi = parseinfo(l);
		map<string,vector<Inference*> >::iterator it = _inferences.find(cname);
		if(it != _inferences.end()) {
			vector<Inference*> vi;
			for(unsigned int n = 0; n < (it->second).size(); ++n) {
				if(args.size() == (it->second)[n]->arity()) vi.push_back((it->second)[n]);
			}
			if(vi.empty()) {
				Error::unkncommand(cname + '/' + itos(args.size()),pi);
			}
			else {
				vector<Inference*> vi2;
				for(unsigned int n = 0; n < vi.size(); ++n) {
					bool ok = true;
					for(unsigned int m = 0; m < args.size(); ++m) {
						ok = checkarg(args[m],(vi[n]->intypes())[m],pi);
						if(!ok) break;
					}
					if(ok) {
						if(type == vi[n]->outtype()) vi2.push_back(vi[n]);
					}
				}
				if(vi2.empty()) Error::wrongcommandargs(cname + '/' + itos(args.size()),pi);
				else if(vi2.size() == 1) {
					vector<InfArg> via;
					for(unsigned int m = 0; m < args.size(); ++m)
						via.push_back(convertarg(args[m],(vi2[0]->intypes())[m],pi));
					vi2[0]->execute(via,res,_currspace);
				}
				else Error::ambigcommand(cname + '/' + itos(args.size()),pi);
			}
		}
		else {
			Error::unkncommand(cname + '/' + itos(args.size()),pi);
		}
	}
	
	void command(const string& cname, const vector<string>& args, YYLTYPE l) {
		string res;
		command(IAT_VOID,cname,args,res,l);
	}

	void command(InfArgType t, const string& cname, const string& res, YYLTYPE l) {
		vector<string> args;
		command(t,cname,args,res,l);
	}

	void command(const string& cname, YYLTYPE l) {
		vector<string> args;
		string res;
		command(IAT_VOID,cname,args,res,l);
	}

}

void help_execute() {
	cout << "The available methods in the execute block are:\n";
	for(map<string,vector<Inference*> >::iterator it = Insert::_inferences.begin(); it != Insert::_inferences.end(); ++it) {
		for(unsigned int n = 0; n < (it->second).size(); ++n) {
			cout << "   " << IATUtils::to_string(((it->second)[n])->outtype()) << ' ' << it->first << '(';
			for(unsigned int m = 0; m < ((it->second)[n])->arity(); ++m) {
				cout << IATUtils::to_string((((it->second)[n])->intypes())[m]);
				if(m != ((it->second)[n])->arity()-1) cout << ',';
			}
			cout << ")\n";
			cout << "      " << ((it->second)[n])->description() << '\n';
		}
	}
}
