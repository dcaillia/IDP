/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#include <cstdlib>
#include <memory>

#include "UnsatCoreExtraction.hpp"

#include "IncludeComponents.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"
#include "creation/cppinterface.hpp"
#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "utils/ListUtils.hpp"
#include "theory/TheoryUtils.hpp"

using namespace std;

// TODO add markers for structure information and for function constraints

class AddMarkers: public TheoryMutatingVisitor {
	VISITORFRIENDS()

	vector<Variable*> vars;
	vector<Predicate*> newpreds;
	map<Predicate*, pair<vector<Variable*>, Formula*>> marker2formula;
	map<Predicate*, pair<vector<Variable*>, Rule*>> marker2rule;
	map<Rule*, DefId> rule2defid;
	map<Predicate*, ParseInfo> marker2parseinfo;

public:
	Theory* execute(Theory* t) {
		auto newt = t->accept(this);
		for (auto p : newpreds) {
			newt->vocabulary()->add(p);
		}
		return newt;
	}

	~AddMarkers() {
		for (auto m2form : marker2formula) {
			m2form.second.second->recursiveDelete();
		}
		for (auto m2rule : marker2rule) {
			m2rule.second.second->recursiveDelete();
		}
	}

	const vector<Predicate*>& getMarkers() const {
		return newpreds;
	}
	vector<TheoryComponent*> getComponentsFromMarkers(const vector<DomainAtom>& pfs) const {
		map<DefId, vector<Rule*>> ruleinstances;
		vector<TheoryComponent*> core;
		stringstream outputwithparseinfo;
		outputwithparseinfo <<"The following is an unsatisfiable subset, "
				"given that functions can map to at most one element (and exactly one if not partial) "
				"and the interpretation of types and symbols in the structure:\n";
		for (auto pf : pfs) {
			auto pred = dynamic_cast<Predicate*>(pf.symbol);
			if (contains(marker2formula, pred)) {
				stringstream ss;
				auto varAndForm = marker2formula.at(pred);
				auto var2elems = getVarInstantiation(varAndForm, pf, ss);
				auto newform = FormulaUtils::substituteVarWithDom(varAndForm.second->cloneKeepVars(), var2elems);
				core.push_back(newform);
				outputwithparseinfo <<"\t" <<print(newform)  <<" instantiated from line " <<newform->pi().linenumber() <<ss.str() <<"\n";
			} else if (contains(marker2rule, pred)) {
				auto varAndForm = marker2rule.at(pred);
				stringstream ss;
				auto var2elems = getVarInstantiation(varAndForm, pf, ss);
				auto head = FormulaUtils::substituteVarWithDom(varAndForm.second->head()->cloneKeepVars(), var2elems);
				auto body = FormulaUtils::substituteVarWithDom(varAndForm.second->body()->cloneKeepVars(), var2elems);
				auto newrule = new Rule( { }, dynamic_cast<PredForm*>(head), body, varAndForm.second->pi());
				ruleinstances[rule2defid.at(varAndForm.second)].push_back(newrule);
				outputwithparseinfo <<"\t" <<print(newrule)  <<" instantiated from line " <<newrule->pi().linenumber() <<ss.str() <<"\n";
			} else {
				core.push_back(&Gen::atom(pf.symbol, pf.args));
			}
		}
		clog <<outputwithparseinfo.str();
		for (auto id2rules : ruleinstances) {
			auto def = new Definition();
			def->add(id2rules.second);
			core.push_back(def->clone());
		}
		return core;
	}
protected:
	Formula* addMarker(Formula* f) {
		vector<Sort*> sorts;
		for (auto var : vars) {
			sorts.push_back(var->sort());
		}
		auto p = new Predicate(sorts);
		newpreds.push_back(p);
		auto atom = &Gen::atom(p, vars);
		marker2formula[p]= {vars, f->cloneKeepVars()};
		return &Gen::disj( { f->cloneKeepVars(), atom });
	}
	Formula* visit(PredForm* pf) {
		return addMarker(pf);
	}
	Formula* visit(AggForm* f) {
		return addMarker(f);
	}
	Formula* visit(EqChainForm* f) {
		return addMarker(f);
	}
	Formula* visit(EquivForm* f) {
		return addMarker(f);
	}

	template<class Object>
	map<Variable*, const DomainElement*> getVarInstantiation(const pair<vector<Variable*>, Object*>& varAndForm, const DomainAtom& pf, stringstream& ss) const{
		map<Variable*, const DomainElement*> var2elems;
		auto begin = true;
		for (uint i = 0; i < pf.args.size(); ++i) {
			if(begin){
				ss<<" with ";
			}
			auto var = varAndForm.first[i];
			auto value = pf.args[i];
			var2elems[var] = value;
			if(not begin){
				ss <<", ";
			}
			begin = false;
			ss <<print(var->name()) <<"=" <<print(value);
		}
		return var2elems;
	}

