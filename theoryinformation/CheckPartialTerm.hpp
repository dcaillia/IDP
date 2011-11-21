/************************************
  	CheckPartialTerm.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef CHECKPARTIALTERM_HPP_
#define CHECKPARTIALTERM_HPP_

#include "visitors/TheoryVisitor.hpp"

/**
 * Class to implement TermUtils::isPartial
 */
class CheckPartialTerm : public TheoryVisitor {
	private:
		bool			_result;

		void visit(const VarTerm* ) { }
		void visit(const DomainTerm* ) { }
		void visit(const AggTerm* ) { }	// NOTE: we are not interested whether contains 
										// partial functions. So we don't visit it recursively.

		void visit(const FuncTerm* ft) {
			if(ft->function()->partial()) { 
				_result = true; 
				return;	
			}
			else {
				for(size_t argpos = 0; argpos < ft->subterms().size(); ++argpos) {
					if(not SortUtils::isSubsort(ft->subterms()[argpos]->sort(),ft->function()->insort(argpos))) {
						_result = true;
						return;
					}
				}
				TheoryVisitor::traverse(ft);
			}
		}

	public:
		bool run(Term* t) {
			_result = false;
			t->accept(this);
			return _result;
		}
};

#endif /* CHECKPARTIALTERM_HPP_ */