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
#include "BootstrappingUtils.hpp"
#include "common.hpp"
#include "utils/UniqueNames.hpp"
#include "IncludeComponents.hpp"
#include "theory/TheoryUtils.hpp"

extern void parsefile(const std::string&);


namespace BootstrappingUtils {
Structure* getDefinitionInfo(const std::vector<Definition*>& defs, UniqueNames<PFSymbol*>& uniqueSymbNames, UniqueNames<Rule*>& uniqueRuleNames,
		UniqueNames<Definition*>& uniqueDefNames) {
	if (not getGlobal()->instance()->alreadyParsed("definitions")) {
		parsefile("definitions");
	}

	//SET BASIC DATA:REFERENCE TO FILES ETCETERA
	auto defnamespace = getGlobal()->getGlobalNamespace()->subspace("stdspace")->subspace("definitionbootstrapping");
	Assert(defnamespace != NULL);

	auto defVoc = defnamespace->vocabulary("basicdata");
	auto structure = new Structure(createName(), defVoc, { });

	auto definesFunc = defVoc->func("defines/1");
	Assert(definesFunc != NULL);
	auto definesInter = structure->inter(definesFunc);

	auto openPred = defVoc->pred("inbody/3");
	Assert(openPred != NULL);
	auto openInter = structure->inter(openPred);

	auto infunc = defVoc->func("in/1");
	Assert(infunc != NULL);
	auto inInter = structure->inter(infunc);

	auto posfunc = defVoc->func("pos/0");
	auto negfunc = defVoc->func("neg/0");
	Assert(posfunc != NULL);
	Assert(negfunc != NULL);
	auto pos = structure->inter(posfunc)->value( { });
	auto neg = structure->inter(negfunc)->value( { });
	Assert(pos != NULL);
	Assert(neg != NULL);

	for (auto d : defs) {
		auto defname = mapName(d, uniqueDefNames);

		for (auto r : d->rules()) {
			auto ruleName = mapName(r, uniqueRuleNames);
			inInter->add( { ruleName, defname });

			auto head = r->head();
			PFSymbol* definedSymbol;
			if (is(head->symbol(), STDPRED::EQ)) {
				auto headTerm = head->subterms()[0];
				auto funcHeadTerm = dynamic_cast<FuncTerm*>(headTerm);
				Assert(funcHeadTerm != NULL);
				definedSymbol = funcHeadTerm->function();
			} else {
				definedSymbol = head->symbol();
			}

			auto symbolName = mapName(definedSymbol, uniqueSymbNames);
			definesInter->add( { ruleName, symbolName });

			auto ruleclone = r->clone();
			ruleclone = DefinitionUtils::unnestHeadTermsNotVarsOrDomElems(ruleclone, structure);
			auto opens = FormulaUtils::collectSymbols(ruleclone->body());
			for (auto open : opens) {
				symbolName = mapName(open, uniqueSymbNames);
				openInter->ct()->add( { ruleName, symbolName, pos }); //TODO correct sign!!!
			}
		}

	}
	structure->checkAndAutocomplete();
	makeUnknownsFalse(definesInter->graphInter());
	makeUnknownsFalse(inInter->graphInter());
	makeUnknownsFalse(openInter);
	structure->clean();
	return structure;

}

Structure* getDefinitionInfo(const Definition* d, UniqueNames<PFSymbol*>& usn, UniqueNames<Rule*>& urn,
		UniqueNames<Definition*>& udn) {
	return getDefinitionInfo({d},usn,urn,udn);
}
Structure* getDefinitionInfo(const Theory* t, UniqueNames<PFSymbol*>& usn, UniqueNames<Rule*>& urn,
		UniqueNames<Definition*>& udn) {
	return getDefinitionInfo(t->definitions(),usn,urn,udn);
}

Options* setBootstrappingOptions(){
	auto old = getGlobal()->getOptions();
	auto newoptions = new Options(false);
	getGlobal()->setOptions(newoptions);
	setOption(POSTPROCESS_DEFS, false);//Important since postprocessing is implemented with bootstrapping
	setOption(SPLIT_DEFS, false); //Important since splitting is implemented with bootstrapping
	setOption(GROUNDWITHBOUNDS, true);
	setOption(LIFTEDUNITPROPAGATION, true);
	setOption(LONGESTBRANCH, 12);
	setOption(NRPROPSTEPS, 12);
	setOption(CPSUPPORT, true);
	setOption(TSEITINDELAY, false);
	setOption(SATISFIABILITYDELAY, false);
	setOption(NBMODELS, 1);
	setOption(AUTOCOMPLETE, true);
	setOption(BoolType::SHOWWARNINGS, false);
	return old;
}





}
