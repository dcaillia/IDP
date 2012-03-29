/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "groundtheories/SolverPolicy.hpp"

#include "IncludeComponents.hpp"
#include "inferences/grounding/grounders/LazyFormulaGrounders.hpp"
#include "inferences/grounding/grounders/DefinitionGrounders.hpp"

#include "inferences/grounding/GroundTermTranslator.hpp"

#include "external/Constraints.hpp"

#include <cmath>

using namespace std;

inline MinisatID::Atom createAtom(int lit) {
	return MinisatID::Atom(abs(lit));
}

inline MinisatID::Literal createLiteral(int lit) {
	return MinisatID::Literal(abs(lit), lit < 0);
}

MinisatID::literallist createList(const litlist& origlist){
	MinisatID::literallist list;
	for(auto i=origlist.cbegin(); i<origlist.cend(); i++){
		list.push_back(createLiteral(*i));
	}
	return list;
}

template<typename Solver>
void SolverPolicy<Solver>::initialize(Solver* solver, int verbosity, GroundTermTranslator* termtranslator) {
	_solver = solver;
	_verbosity = verbosity;
	_termtranslator = termtranslator;
}

template<typename Solver>
void SolverPolicy<Solver>::polEndTheory(){
}

/*template<>
void SolverPolicy<MinisatID::FlatZincRewriter>::polEndTheory(){
	getSolver().finishParsing();
}*/

double test;

