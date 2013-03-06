/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum 
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

#include <vector>
#include <map>
#include "visitors/TheoryMutatingVisitor.hpp"

class Variable;
class PFSymbol;

class AddCompletion: public TheoryMutatingVisitor {
	VISITORFRIENDS()
private:
	std::vector<Formula*> _sentences;
	std::map<PFSymbol*, std::vector<Variable*> > _headvars;
	std::map<PFSymbol*, std::vector<Formula*> > _symbol2sentences;

public:
	template<typename T>
	T execute(T t) {
		return t->accept(this);
	}

protected:
	Theory* visit(Theory*);
	Definition* visit(Definition*);
	Rule* visit(Rule*);
};
