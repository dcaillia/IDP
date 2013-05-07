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

#include "LTCTheorySplitter.hpp"
#include "data/LTCData.hpp"
#include "IncludeComponents.hpp"
#include "data/SplitLTCTheory.hpp"
#include "ReplaceLTCSymbols.hpp"
#include "data/StateVocInfo.hpp"
#include "theory/TheoryUtils.hpp"
#include "errorhandling/error.hpp"
#include "utils/ListUtils.hpp"
#include "creation/cppinterface.hpp"

SplitLTCTheory* LTCTheorySplitter::SplitTheory(const AbstractTheory* ltcTheo) {
	auto g = LTCTheorySplitter();
	if (not isa<const Theory>(*ltcTheo)) {
		throw IdpException("Can only perform progression on theories");
	}
	auto theo = dynamic_cast<const Theory*>(ltcTheo);
	return g.split(theo);
}

LTCTheorySplitter::LTCTheorySplitter()
		: 	_initialTheory(NULL),
			_bistateTheory(NULL),
			_ltcVoc(NULL),
			_time(NULL),
			_start(NULL),
			_next(NULL),
			_vocInfo(NULL) {
}
LTCTheorySplitter::~LTCTheorySplitter() {

}
template<class T>
LTCFormulaInfo LTCTheorySplitter::info(T* t) {
	LTCFormulaInfo result;
	auto symbols = FormulaUtils::collectSymbols(t);

	result.containsStart = contains(symbols, _start);

	result.containsNext = contains(symbols, _next);

	auto vars = FormulaUtils::collectQuantifiedVariables(t, true);
	result.hasTimeVariable = false;
	for (auto tuple : vars) {
		auto var = tuple.first;
		if (var->sort() == _time) {
			result.hasTimeVariable = true;
		}
	}

	return result;
}

void LTCTheorySplitter::createTheories(const Theory* theo) {
	_initialTheory = new Theory(theo->name() + "_init", _vocInfo->stateVoc, theo->pi());
	_bistateTheory = new Theory(theo->name() + "_bistate", _vocInfo->biStateVoc, theo->pi());

	auto workingTheo = theo->clone();
	FormulaUtils::removeEquivalences(workingTheo);
	FormulaUtils::pushNegations(workingTheo);
	FormulaUtils::flatten(workingTheo);
	checkQuantificationsForTheory(workingTheo);

	/*
	 * For the initial theory:
	 * * we remove all bistate formulas.
	 * * We keep static formulas
	 * * We remove formulas over two states
	 * * We instantiate one-state formulas with the state-predicate
	 *
	 * For the bistate theory:
	 * * we remove all bistate formulas.
	 * * We keep static formulas
	 * * We remove formulas over two states
	 * * We instantiate one-state formulas with the state-predicate
	 */
	for (auto sentence : workingTheo->sentences()) {
		handleAndAddToConstruct(sentence, _initialTheory, _bistateTheory);
	}
	for (auto def : workingTheo->definitions()) {
		auto initDef = new Definition();
		auto biStateDef = new Definition();
		for (auto rule : def->rules()) {
			auto head = rule->head();
			auto body = rule->body();
			auto headinfo = info(head);
			auto bodyinfo = info(body);
			if (bodyinfo.hasTimeVariable && not headinfo.hasTimeVariable) {
				throw IdpException(
						"In LTC theories, it is not allowed to define static predicates in terms of dynamic predicates. This occurs in rule " + toString(rule));
			}
			if (bodyinfo.containsNext && not headinfo.containsNext) {
				throw IdpException(
						"In LTC theories, it is not allowed to define the state at time $t$ in terms of next(t). This occurs in rule " + toString(rule));
			}
			handleAndAddToConstruct(rule, initDef, biStateDef);
		}
		auto defsymbols = def->defsymbols();
		auto initDefsymbols = initDef->defsymbols();
		auto biDefsymbols = biStateDef->defsymbols();

		//Add false defined symbols: for defined symbols that no longer have a rule now, we add a rule defining it false.
		for (auto sym : defsymbols) {
			if (contains(_vocInfo->LTC2State, sym)) {
				auto statesym = _vocInfo->LTC2State.at(sym);
				auto nextsym = _vocInfo->LTC2NextState.at(sym);
				if (not contains(initDefsymbols, statesym)) {
					initDef->add(DefinitionUtils::falseRule(statesym));
				}
				if (not contains(biDefsymbols, nextsym)) {
					biStateDef->add(DefinitionUtils::falseRule(nextsym));
				}
			}
		}

		if (initDef->rules().size() != 0) {
			_initialTheory->add(initDef);
		} else {
			delete (initDef);
		}
		if (biStateDef->rules().size() != 0) {
			_bistateTheory->add(biStateDef);
		} else {
			delete (biStateDef);
		}
	}

	workingTheo->recursiveDelete();
}

