/************************************
	simplify.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SIMPLIFY_HPP_
#define SIMPLIFY_HPP_

#include <vector>
#include "commandinterface.hpp"

class SimplifyInference : public Inference {
public:
	SimplifyInference() : Inference("simplify") {
		add(AT_QUERY);
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		Query* q = args[0]._value._query;

		// Translate the query to a bdd
		FOBDDManager manager;
		FOBDDFactory factory(&manager);
		q->query()->accept(&factory);
		const FOBDD* bdd = factory.bdd();
		
		// Simplify the bdd
		const FOBDD* simplifiedbdd = manager.simplify(bdd);

		// Return the result
		std::stringstream sstr;
		manager.put(sstr,simplifiedbdd);
		InternalArgument ia(StringPointer(sstr.str()));
		return ia;
	}
};

#endif /* SIMPLIFY_HPP_ */
