/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "IncludeComponents.hpp"
#include "parseinfo.hpp"
#include "errorhandling/error.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddSetExpr.hpp"

#include "Assert.h"

#include "GeneratorFactory.hpp"

#include "BDDBasedGeneratorFactory.hpp"

#include "SimpleFuncGenerator.hpp"
#include "UnionGenerator.hpp"
#include "TreeInstGenerator.hpp"
#include "EnumLookupGenerator.hpp"
#include "UnionGenerator.hpp"
#include "SortGenAndChecker.hpp"
#include "TableCheckerAndGenerators.hpp"
#include "ComparisonGenerator.hpp"
#include "SimpleFuncGenerator.hpp"
#include "BasicCheckersAndGenerators.hpp"
#include "BinaryArithmeticOperatorsGenerator.hpp"
#include "BinaryArithmeticOperatorsChecker.hpp"
#include "InverseUnaFunctionGenerator.hpp"
#include "UnaryArithmeticOperators.hpp"
#include "fobdds/FoBddVariable.hpp"

#include "structure/StructureComponents.hpp"
using namespace std;

// NOTE original can be NULL
template<typename Table>
bool isCertainlyFinite(Table t) {
	return t->finite();
}

InstGenerator* GeneratorFactory::create(const vector<const DomElemContainer*>& vars, const vector<SortTable*>& tabs, const Formula*) {
	Assert(vars.size()==tabs.size());
	if (vars.size() == 0) {
		return new FullGenerator(); // TODO check if this is always correct?
	}
	InstGenerator* gen = NULL;
	InstGenerator* node = NULL;
	auto jt = tabs.crbegin();
	bool certainlyfinite = true;
	for (auto it = vars.crbegin(); it != vars.crend(); ++it, ++jt) {
		if (not isCertainlyFinite((*jt)->internTable())) {
			certainlyfinite = false;
		}
		auto tig = new SortGenerator((*jt)->internTable(), *it);
		if (vars.size() == 1) {
			gen = tig;
			break;
		} else if (it == vars.rbegin()) {
			node = tig;
		} else {
			node = new OneChildGenerator(tig, node);
		}
	}
	if (gen == NULL) {
		gen = node;
	}
	if (not certainlyfinite) {
		gen->notifyIsInfiniteGenerator();
	}
	return gen;
}

InstGenerator* GeneratorFactory::create(const PredTable* pt, const vector<Pattern>& pattern, const vector<const DomElemContainer*>& vars,
		const Universe& universe) {
	GeneratorFactory factory;
	Assert(universe.tables().size()== pattern.size());
	// Check for infinite grounding
	bool certainlyfinite = true;
	for (size_t i = 0; i < universe.tables().size() && certainlyfinite; ++i) {
		if (pattern[i] == Pattern::OUTPUT) {
			if (not isCertainlyFinite(universe.tables()[i])) {
				certainlyfinite = false;
			}
		}
	}
	auto gen = factory.internalCreate(pt, pattern, vars, universe);
	if (not certainlyfinite) {
		gen->notifyIsInfiniteGenerator();
	}
	return gen;
}

InstGenerator* GeneratorFactory::create(const PFSymbol* symbol, const AbstractStructure* structure, bool inverse, const vector<Pattern>& pattern,
		const vector<const DomElemContainer*>& vars, const Universe& universe) {
	const PredTable* table = NULL;
	if (symbol->isPredicate()) {
		auto predicate = dynamic_cast<const Predicate*>(symbol);
		PredInter* inter;
		if (predicate->type() == ST_NONE) {
			inter = structure->inter(predicate);
		} else {
			inter = structure->inter(predicate->parent());
		}
		switch (predicate->type()) {
		case ST_NONE:
			table = inverse ? inter->pf() : inter->ct();
			break;
		case ST_CT:
			table = inverse ? inter->pf() : inter->ct();
			break;
		case ST_CF:
			table = inverse ? inter->pt() : inter->cf();
			break;
		case ST_PT:
			table = inverse ? inter->cf() : inter->pt();
			break;
		case ST_PF:
			table = inverse ? inter->ct() : inter->pf();
			break;
		}
	} else {
		Assert(symbol->isFunction());
		auto inter = structure->inter(dynamic_cast<const Function*>(symbol))->graphInter();
		table = inverse ? inter->cf() : inter->ct();
	}
	auto tablegenerator = GeneratorFactory::create(table, pattern, vars, universe);

	if (not inverse || vars.size() == 0) {
		return tablegenerator;
	}

	//If the universe does not match the universe of the predicate symbol,
	//And if we are generating all false instances,
	//we must also generate all out-of-bounds tuples.

	InstGenerator* univgenerator = NULL;

	auto predsorts = symbol->sorts();
	for (size_t i = 0; i < vars.size(); i++) {
		if (pattern[i] == Pattern::OUTPUT) {
			if (univgenerator != NULL) {
				univgenerator = new OneChildGenerator(univgenerator, new SortGenerator(universe.tables()[i]->internTable(), vars[i]));
			} else {
				univgenerator = new SortGenerator(universe.tables()[i]->internTable(), vars[i]);
			}
		}
	}

	InstGenerator* outofboundsgenerator = univgenerator;
	if (outofboundsgenerator == NULL) { //no output
		Assert(vars.size()>0);
		outofboundsgenerator = new FullGenerator(); //We know vars.size > 0, thus we will at least do one "sortchecker" and this fullgenerator is safe.
	}

	for (size_t i = 0; i < vars.size(); i++) {
		auto sortchecker = new SortChecker(structure->inter(predsorts[i])->internTable(), vars[i]);
		outofboundsgenerator = new TwoChildGenerator(sortchecker, outofboundsgenerator, new FullGenerator(), new EmptyGenerator());
	}

	std::vector<InstGenerator*> generators = { tablegenerator, outofboundsgenerator };
	auto tablechecker = GeneratorFactory::create(table, std::vector<Pattern>(pattern.size(), Pattern::INPUT), vars, universe);

	std::vector<InstGenerator*> checkers = { tablechecker, new FullGenerator() };

	return new UnionGenerator(generators, checkers);

}

