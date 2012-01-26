/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef UNNESTTERMS_HPP_
#define UNNESTTERMS_HPP_

#include "IncludeComponents.hpp"
#include "visitors/TheoryMutatingVisitor.hpp"

class AbstractStructure;
class Vocabulary;
class Formula;
class Variable;
class Sort;
class Term;

/**
 * Moves nested terms out
 *
 * NOTE: equality is NOT rewritten! (rewriting f(x)=y to ?z: f(x)=z & z=y is quite useless)
 */
class UnnestTerms: public TheoryMutatingVisitor {
	VISITORFRIENDS()
protected:
	AbstractStructure* _structure; //!< Used to find bounds on introduced variables for aggregates
	Vocabulary* _vocabulary; //!< Used to do type derivation during rewrites

private:
	Context _context; //!< Keeps track of the current context where terms are moved
	bool _allowedToUnnest; // Indicates whether in the current context, it is allowed to unnest terms
	std::vector<Formula*> _equalities; //!< used to temporarily store the equalities generated when moving terms
	std::set<Variable*> _variables; //!< used to temporarily store the freshly introduced variables
	Sort* _chosenVarSort;

	void contextProblem(Term* t);

protected:
	virtual bool shouldMove(Term* t);

	bool getAllowedToUnnest() const {
		return _allowedToUnnest;
	}
	void setAllowedToUnnest(bool allowed) {
		_allowedToUnnest = allowed;
	}
	const Context& getContext() const {
		return _context;
	}
	void setContext(const Context& context) {
		_context = context;
	}

public:
	UnnestTerms();

	template<typename T>
	T execute(T t, Context con = Context::POSITIVE, AbstractStructure* str = NULL, Vocabulary* voc = NULL) {
		_context = con;
		_structure = str;
		_vocabulary = voc;
		_allowedToUnnest = false;
		return t->accept(this);
	}

protected:
	Formula* rewrite(Formula*);

	VarTerm* move(Term*);

	Theory* visit(Theory*);
	virtual Rule* visit(Rule*);
	virtual Formula* traverse(Formula*);
	virtual Formula* traverse(PredForm*);
	virtual Formula* visit(EquivForm*);
	virtual Formula* visit(AggForm*);
	virtual Formula* visit(EqChainForm*);
	virtual Formula* visit(PredForm*);
	virtual Term* traverse(Term*);
	VarTerm* visit(VarTerm*);
	virtual Term* visit(DomainTerm*);
	virtual Term* visit(AggTerm*);
	virtual Term* visit(FuncTerm*);
	virtual SetExpr* visit(EnumSetExpr*);
	virtual SetExpr* visit(QuantSetExpr*);

private:
	template<typename T>
	Formula* doRewrite(T origformula);

	Sort* deriveSort(Term*);
};

#endif /* UNNESTTERMS_HPP_ */