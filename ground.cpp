/************************************
	ground.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "ground.hpp"
#include "ecnf.hpp"
#include <typeinfo>
#include <iostream>

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

int NaiveTranslator::translate(PFSymbol* s, const vector<TypedElement>& args) {
	map<PFSymbol*,map<vector<TypedElement>,int> >::iterator it = _table.find(s);
	if(it != _table.end()) {
		map<vector<TypedElement>,int>::iterator jt = (it->second).find(args);
		if(jt != (it->second).end()) return jt->second;
	}
	_table[s][args] = _nextnumber;
	_backsymbtable.push_back(s);
	_backargstable.push_back(args);
	return _nextnumber++;
}

int GroundTranslator::translate(PFSymbol* s,const vector<TypedElement*>& args) {
	vector<TypedElement> vte(args.size());
	for(unsigned int n = 0; n < args.size(); ++n) {
		vte[n] = *(args[n]);
	}
	return translate(s,vte);
}

int NaiveTranslator::nextTseitin() {
	_backsymbtable.push_back(0);
	_backargstable.push_back(vector<TypedElement>(0));
	return _nextnumber++;
}

/********************************************************
	Basic top-down, non-optimized grounding algorithm
********************************************************/

void NaiveGrounder::visit(VarTerm* vt) {
	assert(_varmapping.find(vt->var()) != _varmapping.end());
	TypedElement te = _varmapping[vt->var()];
	Element e = te._element;
	_returnTerm = new DomainTerm(vt->sort(),te._type,e,ParseInfo());
}

void NaiveGrounder::visit(DomainTerm* dt) {
	_returnTerm = dt->clone();
}

void NaiveGrounder::visit(FuncTerm* ft) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < ft->nrSubterms(); ++n) {
		_returnTerm = 0;
		ft->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnTerm = new FuncTerm(ft->func(),vt,ParseInfo());
}

void NaiveGrounder::visit(AggTerm* at) {
	_returnSet = 0;
	at->set()->accept(this);
	assert(_returnSet);
	_returnTerm = new AggTerm(_returnSet,at->type(),ParseInfo());
}

void NaiveGrounder::visit(EnumSetExpr* s) {
	vector<Formula*> vf;
	for(unsigned int n = 0; n < s->nrSubforms(); ++n) {
		_returnFormula = 0;
		s->subform(n)->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
	}
	vector<Term*> vt;
	for(unsigned int n = 0; n < s->nrSubterms(); ++n) {
		_returnTerm = 0;
		s->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnSet = new EnumSetExpr(vf,vt,ParseInfo());
}

void NaiveGrounder::visit(QuantSetExpr* s) {
	SortTableTupleIterator stti(s->qvars(),_structure);
	vector<SortTable*> tables;
	vector<Formula*> vf(0);
	vector<Term*> vt(0);
	for(unsigned int n = 0; n < s->nrQvars(); ++n) {
		TypedElement e; 
		e._type = stti.type(n); 
		_varmapping[s->qvar(n)] = e;
	}
	if(stti.empty()) {
		_returnSet = new EnumSetExpr(vf,vt,ParseInfo());
	}
	else {
		ElementType weighttype = tables[0]->type();
		do {
			Element weight = stti.value(0);
			for(unsigned int n = 0; n < s->nrQvars(); ++n) {
				_varmapping[s->qvar(n)]._element = stti.value(n);
			}
			_returnFormula = 0;
			s->subf()->accept(this);
			assert(_returnFormula);
			vf.push_back(_returnFormula);
			vt.push_back(new DomainTerm(s->qvar(0)->sort(),weighttype,weight,ParseInfo()));
		} while(stti.nextvalue());
		_returnSet = new EnumSetExpr(vf,vt,ParseInfo());
	}
}

void NaiveGrounder::visit(PredForm* pf) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		_returnTerm = 0;
		pf->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new PredForm(pf->sign(),pf->symb(),vt,FormParseInfo());
}