InstGenerator* GeneratorFactory::internalCreate(const PredTable* pt, vector<Pattern> pattern, const vector<const DomElemContainer*>& vars,
		const Universe& universe) {
	Assert(pt->arity()==pattern.size());
	Assert(pattern.size()==vars.size());
	Assert(pattern.size()==universe.tables().size());

	_table = pt;
	_pattern = pattern;
	_vars = vars;
	_universe = universe;

	for (size_t n = 0; n < _vars.size(); ++n) {
		_firstocc.push_back(n);
		for (size_t m = 0; m < n; ++m) {
			if (_vars[n] == _vars[m]) {
				_firstocc[n] = m;
				break;
			}
		}
	}

	size_t firstout = 0;
	for (; firstout < pattern.size(); ++firstout) {
		if (pattern[firstout] == Pattern::OUTPUT) {
			break;
		}
	}
	/*if (firstout == pattern.size()) { // no output variables
	 if (isa<BDDInternalPredTable>(*(pt->internTable()))) {
	 return new LookupGenerator(pt, vars, _universe);
	 } else {
	 StructureVisitor::visit(pt);
	 return _generator;
	 }
	 } else {*/
	StructureVisitor::visit(pt);
	return _generator;
	//}
}

void GeneratorFactory::visit(const ProcInternalPredTable*) {
	_generator = new TableGenerator(_table, _pattern, _vars, _firstocc, _universe);
}

void GeneratorFactory::visit(const BDDInternalPredTable* table) {
	BddGeneratorData data;
	data.pattern = _pattern;
	data.vars = _vars;
	data.structure = table->structure();
	data.universe = _universe;

	// Add necessary types to the bdd to ensure, if possible, finite querying
	FOBDDManager optimizemanager;
	data.bdd = optimizemanager.getBDD(table->bdd(), table->manager());

	// Optimize the bdd for querying
	set<const FOBDDVariable*, CompareBDDVars> outvars;
	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		const FOBDDVariable* var = optimizemanager.getVariable(table->vars()[n]);
		data.bddvars.push_back(var);
		if (_pattern[n] == Pattern::OUTPUT) {
			outvars.insert(var);
		}
	}
	set<const FOBDDDeBruijnIndex*> indices;
	optimizemanager.optimizeQuery(data.bdd, outvars, indices, table->structure());

	// Generate a generator for the optimized bdd
	BDDToGenerator btg(&optimizemanager);
	_generator = btg.create(data);
}

void GeneratorFactory::visit(const FullInternalPredTable*) {
	vector<const DomElemContainer*> outvars;
	vector<SortTable*> outtables;
	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		if (_pattern[n] == Pattern::OUTPUT && _firstocc[n] == n) {
			outvars.push_back(_vars[n]);
			outtables.push_back(_universe.tables()[n]);
		}
	}
	_generator = create(outvars, outtables);
}

void GeneratorFactory::visit(const FuncInternalPredTable* fipt) {
	visit(fipt->table());
}

