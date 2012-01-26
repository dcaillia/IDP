/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef FWSTRUCTURE_HPP_
#define FWSTRUCTURE_HPP_

#include <vector>

class DomElemContainer;
class DomainElement;

class Compound;
class DomainAtom;

typedef std::vector<const DomainElement*> ElementTuple;
typedef std::vector<ElementTuple> ElementTable;

#endif /* FWSTRUCTURE_HPP_ */