void NaiveGrounder::visit(EquivForm* ef) {
	_returnFormula = 0;
	ef->left()->accept(this);
	assert(_returnFormula);
	Formula* nl = _returnFormula;
	_returnFormula = 0;
	ef->right()->accept(this);
	assert(_returnFormula);
	Formula* nr = _returnFormula;
	_returnFormula = new EquivForm(ef->sign(),nl,nr,FormParseInfo());
}

void NaiveGrounder::visit(EqChainForm* ef) {
	vector<Term*> vt;
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) {
		_returnTerm = 0;
		ef->subterm(n)->accept(this);
		assert(_returnTerm);
		vt.push_back(_returnTerm);
	}
	_returnFormula = new EqChainForm(ef->sign(),ef->conj(),vt,ef->comps(),ef->compsigns(),FormParseInfo());
}

void NaiveGrounder::visit(BoolForm* bf) {
	vector<Formula*> vf;
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		_returnFormula = 0;
		bf->subform(n)->accept(this);
		assert(_returnFormula);
		vf.push_back(_returnFormula);
	}
	_returnFormula = new BoolForm(bf->sign(),bf->conj(),vf,FormParseInfo());
}

void NaiveGrounder::visit(QuantForm* qf) {
	SortTableTupleIterator stti(qf->qvars(),_structure);
	vector<Formula*> vf(0);
	for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
		TypedElement e; 
		e._type = stti.type(n); 
		_varmapping[qf->qvar(n)] = e;
	}
	if(stti.empty()) {
		_returnFormula = new BoolForm(qf->sign(),qf->univ(),vf,FormParseInfo());
	}
	else {
		do {
			for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
				_varmapping[qf->qvar(n)]._element = stti.value(n);
			}
			_returnFormula = 0;
			qf->subf()->accept(this);
			assert(_returnFormula);
			vf.push_back(_returnFormula);
		} while(stti.nextvalue());
		_returnFormula = new BoolForm(qf->sign(),qf->univ(),vf,FormParseInfo());
	}
}

void NaiveGrounder::visit(Rule* r) {
	SortTableTupleIterator stti(r->qvars(),_structure);
	Definition* d = new Definition();
	for(unsigned int n = 0; n < r->nrQvars(); ++n) {
		TypedElement e; 
		e._type = stti.type(n); 
		_varmapping[r->qvar(n)] = e;
	}
	if(!stti.empty()) {
		do {
			for(unsigned int n = 0; n < r->nrQvars(); ++n) {
				_varmapping[r->qvar(n)]._element = stti.value(n);
			}
			Formula* nb;
			PredForm* nh;

			_returnFormula = 0;
			r->head()->accept(this);
			assert(_returnFormula);
			assert(typeid(*_returnFormula) == typeid(PredForm));
			nh = dynamic_cast<PredForm*>(_returnFormula);

			_returnFormula = 0;
			r->body()->accept(this);
			assert(_returnFormula);
			nb = _returnFormula;

			d->add(new Rule(vector<Variable*>(0),nh,nb,ParseInfo()));

		} while(stti.nextvalue());
	}
	_returnDef = d;
}

void NaiveGrounder::visit(Definition* d) {
	Definition* grounddef = new Definition();
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		_returnDef = 0;
		visit(d->rule(n));
		assert(_returnDef);
		for(unsigned int m = 0; m < _returnDef->nrRules(); ++m) {
			grounddef->add(_returnDef->rule(m));
		}
		delete(_returnDef);
	}
	_returnDef = grounddef;
}

void NaiveGrounder::visit(FixpDef* d) {
	FixpDef* grounddef = new FixpDef(d->lfp());
	for(unsigned int n = 0; n < d->nrDefs(); ++n) {
		_returnFixpDef = 0;
		visit(d->def(n));
		assert(_returnFixpDef);
		grounddef->add(_returnFixpDef);
	}
	for(unsigned int n = 0; n < d->nrRules(); ++n) {
		_returnDef = 0;
		visit(d->rule(n));
		assert(_returnDef);
		for(unsigned int m = 0; m < _returnDef->nrRules(); ++m) {
			grounddef->add(_returnDef->rule(m));
		}
		delete(_returnDef);
	}
	_returnFixpDef = grounddef;
}