template<class Form, class Construct>
void LTCTheorySplitter::handleAndAddToConstruct(Form* sentence, Construct* initConstruct, Construct* biStateConstruct) {
	auto sentenceInfo = info(sentence);
	auto newSentence = sentence->clone();
	//We already checked quantifications. They are okay now. First, we remove all quantifications over time,
	//later, we will check which type of formula we are dealing with and handle it appropriately.
	newSentence = FormulaUtils::removeQuantificationsOverSort(newSentence, _time);

	if (sentenceInfo.containsStart) {
		if (sentenceInfo.containsNext) {
			throw IdpException("LTC sentences containing start cannot contain the next-function.");
		}
		if (sentenceInfo.hasTimeVariable) {
			throw IdpException("LTC sentences containing start cannot also have a time variable.");
		}
		newSentence = ReplaceLTCSymbols::replaceSymbols(newSentence, _ltcVoc, false);
		initConstruct->add(newSentence);
	} else if (sentenceInfo.containsNext) {
		Assert(not sentenceInfo.containsStart);
		//Should be guaranteed by previous case
		Assert(sentenceInfo.hasTimeVariable);
		//Don't know what else could be filled in here.
		newSentence = ReplaceLTCSymbols::replaceSymbols(newSentence, _ltcVoc, false);
		biStateConstruct->add(newSentence);
	} else if (sentenceInfo.hasTimeVariable) {
		auto newNextSentence = newSentence->clone();
		//This kind of sentences needs to be added to both theories. For the initial theory, this sentence constraints the initial state
		//For bistate theory: this sentence constraints the next state.
		newSentence = ReplaceLTCSymbols::replaceSymbols(newSentence, _ltcVoc, false);
		newNextSentence = ReplaceLTCSymbols::replaceSymbols(newNextSentence, _ltcVoc, true);
		initConstruct->add(newSentence);
		biStateConstruct->add(newNextSentence);
	} else {
		//Static formulas are added both to Init and next state. TODO we might also leave it out of the bistate formula.
		auto newNextSentence = newSentence->clone();
		initConstruct->add(newSentence);
		biStateConstruct->add(newNextSentence);
	}
}

void LTCTheorySplitter::initializeVariables(const Theory* theo) {
	_ltcVoc = theo->vocabulary();
	auto data = LTCData::instance();
	Assert(data->hasBeenTransformed(_ltcVoc));
	_vocInfo = data->getStateVocInfo(_ltcVoc);
	_time = _vocInfo->time;
	_start = _vocInfo->start;
	_next = _vocInfo->next;

	Assert(_time != NULL);
	Assert(_start != NULL);
	Assert(_next != NULL);

}

SplitLTCTheory* LTCTheorySplitter::split(const Theory* theo) {
	Assert(theo != NULL);

	initializeVariables(theo);
	createTheories(theo);

	auto result = new SplitLTCTheory();
	result->initialTheory = _initialTheory;
	result->bistateTheory = _bistateTheory;
	if (getOption(IntType::VERBOSE_TRANSFORMATIONS) > 0) {
		std::clog << "Splitting the LTC theory\n" << toString(theo) << "\nresulted in the following two theories: \n" << toString(_initialTheory) << "\n"
				<< toString(_bistateTheory) << "\n";
	}
	return result;
}

template<class T>
void LTCTheorySplitter::checkQuantifications(T* t) {
	auto vars = FormulaUtils::collectQuantifiedVariables(t, true);
	auto topLevelVars = FormulaUtils::collectQuantifiedVariables(t, false);

	bool timefound = false;
	for (auto tuple : vars) {
		auto var = tuple.first;
		if (var->sort() == _time) {
			if (timefound) {
				std::stringstream ss;
				ss << "LTC theories can only contain one time variable in every sentence/formula.";
				throw IdpException(ss.str());
			}
			timefound = true;
			if (tuple.second != QuantType::UNIV) {
				std::stringstream ss;
				ss << "In LTC theories, every variable over type Time should be universally quantified" << " (given its context). This is violated by variable "
						<< toString(var) << " in " << toString(t) << "\n" << " At " << print(t->pi());
				throw IdpException(ss.str());
			}
			if (not contains(topLevelVars, var)) {
				std::stringstream ss;
				ss << "In LTC theories, every variable over type Time should be universally quantified at the toplevel."
						<< " E.g. quantifications such as ? x[type]: ! t[Time] ... are not allowed." << " This is violated by variable  " << toString(var)
						<< " in " << toString(t) << "\n" << " At " << print(t->pi());
				throw IdpException(ss.str());
			}
		}
	}
}
void LTCTheorySplitter::checkQuantificationsForTheory(const Theory* theo) {
	for (auto sentence : theo->sentences()) {
		checkQuantifications(sentence);
	}
	for (auto def : theo->definitions()) {
		for (auto rule : def->rules()) {
			checkQuantifications(rule);
		}
	}
}

