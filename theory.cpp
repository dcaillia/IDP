/************************************
	theory.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "theory.hpp"
#include "visitor.hpp"
#include "builtin.hpp"
#include "ecnf.hpp"
#include "ground.hpp"
#include <iostream>
#include <set>

extern string tabstring(unsigned int);
extern bool nexttuple(vector<unsigned int>&,const vector<unsigned int>&);

/*******************
	Constructors
*******************/

/** Cloning while keeping free variables **/

PredForm* PredForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

EqChainForm* EqChainForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

EquivForm* EquivForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

BoolForm* BoolForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

QuantForm* QuantForm::clone() const {
	map<Variable*,Variable*> mvv;
	return clone(mvv);
}

/** Cloning while substituting free variables **/

PredForm* PredForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> na(_args.size());
	for(unsigned int n = 0; n < _args.size(); ++n) na[n] = _args[n]->clone(mvv);
	ParseInfo* npi = 0;
	if(_pi) npi = new ParseInfo(_pi);
	PredForm* pf = new PredForm(_sign,_symb,na,npi);
	return pf;
}

EqChainForm* EqChainForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Term*> nt(_terms.size());
	for(unsigned int n = 0; n < _terms.size(); ++n) nt[n] = _terms[n]->clone(mvv);
	ParseInfo* npi = 0;
	if(_pi) npi = new ParseInfo(_pi);
	EqChainForm* ef = new EqChainForm(_sign,_conj,nt,_comps,_signs,npi);
	return ef;
}

EquivForm* EquivForm::clone(const map<Variable*,Variable*>& mvv) const {
	Formula* nl = _left->clone(mvv);
	Formula* nr = _right->clone(mvv);
	ParseInfo* npi = 0;
	if(_pi) npi = new ParseInfo(_pi);
	EquivForm* ef =  new EquivForm(_sign,nl,nr,npi);
	return ef;
}

BoolForm* BoolForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Formula*> ns(_subf.size());
	for(unsigned int n = 0; n < _subf.size(); ++n) ns[n] = _subf[n]->clone(mvv);
	ParseInfo* npi = 0;
	if(_pi) npi = new ParseInfo(_pi);
	BoolForm* bf = new BoolForm(_sign,_conj,ns,npi);
	return bf;
}

QuantForm* QuantForm::clone(const map<Variable*,Variable*>& mvv) const {
	vector<Variable*> nv(_vars.size());
	map<Variable*,Variable*> nmvv = mvv;
	for(unsigned int n = 0; n < _vars.size(); ++n) {
		ParseInfo* npi = 0;
		if(_pi) npi = new ParseInfo(_pi);
		nv[n] = new Variable(_vars[n]->name(),_vars[n]->sort(),npi);
		nmvv[_vars[n]] = nv[n];
	}
	Formula* nf = _subf->clone(nmvv);
	ParseInfo* npi = 0;
	if(_pi) npi = new ParseInfo(_pi);
	QuantForm* qf = new QuantForm(_sign,_univ,nv,nf,npi);
	return qf;
}


/*****************
	Destructors
*****************/

void PredForm::recursiveDelete() {
	for(unsigned int n = 0; n < _args.size(); ++n) _args[n]->recursiveDelete();
	delete(this);
}

void EqChainForm::recursiveDelete() {
	for(unsigned int n = 0; n < _terms.size(); ++n) _terms[n]->recursiveDelete();
	delete(this);
}

void EquivForm::recursiveDelete() {
	_left->recursiveDelete();
	_right->recursiveDelete();
	delete(this);
}

void BoolForm::recursiveDelete() {
	for(unsigned int n = 0; n < _subf.size(); ++n) _subf[n]->recursiveDelete();
	delete(this);
}

void QuantForm::recursiveDelete() {
	_subf->recursiveDelete();
	for(unsigned int n = 0; n < _vars.size(); ++n) delete(_vars[n]);
	delete(this);
}

void Rule::recursiveDelete() {
	_head->recursiveDelete();
	_body->recursiveDelete();
	for(unsigned int n = 0; n < nrQvars(); ++n) delete(qvar(n));
	delete(this);
}

void Definition::recursiveDelete() {
	for(unsigned int n = 0; n < _rules.size(); ++n) _rules[n]->recursiveDelete();
	delete(this);
}

void FixpDef::recursiveDelete() {
	for(unsigned int n = 0; n < _rules.size(); ++n) _rules[n]->recursiveDelete();
	for(unsigned int n = 0; n < _defs.size(); ++n) _defs[n]->recursiveDelete();
	delete(this);
}

void Theory::recursiveDelete() {
	for(unsigned int n = 0; n < _sentences.size(); ++n) _sentences[n]->recursiveDelete();
	for(unsigned int n = 0; n < _definitions.size(); ++n) _definitions[n]->recursiveDelete();
	for(unsigned int n = 0; n < _fixpdefs.size(); ++n) _fixpdefs[n]->recursiveDelete();
	delete(this);
}

/*******************************
	Computing free variables
*******************************/

void Formula::setfvars() {
	_fvars.clear();
	for(unsigned int n = 0; n < nrSubterms(); ++n) {
		Term* t = subterm(n);
		t->setfvars();
		for(unsigned int m = 0; m < t->nrFvars(); ++m) { 
			unsigned int k = 0;
			for(; k < nrQvars(); ++k) {
				if(qvar(k) == t->fvar(m)) break;
			}
			if(k == nrQvars()) _fvars.push_back(t->fvar(m));
		}
	}
	for(unsigned int n = 0; n < nrSubforms(); ++n) {
		Formula* f = subform(n);
		f->setfvars();
		for(unsigned int m = 0; m < f->nrFvars(); ++m) {
			unsigned int k = 0;
			for(; k < nrQvars(); ++k) {
				if(qvar(k) == f->fvar(m)) break;
			}
			if(k == nrQvars()) _fvars.push_back(f->fvar(m));
		}
	}
	VarUtils::sortunique(_fvars);
}

