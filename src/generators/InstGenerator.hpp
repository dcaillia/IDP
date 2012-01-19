/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#ifndef INSTGENERATOR_HPP_
#define INSTGENERATOR_HPP_

#include <typeinfo>
#include <sstream>

enum class Pattern {
	INPUT, OUTPUT
};

class InstChecker {
public:
	virtual ~InstChecker() {}

	// FIXME Checker should only be created if there are no output variables
	virtual bool check() = 0;

	// NOTE: should be a deep clone
	virtual InstChecker* clone() const = 0; // FIXME need to reimplemnt some as a deep clone!

	virtual void put(std::ostream& stream);
};

class InstGenerator: public InstChecker {
private:
	bool end;
protected:
	void notifyAtEnd() {
		end = true;
	}

	// Semantics: next should set the variables to a NOT-YET-SEEN instantiation.
	//		the combination of reset + next should set the variables to the first matching instantation.
	//	No assumption is made about calling reset on its own.
	//	Next will never be called when already at end.
	virtual void next() = 0;
	virtual void reset() = 0;

public:
	virtual ~InstGenerator() {
	}

	virtual bool check() { // TODO probably do not need to be virtual any longer
		begin();
		return not isAtEnd();
	}

	// Can also be used for resets
	// SETS the instance to the FIRST value if it exists
	void begin();

	/**
	 * Returns true if the last element has already been set as an instance
	 */
	bool isAtEnd() const {
		return end;
	}

	void operator++();

	virtual InstGenerator* clone() const = 0;
};

#endif /* INSTGENERATOR_HPP_ */
