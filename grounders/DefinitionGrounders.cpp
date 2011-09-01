/************************************
 DefinitionGrounders.cpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#include "grounders/DefinitionGrounders.hpp"

#include "grounders/TermGrounders.hpp"

#include "grounders/FormulaGrounders.hpp"
#include "grounders/GroundUtils.hpp"
#include "groundtheories/AbstractGroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

#include "generator.hpp"
#include "checker.hpp"
#include "common.hpp"
#include "vocabulary.hpp"
#include "term.hpp"

#include <iostream>

using namespace std;

unsigned int DefinitionGrounder::_currentdefnb = 1;

DefinitionGrounder::DefinitionGrounder(AbstractGroundTheory* gt, std::vector<RuleGrounder*> subgr,int verb)
		: TopLevelGrounder(gt,verb), _defnb(_currentdefnb++), _subgrounders(subgr), _grounddefinition(new GroundDefinition(_defnb, gt->translator())) {
}

bool DefinitionGrounder::run() const {
	for(auto grounder = _subgrounders.begin(); grounder<_subgrounders.end(); ++grounder){
		(*grounder)->run(id(), _grounddefinition);
	}
	return true;
}

RuleGrounder::RuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct)
			: _headgrounder(hgr), _bodygrounder(bgr), _headgenerator(hig), _bodygenerator(big), _context(ct) {
}

void RuleGrounder::run(unsigned int defid, GroundDefinition* grounddefinition) const {
	bool conj = _bodygrounder->conjunctive();
	if(not _bodygenerator->first()){
		return;
	}
	do {
		vector<int>	body;
		_bodygrounder->run(body);
		bool falsebody = (body.empty() && !conj) || (body.size() == 1 && body[0] == _false);
		bool truebody = (body.empty() && conj) || (body.size() == 1 && body[0] == _true);
		if(falsebody){
			continue;
		}

		if(not _headgenerator->first()) {
			continue;
		}
		do{
			Lit head = _headgrounder->run();
			assert(head != _true);
			if(head != _false) {
				if(truebody){
					addTrueRule(grounddefinition, head);
				} else{
					addPCRule(grounddefinition, head,body,conj,_context._tseitin == TsType::RULE);
				}
			}
		}while(_headgenerator->next());
	}while(_bodygenerator->next());
}

void RuleGrounder::addTrueRule(GroundDefinition* const grounddefinition, int head) const {
	addPCRule(grounddefinition, head,vector<int>(0),true,false);
}

void RuleGrounder::addFalseRule(GroundDefinition* const grounddefinition, int head) const {
	addPCRule(grounddefinition, head,vector<int>(0),false,false);
}

void RuleGrounder::addPCRule(GroundDefinition* grounddefinition, int head, const vector<int>& body, bool conj, bool recursive) const {
	grounddefinition->addPCRule(head, body, conj, recursive);
}

void RuleGrounder::addAggRule(GroundDefinition* grounddefinition, int head, int setnr, AggFunction aggtype, bool lower, double bound, bool recursive) const {
	grounddefinition->addAggRule(head, setnr, aggtype, lower, bound, recursive);
}

HeadGrounder::HeadGrounder(AbstractGroundTheory* gt,
							InstanceChecker* pc,
							InstanceChecker* cc,
							PFSymbol* s,
							const vector<TermGrounder*>& sg,
							const vector<SortTable*>& vst)
		: _grounding(gt), _subtermgrounders(sg),
		  _truechecker(pc), _falsechecker(cc),
		  _symbol(gt->translator()->addSymbol(s)), // FIXME what does this do?
		  _tables(vst),
		  _pfsymbol(s){
}

int HeadGrounder::run() const {
	// Run subterm grounders
	bool alldomelts = true;
	vector<GroundTerm> groundsubterms(_subtermgrounders.size());
	ElementTuple args(_subtermgrounders.size());
	for(unsigned int n = 0; n < _subtermgrounders.size(); ++n) {
		groundsubterms[n] = _subtermgrounders[n]->run();
		if(groundsubterms[n]._isvarid) {
			alldomelts = false;
		} else {
			args[n] = groundsubterms[n]._domelement;
		}
	}
	//XXX All subterm grounders should return domain elements.
	assert(alldomelts);

	// Checking partial functions
	for(unsigned int n = 0; n < args.size(); ++n) {
		//TODO: only check positions that can be out of bounds or ...!
		//TODO: produce a warning!
		if(not args[n]) return _false;
		if(not _tables[n]->contains(args[n])) return _false;
	}

	// Run instance checkers and return grounding
	int atom = _grounding->translator()->translate(_symbol,args);
	if(_truechecker->isInInterpretation(args)) {
		_grounding->addUnitClause(atom);
	}else if(_falsechecker->isInInterpretation(args)) {
		_grounding->addUnitClause(-atom);
	}
	return atom;
}

LazyRuleGrounder::LazyRuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct)
			: RuleGrounder(hgr, bgr, NULL, big, ct), _grounding(dynamic_cast<SolverTheory*>(headgrounder()->grounding())) {
	grounding()->translator()->notifyDefined(headgrounder()->pfsymbol(), this);
}

dominstlist LazyRuleGrounder::createInst(const ElementTuple& headargs){
	dominstlist domlist;

	// set the variable instantiations
	for(uint i=0; i<headargs.size(); ++i){
		assert(typeid(*headgrounder()->subtermgrounders()[i])==typeid(VarTermGrounder));
		auto var = (dynamic_cast<VarTermGrounder*>(headgrounder()->subtermgrounders()[i]))->getElement();
		domlist.push_back(dominst(var, headargs[i]));
	}
	return domlist;
}

void LazyRuleGrounder::notify(const Lit& lit, const ElementTuple& headargs){
	grounding()->polNotifyDefined(lit, headargs, this);
}

void LazyRuleGrounder::ground(const Lit& head, const ElementTuple& headargs){
	dominstlist headvarinstlist = createInst(headargs);

	vector<const DomainElement*> originstantiation;
	overwriteVars(originstantiation, headvarinstlist);

	bool conj = bodygrounder()->conjunctive();
	if(not bodygenerator()->first()){
		return;
	}
	do {
		vector<int>	body;
		bodygrounder()->run(body);
		bool falsebody = (body.empty() && !conj) || (body.size() == 1 && body[0] == _false);
		bool truebody = (body.empty() && conj) || (body.size() == 1 && body[0] == _true);
		if(falsebody){
			continue;
		}

		assert(head != _true && head!=_false);
		if(truebody){
			// FIXME addTrueRule(head);
		} else{
			// FIXME addPCRule(head,body,conj,_context._tseitin == TsType::RULE);
		}
	}while(bodygenerator()->next());

	restoreOrigVars(originstantiation, headvarinstlist);
}