/***********************************
	Adding rules and definitions
***********************************/

void Definition::add(Rule* r) {
	_rules.push_back(r);
	add(r->head()->symb());
}

void FixpDef::add(Rule* r) {
	_rules.push_back(r);
	add(r->head()->symb());
}

void Definition::add(PFSymbol* p) {
	unsigned int n = 0;
	for(; n < _defsyms.size(); ++n) {
		if(p == _defsyms[n]) return;
	}
	_defsyms.push_back(p);
}

void FixpDef::add(PFSymbol* p) {
	unsigned int n = 0;
	for(; n < _defsyms.size(); ++n) {
		if(p == _defsyms[n]) return;
	}
	_defsyms.push_back(p);
}

void Definition::defsyms() {
	_defsyms.clear();
	for(unsigned int n = 0; n < _rules.size(); ++n) 
		add(_rules[n]->head()->symb());
}

void FixpDef::defsyms() {
	_defsyms.clear();
	for(unsigned int n = 0; n < _rules.size(); ++n) 
		add(_rules[n]->head()->symb());
}

/***************************
	Containment checking
***************************/

bool Formula::contains(Variable* v) const {
	for(unsigned int n = 0; n < nrQvars(); ++n) {
		if(qvar(n) == v) return true;
	}
	for(unsigned int n = 0; n < nrSubterms(); ++n) {
		if(subterm(n)->contains(v)) return true;
	}
	for(unsigned int n = 0; n < nrSubforms(); ++n) {
		if(subform(n)->contains(v)) return true;
	}
	return false;
}

/****************
	Debugging
****************/

string PredForm::to_string() const {
	string s;
	if(!_sign) s = s + '~';
	s = s + _symb->to_string();
	if(_args.size()) {
		s = s + '(' + _args[0]->to_string();
		for(unsigned int n = 1; n < _args.size(); ++n) {
			s = s + ',' + _args[n]->to_string();
		}
		s = s + ')';
	}
	return s;
}

string EqChainForm::to_string() const {
	string s;
	if(!_sign) s = s + '~';
	s = s + "(" + _terms[0]->to_string();
	for(unsigned int n = 0; n < _comps.size(); ++n) {
		switch(_comps[n]) {
			case '=':
				if(_signs[n]) s = s + " = ";
				else s = s + " ~= ";
				break;
			case '<':
				if(_signs[n]) s = s + " < ";
				else s = s + " >= ";
				break;
			case '>':
				if(_signs[n]) s = s + " > ";
				else s = s + " =< ";
				break;
		}
		s = s + _terms[n+1]->to_string();
		if(!_conj && n+1 < _comps.size()) s = s + " | " + _terms[n+1]->to_string();
	}
	s = s + ")";
	return s;
}

string EquivForm::to_string() const {
	string s = '(' + _left->to_string() + " <=> " + _right->to_string() + ')';
	return s;
}

string BoolForm::to_string() const {
	string s;
	if(_subf.empty()) {
		if(_sign == _conj) return "true";
		else return "false";
	}
	if(!_sign) s = '~';
	s = s + '(' + _subf[0]->to_string();
	for(unsigned int n = 1; n < _subf.size(); ++n) {
		if(_conj) s = s + " & ";
		else s = s +  " | ";
		s = s + _subf[n]->to_string(); 
	}
	return s + ')';
}

string QuantForm::to_string() const {
	string s;
	if(!_sign) s = s + '~';
	s = s + '(';
	if(_univ) s = s + '!';
	else s = s + '?';
	for(unsigned int n = 0; n < _vars.size(); ++n) {
		s = s + ' ' +  _vars[n]->to_string();
		if(_vars[n]->sort()) s = s + '[' + _vars[n]->sort()->name() + ']';
	}
	s = s + " : " + _subf->to_string();
	return s + ')';
}

string Rule::to_string() const {
	string s;
	if(!_vars.empty()) {
		s = "!";
		for(unsigned int n = 0; n < _vars.size(); ++n) {
			s = s + ' ' + _vars[n]->name();
			if(_vars[n]->sort()) s = s + '[' + _vars[n]->sort()->name() + ']';
		}
		s = s + " : ";
	}
	s = s + _head->to_string() + " <- " + _body->to_string() + '.';
	return s;
}

string Definition::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + "{\n";
	for(unsigned int n = 0; n < _rules.size(); ++n) {
		s = s + tab + "  " + _rules[n]->to_string() + '\n';
	}
	s = s + tab + "}\n";
	return s;
}

string FixpDef::to_string(unsigned int spaces) const {
	string tab = tabstring(spaces);
	string s = tab + (_lfp ? "LFD " : "GFD ") + "[\n";
	for(unsigned int n = 0; n < _rules.size(); ++n) {
		s = s + tab + "  " + _rules[n]->to_string() + '\n';
	}
	for(unsigned int n = 0; n < _defs.size(); ++n) {
		s = s + _defs[n]->to_string(spaces+2);
	}
	s = s + tab + "]\n";
	return s;
}

string Theory::to_string() const {
	string s = "#theory " + _name;
	if(_vocabulary) {
		s = s + " : " + _vocabulary->name();
	}
	s = s + " {\n";
	for(unsigned int n = 0; n < _sentences.size(); ++n) {
		s = s + "   " + _sentences[n]->to_string() + ".\n";
	}
	for(unsigned int n = 0; n < _definitions.size(); ++n) {
		s = s + _definitions[n]->to_string(3);
	}
	for(unsigned int n = 0; n < _fixpdefs.size(); ++n) {
		s = s + _fixpdefs[n]->to_string(3);
	}
	return s + "}\n";
}