void GeneratorFactory::visit(const UnionInternalPredTable* uipt) {
	std::vector<InstGenerator*> ingenerators(uipt->inTables().size());
	std::vector<InstGenerator*> incheckers(uipt->inTables().size());
	std::vector<InstGenerator*> outcheckers(uipt->outTables().size());
	unsigned int i = 0;
	for (auto it = uipt->inTables().cbegin(); it != uipt->inTables().cend(); ++it, ++i) {
		(*it)->accept(this);
		ingenerators[i] = _generator;
		auto backuppattern = _pattern;
		_pattern = vector<Pattern>(_pattern.size(), Pattern::INPUT);
		(*it)->accept(this);
		incheckers[i] = _generator;
		_pattern = backuppattern;
	}
	auto backuppattern = _pattern;
	_pattern = vector<Pattern>(_pattern.size(), Pattern::INPUT);

	i = 0;
	for (auto it = uipt->outTables().cbegin(); it != uipt->outTables().cend(); ++it, ++i) {
		(*it)->accept(this);
		outcheckers[i] = _generator;
	}
	_pattern = backuppattern;

	auto ingenereator = new UnionGenerator(ingenerators, incheckers);
	auto outchecker = new UnionGenerator(outcheckers, outcheckers);
	_generator = new TwoChildGenerator(outchecker, ingenereator, new FullGenerator(), new EmptyGenerator());
}

void GeneratorFactory::visit(const UnionInternalSortTable*) {
	throw notyetimplemented("Create a generator from a union sort table");
}

void GeneratorFactory::visit(const AllNaturalNumbers* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortChecker(t, _vars[0]);
	} else {
		_generator = new SortGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const AllIntegers* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortChecker(t, _vars[0]);
	} else {
		_generator = new SortGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const AllFloats* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortChecker(t, _vars[0]);
	} else {
		_generator = new SortGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const AllChars* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortChecker(t, _vars[0]);
	} else {
		_generator = new SortGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const AllStrings* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortChecker(t, _vars[0]);
	} else {
		_generator = new SortGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const EnumeratedInternalSortTable* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortChecker(t, _vars[0]);
	} else {
		_generator = new SortGenerator(t, _vars[0]);
	}
}
void GeneratorFactory::visit(const IntRangeInternalSortTable* t) {
	if (_pattern[0] == Pattern::INPUT) {
		_generator = new SortChecker(t, _vars[0]);
	} else {
		_generator = new SortGenerator(t, _vars[0]);
	}
}

void GeneratorFactory::visit(const EnumeratedInternalPredTable*) {
	LookupTable lookuptab;
	vector<const DomElemContainer*> invars, outvars;

	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		if (_firstocc[n] != n) {
			continue;
		}
		if (_pattern[n] == Pattern::INPUT) {
			invars.push_back(_vars[n]);
		} else {
			outvars.push_back(_vars[n]);
		}
	}

	// TODO make this cheaper by adding domelem to index mappings
	// TODO only create generator ONCE instead of multiple times if possible

	//TODO: this doesn't work currently (wrong version of gcc? thus it is commented)
	/*if(_pattern.size()>0 && invars.size()>0){
	 lookuptab.reserve(_table->size()._size* (invars.size()/_pattern.size())); // Prevents too many rehashes
	 }*/

	for (auto it = _table->begin(); not it.isAtEnd(); ++it) {
		CHECKTERMINATION

		auto tuple = *it;
		bool validunderocc = true;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			// Skip tuples which do no have the same values for multiple occurrences of some variable
			// TODO: Use dynamic programming to improve this
			if (_firstocc[n] != n && tuple[n] != tuple[_firstocc[n]]) {
				validunderocc = false;
				break;
			}
		}
		if (not validunderocc) {
			continue;
		}
		ElementTuple intuple, outtuple;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_firstocc[n] != n) {
				continue;
			}
			if (_pattern[n] == Pattern::INPUT) {
				intuple.push_back(tuple[n]);
			} else if (_firstocc[n] == n) {
				outtuple.push_back(tuple[n]);
			}
		}
		lookuptab[intuple].push_back(outtuple);
	}
	_generator = new EnumLookupGenerator(lookuptab, invars, outvars);
}

Input getBinaryPattern(Pattern left, Pattern right) {
	Input input = Input::BOTH;
	if (left == Pattern::OUTPUT && right == Pattern::OUTPUT) {
		input = Input::NONE;
	} else if (left == Pattern::INPUT && right == Pattern::OUTPUT) {
		input = Input::LEFT;
	} else if (left == Pattern::OUTPUT && right == Pattern::INPUT) {
		input = Input::RIGHT;
	}
	return input;
}

void GeneratorFactory::visit(const EqualInternalPredTable*) {
	if (_firstocc[1] == 0) {
		_generator = create(vector<const DomElemContainer*>(1, _vars[0]), vector<SortTable*>(1, _universe.tables()[0]));
	}
	Input input = getBinaryPattern(_pattern[0], _pattern[1]);
	_generator = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[1], _vars[0], _vars[1], input, CompType::EQ);
}