void NaiveGrounder::visit(Theory* t) {
	Theory* grounding = new Theory("",t->vocabulary(),ParseInfo());

	// Ground the theory
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_returnDef = 0;
		t->definition(n)->accept(this);
		assert(_returnDef);
		grounding->add(_returnDef);
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_returnFixpDef = 0;
		t->fixpdef(n)->accept(this);
		assert(_returnFixpDef);
		grounding->add(_returnFixpDef);
	}
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_returnFormula = 0;
		t->sentence(n)->accept(this);
		assert(_returnFormula);
		grounding->add(_returnFormula);
	}
	
	// Add the structure
	AbstractTheory* structtheo = StructUtils::convert_to_theory(_structure);
	grounding->add(structtheo);
	delete(structtheo);

	_returnTheory = grounding;
}

/*************************************
	Optimized grounding algorithm
*************************************/

int Grounder::_true = numeric_limits<int>::max();
int Grounder::_false = 0;


int TheoryGrounder::run() const {
	for(unsigned int n = 0; n < _children.size(); ++n) {
		int result = _children[n]->run();
		if(result == _false) {
			_grounding->addEmptyClause();
			return _false;
		}
	}
	return _true;
}

int AtomGrounder::run() const {
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) 
		_subtermgrounders[n]->run();
	
	// Checking partial functions
	for(unsigned int n = 0; n < _args.size(); ++n) {
		//TODO: only check positions that can be out of bounds!
		//TODO: produce a warning!
		if(!ElementUtil::exists(*(_args[n]))) {
			return _poscontext ? _true : _false;
		}
	}

	// Checking out-of-bounds
	for(unsigned int n = 0; n < _args.size(); ++n) {
		if(!_tables[n]->contains(*(_args[n]))) 
			return _sign ? _false : _true;
	}

	if(!(_pchecker->run())) return _certainvalue ? _false : _true;	// TODO: dit is lelijk
	if(_cchecker->run()) return _certainvalue;
	else {
		int atom = _grounding->translator()->translate(_symbol,_args);
		if(!_sign) atom = -atom;
		if(_sentence) _grounding->addUnitClause(atom);
		return atom;
	}
}

/*void VarTermGrounder::run() const {
	if(_table->contains(*_arg)) {
		_result->_type = _arg->_type;
		_result->_element = _arg->_element;
	}
	else {
		_result->_type = _arg->_type;
		_result->_element = ElementUtil::nonexist(_arg->_type);
	}
}*/

void FuncTermGrounder::run() const {
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n)
		_subtermgrounders[n]->run();
	_result->_type = _function->type(_function->arity());
	_result->_element = (*_function)[_args];
/*	if(!_table->contains(*_result)) {
		_result->_element = ElementUtil::nonexist(_result->_type);
	}*/
}

void GrounderFactory::visit(EcnfTheory* ecnf) {
	_grounder = new EcnfGrounder(ecnf);		// TODO: add the structure?
}

void GrounderFactory::visit(Theory* theory) {

	// Collect components of the theory
	vector<TheoryComponent*> components(theory->nrComponents());
	for(unsigned int n = 0; n < theory->nrComponents(); ++n) {
		components[n] = theory->component(n);
	}

	// TODO (OPTIMIZATION) order components

	// Allocate the theory to be returned by the grounder
	_grounding = new EcnfTheory(theory->vocabulary());

	// Create grounders for the components
	vector<Grounder*> children(components.size());
	for(unsigned int n = 0; n < components.size(); ++n) {
		_poscontext = true;
		_truegencontext = false;
		_sentence = true;
		components[n]->accept(this);
		children[n] = _grounder; 
	}

	_grounder = new TheoryGrounder(_grounding,children);
}

