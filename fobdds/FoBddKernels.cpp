/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittockx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "fobdds/FoBddAtomKernel.hpp"
#include "fobdds/FoBddQuantKernel.hpp"
#include "fobdds/FoBddVisitor.hpp"
#include "fobdds/FoBddKernel.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddTerm.hpp"

using namespace std;

bool FOBDDQuantKernel::containsDeBruijnIndex(unsigned int index) const {
	return _bdd->containsDeBruijnIndex(index+1);
}

bool FOBDDAtomKernel::containsDeBruijnIndex(unsigned int index) const {
	for (size_t n = 0; n < _args.size(); ++n) {
		if (_args[n]->containsDeBruijnIndex(index)) {
			return true;
		}
	}
	return false;
}

void FOBDDAtomKernel::accept(FOBDDVisitor* v) const {
	v->visit(this);
}
void FOBDDQuantKernel::accept(FOBDDVisitor* v) const {
	v->visit(this);
}

const FOBDDKernel* FOBDDAtomKernel::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}
const FOBDDKernel* FOBDDQuantKernel::acceptchange(FOBDDVisitor* v) const {
	return v->change(this);
}