void GeneratorFactory::visit(const StrLessInternalPredTable*) {
	if (_firstocc[1] == 0) { // If both variables are the same, this will never hold any tuples.
		_generator = new EmptyGenerator();
		return;
	}
	Input input = getBinaryPattern(_pattern[0], _pattern[1]);
	_generator = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[1], _vars[0], _vars[1], input, CompType::LT);
}

void GeneratorFactory::visit(const StrGreaterInternalPredTable*) {
	if (_firstocc[1] == 0) {
		_generator = new EmptyGenerator();
		return;
	}
	Input input = getBinaryPattern(_pattern[0], _pattern[1]);
	_generator = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[1], _vars[0], _vars[1], input, CompType::GT);
}

void GeneratorFactory::visit(const InverseInternalPredTable* iip) {
	InternalPredTable* interntable = iip->table();
	if (typeid(*interntable) == typeid(InverseInternalPredTable)) {
		dynamic_cast<InverseInternalPredTable*>(interntable)->table()->accept(this);
	} else if (typeid(*interntable) == typeid(BDDInternalPredTable)) {
		BDDInternalPredTable* bddintern = dynamic_cast<BDDInternalPredTable*>(interntable);
		const FOBDD* invertedbdd = bddintern->manager()->negation(bddintern->bdd());
		BDDInternalPredTable* invertedbddtable = new BDDInternalPredTable(invertedbdd, bddintern->manager(), bddintern->vars(), bddintern->structure());
		visit(invertedbddtable);
	} else if (typeid(*interntable) == typeid(FullInternalPredTable)) {
		_generator = new EmptyGenerator();
	} else {
		PredTable* temp = new PredTable(iip->table(), _universe);
		//Optimization for Comparisonpredtables
		if (dynamic_cast<ComparisonInternalPredTable*>(temp->internTable()) != NULL) {
			Assert(_pattern.size() == 2);
			Input input;
			if (_pattern[0] == Pattern::INPUT) {
				if (_pattern[1] == Pattern::INPUT) {
					input = Input::BOTH;
				} else {
					input = Input::LEFT;
				}
			} else {
				if (_pattern[1] == Pattern::INPUT) {
					input = Input::RIGHT;
				} else {
					input = Input::NONE;
				}
			}
			if (isa<StrLessInternalPredTable>(*temp->internTable())) {
				_generator = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[1], _vars[0], _vars[1], input, CompType::GEQ);
			} else if (isa<StrGreaterInternalPredTable>(*temp->internTable())) {
				_generator = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[1], _vars[0], _vars[1], input, CompType::LEQ);
			} else {
				Assert(isa<EqualInternalPredTable>(*temp->internTable()));
				_generator = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[1], _vars[0], _vars[1], input, CompType::NEQ);
			}
		} else {
			_generator = new InverseTableGenerator(temp, _pattern, _vars);
		}
	}
}

// NOTE: for any function, if the range is an output variable, we can use the simple func generator
// if the range is input, we need more specialized generators depending on the function type
void GeneratorFactory::visit(const FuncTable* ft) {
	if (_pattern.back() != Pattern::OUTPUT) {
		ft->internTable()->accept(this);
		return;
	}
	bool finite = true;
	for (unsigned int i = 0; i < _pattern.size() - 1; i++) {
		if (_pattern[i] == Pattern::OUTPUT) {
			if (not _universe.tables()[i]->approxFinite()) {
				finite = false;
				break;
			}
		}
	}
	if (not finite && _universe.tables().back()->approxFinite()) {
		Assert(_pattern.back() == Pattern::OUTPUT);
		unsigned int lastindex = _pattern.size() - 1;
		Assert(_universe.tables().size()==lastindex+1 && _vars.size()==lastindex+1);
		_pattern[lastindex] = Pattern::INPUT;
		ft->internTable()->accept(this);
		auto outgen = new SortGenerator(_universe.tables()[lastindex]->internTable(), _vars[lastindex]);
		_generator = new OneChildGenerator(outgen, _generator);
		_pattern[lastindex] = Pattern::OUTPUT;
	} else {
		// TODO: for the input positions, change universe to the universe of ft if this is smaller
		//TODO: and vice versa: if the other is smaller, we can also make the universe of ft smaller!
		Assert(_pattern.back() == Pattern::OUTPUT);
		Assert(_pattern.size()==_vars.size() && _pattern.size()==_universe.tables().size());
		_generator = new SimpleFuncGenerator(ft, _pattern, _vars, _universe, _firstocc);
	}
}

void GeneratorFactory::visit(const ProcInternalFuncTable*) {
	_generator = new TableGenerator(_table, _pattern, _vars, _firstocc, _universe);
}

//
void GeneratorFactory::visit(const UNAInternalFuncTable*) {
	_generator = new InverseUNAFuncGenerator(_pattern, _vars, _universe);
}