/********************
	Formula utils
********************/

/** Evaluate formulas **/

namespace TVUtils {

	TruthValue swaptruthvalue(TruthValue v) {
		switch(v) {
			case TV_TRUE: return TV_FALSE;
			case TV_FALSE: return TV_TRUE;
			default: return v;
		}
	}

	TruthValue glbt(TruthValue v1, TruthValue v2) {
		if(v1 == TV_UNDEF || v2 == TV_UNDEF) return TV_UNDEF;
		switch(v1) {
			case TV_TRUE:
				return v2;
			case TV_FALSE:
				return TV_FALSE;
			case TV_UNKN:
				switch(v2) {
					case TV_TRUE: 
					case TV_UNKN: 
						return TV_UNKN;
					case TV_FALSE: 
					case TV_INCO: 
						return TV_FALSE;
					case TV_UNDEF:
						assert(false);
				}
			case TV_INCO:
				switch(v2) {
					case TV_TRUE: 
					case TV_INCO: 
						return TV_INCO;
					case TV_FALSE: 
					case TV_UNKN: 
						return TV_FALSE;
					case TV_UNDEF:
						assert(false);
				}
			default:
				return TV_UNDEF;
		}
	}

	TruthValue lubt(TruthValue v1, TruthValue v2) {
		if(v1 == TV_UNDEF || v2 == TV_UNDEF) return TV_UNDEF;
		switch(v1) {
			case TV_TRUE:
				return TV_TRUE;
			case TV_FALSE:
				return v2;
			case TV_UNKN:
				switch(v2) {
					case TV_TRUE: 
					case TV_INCO: 
						return TV_TRUE;
					case TV_FALSE: 
					case TV_UNKN: 
						return TV_UNKN;
					case TV_UNDEF:
						assert(false);
				}
			case TV_INCO:
				switch(v2) {
					case TV_TRUE: 
					case TV_UNKN:
						return TV_TRUE;
					case TV_FALSE: 
					case TV_INCO: 
						return TV_INCO;
					case TV_UNDEF:
						assert(false);
				}
			default:
				return TV_UNDEF;
		}
	}

	string to_string(TruthValue v) {
		switch(v) {
			case TV_TRUE: return "true";
			case TV_FALSE: return "false";
			case TV_INCO: return "inconsistent";
			case TV_UNKN: return "unknown";
			case TV_UNDEF: return "undefined";
			default: assert(false); return "";
		}
	}
}

class FormulaEvaluator : public Visitor {

	private:
		Structure*					_structure;
		map<Variable*,TypedElement>	_varmapping;
		TruthValue					_returnvalue;

	public:
		
		FormulaEvaluator(Formula* f, Structure* s, const map<Variable*,TypedElement>& m) :
			Visitor(), _structure(s), _varmapping(m) { f->accept(this);	}
		
		TruthValue returnvalue()	const { return _returnvalue;	}

		void visit(PredForm*);
		void visit(BoolForm*);
		void visit(EqChainForm*);
		void visit(EquivForm*);
		void visit(QuantForm*);
		
};

void FormulaEvaluator::visit(PredForm* pf) {
	vector<TypedElement> argvalues(pf->nrSubterms());
	TermEvaluator* te = new TermEvaluator(_structure,_varmapping);
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		pf->subterm(n)->accept(te);
		if(ElementUtil::exists(te->returnvalue())) argvalues[n] = te->returnvalue();
		else {
			_returnvalue = TV_UNDEF;
			return;
		}
	}
	delete(te);
	PredInter* pt = _structure->inter(pf->symb());
	assert(pt);
	if(pt->istrue(argvalues)) {
		if(pt->ctpf() != pt->cfpt() && pt->isfalse(argvalues)) _returnvalue = TV_INCO;
		else _returnvalue = TV_TRUE;
	}
	else if(pt->isfalse(argvalues)) _returnvalue = TV_FALSE;
	else _returnvalue = TV_UNKN;
	if(!pf->sign()) {
		_returnvalue = TVUtils::swaptruthvalue(_returnvalue);
	}
}

void FormulaEvaluator::visit(EquivForm* ef) {
	assert(false);
}

void FormulaEvaluator::visit(EqChainForm* ef) {
	assert(false);
}

void FormulaEvaluator::visit(BoolForm* bf) {
	TruthValue result;
	if(bf->conj()) result = TV_TRUE;
	else result = TV_FALSE;
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		bf->subform(n)->accept(this);
		result = (bf->conj()) ? TVUtils::glbt(result,_returnvalue) : TVUtils::lubt(result,_returnvalue) ;
		if((bf->conj() && result == TV_FALSE) || (!(bf->conj()) && result == TV_TRUE)) break;
	}
	_returnvalue = result;
	if(!bf->sign()) _returnvalue = TVUtils::swaptruthvalue(_returnvalue);
	return;
}