template<typename Solver>
inline MinisatID::Weight SolverPolicy<Solver>::createWeight(double weight) {
	if(modf(weight, &test)!=0){
		throw notyetimplemented("MinisatID does not support doubles yet.");
	}
	return int(weight);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const GroundClause& cl) {
	MinisatID::Disjunction clause;
	for (size_t n = 0; n < cl.size(); ++n) {
		clause.literals.push_back(createLiteral(cl[n]));
	}
	add(getSolver(), clause);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(const TsSet& tsset, int setnr, bool weighted) {
	if (not weighted) {
		MinisatID::Set set;
		set.setID = setnr;
		for (size_t n = 0; n < tsset.size(); ++n) {
			set.literals.push_back(createLiteral(tsset.literal(n)));
		}
		add(getSolver(), set);
	} else {
		MinisatID::WSet set;
		set.setID = setnr;
		for (size_t n = 0; n < tsset.size(); ++n) {
			set.literals.push_back(createLiteral(tsset.literal(n)));
			set.weights.push_back(createWeight(tsset.weight(n)));
		}
		add(getSolver(), set);
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(int defnr, PCGroundRule* rule) {
	polAddPCRule(defnr, rule->head(), rule->body(), (rule->type() == RuleType::CONJ), rule->recursive());
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(int defnr, AggGroundRule* rule) {
	polAddAggregate(defnr, rule->head(), rule->lower(), rule->setnr(), rule->aggtype(), TsType::RULE, rule->bound());
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(int defnr, int head, AggGroundRule* body, bool) {
	polAddAggregate(defnr, head, body->lower(), body->setnr(), body->aggtype(), TsType::RULE, body->bound());
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(int head, AggTsBody* body) {
	Assert(body->type() != TsType::RULE);
	//FIXME getIDForUndefined() should be replaced by the number the SOLVER takes as undefined
	polAddAggregate(getIDForUndefined(), head, body->lower(), body->setnr(), body->aggtype(), body->type(), body->bound());
}

template<typename Solver>
void SolverPolicy<Solver>::polAddWeightedSum(const MinisatID::Atom& head, const std::vector<VarId>& varids, const std::vector<int> weights, const int& bound,
		MinisatID::EqType rel) {
	MinisatID::CPSumWeighted sentence;
	sentence.head = head;
	sentence.varIDs = varids;
	sentence.weights = weights;
	sentence.bound = bound;
	sentence.rel = rel;
	add(getSolver(), sentence);
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(int tseitin, CPTsBody* body) {
	MinisatID::EqType comp;
	switch (body->comp()) {
	case CompType::EQ:
		comp = MinisatID::EqType::EQ;
		break;
	case CompType::NEQ:
		comp = MinisatID::EqType::NEQ;
		break;
	case CompType::LEQ:
		comp = MinisatID::EqType::LEQ;
		break;
	case CompType::GEQ:
		comp = MinisatID::EqType::GEQ;
		break;
	case CompType::LT:
		comp = MinisatID::EqType::L;
		break;
	case CompType::GT:
		comp = MinisatID::EqType::G;
		break;
	}
	CPTerm* left = body->left();
	CPBound right = body->right();
	if (sametypeid<CPVarTerm>(*left)) {
		CPVarTerm* term = dynamic_cast<CPVarTerm*>(left);
		polAddCPVariable(term->varid(), _termtranslator);
		if (right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			MinisatID::CPBinaryRelVar sentence;
			sentence.head = createAtom(tseitin);
			sentence.lhsvarID = term->varid();
			sentence.rhsvarID = right._varid;
			sentence.rel = comp;
			add(getSolver(), sentence);
		} else {
			MinisatID::CPBinaryRel sentence;
			sentence.head = createAtom(tseitin);
			sentence.varID = term->varid();
			sentence.bound = right._bound;
			sentence.rel = comp;
			add(getSolver(), sentence);
		}
	} else if (sametypeid<CPSumTerm>(*left)) {
		CPSumTerm* term = dynamic_cast<CPSumTerm*>(left);
		polAddCPVariables(term->varids(), _termtranslator);
		if (right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			std::vector<VarId> varids = term->varids();
			std::vector<int> weights;
			weights.resize(1, term->varids().size());

			int bound = 0;
			varids.push_back(right._varid);
			weights.push_back(-1);

			polAddWeightedSum(createAtom(tseitin), varids, weights, bound, comp);
		} else {
			std::vector<int> weights { (int) term->varids().size() };
			polAddWeightedSum(createAtom(tseitin), term->varids(), weights, right._bound, comp);
		}
	} else {
		Assert(sametypeid<CPWSumTerm>(*left));
		CPWSumTerm* term = dynamic_cast<CPWSumTerm*>(left);
		polAddCPVariables(term->varids(), _termtranslator);
		if (right._isvarid) {
			polAddCPVariable(right._varid, _termtranslator);
			std::vector<VarId> varids = term->varids();
			std::vector<int> weights = term->weights();

			int bound = 0;
			varids.push_back(right._varid);
			weights.push_back(-1);

			polAddWeightedSum(createAtom(tseitin), varids, weights, bound, comp);
		} else {
			polAddWeightedSum(createAtom(tseitin), term->varids(), term->weights(), right._bound, comp);
		}
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAdd(Lit tseitin, TsType type, const GroundClause& rhs, bool conjunction) {
	MinisatID::ImplicationType impltype;
	switch(type){
	case TsType::RULE:
		Assert(false);
		break;
	case TsType::IMPL:
		impltype = MinisatID::ImplicationType::IMPLIES;
		break;
	case TsType::RIMPL:
		impltype = MinisatID::ImplicationType::IMPLIEDBY;
		break;
	case TsType::EQ:
		impltype = MinisatID::ImplicationType::EQUIVALENT;
		break;
	}
	add(getSolver(), MinisatID::Implication(createLiteral(tseitin), impltype, createList(rhs), conjunction));
}

MinisatID::AggType convert(AggFunction agg){
	MinisatID::AggType type = MinisatID::AggType::CARD;
	switch (agg) {
	case AggFunction::CARD:
		type= MinisatID::AggType::CARD;
		break;
	case AggFunction::SUM:
		type= MinisatID::AggType::SUM;
		break;
	case AggFunction::PROD:
		type= MinisatID::AggType::PROD;
		break;
	case AggFunction::MIN:
		type= MinisatID::AggType::MIN;
		break;
	case AggFunction::MAX:
		type= MinisatID::AggType::MAX;
		break;
	}
	return type;
}

template<typename Solver>
void SolverPolicy<Solver>::polAddAggregate(int definitionID, int head, bool lowerbound, int setnr, AggFunction aggtype, TsType sem, double bound) {
	MinisatID::Aggregate agg;
	agg.sign = lowerbound ? MinisatID::AggSign::LB : MinisatID::AggSign::UB;
	agg.setID = setnr;
	agg.type = convert(aggtype);
	if (_verbosity > 1){
		std::clog <<toString(aggtype) << setnr << ' ';
	}
	switch (sem) {
	case TsType::EQ:
	case TsType::IMPL:
	case TsType::RIMPL:
		agg.sem = MinisatID::AggSem::COMP;
		break;
	case TsType::RULE:
		agg.sem = MinisatID::AggSem::DEF;
		break;
	}
	if (_verbosity > 1)
		std::clog << (lowerbound ? " >= " : " =< ") << bound << "\n";
	agg.defID = definitionID;
	agg.head = createAtom(head);
	agg.bound = createWeight(bound);
	add(getSolver(), agg);
}

template<typename Solver>
void SolverPolicy<Solver>::polAddCPVariables(const std::vector<VarId>& varids, GroundTermTranslator* termtranslator) {
	for (auto it = varids.begin(); it != varids.end(); ++it) {
		polAddCPVariable(*it, termtranslator);
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAddCPVariable(const VarId& varid, GroundTermTranslator* termtranslator) {
	if (_addedvarids.find(varid) == _addedvarids.end()) {
		_addedvarids.insert(varid);
		SortTable* domain = termtranslator->domain(varid);
		Assert(domain);
		Assert(domain->approxFinite());
		if (domain->isRange()) {
			// the domain is a complete range from minvalue to maxvalue.
			MinisatID::CPIntVarRange cpvar;
			cpvar.varID = varid;
			cpvar.minvalue = domain->first()->value()._int;
			cpvar.maxvalue = domain->last()->value()._int;
			if (_verbosity > 0)
				std::clog << "[" << cpvar.minvalue << "," << cpvar.maxvalue << "]";
			add(getSolver(), cpvar);
		} else {
			// the domain is not a complete range.
			MinisatID::CPIntVarEnum cpvar;
			cpvar.varID = varid;
			if (_verbosity > 0)
				std::clog << "{ ";
			for (SortIterator it = domain->sortBegin(); not it.isAtEnd(); ++it) {
				int value = (*it)->value()._int;
				cpvar.values.push_back(value);
				if (_verbosity > 0)
					std::clog << value << "; ";
			}
			if (_verbosity > 0)
				std::clog << "}";
			add(getSolver(), cpvar);
		}
		if (_verbosity > 0)
			std::clog << "\n";
	}
}

template<typename Solver>
void SolverPolicy<Solver>::polAddPCRule(int defnr, int head, std::vector<int> body, bool conjunctive, bool) {
	MinisatID::Rule rule;
	rule.head = createAtom(head);
	for (unsigned int n = 0; n < body.size(); ++n) {
		rule.body.push_back(createLiteral(body[n]));
	}
	rule.conjunctive = conjunctive;
	rule.definitionID = defnr;
	add(getSolver(), rule);
}

template<typename Solver>
void SolverPolicy<Solver>::polAddOptimization(AggFunction function, int setid){
	MinisatID::MinimizeAgg minim;
	minim.setid = setid;
	minim.type = convert(function);
	add(getSolver(), minim);
}

class LazyRuleMon: public MinisatID::LazyGroundingCommand {
private:
	Lit lit;
	ElementTuple args;
	std::vector<DelayGrounder*> grounders;

public:
	LazyRuleMon(const Lit& lit, const ElementTuple& args, const std::vector<DelayGrounder*>& grounders)
			: lit(lit), args(args), grounders(grounders) {
	}

	virtual void requestGrounding() {
		if (not isAlreadyGround()) {
			notifyGrounded();
			for (auto i = grounders.begin(); i < grounders.end(); ++i) {
				(*i)->ground(lit, args);
			}
		}
	}
};

template<>
void SolverPolicy<PCSolver>::polNotifyUnknBound(Context context, const Lit& delaylit, const ElementTuple& args, std::vector<DelayGrounder*> grounders){
	auto mon = new LazyRuleMon(delaylit, args, grounders);
	auto literal = createLiteral(delaylit);
	if(context==Context::POSITIVE){ // In a positive context, should watch when the literal becomes false, or it's negation becomes true
		literal = not literal;
	}
	MinisatID::LazyGroundLit lc(context==Context::BOTH, literal, mon);
	add(getSolver(), lc);
}

typedef cb::Callback1<void, ResidualAndFreeInst*> callbackgrounding;
class LazyClauseMon: public MinisatID::LazyGroundingCommand {
private:
	ResidualAndFreeInst* inst;
	LazyGroundingManager const * const grounder;

public:
	LazyClauseMon(ResidualAndFreeInst* inst, LazyGroundingManager const * const grounder)
			: inst(inst), grounder(grounder) {
	}

	virtual void requestGrounding() {
		if (not isAlreadyGround()) {
			notifyGrounded();
			grounder->notifyDelayTriggered(inst);
		}
	}
};

template<>
void SolverPolicy<PCSolver>::polNotifyLazyResidual(ResidualAndFreeInst* inst, TsType type, LazyGroundingManager const* const grounder) {
	auto mon = new LazyClauseMon(inst, grounder);
	auto watchboth = type==TsType::RULE || type==TsType::EQ;
	auto lit = createLiteral(inst->residual);
	if(type==TsType::RIMPL){
		lit = not lit;
	}
	MinisatID::LazyGroundLit lc(watchboth, lit, mon);
	add(getSolver(), lc);
}

// Explicit instantiations
template class SolverPolicy<PCSolver>;
