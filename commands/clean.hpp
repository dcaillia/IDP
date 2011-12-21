/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef CLEAN_HPP_
#define CLEAN_HPP_

#include <vector>
#include "commandinterface.hpp"
#include "structure.hpp"

class CleanInference: public TypedInference<LIST(AbstractStructure*)> {
public:
	CleanInference(): TypedInference("clean", "Combines fully specified three-valued relations into two-valued ones.") {
	}

	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		get<0>(args)->clean();
		return nilarg();
	}
};

#endif /* CLEAN_HPP_ */