void GrounderFactory::visit(PredForm* pf) {
	// Check if each of the arguments of pf evaluates to a single value 
	PredForm* newpf = pf->clone();
	Formula* transpf = newpf; // TODO: replace this by the appropriate rewriting

	if(newpf == transpf) {	// all arguments indeed evaluate to a single value

		// create grounders for the subterms
		bool posc = _poscontext; bool truegc = _truegencontext; bool sent = _sentence;
		vector<TermGrounder*> vtg;
		vector<TypedElement*> vte;
		vector<SortTable*>	  vst;
		for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
			pf->subterm(n)->accept(this);
			if(_termgrounder) vtg.push_back(_termgrounder);
			vte.push_back(_value);
			vst.push_back(_structure->inter(pf->symb()->sort(n)));
		}
		_poscontext = posc; _truegencontext = truegc; _sentence = sent;

		// create checker
		InstanceChecker* pch;
		InstanceChecker* cch;
		PredInter* inter = _structure->inter(pf->symb());

		if(_truegencontext == pf->sign()) {		// check according to ct-table

			if(inter->cf()) {
				if(inter->cfpt()->empty()) pch = new TrueInstanceChecker();
				else pch = new InvTableInstanceChecker(inter->cfpt(),vte);
			}
			else {
				if(inter->cfpt()->empty()) pch = new FalseInstanceChecker();
				else pch = new TableInstanceChecker(inter->cfpt(),vte);
			}

			if(inter->ct()) {
				if(inter->ctpf()->empty()) cch = new FalseInstanceChecker();
				else cch = new TableInstanceChecker(inter->ctpf(),vte);
			}
			else {
				if(inter->ctpf()->empty()) cch = new TrueInstanceChecker();
				else cch = new InvTableInstanceChecker(inter->ctpf(),vte);
			}
		}
		else {	// check according to cf-table

			if(inter->ct()) {
				if(inter->ctpf()->empty()) pch = new TrueInstanceChecker();
				else pch = new InvTableInstanceChecker(inter->ctpf(),vte);
			}
			else {
				if(inter->ctpf()->empty()) pch = new FalseInstanceChecker();
				else pch = new TableInstanceChecker(inter->ctpf(),vte);
			}

			if(inter->cf()) {
				if(inter->cfpt()->empty()) cch = new FalseInstanceChecker();
				else cch = new TableInstanceChecker(inter->cfpt(),vte);
			}
			else {
				if(inter->cfpt()->empty()) cch = new TrueInstanceChecker();
				else cch = new InvTableInstanceChecker(inter->cfpt(),vte);
			}
		}
		
		// create the actual grounder
		_grounder = new AtomGrounder(_grounding,pf->sign(),_sentence,pf->symb(),vte,vtg,pch,cch,vst,_poscontext,_truegencontext);
	}
	else {
		transpf->accept(this);
	}

	// TODO: delete temporary values

}

void GrounderFactory::visit(VarTerm* t) {
/*	_value = new TypedElement();
	SortTable* table = _structure->inter(_currsort);
	TypedElement* arg = (_varmapping.find(t->var()))->second;
	_termgrounder = new VarTermGrounder(_value,table,arg);*/
	_termgrounder = 0;
	_value = _varmapping.find(t->var())->second;
}

void GrounderFactory::visit(DomainTerm* t) {
	_termgrounder = 0;
	// Check whether value is within bounds of the current position.
//	if(_structure->inter(_currsort)->contains(t->value(),t->type()))
		_value = new TypedElement(t->value(),t->type());
//	else {
//		_value = new TypedElement();
//		_value->_type = t->type();
//		_value->_element = ElementUtil::nonexist(t->type());
//	}
}

void GrounderFactory::visit(FuncTerm* t) {
	vector<TermGrounder*> sub;
	vector<TypedElement*> args(t->func()->arity());
	for(unsigned int n = 0; n < args.size(); ++n) {
		t->subterm(n)->accept(this);
		if(_termgrounder) sub.push_back(_termgrounder);
		args[n] = _value;
	}
	FuncTable* ft = _structure->inter(t->func())->functable();
	_value = new TypedElement();
	_termgrounder = new FuncTermGrounder(_value,sub,ft,args);
}

void GrounderFactory::visit(AggTerm* t) {
	// TODO
}
