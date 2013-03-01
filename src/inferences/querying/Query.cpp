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

#include "Query.hpp"

#include "IncludeComponents.hpp"
#include "theory/Query.hpp"
#include "generators/BDDBasedGeneratorFactory.hpp"
#include "inferences/propagation/PropagatorFactory.hpp"
#include "inferences/propagation/GenerateBDDAccordingToBounds.hpp"
#include "generators/InstGenerator.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddManager.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddVariable.hpp"
#include "theory/TheoryUtils.hpp"

PredTable* Querying::solveQuery(Query* q, AbstractStructure* structure) const {
	// translate the formula to a bdd
	FOBDDManager* manager;
	const FOBDD* bdd;
	auto newquery = q->query()->clone();
	newquery = FormulaUtils::calculateArithmetic(newquery);
	if (not structure->approxTwoValued()) {
		auto generateBDDaccToBounds = generateBounds(new Theory("", structure->vocabulary(), ParseInfo()), structure, false, false);
		bdd = generateBDDaccToBounds->evaluate(newquery, TruthType::CERTAIN_TRUE);
		manager = generateBDDaccToBounds->obtainManager();
		delete generateBDDaccToBounds;
	} else {
		//When working two-valued, we can simply turn formula to BDD
		manager = new FOBDDManager();
		FOBDDFactory factory(manager);
		bdd = factory.turnIntoBdd(newquery);
	}
	newquery->recursiveDelete();

	Assert(bdd != NULL);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "Query-BDD:" << "\n" << print(bdd) << "\n";
	}
	Assert(manager != NULL);
	varset vars(q->variables().cbegin(), q->variables().cend());
	auto bddvars = manager->getVariables(vars);
	fobddindexset bddindices;

	Assert(bdd != NULL);

	// create a generator
	BddGeneratorData data;
	data.bdd = bdd;
	data.structure = structure;
	std::map<Variable*,const DomElemContainer*> varsToDomElemContainers;
	for (auto it = q->variables().cbegin(); it != q->variables().cend(); ++it) {
		data.pattern.push_back(Pattern::OUTPUT);
		auto dec = varsToDomElemContainers.find(*it);
		if (dec == varsToDomElemContainers.cend()) {
			auto res = new const DomElemContainer();
			varsToDomElemContainers[*it] = res;
			data.vars.push_back(res);

		} else {
			data.vars.push_back(dec->second);
		}
		data.bddvars.push_back(manager->getVariable(*it));
		data.universe.addTable(structure->inter((*it)->sort()));
	}
	BDDToGenerator btg(manager);

	InstGenerator* generator = btg.create(data);
	if (getOption(IntType::VERBOSE_QUERY) > 0) {
		clog << "Query-Generator:" << "\n" << print(generator) << "\n";
	}

// Create an empty table
	std::vector<SortTable*> vst;
	for (auto it = q->variables().cbegin(); it != q->variables().cend(); ++it) {
		vst.push_back(structure->inter((*it)->sort()));
	}
	Universe univ(vst);
	auto result = TableUtils::createPredTable(univ);
	// execute the query
	ElementTuple currtuple(q->variables().size());
	//cerr <<"Generator: " <<print(generator) <<"\n";
	for (generator->begin(); not generator->isAtEnd(); generator->operator ++()) {
		for (unsigned int n = 0; n < q->variables().size(); ++n) {
			currtuple[n] = data.vars[n]->get();
		}
		result->add(currtuple);
	}
	delete generator;
	delete (manager);
	return result;
}
