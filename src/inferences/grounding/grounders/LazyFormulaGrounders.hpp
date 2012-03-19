/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef LAZYQUANTGROUNDER_HPP_
#define LAZYQUANTGROUNDER_HPP_

#include "FormulaGrounders.hpp"

class LazyGroundingManager {
private:
	mutable bool currentlyGrounding; // If true, groundMore is currently already on the stack, so do not call it again!
	mutable std::queue<ResidualAndFreeInst *> queuedtseitinstoground; // Stack of what we still have to ground

public:
	LazyGroundingManager()
			: currentlyGrounding(false) {
	}
	void groundMore() const;

	void notifyDelayTriggered(ResidualAndFreeInst* instance) const;
};

class LazyGrounder: public ClauseGrounder {
private:
	const std::set<Variable*> freevars; // The freevariables according to which we have to ground
	LazyGroundingManager lazyManager;
	mutable tablesize alreadyground;
public:
	LazyGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, SIGN sign, bool conj, const GroundingContext& ct);
	virtual ~LazyGrounder() {
	}
	bool groundMore(ResidualAndFreeInst* instance) const;

	virtual tablesize getGroundedSize() const;

protected:
	virtual void internalRun(ConjOrDisj& formula) const;
	virtual bool grounderIsEmpty() const = 0;
	virtual void initializeInst(ResidualAndFreeInst* inst) const = 0;
	virtual Grounder* getLazySubGrounder(ResidualAndFreeInst* instance) const = 0;
	virtual void increment(ResidualAndFreeInst* instance) const = 0;
	virtual bool isAtEnd(ResidualAndFreeInst* instance) const = 0;
	virtual void initializeGroundMore(ResidualAndFreeInst*) const {
	}

	virtual tablesize getSubGroundingSize() const = 0;
};

class LazyQuantGrounder: public LazyGrounder {
private:
	FormulaGrounder* _subgrounder;
	InstGenerator* _generator;
	InstChecker* _checker;
public:
	LazyQuantGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, FormulaGrounder* sub, SIGN sign, QUANT q, InstGenerator* gen,
			InstChecker* checker, const GroundingContext& ct, const tablesize& quantunivsize);

protected:
	virtual bool grounderIsEmpty() const;
	virtual void initializeInst(ResidualAndFreeInst* inst) const;
	virtual Grounder* getLazySubGrounder(ResidualAndFreeInst* instance) const;
	virtual void increment(ResidualAndFreeInst* instance) const;
	virtual bool isAtEnd(ResidualAndFreeInst* instance) const;
	virtual void initializeGroundMore(ResidualAndFreeInst* instance) const;

	virtual tablesize getSubGroundingSize() const;

	FormulaGrounder* getSubGrounder() const {
		return _subgrounder;
	}
};

class LazyBoolGrounder: public LazyGrounder {
private:
	std::vector<Grounder*> _subgrounders;
public:
	LazyBoolGrounder(const std::set<Variable*>& freevars, AbstractGroundTheory* groundtheory, std::vector<Grounder*> sub, SIGN sign, bool conj,
			const GroundingContext& ct);

protected:
	virtual bool grounderIsEmpty() const;
	virtual void initializeInst(ResidualAndFreeInst* inst) const;
	virtual Grounder* getLazySubGrounder(ResidualAndFreeInst* instance) const;
	virtual void increment(ResidualAndFreeInst* instance) const;
	virtual bool isAtEnd(ResidualAndFreeInst* instance) const;

	virtual tablesize getSubGroundingSize() const;

	const std::vector<Grounder*>& getSubGrounders() const {
		return _subgrounders;
	}
};

class DelayGrounder {
private:
	int _id;
	Context _context;

	bool _isGrounding;
	std::queue<std::pair<Lit, ElementTuple>> _stilltoground;

	AbstractGroundTheory* _grounding;

	std::vector<std::pair<int, int> > sameargs; // a list of indices into the head terms which are the same variables

public:
	// @precondition: two IDs HAVE to be different if referring to an instance of a symbol in a DIFFERENT definition (if it is a head)
	//		it HAS to be -1 if it is not a head occurrence
	// 		in all other cases, they should preferably be equal
	DelayGrounder(PFSymbol* symbol, const std::vector<Term*>& terms, Context context, int id, AbstractGroundTheory* gt);
	virtual ~DelayGrounder() {
	}

	void ground(const Lit& boundlit, const ElementTuple& args);
	void notify(const Lit& boundlit, const ElementTuple& args, const std::vector<DelayGrounder*>& grounders);

	int getID() const {
		return _id;
	}
	Context getContext() const {
		return _context;
	}

protected:
	AbstractGroundTheory* getGrounding() const {
		return _grounding;
	}

	const std::vector<std::pair<int, int> >& getSameargs() const{
		return sameargs;
	}

	void doGrounding();
	virtual void doGround(const Lit& boundlit, const ElementTuple& args) = 0;
};

class LazyUnknUnivGrounder: public FormulaGrounder, public DelayGrounder {
private:
	bool _isGrounding;
	std::vector<const DomElemContainer*> _varcontainers;
	std::queue<std::pair<Lit, ElementTuple>> _stilltoground;

	FormulaGrounder* _subgrounder;

public:
	LazyUnknUnivGrounder(const PredForm* pf, Context context, const var2dommap& varmapping, AbstractGroundTheory* groundtheory, FormulaGrounder* sub,
			const GroundingContext& ct);

	virtual void run(ConjOrDisj& formula) const;

	tablesize getGroundedSize() const{
		return tablesize(TableSizeType::TST_UNKNOWN, 0); // TODO
	}

protected:
	FormulaGrounder* getSubGrounder() const {
		return _subgrounder;
	}

	dominstlist createInst(const ElementTuple& args);
	void doGround(const Lit& boundlit, const ElementTuple& args);
};

class LazyTwinDelayUnivGrounder: public FormulaGrounder, public DelayGrounder {
private:
	std::vector<ElementTuple> _seen;

	bool _isGrounding;
	std::vector<const DomElemContainer*> _varcontainers;
	std::queue<std::pair<Lit, ElementTuple>> _stilltoground;

	FormulaGrounder* _subgrounder;

public:
	LazyTwinDelayUnivGrounder(PFSymbol* symbol, const std::vector<Term*>& terms, Context context, const var2dommap& varmapping, AbstractGroundTheory* groundtheory,
			FormulaGrounder* sub, const GroundingContext& ct);

	tablesize getGroundedSize() const{
		return tablesize(TableSizeType::TST_UNKNOWN, 0); // TODO
	}

	virtual void run(ConjOrDisj& formula) const;

protected:
	FormulaGrounder* getSubGrounder() const {
		return _subgrounder;
	}

	dominstlist createInst(const ElementTuple& args);
	void doGround(const Lit& boundlit, const ElementTuple& args);
};

#endif /* LAZYQUANTGROUNDER_HPP_ */