void FormulaEvaluator::visit(QuantForm* qf) {
	TruthValue result = (qf->univ()) ? TV_TRUE : TV_FALSE;

	vector<unsigned int> limits;
	vector<SortTable*> tables;
	for(unsigned int n = 0; n < qf->nrQvars(); ++n) {
		SortTable* st = _structure->inter(qf->qvar(n)->sort());
		assert(st);
		assert(st->finite());
		if(st->empty()) {
			_returnvalue = result;
			if(!qf->sign()) _returnvalue = TVUtils::swaptruthvalue(_returnvalue);
			return;
		}
		else {
			limits.push_back(st->size());
			tables.push_back(st);
			TypedElement e; 
			e._type = st->type(); 
			_varmapping[qf->qvar(n)] = e;
		}
	}
	vector<unsigned int> tuple(limits.size(),0);

	do {
		for(unsigned int n = 0; n < tuple.size(); ++n) 
			_varmapping[qf->qvar(n)]._element = tables[n]->element(tuple[n]);
		qf->subf()->accept(this);
		result = (qf->univ()) ? TVUtils::glbt(result,_returnvalue) : TVUtils::lubt(result,_returnvalue) ;
		if((qf->univ() && result == TV_FALSE) || ((!qf->univ()) && result == TV_TRUE)) break;
	} while(nexttuple(tuple,limits));
	_returnvalue = result;
	if(!qf->sign()) _returnvalue = TVUtils::swaptruthvalue(_returnvalue);
	return;
}

namespace FormulaUtils {
	TruthValue evaluate(Formula* f, Structure* s, const map<Variable*,TypedElement>& m) {
		FormulaEvaluator fe(f,s,m);
		return fe.returnvalue();
	}
}

/*************************
	Rewriting theories
*************************/

void Theory::add(Theory* t) {
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_definitions.push_back(t->definition(n));
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_fixpdefs.push_back(t->fixpdef(n));
	}
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_sentences.push_back(t->sentence(n));
	}

}

/** Push negations inside **/

class NegationPush : public Visitor {

	public:
		NegationPush(AbstractTheory* t)	: Visitor() { t->accept(this);	}

		void visit(EqChainForm*);
		void visit(EquivForm*);
		void visit(BoolForm*);
		void visit(QuantForm*);

};

void NegationPush::visit(EqChainForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->conj(!f->conj());
		for(unsigned int n = 0; n < f->nrSubterms()-1; ++n)  f->compsign(n,!f->compsign(n));
	}
	traverse(f);
}

void NegationPush::visit(EquivForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->right()->swapsign();
	}
	traverse(f);
}

void NegationPush::visit(BoolForm* f) {
	if(!f->sign()) {
		f->swapsign();
		for(unsigned int n = 0; n < f->nrSubforms(); ++n) 
			f->subform(n)->swapsign();
		f->conj(!f->conj());
	}
	traverse(f);
}

void NegationPush::visit(QuantForm* f) {
	if(!f->sign()) {
		f->swapsign();
		f->subf()->swapsign();
		f->univ(!f->univ());
	}
	traverse(f);
}

/** Rewrite A <=> B to (A => B) & (B => A) **/

class EquivRemover : public MutatingVisitor {

	public:
		EquivRemover(AbstractTheory* t)	: MutatingVisitor() { t->accept(this);	}

		BoolForm* visit(EquivForm*);

};

BoolForm* EquivRemover::visit(EquivForm* ef) {
	Formula* nl = ef->left()->accept(this);
	Formula* nr = ef->right()->accept(this);
	if(nl != ef->left()) delete(ef->left());
	if(nr != ef->right()) delete(ef->right());
	vector<Formula*> vf1(2);
	vector<Formula*> vf2(2);
	vf1[0] = nl; vf1[1] = nr;
	vf2[0] = nl->clone(); vf2[1] = nr->clone();
	vf1[0]->swapsign(); vf2[1]->swapsign();
	ParseInfo* nip1 = 0; ParseInfo* nip2 = 0; ParseInfo* nip = 0;
	if(ef->pi()) {
		nip1 = new ParseInfo(ef->pi());
		nip2 = new ParseInfo(ef->pi());
		nip = new ParseInfo(ef->pi());
	}
	BoolForm* bf1 = new BoolForm(true,false,vf1,nip1);
	BoolForm* bf2 = new BoolForm(true,false,vf2,nip2);
	vector<Formula*> vf(2); vf[0] = bf1; vf[1] = bf2;
	BoolForm* bf = new BoolForm(ef->sign(),true,vf,nip);
	return bf;
}

/** Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc. **/

class Flattener : public Visitor {

	public:
		Flattener(AbstractTheory* t) : Visitor() { t->accept(this);	}

		void visit(BoolForm*);
		void visit(QuantForm*);

};

void Flattener::visit(BoolForm* bf) {
	vector<Formula*> newsubf;
	traverse(bf);
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		if(typeid(*(bf->subform(n))) == typeid(BoolForm)) {
			BoolForm* sbf = dynamic_cast<BoolForm*>(bf->subform(n));
			if((bf->conj() == sbf->conj()) && sbf->sign()) {
				for(unsigned int m = 0; m < sbf->nrSubforms(); ++m) newsubf.push_back(sbf->subform(m));
				delete(sbf);
			}
			else newsubf.push_back(bf->subform(n));
		}
		else newsubf.push_back(bf->subform(n));
	}
	bf->subf(newsubf);
}

void Flattener::visit(QuantForm* qf) {
	traverse(qf);	
	if(typeid(*(qf->subf())) == typeid(QuantForm)) {
		QuantForm* sqf = dynamic_cast<QuantForm*>(qf->subf());
		if((qf->univ() == sqf->univ()) && sqf->sign()) {
			qf->subf(sqf->subf());
			for(unsigned int n = 0; n < sqf->nrQvars(); ++n) 
				qf->add(sqf->qvar(n));
			delete(sqf);
		}
	}
}

/**  Rewrite chains of equalities to a conjunction or disjunction of atoms **/

class EqChainRemover : public MutatingVisitor {

	public:
		EqChainRemover(AbstractTheory* t)	: MutatingVisitor() { t->accept(this);	}
		EqChainRemover()	: MutatingVisitor() {	}

		Formula* visit(EqChainForm*);

};

