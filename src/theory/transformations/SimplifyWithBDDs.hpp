/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef SIMPLIFYWITHBDDDS5346546_HPP_
#define SIMPLIFYWITHBDDDS5346546_HPP_

class Theory;
class Formula;



class SimplifyWithBdds{
private:
public:
	//NOTE: if the structure is given, more efficient unnesting can happen (deriving of term bounds)
	SimplifyWithBdds();

	//Simplifies all formulas by making a BDD from them and then transforming it back to formulas
	Theory* execute(Theory* t);
	Formula* execute(Formula* f);
};
#endif /* SIMPLIFYWITHBDDDS5346546_HPP_ */