void GeneratorFactory::visit(const EnumeratedInternalFuncTable*) {
	// TODO: Use dynamic programming to improve this
	LookupTable lookuptab;
	vector<const DomElemContainer*> invars;
	vector<const DomElemContainer*> outvars;
	for (unsigned int n = 0; n < _pattern.size(); ++n) {
		if (_firstocc[n] == n) {
			if (_pattern[n] == Pattern::INPUT) {
				invars.push_back(_vars[n]);
			} else {
				outvars.push_back(_vars[n]);
			}
		}
	}
	for (TableIterator it = _table->begin(); not it.isAtEnd(); ++it) {
		const ElementTuple& tuple = *it;
		bool ok = true;
		for (unsigned int n = 0; n < _pattern.size(); ++n) {
			if (_firstocc[n] != n && tuple[n] != tuple[_firstocc[n]]) {
				ok = false;
				break;
			}
		}
		if (ok) {
			ElementTuple intuple;
			ElementTuple outtuple;
			for (unsigned int n = 0; n < _pattern.size(); ++n) {
				if (_firstocc[n] == n) {
					if (_pattern[n] == Pattern::INPUT) {
						intuple.push_back(tuple[n]);
					} else {
						outtuple.push_back(tuple[n]);
					}
				}
			}
			lookuptab[intuple].push_back(outtuple);
		}
	}
	_generator = new EnumLookupGenerator(lookuptab, invars, outvars);
}

void GeneratorFactory::visit(const PlusInternalFuncTable* pift) {
	if (_pattern[0] == Pattern::INPUT) {
		if (_pattern[1] == Pattern::INPUT) {
			_generator = new PlusChecker(_vars[0], _vars[1], _vars[2], _universe);
		} else {
			_generator = new MinusGenerator(_vars[2], _vars[0], _vars[1], pift->getType(), _universe.tables()[1]);
		}
	} else if (_pattern[1] == Pattern::INPUT) {
		_generator = new MinusGenerator(_vars[2], _vars[1], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		// if (pift->getType() == NumType::CERTAINLYINT) { // TODO (optimize code)
		const DomainElement* two = createDomElem(2);
		const DomElemContainer* twopointer = new const DomElemContainer();
		*twopointer = two;
		_generator = new DivGenerator(_vars[2], twopointer, _vars[0], NumType::POSSIBLYINT, _universe.tables()[0]);
		//}
	} else {
		if (_universe.tables()[0]->approxFinite()) {
			auto xgen = new SortGenerator(_universe.tables()[0]->internTable(), _vars[0]);
			auto ygen = new MinusGenerator(_vars[2], _vars[0], _vars[1], pift->getType(), _universe.tables()[1]);
			_generator = new OneChildGenerator(xgen, ygen);
		} else {
			//NOTE: this will also happen if both sorts are infinitely large.  Hence infinite generators might be made
			//TODO: make this smarter: don't run over the whole universe but only over elements that have a chance to be make the sum right
			auto ygen = new SortGenerator(_universe.tables()[1]->internTable(), _vars[1]);
			auto xgen = new MinusGenerator(_vars[2], _vars[1], _vars[0], pift->getType(), _universe.tables()[0]);
			_generator = new OneChildGenerator(ygen, xgen);
		}
	}
}

void GeneratorFactory::visit(const MinusInternalFuncTable* pift) {
	if (_pattern[0] == Pattern::INPUT) {
		if (_pattern[1] == Pattern::INPUT) {
			_generator = new MinusChecker(_vars[0], _vars[1], _vars[2], _universe);
		} else {
			_generator = new MinusGenerator(_vars[0], _vars[2], _vars[1], pift->getType(), _universe.tables()[1]);
		}
	} else if (_pattern[1] == Pattern::INPUT) {
		_generator = new PlusGenerator(_vars[1], _vars[2], _vars[0], pift->getType(), _universe.tables()[0]);
	} else if (_firstocc[1] == 0) {
		throw notyetimplemented("Create a generator for x-x=y, with x output");
	} else {
		if (_universe.tables()[0]->approxFinite()) {
			auto xgen = new SortGenerator(_universe.tables()[0]->internTable(), _vars[0]);
			auto ygen = new MinusGenerator(_vars[0], _vars[2], _vars[1], pift->getType(), _universe.tables()[1]);
			_generator = new OneChildGenerator(xgen, ygen);
		} else {
			//NOTE: this will also happen if both sorts are infinitely large.  Hence infinite generators might be made
			//TODO: make this smarter: don't run over the whole universe but only over elements that have a chance to be make it right
			auto ygen = new SortGenerator(_universe.tables()[1]->internTable(), _vars[1]);
			auto xgen = new PlusGenerator(_vars[1], _vars[2], _vars[0], pift->getType(), _universe.tables()[0]);
			_generator = new OneChildGenerator(ygen, xgen);
		}
	}
}

void GeneratorFactory::visit(const TimesInternalFuncTable* pift) {
	if (_pattern[0] == Pattern::INPUT) {
		if (_pattern[1] == Pattern::INPUT) {
			_generator = new TimesChecker(_vars[0], _vars[1], _vars[2], _universe);
		} else {
			//generate x*y = z with xand z input and y output
			//transform to z/y = x | x=z=0
			auto standardsolution = new DivGenerator(_vars[2], _vars[0], _vars[1], pift->getType(), _universe.tables()[1]);

			auto varzero = new DomElemContainer();
			varzero->operator =(GlobalData::getGlobalDomElemFactory()->create(0, NumType::POSSIBLYINT));
			auto xiszero = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[0], _vars[0], varzero, Input::BOTH, CompType::EQ);
			auto ziszero = new ComparisonGenerator(_universe.tables()[2], _universe.tables()[2], _vars[2], varzero, Input::BOTH, CompType::EQ);
			auto xandzarezero = new OneChildGenerator(xiszero, ziszero);
			auto yisanything = new SortGenerator(_universe.tables()[1]->internTable(), _vars[1]);
			_generator = new TwoChildGenerator(xandzarezero, new FullGenerator(), standardsolution, yisanything);
		}
	} else if (_pattern[1] == Pattern::INPUT) {
		//generate x*y = z with y and z input and x output
		//transform to z/x = y | y=z=0
		auto standardsolution = new DivGenerator(_vars[2], _vars[1], _vars[0], pift->getType(), _universe.tables()[0]);

		auto varzero = new DomElemContainer();
		varzero->operator =(GlobalData::getGlobalDomElemFactory()->create(0, NumType::POSSIBLYINT));
		auto yiszero = new ComparisonGenerator(_universe.tables()[1], _universe.tables()[1], _vars[1], varzero, Input::BOTH, CompType::EQ);
		auto ziszero = new ComparisonGenerator(_universe.tables()[2], _universe.tables()[2], _vars[2], varzero, Input::BOTH, CompType::EQ);
		auto yandzarezero = new OneChildGenerator(yiszero, ziszero);
		auto xisanything = new SortGenerator(_universe.tables()[0]->internTable(), _vars[0]);
		_generator = new TwoChildGenerator(yandzarezero, new FullGenerator(), standardsolution, xisanything);
	} else if (_firstocc[1] == 0) {
		throw notyetimplemented("Create a generator for x*x=y, with x output");
	} else {
		if (_universe.tables()[0]->approxFinite()) {
			//Same solution as before (see the long explanation), but now: first generate all x instead of using a fullgenerator
			auto xgen = new SortGenerator(_universe.tables()[0]->internTable(), _vars[0]);
			_pattern[0] = Pattern::INPUT;
			visit(pift);
			_generator = new OneChildGenerator(xgen, _generator);
		} else {
			//NOTE: this will also happen if both sorts are infinitely large.  Hence infinite generators might be made
			//TODO: make this smarter: don't run over the whole universe but only over elements that have a chance to be make it right
			auto ygen = new SortGenerator(_universe.tables()[1]->internTable(), _vars[1]);
			_pattern[1] = Pattern::INPUT;
			visit(pift);
			_generator = new OneChildGenerator(ygen, _generator);
		}
	}
}

