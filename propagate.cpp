/************************************
	propagate.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <typeinfo>
#include <iostream>
#include "fobdd.hpp"
#include "vocabulary.hpp"
#include "term.hpp"
#include "theory.hpp"
#include "structure.hpp"
#include "propagate.hpp"

using namespace std;

/****************************
	Propagation scheduler
****************************/

void FOPropScheduler::add(FOPropagation* propagation) {
	// TODO: don't schedule a propagation that is already on the queue
	_queue.push(propagation);
}

bool FOPropScheduler::hasNext() const {
	return (not _queue.empty());
}

FOPropagation* FOPropScheduler::next() {
	FOPropagation* propagation = _queue.front();
	_queue.pop();
	return propagation;
}

/**************************
	Propagation domains
**************************/

FOPropBDDDomainFactory::FOPropBDDDomainFactory() {
	_manager = new FOBDDManager();
}

ostream& FOPropBDDDomainFactory::put(ostream& output, FOPropDomain* domain) const {
	FOPropBDDDomain* bdddomain = dynamic_cast<FOPropBDDDomain*>(domain);
	_manager->put(output,bdddomain->bdd(),6);
	return output;
}

FOPropBDDDomain* FOPropBDDDomainFactory::trueDomain(const Formula* f) const {
	const FOBDD* bdd = _manager->truebdd();
	vector<Variable*> vv(f->freeVars().begin(),f->freeVars().end());
	return new FOPropBDDDomain(bdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::falseDomain(const Formula* f) const {
	const FOBDD* bdd = _manager->falsebdd();
	vector<Variable*> vv(f->freeVars().begin(),f->freeVars().end());
	return new FOPropBDDDomain(bdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::formuladomain(const Formula* f) const {
	FOBDDFactory bddfactory(_manager);
	vector<Variable*> vv(f->freeVars().begin(),f->freeVars().end());
	return new FOPropBDDDomain(bddfactory.run(f),vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::ctDomain(const PredForm* pf) const {
	vector<const FOBDDArgument*> args;
	FOBDDFactory bddfactory(_manager);
	for(vector<Term*>::const_iterator it = pf->subterms().begin(); it != pf->subterms().end(); ++it) {
		args.push_back(bddfactory.run(*it));
	}
	const FOBDDKernel* k = _manager->getAtomKernel(pf->symbol(),AKT_CT,args);
	const FOBDD* bdd = _manager->getBDD(k,_manager->truebdd(),_manager->falsebdd());
	vector<Variable*> vv(pf->freeVars().begin(),pf->freeVars().end());
	return new FOPropBDDDomain(bdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::cfDomain(const PredForm* pf) const {
	vector<const FOBDDArgument*> args;
	FOBDDFactory bddfactory(_manager);
	for(vector<Term*>::const_iterator it = pf->subterms().begin(); it != pf->subterms().end(); ++it) {
		args.push_back(bddfactory.run(*it));
	}
	const FOBDDKernel* k = _manager->getAtomKernel(pf->symbol(),AKT_CF,args);
	const FOBDD* bdd = _manager->getBDD(k,_manager->truebdd(),_manager->falsebdd());
	vector<Variable*> vv(pf->freeVars().begin(),pf->freeVars().end());
	return new FOPropBDDDomain(bdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::exists(FOPropDomain* domain, const set<Variable*>& qvars) const {
	FOPropBDDDomain* bdddomain = dynamic_cast<FOPropBDDDomain*>(domain);
	set<const FOBDDVariable*> bddqvars = _manager->getVariables(qvars);
	const FOBDD* qbdd = _manager->existsquantify(bddqvars,bdddomain->bdd());
	vector<Variable*> vv;
	for(vector<Variable*>::const_iterator it = domain->vars().begin(); it != domain->vars().end(); ++it) {
		if(qvars.find(*it) == qvars.end()) { vv.push_back(*it); }
	}
	return new FOPropBDDDomain(qbdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::forall(FOPropDomain* domain, const std::set<Variable*>& qvars) const {
	FOPropBDDDomain* bdddomain = dynamic_cast<FOPropBDDDomain*>(domain);
	set<const FOBDDVariable*> bddqvars = _manager->getVariables(qvars);
	const FOBDD* qbdd = _manager->univquantify(bddqvars,bdddomain->bdd());
	vector<Variable*> vv;
	for(vector<Variable*>::const_iterator it = domain->vars().begin(); it != domain->vars().end(); ++it) {
		if(qvars.find(*it) == qvars.end()) { vv.push_back(*it); }
	}
	return new FOPropBDDDomain(qbdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::conjunction(FOPropDomain* dom1,FOPropDomain* dom2) const {
	FOPropBDDDomain* bdddomain1 = dynamic_cast<FOPropBDDDomain*>(dom1);
	FOPropBDDDomain* bdddomain2 = dynamic_cast<FOPropBDDDomain*>(dom2);
	const FOBDD* conjbdd = _manager->conjunction(bdddomain1->bdd(),bdddomain2->bdd());
	set<Variable*> sv; 
	sv.insert(dom1->vars().begin(),dom1->vars().end());
	sv.insert(dom2->vars().begin(),dom2->vars().end());
	vector<Variable*> vv(sv.begin(),sv.end());
	return new FOPropBDDDomain(conjbdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::disjunction(FOPropDomain* dom1,FOPropDomain* dom2) const {
	//assert(dom1!=NULL && safetypeid<FOPropBDDDomain>(*dom1) && dom2!=NULL && safetypeid<FOPropBDDDomain>(*dom2));
	FOPropBDDDomain* bdddomain1 = dynamic_cast<FOPropBDDDomain*>(dom1);
	FOPropBDDDomain* bdddomain2 = dynamic_cast<FOPropBDDDomain*>(dom2);
	const FOBDD* disjbdd = _manager->disjunction(bdddomain1->bdd(),bdddomain2->bdd());
	set<Variable*> sv; 
	sv.insert(dom1->vars().begin(),dom1->vars().end());
	sv.insert(dom2->vars().begin(),dom2->vars().end());
	vector<Variable*> vv(sv.begin(),sv.end());
	return new FOPropBDDDomain(disjbdd,vv);
}

FOPropBDDDomain* FOPropBDDDomainFactory::substitute(FOPropDomain* domain,const map<Variable*,Variable*>& mvv) const {
	FOPropBDDDomain* bdddomain = dynamic_cast<FOPropBDDDomain*>(domain);
	map<const FOBDDVariable*,const FOBDDVariable*> newmvv;
	for(map<Variable*,Variable*>::const_iterator it = mvv.begin(); it != mvv.end(); ++it) {
		const FOBDDVariable* oldvar = _manager->getVariable(it->first);
		const FOBDDVariable* newvar = _manager->getVariable(it->second);
		newmvv[oldvar] = newvar;
	}
	const FOBDD* substbdd = _manager->substitute(bdddomain->bdd(),newmvv);
	vector<Variable*> vv = domain->vars();
	for(size_t n = 0; n < vv.size(); ++n) {
		map<Variable*,Variable*>::const_iterator it = mvv.find(vv[n]);
		if(it != mvv.end()) { vv[n] = it->second; }
	}
	return new FOPropBDDDomain(substbdd,vv);
}

bool FOPropBDDDomainFactory::approxequals(FOPropDomain* dom1, FOPropDomain* dom2) const {
	FOPropBDDDomain* bdddomain1 = dynamic_cast<FOPropBDDDomain*>(dom1);
	FOPropBDDDomain* bdddomain2 = dynamic_cast<FOPropBDDDomain*>(dom2);
	return bdddomain1->bdd() == bdddomain2->bdd();
}

PredInter* FOPropBDDDomainFactory::inter(const vector<Variable*>& vars, const ThreeValuedDomain& dom, AbstractStructure* str) const {
	// Construct the universe of the interpretation and two sets of new variables
	vector<SortTable*> vst;
	vector<Variable*> newctvars;
	vector<Variable*> newcfvars;
	map<const FOBDDVariable*,const FOBDDVariable*> ctmvv;
	map<const FOBDDVariable*,const FOBDDVariable*> cfmvv;
	bool twovalued = dom._twovalued;	
	for(vector<Variable*>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
		const FOBDDVariable* oldvar = _manager->getVariable(*it);
		vst.push_back(str->inter((*it)->sort()));
		Variable* ctv = new Variable((*it)->sort());
		newctvars.push_back(ctv);
		const FOBDDVariable* newctvar = _manager->getVariable(ctv);
		ctmvv[oldvar] = newctvar;
		if(not twovalued) {
			Variable* cfv = new Variable((*it)->sort());
			newcfvars.push_back(cfv);
			const FOBDDVariable* newcfvar = _manager->getVariable(cfv);
			cfmvv[oldvar] = newcfvar;
		}
	}
	Universe univ(vst);

	// Construct the ct-table and cf-table
	FOPropBDDDomain* ctdom = dynamic_cast<FOPropBDDDomain*>(dom._ctdomain);
	const FOBDD* newctbdd = _manager->substitute(ctdom->bdd(),ctmvv);
	PredTable* ct = new PredTable(new BDDInternalPredTable(newctbdd,_manager,newctvars,str),univ);
	if(twovalued) { return new PredInter(ct,true); }
	else {
		FOPropBDDDomain* cfdom = dynamic_cast<FOPropBDDDomain*>(dom._cfdomain);
		const FOBDD* newcfbdd = _manager->substitute(cfdom->bdd(),cfmvv);
		PredTable* cf = new PredTable(new BDDInternalPredTable(newcfbdd,_manager,newcfvars,str),univ);
		return new PredInter(ct,cf,true,true);
	}
}

FOPropTableDomain* FOPropTableDomain::clone() const {
	return new FOPropTableDomain(new PredTable(_table->internTable(),_table->universe()),_vars);
}

FOPropTableDomain* FOPropTableDomainFactory::exists(FOPropDomain* dom, const set<Variable*>& sv) const {
	FOPropTableDomain* tabledomain = dynamic_cast<FOPropTableDomain*>(dom);
	vector<bool> keepcol;
	vector<Variable*> newvars;
	vector<SortTable*> newunivcols;
	for(unsigned int n = 0; n < tabledomain->vars().size(); ++n) {
		Variable* v= tabledomain->vars()[n];
		if(sv.find(v) == sv.end()) {
			keepcol.push_back(true);
			newvars.push_back(v);
			newunivcols.push_back(tabledomain->table()->universe().tables()[n]);
		}
		else { keepcol.push_back(false); }
	}

	if(not tabledomain->table()->approxFinite()) { 
		cerr << "Probably entering an infinte loop when trying to project a possibly infinite table...\n";
	}
	PredTable* npt = new PredTable(new EnumeratedInternalPredTable(),Universe(newunivcols));
	for(TableIterator it = tabledomain->table()->begin(); it.hasNext(); ++it) {
		const ElementTuple& tuple = *it;
		ElementTuple newtuple;
		for(size_t n = 0; n < tuple.size(); ++n) {
			if(keepcol[n]) { newtuple.push_back(tuple[n]); }
		}
		npt->add(newtuple);
	}

	return new FOPropTableDomain(npt,newvars);
}

/*****************
	Propagator
*****************/

ThreeValuedDomain::ThreeValuedDomain(const FOPropDomainFactory* factory, bool ctdom, bool cfdom, const Formula* f) {
	_ctdomain = ctdom ? factory->trueDomain(f) : factory->falseDomain(f);
	_cfdomain = cfdom ? factory->trueDomain(f) : factory->falseDomain(f);
	_twovalued = ctdom || cfdom;
	assert(_ctdomain);
	assert(_cfdomain);
}

ThreeValuedDomain::ThreeValuedDomain(const FOPropDomainFactory* factory, const Formula* f) {
	_ctdomain = factory->formuladomain(f);
	Formula* negf = f->clone(); negf->negate();
	_cfdomain = factory->formuladomain(negf);
	_twovalued = true;
	assert(_ctdomain);
	assert(_cfdomain);
}

ThreeValuedDomain::ThreeValuedDomain(const FOPropDomainFactory* factory, const PredForm* pf, InitBoundType ibt) {
	_twovalued = false;
	switch(ibt) {
		case IBT_CT:
			_ctdomain = factory->ctDomain(pf);
			_cfdomain = factory->falseDomain(pf);
			break;
		case IBT_CF:
			_ctdomain = factory->falseDomain(pf);
			_cfdomain = factory->cfDomain(pf);
			break;
		case IBT_BOTH:
			_ctdomain = factory->ctDomain(pf);
			_cfdomain = factory->cfDomain(pf);
			break;
		default:
			assert(false);
	}
}

FOPropagator::FOPropagator(FOPropDomainFactory* f, FOPropScheduler* s, Options* opts)
		: _verbosity(opts->getValue(IntType::PROPAGATEVERBOSITY)), _factory(f), _scheduler(s) {
	_maxsteps = opts->getValue(IntType::NRPROPSTEPS);
	_options = opts;
	if(typeid(*f) == typeid(FOPropBDDDomainFactory)) {
		FOPropBDDDomainFactory* bddf = dynamic_cast<FOPropBDDDomainFactory*>(f);
		if(_options->getValue(IntType::LONGESTBRANCH)!=0) {
			_admissiblecheckers.push_back(new LongestBranchChecker(bddf->manager(),_options->getValue(IntType::LONGESTBRANCH)));
		}
	}
} 

void FOPropagator::run() {
	if(_verbosity > 1) { cerr << "=== Start propagation ===\n"; }
	while(_scheduler->hasNext()) {
		FOPropagation* propagation = _scheduler->next();
		_direction = propagation->_direction;
		_ct = propagation->_ct;
		_child = propagation->_child;
		if(_verbosity > 1) { 
			const Formula* p = propagation->_parent;
			cerr << "  Propagate ";
			if(_direction == DOWN) {
				cerr << "downward from " << (_ct ? "the ct-bound of " : "the cf-bound of "); 
				p->put(cerr,_options->getValue(BoolType::LONGNAMES));
				if(_child) { cerr << " to "; _child->put(cerr,_options->getValue(BoolType::LONGNAMES));	}
			}
			else {
				cerr << "upward to " << ((_ct == isPos(p->sign())) ? "the ct-bound of " : "the cf-bound of ");
				p->put(cerr,_options->getValue(BoolType::LONGNAMES));
				if(_child) { cerr << " from "; _child->put(cerr,_options->getValue(BoolType::LONGNAMES));	}
			}
			cerr << "\n";
		}
		propagation->_parent->accept(this);
		delete(propagation);
	}
	if(_verbosity > 1) { cerr << "=== End propagation ===\n"; }
}

AbstractStructure* FOPropagator::currstructure(AbstractStructure* structure) const {
	Vocabulary* vocabulary = new Vocabulary("");
	for(map<const PredForm*,set<const PredForm*> >::const_iterator it = _leafupward.begin(); it != _leafupward.end(); ++it) {
		PFSymbol* symbol = it->first->symbol();
		if(typeid(*symbol) == typeid(Predicate)) { vocabulary->addPred(dynamic_cast<Predicate*>(symbol)); }
		else { vocabulary->addFunc(dynamic_cast<Function*>(symbol)); }
	}
	AbstractStructure* res = structure->clone();
	res->vocabulary(vocabulary);

	for(map<const PredForm*,set<const PredForm*> >::const_iterator it = _leafupward.begin(); it != _leafupward.end(); ++it) {
		const PredForm* connector = it->first;
		PFSymbol* symbol = connector->symbol();
		vector<Variable*> vv;
		for(vector<Term*>::const_iterator jt = connector->subterms().begin(); jt != connector->subterms().end(); ++jt) {
			vv.push_back(*((*jt)->freeVars().begin()));
		}
		PredInter* pinter = _factory->inter(vv,_domains.find(connector)->second,structure);
		if(typeid(*symbol) == typeid(Predicate)) { res->inter(dynamic_cast<Predicate*>(symbol),pinter); }
		else {
			FuncInter* finter = new FuncInter(pinter);
			res->inter(dynamic_cast<Function*>(symbol),finter);
		}
	}
	return res;
}

SymbolicStructure* FOPropagator::symbolicstructure() const {
	map<PFSymbol*,vector<const FOBDDVariable*> > vars;
	map<PFSymbol*,const FOBDD*> ctbounds;
	map<PFSymbol*,const FOBDD*> cfbounds;
	assert(typeid(*_factory) == typeid(FOPropBDDDomainFactory));
	FOBDDManager* manager = (dynamic_cast<FOPropBDDDomainFactory*>(_factory))->manager();
	for(auto it = _leafconnectors.begin(); it != _leafconnectors.end(); ++it) {
		ThreeValuedDomain tvd = (_domains.find(it->second))->second;
		ctbounds[it->first] = dynamic_cast<FOPropBDDDomain*>(tvd._ctdomain)->bdd();
		cfbounds[it->first] = dynamic_cast<FOPropBDDDomain*>(tvd._cfdomain)->bdd();
		vector<const FOBDDVariable*> bddvars;
		for(auto jt = it->second->subterms().begin(); jt != it->second->subterms().end(); ++jt) {
			assert(typeid(*(*jt)) == typeid(VarTerm));
			bddvars.push_back(manager->getVariable((dynamic_cast<VarTerm*>(*jt))->var()));
		}
		vars[it->first] = bddvars;
	}
	return new SymbolicStructure(manager,ctbounds,cfbounds,vars);
}

FuncInter* FOPropagator::interpretation(Function* ) const {
	assert(false);
	// TODO
	return 0;
}

PredInter* FOPropagator::interpretation(Predicate* ) const {
	assert(false);
	// TODO
	return 0;
}

FOPropDomain* FOPropagator::addToConjunction(FOPropDomain* conjunction, FOPropDomain* newconjunct) {
	FOPropDomain* temp = conjunction;
	conjunction = _factory->conjunction(conjunction,newconjunct);
	delete(temp);
	return conjunction;
}

FOPropDomain* FOPropagator::addToDisjunction(FOPropDomain* disjunction, FOPropDomain* newdisjunct) {
	FOPropDomain* temp = disjunction;
	disjunction = _factory->disjunction(disjunction,newdisjunct);
	delete(temp);
	return disjunction;
}

FOPropDomain* FOPropagator::addToExists(FOPropDomain* exists, const set<Variable*>& qvars) {
	set<Variable*> newqvars;
	map<Variable*,Variable*> mvv;
	for(set<Variable*>::const_iterator it = qvars.begin(); it != qvars.end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),(*it)->pi());
		newqvars.insert(v);
		mvv[*it] = v;
	}
	FOPropDomain* cl = _factory->substitute(exists,mvv);
	delete(exists);
	FOPropDomain* q = _factory->exists(cl,newqvars);
	delete(cl);
	return q;
}

FOPropDomain* FOPropagator::addToForall(FOPropDomain* forall, const set<Variable*>& qvars) {
	set<Variable*> newqvars;
	map<Variable*,Variable*> mvv;
	for(set<Variable*>::const_iterator it = qvars.begin(); it != qvars.end(); ++it) {
		Variable* v = new Variable((*it)->name(),(*it)->sort(),(*it)->pi());
		newqvars.insert(v);
		mvv[*it] = v;
	}
	FOPropDomain* cl = _factory->substitute(forall,mvv);
	delete(forall);
	FOPropDomain* q = _factory->forall(cl,newqvars);
	delete(cl);
	return q;
}

void FOPropagator::schedule(const Formula* p, FOPropDirection dir, bool ct, const Formula* c) {
	if(_maxsteps > 0) {
		--_maxsteps;
		_scheduler->add(new FOPropagation(p,dir,ct,c));
		if(_verbosity > 1) {
			cerr << "  Schedule ";
			if(dir == DOWN) {
				cerr << "downward propagation from " << (ct ? "the ct-bound of " : "the cf-bound of ") << *p;
				if(c) { cerr << " towards " << *c; }
			}
			else {
				cerr << "upward propagation to " << ((ct == isPos(p->sign())) ? "the ct-bound of " : "the cf-bound of ") << *p;
				if(c){
					cerr << ". Propagation comes from " << *c;
				}
			}
			cerr << "\n";
		}
	}
}

void FOPropagator::updateDomain(const Formula* f, FOPropDirection dir, bool ct, FOPropDomain* newdomain, const Formula* child) {
	if(_verbosity > 2) {
		cerr << "    Derived the following " << (ct ? "ct " : "cf ") << "domain for " << *f << ":\n";
		_factory->put(cerr,newdomain);
	}

	FOPropDomain* olddom = ct ? _domains[f]._ctdomain : _domains[f]._cfdomain;
	FOPropDomain* newdom = _factory->disjunction(olddom,newdomain);

	if((not _factory->approxequals(olddom,newdom)) && admissible(newdom,olddom)) {
		ct ? _domains[f]._ctdomain = newdom : _domains[f]._cfdomain = newdom;
		if(dir == DOWN) {
			for(vector<Formula*>::const_iterator it = f->subformulas().begin(); it != f->subformulas().end(); ++it) {
				schedule(f,DOWN,ct,*it);
			}
			if(typeid(*f) == typeid(PredForm)) {
				const PredForm* pf = dynamic_cast<const PredForm*>(f);
				map<const PredForm*,set<const PredForm*> >::const_iterator it = _leafupward.find(pf);
				if(it != _leafupward.end()) {
					for(set<const PredForm*>::const_iterator jt = it->second.begin(); jt != it->second.end(); ++jt) {
						if(*jt != child) { schedule(*jt,UP,ct,f); }
					}
				}
				else {
					assert(_leafconnectdata.find(pf) != _leafconnectdata.end());
					schedule(pf,DOWN,ct,0);
				}
			}
		}
		else {
			assert(dir == UP);
			map<const Formula*,const Formula*>::const_iterator it = _upward.find(f);
			if(it != _upward.end()) { schedule(it->second,UP,ct,f); }
		}
	}
	else { delete(newdom); }

	if(child && dir == UP) {
		for(vector<Formula*>::const_iterator it = f->subformulas().begin(); it != f->subformulas().end(); ++it) {
			if(*it != child) { schedule(f,DOWN,!ct,*it); }
		}
	}

	delete(newdomain);
}

bool FOPropagator::admissible(FOPropDomain* newd, FOPropDomain* oldd) const {
	for(vector<AdmissibleBoundChecker*>::const_iterator it = _admissiblecheckers.begin(); it != _admissiblecheckers.end(); ++it) {
		if(!((*it)->check(newd,oldd))) return false;
	}
	return true;
}

void FOPropagator::visit(const PredForm* pf) {
	assert(_leafconnectdata.find(pf) != _leafconnectdata.end());
	LeafConnectData lcd = _leafconnectdata[pf];
	PredForm* connector = lcd._connector;
	FOPropDomain* deriveddomain;
	FOPropDomain* temp;
	if(_direction == DOWN) {
		deriveddomain = _ct ? _domains[pf]._ctdomain->clone() : _domains[pf]._cfdomain->clone();
		deriveddomain = _factory->conjunction(deriveddomain,lcd._equalities);
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,lcd._leaftoconnector);
		delete(temp);
		/*set<Variable*> freevars;
		map<Variable*,Variable*> mvv;
		for(set<Variable*>::const_iterator it = pf->freeVars().begin(); it != pf->freeVars().end(); ++it) {
			Variable* clone = new Variable((*it)->sort());
			freevars.insert(clone);
			mvv[(*it)] = clone;
		}
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,mvv);
		delete(temp);
		deriveddomain = addToExists(deriveddomain,freevars);*/
		updateDomain(connector,DOWN,(_ct == isPos(pf->sign())),deriveddomain,pf);
	}
	else {
		assert(_direction == UP);
		assert(_domains.find(connector) != _domains.end());
		deriveddomain = _ct ? _domains[connector]._ctdomain->clone() : _domains[connector]._cfdomain->clone();
		// deriveddomain = _factory->conjunction(deriveddomain,lcd->_equalities);
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,lcd._connectortoleaf);
		delete(temp);
		/*set<Variable*> freevars;
		map<Variable*,Variable*> mvv;
		for(set<Variable*>::const_iterator it = connector->freeVars().begin(); it != connector->freeVars().end(); ++it) {
			Variable* clone = new Variable((*it)->sort());
			freevars.insert(clone);
			mvv[(*it)] = clone;
		}
		temp = deriveddomain;
		deriveddomain = _factory->substitute(deriveddomain,mvv);
		delete(temp);
		deriveddomain = addToExists(deriveddomain,freevars);*/
		updateDomain(pf,UP,(_ct == isPos(pf->sign())),deriveddomain);
	}
}

void FOPropagator::visit(const EqChainForm*) {
	assert(false);
}

void FOPropagator::visit(const EquivForm* ef) {
	Formula* otherchild = (_child == ef->left() ? ef->right() : ef->left());
	if(_direction == DOWN) {
		const ThreeValuedDomain& tvd = _domains[otherchild];
		FOPropDomain* parentdomain = _ct ? _domains[ef]._ctdomain : _domains[ef]._cfdomain;
		FOPropDomain* domain1 = _factory->conjunction(parentdomain,tvd._ctdomain);
		FOPropDomain* domain2 = _factory->conjunction(parentdomain,tvd._cfdomain);
		const set<Variable*>& qvars = _quantvars[_child];
		domain1 = addToExists(domain1,qvars);
		domain2 = addToExists(domain2,qvars);
		updateDomain(_child,DOWN,(_ct == isPos(ef->sign())),domain1);
		updateDomain(_child,DOWN,(_ct != isPos(ef->sign())),domain2);
	}
	else {
		assert(_direction == UP);
		const ThreeValuedDomain& tvd = _domains[otherchild];
		FOPropDomain* childdomain = _ct ? _domains[_child]._ctdomain : _domains[_child]._cfdomain;
		FOPropDomain* domain1 = _factory->conjunction(childdomain,tvd._ctdomain);
		FOPropDomain* domain2 = _factory->conjunction(childdomain,tvd._cfdomain);
		updateDomain(ef,UP,(_ct == isPos(ef->sign())),domain1,_child);
		updateDomain(ef,UP,(_ct != isPos(ef->sign())),domain2,_child);
	}
}

void FOPropagator::visit(const BoolForm* bf) {
	if(_direction == DOWN) {
		FOPropDomain* bfdomain = _ct ? _domains[bf]._ctdomain : _domains[bf]._cfdomain;
		vector<FOPropDomain*> subdomains;
		bool longnewdomain = (_ct != (bf->conj() == isPos(bf->sign())));
		if(longnewdomain) {
			for(size_t n = 0; n < bf->subformulas().size(); ++n) {
				const ThreeValuedDomain& subfdomain = _domains[bf->subformulas()[n]];
				subdomains.push_back(_ct == isPos(bf->sign()) ? subfdomain._cfdomain : subfdomain._ctdomain);
			}
		}
		for(size_t n = 0; n < bf->subformulas().size(); ++n) {
			Formula* subform = bf->subformulas()[n];
			if(not _child || subform == _child) {
				const ThreeValuedDomain& subfdomain = _domains[subform];
				if(not subfdomain._twovalued) {
					FOPropDomain* deriveddomain;
					const set<Variable*>& qvars = _quantvars[subform];
					if(longnewdomain) {
						deriveddomain = bfdomain->clone();
						for(size_t m = 0; m < subdomains.size(); ++m) {
							if(n != m) { deriveddomain = addToConjunction(deriveddomain,subdomains[m]); }
						}
						deriveddomain = addToExists(deriveddomain,qvars);
					}else{
						deriveddomain = _factory->exists(bfdomain,qvars);
					}
					updateDomain(subform,DOWN,(_ct == isPos(bf->sign())),deriveddomain);
				}
			}
		}
	}
	else {
		assert(_direction == UP);
		FOPropDomain* deriveddomain;
		if(_ct == bf->conj()) {
			deriveddomain = _factory->trueDomain(bf);
			for(size_t n = 0; n < bf->subformulas().size(); ++n) {
				const ThreeValuedDomain& tvd = _domains[bf->subformulas()[n]];
				deriveddomain = addToConjunction(deriveddomain,(_ct ? tvd._ctdomain : tvd._cfdomain));
			}
		}
		else {
			const ThreeValuedDomain& tvd = _domains[_child];
			deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		}
		updateDomain(bf,UP,(_ct == isPos(bf->sign())),deriveddomain,_child);
	}
}

void FOPropagator::visit(const QuantForm* qf) {
	if(_direction == DOWN) {
		assert(_domains.find(qf) != _domains.end());
		const ThreeValuedDomain& tvd = _domains[qf];
		FOPropDomain* deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		if(_ct != qf->isUnivWithSign()) {
			set<Variable*> newvars;
			map<Variable*,Variable*> mvv;
			FOPropDomain* conjdomain = _factory->trueDomain(qf->subformula());
			vector<CompType> comps(1,CompType::EQ);
			for(auto it = qf->quantVars().begin(); it != qf->quantVars().end(); ++it) {
				Variable* newvar = new Variable((*it)->sort());
				newvars.insert(newvar);
				mvv[*it] = newvar;
				vector<Term*> terms(2); 
				terms[0] = new VarTerm((*it),TermParseInfo()); 
				terms[1] = new VarTerm(newvar,TermParseInfo());
				EqChainForm* ef = new EqChainForm(SIGN::POS,true,terms,comps,FormulaParseInfo());
				FOPropDomain* equaldomain = _factory->formuladomain(ef); ef->recursiveDelete();
				conjdomain = addToConjunction(conjdomain,equaldomain); delete(equaldomain);
			}
			FOPropDomain* substdomain = _factory->substitute((_ct ? tvd._cfdomain : tvd._ctdomain),mvv);
			FOPropDomain* disjdomain = addToDisjunction(conjdomain,substdomain); delete(substdomain);
			FOPropDomain* univdomain = addToForall(disjdomain,newvars);
			deriveddomain = addToConjunction(deriveddomain,univdomain); delete(univdomain);
		}
		updateDomain(qf->subformula(),DOWN,(_ct == isPos(qf->sign())),deriveddomain);
	}
	else {
		assert(_direction == UP);
		const ThreeValuedDomain& tvd = _domains[qf->subformula()];
		FOPropDomain* deriveddomain = _ct ? tvd._ctdomain->clone() : tvd._cfdomain->clone();
		if(_ct == qf->isUniv()){
			deriveddomain = addToForall(deriveddomain,qf->quantVars());
		}else{
			deriveddomain = addToExists(deriveddomain,qf->quantVars());
		}
		updateDomain(qf,UP,(_ct == isPos(qf->sign())),deriveddomain);
	}
}

void FOPropagator::visit(const AggForm*) {
	// TODO
}

bool LongestBranchChecker::check(FOPropDomain* newd, FOPropDomain*) const {
	return (_treshhold > _manager->longestbranch(dynamic_cast<FOPropBDDDomain*>(newd)->bdd()));
}