Formula* EqChainRemover::visit(EqChainForm* ef) {
	for(unsigned int n = 0; n < ef->nrSubterms(); ++n) ef->subterm(n)->accept(this);
	vector<Formula*> vf;
	for(unsigned int n = 0; n < ef->nrSubterms()-1; ++n) {
		Predicate* p = 0;
		switch(ef->comp(n)) {
			case '=' : p = Builtin::pred("=/2"); break;
			case '<' : p = Builtin::pred("</2"); break;
			case '>' : p = Builtin::pred(">/2"); break;
			default: assert(false);
		}
		vector<Sort*> vs(2); vs[0] = ef->subterm(n)->sort(); vs[1] = ef->subterm(n+1)->sort();
		p = p->disambiguate(vs);
		assert(p);
		vector<Term*> vt(2); 
		if(n) vt[0] = ef->subterm(n)->clone();
		else vt[0] = ef->subterm(n);
		vt[1] = ef->subterm(n+1);
		ParseInfo* nip = 0;
		if(ef->pi()) nip = new ParseInfo(ef->pi());
		PredForm* pf = new PredForm(ef->compsign(n),p,vt,nip);
		vf.push_back(pf);
	}
	if(vf.size() == 1) {
		if(!ef->sign()) vf[0]->swapsign();
		return vf[0];
	}
	else {
		ParseInfo* nip = 0;
		if(ef->pi()) nip = new ParseInfo(ef->pi());
		BoolForm* bf = new BoolForm(ef->sign(),ef->conj(),vf,nip);
		return bf;
	}
}

/** Convert to ecnf **/

enum ECNFCONVRTT { ECTT_ATOM, ECTT_CLAUSE };

class TheoryConvertor : public Visitor {
	
	private:
		GroundTranslator*	_translator;
		EcnfTheory*			_returnvalue;
		int					_curratom;
		vector<int>			_currclause;
		ECNFCONVRTT			_rettype;

	public:
		
		TheoryConvertor(AbstractTheory* t, GroundTranslator* g) : 
			Visitor(), _translator(g), _returnvalue(new EcnfTheory()) { t->accept(this); }

		void visit(PredForm*);
		void visit(EqChainForm*);
		void visit(EquivForm*);
		void visit(BoolForm*);
		void visit(QuantForm*);
		void visit(Theory*);

		EcnfTheory*	returnvalue()	const { return _returnvalue;	}

};

void TheoryConvertor::visit(Theory* t) {
	// sentences
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		t->sentence(n)->accept(this);
		switch(_rettype) {
			case ECTT_ATOM:
				_returnvalue->addClause(vector<int>(1,_curratom));
				break;
			case ECTT_CLAUSE:
				_returnvalue->addClause(_currclause);
				break;
			default:
				assert(false);
		}
	}
	// definitions
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		t->definition(n)->accept(this);
		// TODO
	}
	// fixpoint definitions
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		// TODO
		t->fixpdef(n)->accept(this);
	}
}

void TheoryConvertor::visit(PredForm* pf) {
	// TODO: aggregates!
	vector<string> args(pf->symb()->nrsorts());
	for(unsigned int n = 0; n < pf->symb()->nrsorts(); ++n) {
		assert(typeid(*(pf->subterm(n))) == typeid(DomainTerm));
		DomainTerm* dt = dynamic_cast<DomainTerm*>(pf->subterm(n));
		args[n] = ElementUtil::ElementToString(dt->value(),dt->type());
	}
	_curratom = _translator->translate(pf->symb(),args);
	if(!pf->sign()) _curratom = -_curratom;
	_rettype = ECTT_ATOM;
}

void TheoryConvertor::visit(BoolForm* bf) {
	assert(bf->sign());
	assert(!bf->conj());
	vector<int> literals;
	for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
		bf->subform(n)->accept(this);
		literals.push_back(_curratom);
	}
	_currclause = literals;
	_rettype = ECTT_CLAUSE;
}

void TheoryConvertor::visit(EquivForm*) {
	assert(false);
}

void TheoryConvertor::visit(EqChainForm*) {
	assert(false);
}

void TheoryConvertor::visit(QuantForm*) {
	assert(false);
}

/** Move quantifiers **/

class QuantMover : public MutatingVisitor {

	public:
		QuantMover(AbstractTheory* t)	: MutatingVisitor() { t->accept(this);	}

		Formula* visit(QuantForm*);

};

Formula* QuantMover::visit(QuantForm* qf) {
	if(typeid(*(qf->subf())) == typeid(BoolForm)) {
		BoolForm* bf = dynamic_cast<BoolForm*>(qf->subf());
		bool u = (qf->univ() == qf->sign());
		bool c = (bf->conj() == bf->sign());
		if(u == c) {
			bool s = (qf->sign() == bf->sign());
			vector<Formula*> vf(bf->nrSubforms());
			for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
				QuantForm* nqf = new QuantForm(s,u,qf->qvars(),bf->subform(n),0);
				vf[n] = nqf->clone();
				delete(nqf);
			}
			qf->recursiveDelete();
			BoolForm* nbf = new BoolForm(true,c,vf,0);
			return nbf->accept(this);
		}
	}
	return MutatingVisitor::visit(qf);
}

/** Tseitin transformation **/

enum TseitinType { TsT_RULE, TsT_EQUIV, TsT_IMPL };

class Tseitinizer : public MutatingVisitor {
	private:
		bool	_insiderule;
		bool	_maketseitin;
		Theory*	_theory;
	public:

		Tseitinizer(Theory* t);

		Theory*		visit(Theory*);
		Formula*	visit(BoolForm*);

};