void GeneratorFactory::visit(const DivInternalFuncTable* pift) {
	if (_pattern[0] == Pattern::INPUT) {
		if (_pattern[1] == Pattern::INPUT) {
			_generator = new DivChecker(_vars[0], _vars[1], _vars[2], _universe);
		} else {
			//x/y=z with x and z input.
			//Thus either x/z = y, with y not zero (standardsolution)
			//or x=z=0 with y anything (except for zero) (othersolution)
			// Solution: create a twoChildGenerator that
			//* 1) Checks for x and z to be zero.
			//* 2) if so, generate y universe
			//* 2) if not, do the standardsolution
			auto temp = new DivGenerator(_vars[0], _vars[2], _vars[1], pift->getType(), _universe.tables()[1]);
			auto varzero = new DomElemContainer();
			varzero->operator =(GlobalData::getGlobalDomElemFactory()->create(0, NumType::POSSIBLYINT));
			auto ynotzerochecker = new ComparisonGenerator(_universe.tables()[1], _universe.tables()[1], _vars[1], varzero, Input::BOTH, CompType::NEQ);
			auto standardsolution = new OneChildGenerator(temp, ynotzerochecker);

			auto xiszero = new ComparisonGenerator(_universe.tables()[0], _universe.tables()[0], _vars[0], varzero, Input::BOTH, CompType::EQ);
			auto ziszero = new ComparisonGenerator(_universe.tables()[2], _universe.tables()[2], _vars[2], varzero, Input::BOTH, CompType::EQ);
			auto xandzarezero = new OneChildGenerator(xiszero, ziszero);
			auto ynotzerogenerator = new ComparisonGenerator(_universe.tables()[1], _universe.tables()[1], _vars[1], varzero, Input::RIGHT, CompType::NEQ);
			_generator = new TwoChildGenerator(xandzarezero, new FullGenerator(), standardsolution, ynotzerogenerator);
		}
	} else if (_pattern[1] == Pattern::INPUT) {
		auto temp = new TimesGenerator(_vars[1], _vars[2], _vars[0], pift->getType(), _universe.tables()[0]);
		auto varzero = new DomElemContainer();
		varzero->operator =(GlobalData::getGlobalDomElemFactory()->create(0, NumType::POSSIBLYINT));
		auto notzero = new ComparisonGenerator(_universe.tables()[1], _universe.tables()[1], _vars[1], varzero, Input::BOTH, CompType::NEQ);
		_generator = new OneChildGenerator(temp, notzero);
	} else if (_firstocc[1] == 0) {
		throw notyetimplemented("Create a generator for x/x=z, with x output");
		//x/x=z with x output, z input
		//Thus, if z = 1: x =/= 0 generator, otherwise emptygenerator
		auto varzero = new DomElemContainer();
		varzero->operator =(GlobalData::getGlobalDomElemFactory()->create(0, NumType::POSSIBLYINT));
		auto varone = new DomElemContainer();
		varone->operator =(GlobalData::getGlobalDomElemFactory()->create(1, NumType::POSSIBLYINT));
		auto natSortTable = get(STDSORT::NATSORT)->interpretation();
		auto zIsOneChecker = new ComparisonGenerator(natSortTable, natSortTable, _vars[2], varone, Input::BOTH, CompType::EQ);
		auto xNotZeroGenerator = new ComparisonGenerator(natSortTable, _universe.tables()[0], varzero, _vars[0], Input::LEFT, CompType::NEQ);
		_generator = new TwoChildGenerator(zIsOneChecker, new FullGenerator(), new EmptyGenerator(), xNotZeroGenerator);
	} else {
		if (_universe.tables()[0]->approxFinite()) {
			//Same solution as before (see the long explanation), but now: first generate all x instead of using a fullgenerator
			auto xgen = new SortGenerator(_universe.tables()[0]->internTable(), _vars[0]);
			_pattern[0] = Pattern::INPUT;
			visit(pift);
			_generator = new OneChildGenerator(xgen, _generator);
		} else {
			//NOTE: this will also happen if both sorts are infinitely large.  Hence infinite generators might be made
			//TODO: make this smarter: don't run over the whole universe but only over elements that have a chance to be make it right
			auto ygen = new SortGenerator(_universe.tables()[1]->internTable(), _vars[1]);
			_pattern[1] = Pattern::INPUT;
			visit(pift);
			_generator = new OneChildGenerator(ygen, _generator);
		}
	}
}

