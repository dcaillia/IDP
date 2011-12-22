/****************************************************************
* Copyright 2010-2012 Katholieke Universiteit Leuven
*  
* Use of this software is governed by the GNU LGPLv3.0 license
* 
* Written by Broes De Cat, Stef De Pooter, Johan Wittocx
* and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
* Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "utils/TheoryUtils.hpp"

#include "theory.hpp"
#include "vocabulary.hpp"
#include "term.hpp"
#include "theoryinformation/ApproxCheckTwoValued.hpp"
#include "fobdds/FoBdd.hpp"
#include "fobdds/FoBddFactory.hpp"
#include "fobdds/FoBddManager.hpp"
#include "theoryinformation/CollectOpensOfDefinitions.hpp"
#include "theoryinformation/CheckContainment.hpp"
#include "theoryinformation/CheckContainsFuncTerms.hpp"
#include "theoryinformation/CheckContainsAggTerms.hpp"
#include "theoryinformation/CheckFuncTerms.hpp"
#include "theoryinformation/CheckPartialTerm.hpp"
#include "theoryinformation/CheckSorts.hpp"
#include "theoryinformation/CollectOpensOfDefinitions.hpp"
#include "theoryinformation/CountNbOfSubFormulas.hpp"
#include "theoryinformation/DeriveTermBounds.hpp"
#include "theorytransformations/PushNegations.hpp"
#include "theorytransformations/Flatten.hpp"
#include "theorytransformations/DeriveSorts.hpp"
#include "theorytransformations/AddCompletion.hpp"
#include "theorytransformations/GraphFuncsAndAggs.hpp"
#include "theorytransformations/RemoveEquivalences.hpp"
#include "theorytransformations/PushQuantifications.hpp"
#include "theorytransformations/SplitComparisonChains.hpp"
#include "theorytransformations/SubstituteTerm.hpp"
#include "theorytransformations/UnnestFuncsAndAggs.hpp"
#include "theorytransformations/UnnestPartialTerms.hpp"
#include "theorytransformations/UnnestTerms.hpp"
#include "theorytransformations/UnnestThreeValuedTerms.hpp"
#include "theorytransformations/SplitProducts.hpp"
#include "theorytransformations/SplitIntoMonotoneAgg.hpp"

using namespace std;

/* TermUtils */
namespace TermUtils {
void checkSorts(Vocabulary* voc, Term* term) {
	transform<CheckSorts>(term, voc);
}

void deriveSorts(Vocabulary* voc, Term* term) {
	transform<DeriveSorts>(term, voc, false);
	transform<DeriveSorts>(term, voc, true);
}

ElementTuple deriveTermBounds(Term* term, const AbstractStructure* str) {
	return transform<DeriveTermBounds, ElementTuple>(term, str);
}

bool isPartial(Term* term) {
	return transform<CheckPartialTerm, bool>(term);
}
}

/* SetUtils */
namespace SetUtils {
bool approxTwoValued(SetExpr* exp, AbstractStructure* str) {
	return transform<ApproxCheckTwoValued, bool>(str, exp);
}

SetExpr* unnestThreeValuedTerms(SetExpr* exp, AbstractStructure* structure, Context context, 
		bool cpsupport, const std::set<const PFSymbol*> cpsymbols) {
	return transform<UnnestThreeValuedTerms, SetExpr*>(exp, structure, context, cpsupport, cpsymbols);
}
}

/* DefinitionUtils */
namespace DefinitionUtils {
void checkSorts(Vocabulary* voc, Rule* rule) {
	transform<CheckSorts>(rule, voc);
}

void deriveSorts(Vocabulary* voc, Rule* rule) {
	transform<DeriveSorts>(rule, voc, false);
	transform<DeriveSorts>(rule, voc, true);
}

std::set<PFSymbol*> opens(Definition* d) {
	return transform<CollectOpensOfDefinitions, std::set<PFSymbol*>>(d);
}

Rule* unnestThreeValuedTerms(Rule* rule, AbstractStructure* structure, Context context, 
		bool cpsupport, const std::set<const PFSymbol*> cpsymbols) {
	return transform<UnnestThreeValuedTerms, Rule*>(rule, structure, context, cpsupport, cpsymbols);
}
}