Tseitinizer::Tseitinizer(Theory* t) {
	// TODO: clone t's vocabulary!
	
	// Prepare the theory
	TheoryUtils::remove_eqchains(t);
	TheoryUtils::push_negations(t);
	TheoryUtils::flatten(t);
	TheoryUtils::move_quantifiers(t);

	// Apply the transformation
	_theory = t;
	t->accept(this);
}

Theory* Tseitinizer::visit(Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_maketseitin = false;
		_insiderule = false;
		Formula* f = t->sentence(n)->accept(this);
		if(f != t->sentence(n)) {
			vector<Variable*> vv;
			for(unsigned int m = 0; m < f->nrFvars(); ++m) {
				vv.push_back(f->fvar(m));
			}
			if(!vv.empty()) {
				ParseInfo* npi = 0;
				if(f->pi()) npi = new ParseInfo(f->pi());
				f = new QuantForm(true,true,vv,f,npi);
			}
			delete(t->sentence(n));
			t->sentence(n,f);
		}
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_maketseitin = false;
		_insiderule = true;
		Definition* d = t->definition(n)->accept(this);
		if(d != t->definition(n)) {
			delete(t->definition(n));
			t->definition(n,d);
		}
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		// TODO: predicate introduction for fixpoint definitions???
	}
	return t;
}

Formula* Tseitinizer::visit(BoolForm* bf) {
	if(_insiderule) {
		// TODO
	}
	else {
		if(_maketseitin) {
			vector<Sort*> vs(bf->nrFvars());
			for(unsigned int n = 0; n < bf->nrFvars(); ++n) vs[n] = bf->fvar(n)->sort();
			Predicate* tspred = new Predicate(vs);
			_theory->vocabulary()->addPred(tspred->name(),tspred);
			if(bf->conj()) {	// A conjunction deeper than sentence level
				for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
					map<Variable*,Variable*> mvv;
					vector<Variable*> vv(bf->nrFvars());
					vector<Term*> vt(bf->nrFvars());
					for(unsigned int m = 0; m < bf->nrFvars(); ++m) {
						vv[m] = new Variable(bf->fvar(m)->name(),bf->fvar(m)->sort(),0);
						vt[m] = new VarTerm(vv[m],0);
						mvv[bf->fvar(m)] = vv[m];
					}
					vector<Formula*> vf(2);
					vf[0] = bf->subform(n)->clone(mvv);
					vf[1] = new PredForm(false,tspred,vt,0);
					BoolForm* f = new BoolForm(true,false,vf,0);
					if(vv.empty()) _theory->add(f);
					else _theory->add(new QuantForm(true,true,vv,f,0));
					bf->subform(n)->recursiveDelete();
				}
			}
			else {	// A disjunction deeper than sentence level
				map<Variable*,Variable*> mvv;
				vector<Variable*> vv(bf->nrFvars());
				vector<Term*> vt(bf->nrFvars());
				for(unsigned int n = 0; n < bf->nrFvars(); ++n) {
					vv[n] = new Variable(bf->fvar(n)->name(),bf->fvar(n)->sort(),0);
					vt[n] = new VarTerm(vv[n],0);
					mvv[bf->fvar(n)] = vv[n];
				}
				vector<Formula*> vf;
				vf.push_back(new PredForm(false,tspred,vt,0));
				for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
					vf.push_back(bf->subform(n)->clone(mvv));
					bf->subform(n)->recursiveDelete();
				}
				BoolForm* f = new BoolForm(true,false,vf,0);
				if(vv.empty()) _theory->add(f);
				else _theory->add(new QuantForm(true,true,vv,f,0));
			}
			vector<Term*> vt(bf->nrFvars());
			for(unsigned int n = 0; n < bf->nrFvars(); ++n) vt[n] = new VarTerm(bf->fvar(n),0);
			return new PredForm(true,tspred,vt,0);
		}
		else {
			if(bf->conj()) {	// A conjunction at sentence level: split the conjunction
				if(bf->nrSubforms()) {
					for(unsigned int n = 1; n < bf->nrSubforms(); ++n) {
						_theory->add(bf->subform(n));
					}
					return bf->subform(0)->accept(this);
				}
				else return bf;
			}
			else {	// A disjunction at sentence level: go one level deeper
				for(unsigned int n = 0; n < bf->nrSubforms(); ++n) {
					_maketseitin = true;
					Formula* nf = bf->subform(n)->accept(this);
					if(nf != bf->subform(n)) {
						delete(bf->subform(n));
						bf->subf(n,nf);
					}
				}
				return bf;
			}
		}
	}
	return bf;
}

/** Partial evaluation over structure **/

class Simplifier : public MutatingVisitor {
	private:
		Structure*	_structure;
	public:
		Simplifier(Theory* t, Structure* s) : MutatingVisitor(), _structure(s) { t->accept(this);	}
		Theory*		visit(Theory* t);
		Formula*	visit(PredForm* pf);
		Formula*	visit(BoolForm* bf);
		// TODO:
		//Formula*	visit(QuantForm* qf);
		//Formula*	visit(EquivForm* ef);
};

