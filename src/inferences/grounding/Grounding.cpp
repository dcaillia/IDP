/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "Grounding.hpp"

#include "inferences/SolverInclude.hpp"
#include "inferences/modelexpansion/TraceMonitor.hpp"
#include "inferences/grounding/grounders/Grounder.hpp"

template<>
void fixTraceMonitor(TraceMonitor* t, Grounder* grounder, PCSolver* solver) {
	t->setTranslator(grounder->getTranslator());
	t->setSolver(solver);
}

void addSymmetryBreaking(AbstractTheory* theory, AbstractStructure* structure, AbstractGroundTheory* grounding, const Term* minimizeTerm) {
	switch (getGlobal()->getOptions()->symmetryBreaking()) {
	case SymmetryBreaking::NONE:
		break;
	case SymmetryBreaking::STATIC: {
		auto ivsets = findIVSets(theory, structure, minimizeTerm);
		addSymBreakingPredicates(grounding, ivsets);
		break;
	}
	case SymmetryBreaking::DYNAMIC: {
		auto ivsets = findIVSets(theory, structure, minimizeTerm);
		for (auto ivsets_it = ivsets.cbegin(); ivsets_it != ivsets.cend(); ++ivsets_it) {
			grounding->addSymmetries((*ivsets_it)->getBreakingSymmetries(grounding));
		}
		break;
	}
	}
}