/* FormulaUtils */
namespace FormulaUtils {
void checkSorts(Vocabulary* v, Formula* f) {
	transform<CheckSorts>(f, v);
}

bool containsFuncTerms(Formula* f) {
	return transform<CheckContainsFuncTerms, bool>(f);
}

bool containsAggTerms(Formula* f) {
	return transform<CheckContainsFuncTerms, bool>(f);
}

bool containsSymbol(const PFSymbol* s, const Formula* f) {
	return transform<CheckContainment, bool>(s, f);
}

void deriveSorts(Vocabulary* v, Formula* f){
	transform<DeriveSorts>(f, v, false);
	transform<DeriveSorts>(f, v, true);
}

Formula* flatten(Formula* f) {
	return transform<Flatten, Formula*>(f);
}

Formula* graphFuncsAndAggs(Formula* f, AbstractStructure* str, Context con) {
	return transform<GraphFuncsAndAggs, Formula*>(f,str,con);
}

Formula* pushNegations(Formula* f) {
	return transform<PushNegations, Formula*>(f);
}

Formula* removeEquivalences(Formula* f) {
	return transform<RemoveEquivalences, Formula*>(f);
}

Formula* splitComparisonChains(Formula* f, Vocabulary* v) {
	return transform<SplitComparisonChains, Formula*>(f, v);
}

Formula* splitIntoMonotoneAgg(Formula* f) {
	return transform<SplitIntoMonotoneAgg, Formula*>(f);
}

Formula* splitProducts(Formula* f){
	return transform<SplitProducts, Formula*>(f);
}

Formula* substituteTerm(Formula* f, Term* t, Variable* v) {
	return transform<SubstituteTerm, Formula*>(f, t, v);
}

Formula* unnestFuncsAndAggs(Formula* f, AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggs, Formula*>(f, str, con);
}

Formula* unnestPartialTerms(Formula* f, Context con, AbstractStructure* str, Vocabulary* voc) {
	return transform<UnnestPartialTerms, Formula*>(f, con, str, voc);
}

Formula* unnestTerms(Formula* f, Context con, AbstractStructure* str, Vocabulary* voc) {
	return transform<UnnestTerms, Formula*>(f, con, str, voc);
}

Formula* unnestThreeValuedTerms(Formula* f, AbstractStructure* structure, Context context, 
		bool cpsupport, const std::set<const PFSymbol*> cpsymbols) {
	return transform<UnnestThreeValuedTerms, Formula*>(f, structure, context, cpsupport, cpsymbols);
}

void addCompletion(AbstractTheory* t) {
	auto newt = transform<AddCompletion, AbstractTheory*>(t);
	Assert(newt==t);
}

void flatten(AbstractTheory* t) {
	auto newt = transform<Flatten, AbstractTheory*>(t);
	Assert(newt==t);
}

AbstractTheory* graphFuncsAndAggs(AbstractTheory* t, AbstractStructure* str, Context con) {
	return transform<GraphFuncsAndAggs, AbstractTheory*>(t,str,con);
}

void pushNegations(AbstractTheory* t) {
	auto newt = transform<PushNegations, AbstractTheory*>(t);
	Assert(newt==t);
}

AbstractTheory* pushQuantifiers(AbstractTheory* t) {
	return transform<PushQuantifications, AbstractTheory*>(t);
}

AbstractTheory* removeEquivalences(AbstractTheory* t) {
	return transform<RemoveEquivalences, AbstractTheory*>(t);
}

AbstractTheory* splitComparisonChains(AbstractTheory* t, Vocabulary* voc) {
	return transform<SplitComparisonChains, AbstractTheory*>(t, voc);
}

AbstractTheory* splitProducts(AbstractTheory* t) {
	return transform<SplitProducts, AbstractTheory*>(t);
}

AbstractTheory* unnestFuncsAndAggs(AbstractTheory* t, AbstractStructure* str, Context con) {
	return transform<UnnestFuncsAndAggs, AbstractTheory*>(t, str, con);
}

void unnestTerms(AbstractTheory* t, Context con, AbstractStructure* str, Vocabulary* voc) {
	auto newt = transform<UnnestTerms, AbstractTheory*>(t, con, str, voc);
	Assert(newt==t);
}

int nrSubformulas(AbstractTheory* t) {
	return transform<CountNbOfSubFormulas, int>(t);
}

AbstractTheory* merge(AbstractTheory* at1, AbstractTheory* at2) {
	if (not sametypeid<Theory>(*at1) || not sametypeid<Theory>(*at2)) {
		throw notyetimplemented("Only merging of normal theories has been implemented...");
	}
	if (at1->vocabulary() != at2->vocabulary()) {
		throw notyetimplemented("Only merging of theories over the same vocabularies has been implemented...");
	}
	AbstractTheory* at = at1->clone();
	Theory* t2 = static_cast<Theory*>(at2);
	for (auto it = t2->sentences().cbegin(); it != t2->sentences().cend(); ++it) {
		at->add((*it)->clone());
	}
	for (auto it = t2->definitions().cbegin(); it != t2->definitions().cend(); ++it) {
		at->add((*it)->clone());
	}
	for (auto it = t2->fixpdefs().cbegin(); it != t2->fixpdefs().cend(); ++it) {
		at->add((*it)->clone());
	}
	return at;
}

double estimatedCostAll(PredForm* query, const std::set<Variable*> freevars, bool inverse, AbstractStructure* structure) {
	FOBDDManager manager;
	FOBDDFactory factory(&manager);
	auto bdd = factory.turnIntoBdd(query);
	if (inverse) {
		bdd = manager.negation(bdd);
	}
	double res = manager.estimatedCostAll(bdd, manager.getVariables(freevars), { }, structure);
	return res;
}

BoolForm* trueFormula() {
	return new BoolForm(SIGN::POS, true, vector<Formula*>(0), FormulaParseInfo());
}

BoolForm* falseFormula() {
	return new BoolForm(SIGN::POS, false, vector<Formula*>(0), FormulaParseInfo());
}

bool isMonotone(const AggForm* af) {
	switch (af->comp()) {
	case CompType::EQ:
	case CompType::NEQ:
		return false;
	case CompType::LT:
	case CompType::LEQ: {
		switch (af->right()->function()) {
		case AggFunction::CARD:
		case AggFunction::MAX:
			return isPos(af->sign());
		case AggFunction::MIN:
			return isNeg(af->sign());
		case AggFunction::SUM:
			return isPos(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
		case AggFunction::PROD:
			return isPos(af->sign()); //FIXME: Asserts that weights are larger than one! Not correct otherwise.
		}
		break;
	}
	case CompType::GT:
	case CompType::GEQ: {
		switch (af->right()->function()) {
		case AggFunction::CARD:
		case AggFunction::MAX:
			return isNeg(af->sign());
		case AggFunction::MIN:
			return isPos(af->sign());
		case AggFunction::SUM:
			return isNeg(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
		case AggFunction::PROD:
			return isNeg(af->sign()); //FIXME: Asserts that weights are larger than one! Not correct otherwise.
		}
		break;
	}
	}
	return false;
}

bool isAntimonotone(const AggForm* af) {
	switch (af->comp()) {
	case CompType::EQ:
	case CompType::NEQ:
		return false;
	case CompType::LT:
	case CompType::LEQ: {
		switch (af->right()->function()) {
		case AggFunction::CARD:
		case AggFunction::MAX:
			return isNeg(af->sign());
		case AggFunction::MIN:
			return isPos(af->sign());
		case AggFunction::SUM:
			return isNeg(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
		case AggFunction::PROD:
			return isNeg(af->sign()); //FIXME: Asserts that weights are larger than one! Not correct otherwise.
		}
		break;
	}
	case CompType::GT:
	case CompType::GEQ: {
		switch (af->right()->function()) {
		case AggFunction::CARD:
		case AggFunction::MAX:
			return isPos(af->sign());
		case AggFunction::MIN:
			return isNeg(af->sign());
		case AggFunction::SUM:
			return isPos(af->sign()); //FIXME: Asserts that weights are positive! Not correct otherwise.
		case AggFunction::PROD:
			return isPos(af->sign()); //FIXME: Asserts that weights are larger than one! Not correct otherwise.
		}
		break;
	}
	}
	return false;
}
}