/************************************
 LazyQuantGrounder.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef LAZYQUANTGROUNDER_HPP_
#define LAZYQUANTGROUNDER_HPP_

#include "ground.hpp"
#include "grounders/FormulaGrounders.hpp"

#include <map>

typedef GroundTheory<SolverPolicy> SolverTheory;

class LazyQuantGrounder : public QuantGrounder {
private:
	static unsigned int maxid;
	unsigned int id_;
	mutable bool negatedclause_;
	SolverTheory* groundtheory_;
	mutable std::queue<ResidualAndFreeInst *> queuedtseitinstoground;
	mutable bool grounding;
	const std::set<Variable*> freevars;

	public:
#warning only create in cases where it reduces to an existential quantifier!!!!
#warning currently only non-defined!
	LazyQuantGrounder(const std::set<Variable*>& freevars,
						SolverTheory* groundtheory,
						GroundTranslator* gt,
						FormulaGrounder* sub,
						SIGN sign,
						QUANT q,
						InstGenerator* gen,
						const GroundingContext& ct):
			QuantGrounder(gt,sub,sign, q, gen,ct),
			id_(maxid++),
			negatedclause_(false),
			groundtheory_(groundtheory), // FIXME is there a reason why this is not part of every grounder, but translator always is?
			grounding(false),
			freevars(freevars){
	}

	// TODO for some reason, the callback framework does not compile when using the const method groundmore directly.
	void requestGroundMore(ResidualAndFreeInst* instance);
	void groundMore() const;

	unsigned int id() const { return id_; }

	void notifyTheoryOccurence(ResidualAndFreeInst* instance) const;

protected:
	virtual void	run(litlist&, bool negatedclause = true)	const;
};

#endif /* LAZYQUANTGROUNDER_HPP_ */
