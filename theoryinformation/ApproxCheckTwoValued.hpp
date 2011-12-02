#ifndef APPROXCHECKTWOVALUED_HPP_
#define APPROXCHECKTWOVALUED_HPP_

#include "common.hpp"
#include "visitors/TheoryVisitor.hpp"

class AbstractStructure;

class ApproxCheckTwoValued : public TheoryVisitor {
	private:
		AbstractStructure*	_structure;
		bool				_returnvalue;
	public:
		ApproxCheckTwoValued(AbstractStructure* str) : _structure(str), _returnvalue(true) { }
		bool	returnvalue()	const { return _returnvalue;	}
		void	visit(const PredForm*);
		void	visit(const FuncTerm*);
		void 	visit(const SetExpr*);
};

#endif /* APPROXCHECKTWOVALUED_HPP_ */
