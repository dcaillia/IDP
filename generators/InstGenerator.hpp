/************************************
 Generators.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef INSTGENERATOR_HPP_
#define INSTGENERATOR_HPP_

class InstChecker{
public:
	// FIXME Checker should only be created if there are no output variables
	virtual bool check() = 0;
};

class InstGenerator: public InstChecker {
private:
	bool end;
protected:
	void notifyAtEnd(){
		end = true;
	}

	// Semantics: next should set the variables to a NOT-YET-SEEN instantiation.
	//		the combination of reset + next should set the variables to the first matching instantation.
	//	No assumption is made about calling reset on its own.
	//	Next will never be called when already at end.
	virtual void next() = 0;
	virtual void reset() = 0;

public:
	virtual ~InstGenerator(){}

	virtual bool check() {
		begin();
		return isAtEnd();
	}

	// Can also be used for resets
	// SETS the instance to the FIRST value if it exists
	void begin(){
		end = false;
		reset();
		if(not isAtEnd()){
			next();
		}
	}

	/**
	 * Returns true if the last element has already been set as an instance
	 */
	bool isAtEnd() const { return end; }

	void operator++() {
		assert(not isAtEnd());
		next();
	}
};

#endif /* INSTGENERATOR_HPP_ */