Theory* Simplifier::visit(Theory* t) {

	// Sentences
	unsigned int move = 0;
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		Formula* f = t->sentence(n)->accept(this);
		if(f != t->sentence(n)) {
			if(f->trueformula()) {	// skip true formulas
				++move;
				f->recursiveDelete();
				delete(t->sentence(n));
			}
			else if(f->falseformula()) {	// simplify theory to 'false'
				for(unsigned int m = 0; m < n-move; ++m) t->sentence(m)->recursiveDelete();
				for(unsigned int m = n+1; m < t->nrSentences(); ++m) t->sentence(m)->recursiveDelete();
				for(unsigned int m = 0; m < t->nrDefinitions(); ++m) t->definition(m)->recursiveDelete();
				for(unsigned int m = 0; m < t->nrFixpDefs(); ++m) t->fixpdef(m)->recursiveDelete();
				delete(t->sentence(n));
				t->clear_sentences();
				t->clear_definitions();
				t->clear_fixpdefs();
				move = 0;
				t->add(f);
				break;
			}
			else {
				vector<Variable*> vv;
				for(unsigned int m = 0; m < f->nrFvars(); ++m) {
					vv.push_back(f->fvar(m));
				}
				if(!vv.empty()) {
					ParseInfo* npi = 0;
					if(f->pi()) npi = new ParseInfo(f->pi());
					f = new QuantForm(true,true,vv,f,npi);
				}
				delete(t->sentence(n));
				t->sentence(n-move,f);
			}
		}
		else t->sentence(n-move,t->sentence(n));
	}
	for(unsigned int n = 0; n < move; ++n) t->pop_sentence();

	// Definitions
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		Definition* d = t->definition(n)->accept(this);
		if(d != t->definition(n)) {
			delete(t->definition(n));
			t->definition(n,d);
		}
	}

	// Fixpoint definitions
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		FixpDef* fd = t->fixpdef(n)->accept(this);
		if(fd != t->fixpdef(n)) {
			delete(t->fixpdef(n));
			t->fixpdef(n,fd);
		}
	}
	return t;
}

Formula* Simplifier::visit(PredForm* pf) {
	if(pf->nrFvars()) return pf;
	else {
		map<Variable*,TypedElement> mvt;
		TruthValue tv = FormulaUtils::evaluate(pf,_structure,mvt);
		switch(tv) {
			case TV_TRUE:
			case TV_FALSE:
			{
				vector<Formula*> vf(0);
				BoolForm* bf = new BoolForm(true,tv==TV_TRUE,vf,0);
				for(unsigned int n = 0; n < pf->nrSubterms(); ++n) pf->subterm(n)->recursiveDelete();
				return bf;
			}
			default:
				return pf;
		}
	}
}

Formula* Simplifier::visit(BoolForm* bf) {
	vector<Formula*> vf;
	unsigned int n = 0;
	Formula* nf;
	for(; n < bf->nrSubforms(); ++n) {
		nf = bf->subform(n)->accept(this);
		if(nf == bf->subform(n)) {
			nf = nf->clone();
			bf->subform(n)->recursiveDelete();
		}
		else delete(bf->subform(n));
		if(nf->trueformula()) {
			if(!bf->conj()) break; 
			else nf->recursiveDelete();
		}
		else if(nf->falseformula()) {
			if(bf->conj()) break;
			else nf->recursiveDelete();
		}
		else vf.push_back(nf);
	}
	if(n != bf->nrSubforms()) {
		for(unsigned int m = 0; m < vf.size(); ++m) vf[m]->recursiveDelete();
		for(unsigned int m = n+1; m < bf->nrSubforms(); ++m) bf->subform(m)->recursiveDelete();
		if(!bf->sign()) nf->swapsign();
		return nf;
	}
	else if(vf.size() == 1) {
		if(!bf->sign()) vf[0]->swapsign();
		return vf[0];
	}
	else return new BoolForm(bf->sign(),bf->conj(),vf,0);
}

/** Move functions **/

class FuncGrapher : public MutatingVisitor {
	public:
		FuncGrapher(Theory* t) : MutatingVisitor() { t->accept(this);	}
		Formula*	visit(PredForm* pf);
		Formula*	visit(EqChainForm* ef);
};

Formula* FuncGrapher::visit(PredForm* pf) {
	if(pf->symb()->name() == "=/2") {
		if(typeid(*(pf->subterm(0))) == typeid(FuncTerm)) {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterm(0));
			vector<Term*> vt;
			for(unsigned int n = 0; n < ft->nrSubterms(); ++n) vt.push_back(ft->subterm(n));
			vt.push_back(pf->subterm(1));
			pf = new PredForm(pf->sign(),ft->func(),vt,0);
			delete(ft);
		}
		else if(typeid(*(pf->subterm(1))) == typeid(FuncTerm)) {
			FuncTerm* ft = dynamic_cast<FuncTerm*>(pf->subterm(1));
			vector<Term*> vt;
			for(unsigned int n = 0; n < ft->nrSubterms(); ++n) vt.push_back(ft->subterm(n));
			vt.push_back(pf->subterm(0));
			pf = new PredForm(pf->sign(),ft->func(),vt,0);
			delete(ft);
		}
	}
	return MutatingVisitor::visit(pf);
}

Formula* FuncGrapher::visit(EqChainForm* ef) {
	EqChainRemover ecr;
	Formula* f = ecr.visit(ef);
	Formula* nf = f->accept(this);
	if(nf != f) delete(f);
	return nf;
}

class FuncMover : public MutatingVisitor {
	private:
		bool				_poscontext;	// true iff we are in a positive context
		vector<Formula*>	_funcgraphs;
		vector<Variable*>	_newvars;
		set<Function*>		_dontmove;		// don't move the functions in this set
	public:
		FuncMover(Theory* t) : MutatingVisitor() { t->accept(this);	}
		Theory*		visit(Theory* t);
		Formula*	visit(QuantForm* qf);
		Formula*	visit(BoolForm* bf);
		Formula*	visit(EquivForm* ef);
		Formula*	visit(EqChainForm* ef);
		Formula*	visit(PredForm* pf);
		SetExpr*	visit(EnumSetExpr*);
		SetExpr*	visit(QuantSetExpr*);
		Term*		visit(FuncTerm*);
};

