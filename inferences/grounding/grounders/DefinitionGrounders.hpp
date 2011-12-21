/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef DEFINITIONGROUNDERS_HPP_
#define DEFINITIONGROUNDERS_HPP_

#include "inferences/grounding/grounders/Grounder.hpp"
#include "GroundUtils.hpp"

#include "groundtheories/GroundTheory.hpp"
#include "groundtheories/SolverPolicy.hpp"

class TermGrounder;
class InstChecker;
class InstanceChecker;
class GroundDefinition;
class FormulaGrounder;
class TermGrounder;
class InstGenerator;
class SetGrounder;
class PredInter;
class Formula;
class SortTable;
class GroundTranslator;
class GroundTermTranslator;
class PFSymbol;

class RuleGrounder;
typedef GroundTheory<SolverPolicy> SolverTheory;

/** Grounder for a definition **/
// NOTE: definition printing code is based on the INVARIANT that a defintion is ALWAYS grounded as contiguous component: never ground def A a bit, then ground B, then return to A again (code should error on this)
// directly printing in idp language will be incorrect then.
// TODO optimize grounding of definitions by grouping rules with the same head and grounding them atom by atom
//	(using approximation to derive given a head query which bodies might be true). This would allow to write out rules without
//	first constructing and storing the full ground definition to remove duplicate heads
class DefinitionGrounder: public Grounder {
private:
	static unsigned int _currentdefnb;
	unsigned int _defnb;
	std::vector<RuleGrounder*> _subgrounders; //!< Grounders for the rules of the definition.
	GroundDefinition* _grounddefinition;
public:
	DefinitionGrounder(AbstractGroundTheory* groundtheory, std::vector<RuleGrounder*> subgr, const GroundingContext& context);
	void run(ConjOrDisj& formula) const;
	unsigned int id() const {
		return _defnb;
	}
};

class HeadGrounder;

/** Grounder for a single rule **/
class RuleGrounder {
private:
	HeadGrounder* _headgrounder;
	FormulaGrounder* _bodygrounder;
	InstGenerator* _headgenerator;
	InstGenerator* _bodygenerator;
	GroundingContext _context;

protected:
	HeadGrounder* headgrounder() const {
		return _headgrounder;
	}
	FormulaGrounder* bodygrounder() const {
		return _bodygrounder;
	}
	InstGenerator* headgenerator() const {
		return _headgenerator;
	}
	InstGenerator* bodygenerator() const {
		return _bodygenerator;
	}
	GroundingContext context() const {
		return _context;
	}

public:
	RuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* hig, InstGenerator* big, GroundingContext& ct);
	virtual ~RuleGrounder(){}
	virtual void run(unsigned int defid, GroundDefinition* grounddefinition) const;
};

/** Grounder for a head of a rule **/
class HeadGrounder {
private:
	AbstractGroundTheory* _grounding;
	std::vector<TermGrounder*> _subtermgrounders;
	const PredTable* _ct;
	const PredTable* _cf;
	unsigned int _symbol;
	std::vector<SortTable*> _tables;
	PFSymbol* _pfsymbol;

public:
	HeadGrounder(AbstractGroundTheory* gt, const PredTable* ct,const PredTable* cf, PFSymbol* s, const std::vector<TermGrounder*>&,
			const std::vector<SortTable*>&);
	Lit run() const;

	const std::vector<TermGrounder*>& subtermgrounders() const {
		return _subtermgrounders;
	}
	PFSymbol* pfsymbol() const {
		return _pfsymbol;
	}
	AbstractGroundTheory* grounding() const {
		return _grounding;
	}
};

class LazyRuleGrounder;

class LazyDefinitionGrounder: public Grounder {
private:
	unsigned int _defnb;
	std::vector<LazyRuleGrounder*> _subgrounders; //!< Grounders for the rules of the definition.
public:
	LazyDefinitionGrounder(AbstractGroundTheory* groundtheory, std::vector<LazyRuleGrounder*> subgr, int verb);
	bool run() const;
	unsigned int id() const {
		return _defnb;
	}
};

class LazyRuleGrounder: public RuleGrounder {
private:
	SolverTheory* _grounding;
	SolverTheory* grounding() const {
		return _grounding;
	}
public:
	LazyRuleGrounder(HeadGrounder* hgr, FormulaGrounder* bgr, InstGenerator* big, GroundingContext& ct);
	void run(unsigned int defid, GroundDefinition* grounddefinition) const;

	void ground(const Lit& head, const ElementTuple& headargs);
	void notify(const Lit& lit, const ElementTuple& headargs, const std::vector<LazyRuleGrounder*>& grounders);

	dominstlist createInst(const ElementTuple& headargs);
};

#endif /* DEFINITIONGROUNDERS_HPP_ */