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

#include "theory/information/GetQuantifiedVariables.hpp"
#include "data/LTCFormulaInfo.hpp"
#include <map>

class SplitLTCTheory;
class SplitLTCInvariant;
class Sort;
class Variable;
class AbstractTheory;
class Theory;
class QuantForm;
class QuantSetExpr;
class LTCVocInfo;
class Vocabulary;

class LTCTheorySplitter {

private:
	Theory* _initialTheory;
	Theory* _bistateTheory;

	//Store the LTC inputdata for ease of use
	Vocabulary* _ltcVoc;
	const Sort* _time;
	Function* _start;
	Function* _next;
	const LTCVocInfo* _vocInfo;

public:
	/*
	 *
	 */
	static SplitLTCTheory* SplitTheory(const AbstractTheory* ltcTheo);
	static SplitLTCInvariant* SplitInvariant(const AbstractTheory* invar);

private:

	LTCTheorySplitter();
	~LTCTheorySplitter();

	SplitLTCTheory* split(const Theory*);
	SplitLTCInvariant* splitInvar(const Theory*);

	/**
	 * Checks whether all time-variables
	 * are universally quantified and that only one quantification over time
	 * is used in every sentence/rule.
	 * Correctness of this method depends on the fact that negations are pushed!
	 */
	void checkQuantificationsForTheory(const Theory* theo);

	template<class T>
	void checkQuantifications(T* t);
	/** Splits the LTC theory/invariant. If invar==true, extra checks are performed to check that it is really an invariant and the invariant formulas are created*/
	void createTheories(const Theory* theo, bool invar);
	void initializeVariables(const Theory* theo);

	template<class T>
	LTCFormulaInfo info(T* t);

	/** Splits the LTC theory/invariant. If invar==true, extra checks are performed to check that it is really an invariant and the invariant formulas are created*/
	template<class Form, class Construct>
	void handleAndAddToConstruct(Form* sentence, Construct* initConstruct, Construct* biStateConstruct, bool invar);
};