void GeneratorFactory::visit(const ExpInternalFuncTable*) {
	throw notyetimplemented("Infinite generator for exponentiation pattern (?,?,in)");
}

void GeneratorFactory::visit(const ModInternalFuncTable* mift) {
	if (_pattern[0] == Pattern::INPUT) {
		if (_pattern[1] == Pattern::INPUT) {
			_generator = new ModChecker(_vars[0], _vars[1], _vars[2], _universe);
		} else {
			//x%y=z with x and z input.
			//Thus, qy+z=x for some q integer
			//Thus, either x-z = 0 and any y is fine
			//Or, y needs to be between -(x-z) and (x-z) and qy = x-z
			auto xMinusZ = new DomElemContainer();
			auto varzero = new DomElemContainer();
			varzero->operator =(GlobalData::getGlobalDomElemFactory()->create(0, NumType::POSSIBLYINT));

			auto intSortTable = get(STDSORT::INTSORT)->interpretation();
			auto calcXMinusZ = new MinusGenerator(_vars[0], _vars[2], xMinusZ, NumType::CERTAINLYINT, intSortTable);
			auto xMinusZis0Checker = new ComparisonGenerator(intSortTable, intSortTable, xMinusZ, varzero, Input::BOTH, CompType::EQ);
			//TODO: optimize: y needs to be between -(x-z) and (x-z) and qy = x-z
			auto fullYGenerator = new SortGenerator(_universe.tables()[1]->internTable(), _vars[1]);

			auto q = new DomElemContainer();
			_vars[0] = q;
			_vars[2] = xMinusZ;
			std::vector<SortTable*> newtables = { intSortTable, _universe.tables()[1], intSortTable };
			_universe = Universe(newtables);
			_pattern = {Pattern::OUTPUT,Pattern::OUTPUT,Pattern::INPUT};
			visit(new TimesInternalFuncTable(true));

			_generator = new TwoChildGenerator(xMinusZis0Checker, calcXMinusZ, _generator, fullYGenerator);
		}
	} else if (_universe.tables()[0]->approxFinite()) {
		//Same solution as before (see the long explanation), but now: first generate all x instead of using a fullgenerator (might not be optimal)
		auto xgen = new SortGenerator(_universe.tables()[0]->internTable(), _vars[0]);
		_pattern[0] = Pattern::INPUT;
		visit(mift);
		_generator = new OneChildGenerator(xgen, _generator);
	} else if (_pattern[1] == Pattern::INPUT) {
		//x%y=z with y and z input.
		//Thus, qy+z=x for some q integer
		throw notyetimplemented("Infinite generator for modulo pattern (out,in,in)");
		//TODO: code below not yet good:
		//* use Unary minus instead of binary minus
		//* Order of generation...

		auto q = new DomElemContainer();
		auto natSortTable = get(STDSORT::NATSORT)->interpretation();
		auto intSortTable = get(STDSORT::INTSORT)->interpretation();
		auto positiveQGenerator = new ComparisonGenerator(natSortTable, _universe.tables()[1], q, _vars[1], Input::RIGHT, CompType::LEQ);

		auto varzero = new DomElemContainer();
		varzero->operator =(GlobalData::getGlobalDomElemFactory()->create(0, NumType::POSSIBLYINT));
		auto minusQ = new DomElemContainer();
		auto positiveMinusQGenerator = new ComparisonGenerator(natSortTable, _universe.tables()[1], minusQ, _vars[1], Input::RIGHT, CompType::LEQ);
		auto minusQIsZeroChecker = new ComparisonGenerator(natSortTable, natSortTable, minusQ, varzero, Input::BOTH, CompType::EQ);
		auto qFromMinusQGenerator = new MinusGenerator(varzero, minusQ, q, NumType::CERTAINLYINT, intSortTable);
		auto negativeQGenerator = new TwoChildGenerator(minusQIsZeroChecker, positiveMinusQGenerator, qFromMinusQGenerator, new EmptyGenerator);

		std::vector<InstGenerator*> allqs = { negativeQGenerator, positiveQGenerator };
		std::vector<InstGenerator*> trivial = { new EmptyGenerator(), new EmptyGenerator() };
		//By construction, we know that the trivial checkers suffice
		auto allQGenerator = new UnionGenerator(allqs, trivial);
		auto qTimesY = new DomElemContainer();
		auto qTimesYGenerator = new TimesGenerator(q, _vars[1], qTimesY, NumType::CERTAINLYINT, intSortTable);
		auto finalGenerator = new PlusGenerator(qTimesY, _vars[2], _vars[0], NumType::CERTAINLYINT, _universe.tables()[0]);

		_generator = new OneChildGenerator(allQGenerator, new OneChildGenerator(qTimesYGenerator, finalGenerator));

	} else if (_firstocc[1] == 0) {
		//x%x=z with x output, z input
		//Thus, if z = 0: fullgenerator, otherwise emptygenerator
		auto varzero = new DomElemContainer();
		varzero->operator =(GlobalData::getGlobalDomElemFactory()->create(0, NumType::POSSIBLYINT));
		auto natSortTable = get(STDSORT::NATSORT)->interpretation();
		auto zIsZeroChecker = new ComparisonGenerator(natSortTable, natSortTable, _vars[2], varzero, Input::BOTH, CompType::EQ);
		auto fullXGenerator = new SortGenerator(_universe.tables()[0]->internTable(), _vars[0]);
		_generator = new TwoChildGenerator(zIsZeroChecker, new FullGenerator(), new EmptyGenerator(), fullXGenerator);
	} else {
		//NOTE: this will also happen if both sorts are infinitely large.  Hence infinite generators might be made
		//TODO: make this smarter: don't run over the whole universe but only over elements that have a chance to be make it right
		auto ygen = new SortGenerator(_universe.tables()[1]->internTable(), _vars[1]);
		_pattern[1] = Pattern::INPUT;
		visit(mift);
		_generator = new OneChildGenerator(ygen, _generator);
	}

}

void GeneratorFactory::visit(const AbsInternalFuncTable* aift) {
	if (_pattern[0] == Pattern::OUTPUT) {
		_generator = new InverseAbsValueGenerator(_vars[1], _vars[0], _universe.tables()[0], aift->getType());
	} else {
		_generator = new AbsValueChecker(_vars[0], _vars[1], _universe);
	}
}

void GeneratorFactory::visit(const UminInternalFuncTable*) {
	Assert(_pattern[0]==Pattern::OUTPUT);
	_generator = new UnaryMinusGenerator(_vars[1], _vars[0], _universe);
}

