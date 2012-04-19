/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef SORTINSTGENERATOR_HPP_
#define SORTINSTGENERATOR_HPP_

#include "InstGenerator.hpp"
#include "structure/StructureComponents.hpp"

class InternalSortTable;
class DomElemContainer;

/**
 * Generate a domelem if the input is in the sorttable (TODO is in fact a checker).
 */
class SortChecker: public InstGenerator {
private:
	const InternalSortTable* _table;
	const DomElemContainer* _invar;
	bool _reset;
public:
	SortChecker(const InternalSortTable* t, const DomElemContainer* in);
	SortChecker* clone() const;
	void reset();
	void next();
};

/**
 * Instantiate a given variable with all possible values for the given sort
 */
class SortGenerator: public InstGenerator {
private:
	const InternalSortTable* _table;
	const DomElemContainer* _var;
	SortIterator _curr;
	bool _reset;
public:
	SortGenerator(const InternalSortTable* table, const DomElemContainer* var);
	SortGenerator* clone() const;
	void reset();
	void setVarsAgain();
	void next();
	virtual void put(std::ostream& stream);
};

#endif /* SORTINSTGENERATOR_HPP_ */