Term* FuncMover::visit(FuncTerm* ft) {
	Term* t = MutatingVisitor::visit(ft);
	Function* f = ft->func();
	if((f->partial() || !f->builtin()) && _dontmove.find(f) == _dontmove.end()) {
		Variable* v = new Variable(f->outsort());
		VarTerm* vt = new VarTerm(v,0);
		vector<Term*> args;
		for(unsigned int n = 0; n < f->arity(); ++n) args.push_back(t->subterm(n));
		args.push_back(vt);
		PredForm* pf = new PredForm(true,f,args,0);
		_funcgraphs.push_back(pf);
		_newvars.push_back(v);
		return vt->clone();
	}
	else return t;
}

SetExpr* FuncMover::visit(EnumSetExpr* ese) {
	bool save = _poscontext;
	_poscontext = true;
	SetExpr* e = MutatingVisitor::visit(ese);
	_poscontext = save;
	return e;
}

SetExpr* FuncMover::visit(QuantSetExpr* qse) {
	bool save = _poscontext;
	_poscontext = true;
	SetExpr* e = MutatingVisitor::visit(qse);
	_poscontext = save;
	return e;
}

Formula* FuncMover::visit(QuantForm* qf) {
	bool save = _poscontext;
	if(!qf->sign()) _poscontext = !_poscontext;
	Formula* f =  MutatingVisitor::visit(qf);
	_poscontext = save;
	return f;
}

Formula* FuncMover::visit(BoolForm* qf) {
	bool save = _poscontext;
	if(!qf->sign()) _poscontext = !_poscontext;
	Formula* f = MutatingVisitor::visit(qf);
	_poscontext = save;
	return f;
}

Formula* FuncMover::visit(EquivForm* qf) {
	// TODO: check if the formula contains partial functions. If yes: rewrite the equivalence first to get the right contexts
	bool save = _poscontext;
	if(!qf->sign()) _poscontext = !_poscontext;
	Formula* f = MutatingVisitor::visit(qf);
	_poscontext = save;
	return f;
}

Formula* FuncMover::visit(EqChainForm* ef) {
	EqChainRemover ecr;
	Formula* f = ecr.visit(ef);
	Formula* nf = f->accept(this);
	if(nf != f) delete(f);
	return nf;
}

Formula* FuncMover::visit(PredForm* pf) {
	bool save = _poscontext;
	for(unsigned int n = 0; n < pf->nrSubterms(); ++n) {
		Term* nt = pf->subterm(n)->accept(this);
		if(nt != pf->subterm(n)) {
			delete(pf->subterm(n));
			pf->arg(n,nt);
		}
	}
	_poscontext = save;
	if(_funcgraphs.empty()) return pf;
	else {
		if(_poscontext) {
			for(unsigned int n = 0; n < _funcgraphs.size(); ++n)
				_funcgraphs[n]->swapsign();
		}
		_funcgraphs.push_back(pf->clone());
		for(unsigned int n = 0; n < pf->nrSubterms(); ++n) pf->subterm(n)->recursiveDelete();
		BoolForm* bf = new BoolForm(true,!_poscontext,_funcgraphs,0);
		QuantForm* qf = new QuantForm(true,_poscontext,_newvars,bf,0);
		_funcgraphs.clear();
		_newvars.clear();
		Formula* f = visit(qf);
		if(f != qf) delete(qf);
		_poscontext = save;
		return f;
	}
}


Theory* FuncMover::visit(Theory* t) {
	for(unsigned int n = 0; n < t->nrSentences(); ++n) {
		_poscontext = true;
		Formula* f = t->sentence(n)->accept(this);
		if(f != t->sentence(n)) {
			vector<Variable*> vv;
			for(unsigned int m = 0; m < f->nrFvars(); ++m) {
				vv.push_back(f->fvar(m));
			}
			if(!vv.empty()) {
				ParseInfo* npi = 0;
				if(f->pi()) npi = new ParseInfo(f->pi());
				f = new QuantForm(true,true,vv,f,npi);
			}
			delete(t->sentence(n));
			t->sentence(n,f);
		}
	}
	for(unsigned int n = 0; n < t->nrDefinitions(); ++n) {
		_poscontext = false;
		Definition* d = t->definition(n)->accept(this);
		if(d != t->definition(n)) {
			delete(t->definition(n));
			t->definition(n,d);
		}
	}
	for(unsigned int n = 0; n < t->nrFixpDefs(); ++n) {
		_poscontext = false;	// TODO: maybe not correct?
		FixpDef* fd = t->fixpdef(n)->accept(this);
		if(fd != t->fixpdef(n)) {
			delete(t->fixpdef(n));
			t->fixpdef(n,fd);
		}
	}
	TheoryUtils::flatten(t);
	return t;
}

/** Theory utils **/

namespace TheoryUtils {
	
	/** Rewriting theories **/
	void push_negations(AbstractTheory* t)		{ NegationPush np(t);	}
	void remove_equiv(AbstractTheory* t)		{ EquivRemover er(t);	}
	void flatten(AbstractTheory* t)				{ Flattener f(t);		}
	void remove_eqchains(AbstractTheory* t)		{ EqChainRemover er(t);	}
	void move_quantifiers(AbstractTheory* t)	{ QuantMover qm(t);		}
	void move_functions(Theory* t)				{ FuncGrapher fg(t); 
												  FuncMover fm(t);		}

	/** Tseitin transformation **/
	void tseitin(Theory* t)	{ Tseitinizer tz(t); } 

	/** Simplification **/
	void simplify(Theory* t, Structure* s)	{ Simplifier sf(t,s);	}
	
	/** ECNF **/
	EcnfTheory*	convert_to_ecnf(AbstractTheory* t, GroundTranslator* g)	{ TheoryConvertor tc(t,g); return tc.returnvalue();	}

}