	Formula* visit(BoolForm* bf) {
		if (bf->isConjWithSign()) {
			return traverse(bf);
		} else {
			return addMarker(bf);
		}
	}
	Formula* visit(QuantForm* q) {
		if (q->isUnivWithSign()) {
			auto tempvars = vars;
			vars.insert(vars.end(), q->quantVars().cbegin(), q->quantVars().cend());
			q->subformula(q->subformula()->accept(this));
			vars = tempvars;
			return q;
		} else {
			return addMarker(q);
		}
	}
	Definition* visit(Definition* d) {
		auto rules = d->rules();
		for (auto r : d->rules()) {
			// H <- B ===> H <- ~M1 & (M2 | B)
			vector<Sort*> sorts;
			vector<Variable*> vars;
			for (auto var : r->quantVars()) {
				sorts.push_back(var->sort());
				vars.push_back(var);
			}
			auto conjp = new Predicate(sorts);
			auto disjp = new Predicate(sorts);
			newpreds.push_back(disjp);
			newpreds.push_back(conjp);
			auto conjmarker = &Gen::operator !(Gen::atom(conjp, vars));
			auto disjmarker = &Gen::atom(disjp, vars);
			auto rc1 = new Rule( { }, r->head()->cloneKeepVars(), r->body()->cloneKeepVars(), r->pi());
			auto rc2 = new Rule( { }, r->head()->cloneKeepVars(), r->body()->cloneKeepVars(), r->pi());
			rule2defid[rc1] = d->getID();
			rule2defid[rc2] = d->getID();
			marker2rule[conjp]= {vars, rc1};
			marker2rule[disjp]= {vars, rc2};
			r->body(&Gen::conj( { conjmarker, &Gen::disj( { disjmarker, r->body() }) }));
		}
		d->rules(rules);
		return d;
	}

	virtual AbstractGroundTheory* visit(AbstractGroundTheory*) {
		throw IdpException("Invalid code path");
	}
	virtual GroundDefinition* visit(GroundDefinition*) {
		throw IdpException("Invalid code path");
	}
	virtual GroundRule* visit(AggGroundRule*) {
		throw IdpException("Invalid code path");
	}
	virtual GroundRule* visit(PCGroundRule*) {
		throw IdpException("Invalid code path");
	}
	virtual Rule* visit(Rule*) {
		throw IdpException("Invalid code path");
	}
	virtual FixpDef* visit(FixpDef*) {
		throw IdpException("Invalid code path");
	}
	virtual Term* visit(VarTerm*) {
		throw IdpException("Invalid code path");
	}
	virtual Term* visit(FuncTerm*) {
		throw IdpException("Invalid code path");
	}
	virtual Term* visit(DomainTerm*) {
		throw IdpException("Invalid code path");
	}
	virtual Term* visit(AggTerm*) {
		throw IdpException("Invalid code path");
	}
	virtual EnumSetExpr* visit(EnumSetExpr*) {
		throw IdpException("Invalid code path");
	}
	virtual QuantSetExpr* visit(QuantSetExpr*) {
		throw IdpException("Invalid code path");
	}
};

vector<TheoryComponent*> UnsatCoreExtraction::extractCore(AbstractTheory* atheory, Structure* structure) {
	auto intheory = dynamic_cast<Theory*>(atheory);
	if (intheory == NULL) {
		throw notyetimplemented("Unsatcore extraction for non first-order theories");
	}

	clog <<">>> Generating an unsatisfiable subset of the given theory.\n";

	stringstream ss;
	ss <<"unsatcore_voc" <<getGlobal()->getNewID();
	auto voc = new Vocabulary(ss.str());
	voc->add(intheory->vocabulary());
	auto newtheory = intheory->clone();
	newtheory->vocabulary(voc);
	auto s = structure->clone();
	s->changeVocabulary(newtheory->vocabulary());

	//	TODO dropping function constraints is not possible as MX is not able to read a model back in that does not satisfy its functions
	//	solution: replace functions with predicate symbols, without adding function constraints!
//	map<Function*, Formula*> func2form;
//	FormulaUtils::addFuncConstraints(newtheory, newtheory->vocabulary(), func2form, getOption(CPSUPPORT));
//	for (auto f2f : func2form) {
//		newtheory->add(f2f.second);
//	}

	for (auto def : newtheory->definitions()) {
		for (auto p : def->defsymbols()) {
			varset vars;
			vector<Term*> varlist;
			for (uint i = 0; i < p->sorts().size(); ++i) {
				auto var = new Variable(p->sort(i));
				vars.insert(var);
				varlist.push_back(new VarTerm(var, { }));
			}
			def->add(new Rule(vars, new PredForm(SIGN::POS, p, varlist, { }), FormulaUtils::falseFormula(), (*def->rules().cbegin())->pi()));
		}
	}

	auto am = new AddMarkers();
	newtheory = am->execute(newtheory);

	auto mxresult = ModelExpansion::doModelExpansion(newtheory, s, NULL, NULL, {{},am->getMarkers()});
	if (not mxresult.unsat) {
		throw IdpException("The given theory has models that extend the structure, so there are no unsat cores.");
	}

	clog <<">>> Unsatisfiable subset found, trying to reduce its size (might take some time, can be interrupted with ctrl-c.\n";

	// TODO should set remaining markers on true to allow ealier pruning
	auto core = mxresult.unsat_in_function_of_ct_lits;
	auto erased = true;
	auto stop = false;
	while(erased && not stop){
		if(getGlobal()->terminateRequested()){
			getGlobal()->reset();
			stop = true;
			break;
		}
		erased = false;
		auto maxsize = core.size();
		for(uint i=0; i<maxsize;){
			if(getGlobal()->terminateRequested()){
				getGlobal()->reset();
				stop = true;
				break;
			}
			auto elem = core[i];
			swap(core[i], core[maxsize-1]);
			core.pop_back();
			maxsize--;
			auto mxresult = ModelExpansion::doModelExpansion(newtheory, s, NULL, NULL, {core, {}});
			if(mxresult._interrupted){
				stop = true;
				break;
			}
			if (not mxresult.unsat) {
				core.push_back(elem);
			}else{
				erased = true;
				if(mxresult.unsat_in_function_of_ct_lits.size()<core.size()){
					core = mxresult.unsat_in_function_of_ct_lits;
					break;
				}
			}
		}
	}

	auto coreresult = am->getComponentsFromMarkers(mxresult.unsat_in_function_of_ct_lits);
	delete(am);
	newtheory->recursiveDelete();
	delete (s);
	delete (voc);
	return coreresult;
}