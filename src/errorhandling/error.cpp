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

#include <iostream>
#include "parseinfo.hpp"
#include "error.hpp"
#include "GlobalData.hpp"
using namespace std;
using namespace Error;

std::ostream& operator<<(std::ostream& stream, ComponentType t) {
	switch (t) {
	case ComponentType::Namespace:
		stream << "Namespace";
		break;
	case ComponentType::Vocabulary:
		stream << "Vocabulary";
		break;
	case ComponentType::Theory:
		stream << "Theory";
		break;
	case ComponentType::Structure:
		stream << "Structure";
		break;
	case ComponentType::Query:
		stream << "Query";
		break;
	case ComponentType::Term:
		stream << "Term";
		break;
	case ComponentType::Procedure:
		stream << "Procedure";
		break;
	case ComponentType::Predicate:
		stream << "Predicate";
		break;
	case ComponentType::Function:
		stream << "Function";
		break;
	case ComponentType::Symbol:
		stream << "Symbol";
		break;
	case ComponentType::Sort:
		stream << "Sort";
		break;
	case ComponentType::Variable:
		stream << "Variable";
		break;
	}
	return stream;
}

unsigned int Error::nr_of_errors() {
	return GlobalData::instance()->getErrorCount();
}

unsigned int Warning::nr_of_warnings() {
	return GlobalData::instance()->getWarningCount();
}

void Error::error(const std::string& message) {
	stringstream ss;
	ss << "Error: " << message << "\n";
	GlobalData::instance()->notifyOfError(ss.str());
	clog << ss.str();
}

void Error::error(const std::string& message, const ParseInfo& p) {
	stringstream ss;
	ss << message <<" At " << print(p);
	error(ss.str());
}

void Warning::warning(const std::string& message) {
	if (getOption(BoolType::SHOWWARNINGS)) {
		stringstream ss;
		ss << "Warning: " << message << "\n";
		GlobalData::instance()->notifyOfWarning(ss.str());
		clog << ss.str();
	}
}

void Warning::warning(const std::string& message, const ParseInfo& p) {
	stringstream ss;
	ss << message << " At " << print(p);
	warning(ss.str());
}

/** Command line errors **/

void Error::unknoption(const string& s) {
	stringstream ss;
	ss << "'" << s << "' is an unknown option.";
	error(ss.str());
}

void Error::unknoption(const string& s, const ParseInfo& pi) {
	stringstream ss;
	ss << "'" << s << "' is an unknown option.";
	error(ss.str(), pi);
}

void Error::constsetexp() {
	stringstream ss;
	ss << "Constant assignment expected after '-c'.";
	error(ss.str());
}

void Error::stringconsexp(const string& c, const ParseInfo& pi) {
	stringstream ss;
	ss << "Command line constant " << c << " should be a string constant.";
	error(ss.str(), pi);
}

void Error::twicestdin(const ParseInfo& pi) {
	stringstream ss;
	ss << "Stdin can be parsed only once.";
	error(ss.str(), pi);
}

/** File errors **/

void Error::cyclicinclude(const string& s, const ParseInfo& pi) {
	stringstream ss;
	ss << "File " << s << " includes itself.";
	error(ss.str(), pi);
}

void Error::unexistingfile(const string& s) {
	stringstream ss;
	ss << "Could not open file " << s << ".";
	error(ss.str());
}

void Error::unexistingfile(const string& s, const ParseInfo& pi) {
	stringstream ss;
	ss << "Could not open file " << s << ".";
	error(ss.str(), pi);
}

/** Invalid ranges **/
void Error::invalidrange(int n1, int n2, const ParseInfo& pi) {
	stringstream ss;
	ss << n1 << ".." << n2 << " is an invalid range.";
	error(ss.str(), pi);
}

void Error::invalidrange(char c1, char c2, const ParseInfo& pi) {
	stringstream ss;
	ss << "'" << c1 << ".." << c2 << "' is an invalid range.";
	error(ss.str(), pi);
}

/** Invalid tuples **/
void Error::wrongarity(const ParseInfo& pi) {
	stringstream ss;
	ss << "The tuples in this table have different lengths.";
	error(ss.str(), pi);
}

void Error::incompatiblearity(const string& n, const ParseInfo& pi) {
	stringstream ss;
	ss << "The arity of symbol " << n << " is different from the arity of the table assigned to it.";
	error(ss.str(), pi);
}

void Error::prednameexpected(const ParseInfo& pi) {
	stringstream ss;
	ss << "Expected a name of a predicate, instead of a function name.";
	error(ss.str(), pi);
}

void Error::funcnameexpected(const ParseInfo& pi) {
	stringstream ss;
	ss << "Expected a name of a function, instead of a predicate name.";
	error(ss.str(), pi);
}

/** Invalid interpretations **/
void Error::expectedutf(const string& s, const ParseInfo& pi) {
	stringstream ss;
	ss << "Unexpected '" << s << "', expected 'u', 'ct' or 'cf'.";
	error(ss.str(), pi);
}

void Error::sortelnotinsort(const string& el, const string& p, const string& s, const string& str) {
	stringstream ss;
	ss << "In structure " << str << ", element " << el << " occurs in the domain of sort " << p << " but does not belong to the interpretation of sort " << s
			<< ".";
	error(ss.str());
}

void Error::predelnotinsort(const string& el, const string& p, const string& s, const string& str) {
	stringstream ss;
	ss << "In structure " << str << ", element " << el << " occurs in the interpretation of predicate " << p
			<< " but does not belong to the interpretation of sort " << s << ".";
	error(ss.str());
}

void Error::funcelnotinsort(const string& el, const string& p, const string& s, const string& str) {
	stringstream ss;
	ss << "In structure " << str << ", element " << el << " occurs in the interpretation of function " << p
			<< " but does not belong to the interpretation of sort " << s << ".";
	error(ss.str());
}

void Error::notfunction(const string& f, const string& str, const vector<string>& el) {
	stringstream ss;
	ss << "Tuple (";
	if (el.size()) {
		ss << el[0];
		for (unsigned int n = 1; n < el.size(); ++n) {
			ss << "," << el[n];
		}
	}
	ss << ") has more than one image in the interpretation of function " << f << " in structure " << str << ".";
	error(ss.str());
}

void Error::nottotal(const string& f, const string& str) {
	stringstream ss;
	ss << "The interpretation of function " << f << " in structure " << str << " is non-total.";
	error(ss.str());
}

void Error::threevalsort(const string&, const ParseInfo& pi) {
	stringstream ss;
	ss << "Not allowed to assign a three-valued interpretation to a sort.";
	error(ss.str(), pi);
}

/** Multiple incompatible declarations of the same object **/

void Error::declaredEarlier(ComponentType type, const string& whatname, const ParseInfo& pi, const ParseInfo& prevdeclplace) {
	stringstream ss;
	ss << type << " " << whatname << " is already declared in this scope" << ", namely at " << print(prevdeclplace) << ".";
	error(ss.str(), pi);
}

/** Undeclared objects **/

void Error::notDeclared(ComponentType type, const string& name, const ParseInfo& pi) {
	stringstream ss;
	ss << type << " " << name << " is not declared in this scope.";
	error(ss.str(), pi);
}

/** Unavailable objects **/

void Error::notInVocabularyOf(ComponentType type, ComponentType parentType, const string& name, const string& tname, const ParseInfo& pi) {
	stringstream ss;
	ss << type << " " << name << " is not in the vocabulary of " << parentType << " " << tname << ".";
	error(ss.str(), pi);
}

/** Using overlapping symbols **/
void Error::overloaded(ComponentType type, const string& name, const ParseInfo& p1, const ParseInfo& p2, const ParseInfo& pi) {
	stringstream ss;
	ss << "The " << type << " " << name << " used here could be the " << type << " declared at " << print(p1);
	ss << " or the " << type << " declared at " << print(p2) << ".";
	error(ss.str(), pi);
}
void Error::overloaded(ComponentType type, const string& name, const std::vector<ParseInfo>& possiblelocations, const ParseInfo& pi) {
	stringstream ss;
	ss << "The " << type << " " << name << " used here should be disambiguated as it might refer to :\n";
	for (auto info : possiblelocations) {
		ss << "\tThe " << type << " at " << print(info) << "\n";
	}
	error(ss.str(), pi);
}

/** Sort hierarchy errors **/

void Error::notsubsort(const string& c, const string& p, const ParseInfo& pi) {
	stringstream ss;
	ss << "Sort " << c << " is not a subsort of sort " << p << ".";
	error(ss.str(), pi);
}

void Error::cyclichierarchy(const string& c, const string& p, const ParseInfo& pi) {
	stringstream ss;
	ss << "Cycle introduced between sort " << c << " and sort " << p << ".";
	error(ss.str(), pi);
}

/** Free variables **/
void Error::freevars(const string& fv, const ParseInfo& pi) {
	stringstream ss;
	if (fv.size() > 1) {
		ss << "Variables" << fv << " are not quantified.";
	} else {
		ss << "Variable" << fv << " is not quantified.";
	}
	error(ss.str(), pi);
}

/** Type checking **/

void Error::novarsort(const string& name, const ParseInfo& pi) {
	stringstream ss;
	ss << "Could not derive the sort of variable " << name << ".";
	error(ss.str(), pi);
}

void Error::nodomsort(const string& name, const ParseInfo& pi) {
	stringstream ss;
	ss << "Could not derive the sort of domain element " << name << ".";
	error(ss.str(), pi);
}

void Error::nopredsort(const string& name, const ParseInfo& pi) {
	stringstream ss;
	ss << "Could not derive the sorts of predicate " << name << ".";
	error(ss.str(), pi);
}

void Error::nofuncsort(const string& name, const ParseInfo& pi) {
	stringstream ss;
	ss << "Could not derive the sorts of function " << name << ".\n";
	ss << "Possibly the sorts of its subterms are not subtypes of the sorts of the function's arguments? (e.g. missing isa declaration)";
	error(ss.str(), pi);
}

void Error::wrongsort(const string& termname, const string& termsort, const string& possort, const ParseInfo& pi) {
	stringstream ss;
	ss << "Term " << termname << " has sort " << termsort << " but occurs in a position with sort " << possort << ".";
	error(ss.str(), pi);
}

/** Unknown commands or options **/

void Error::unknopt(const string& name, ParseInfo* pi) {
	stringstream ss;
	ss << "Options " << name << " does not exist.";
	pi ? error(ss.str(), *pi) : error(ss.str());
}

void Error::ambigcommand(const string& name) {
	stringstream ss;
	ss << "Ambiguous call to overloaded procedure " << name << ".";
	error(ss.str());
}

void Error::wrongvalue(const std::string& option, const std::string& value, const ParseInfo&) {
	stringstream ss;
	ss << "Value " << value << " cannot be assigned to option " << option << ".";
	error(ss.str());
}

void Error::expected(ComponentType type, const ParseInfo& pi) {
	stringstream ss;
	ss << "Expected a " << type << ".";
	error(ss.str(), pi);
}

void Warning::cumulchance(double c) {
	stringstream ss;
	ss << "Chance of " << c << " is impossible. Using chance 1 instead.";
	warning(ss.str());
}

void Warning::possiblyInfiniteGrounding(const std::string& formula) {
	stringstream ss;
	ss << "Infinite grounding of formula " << formula << ".";
	warning(ss.str());
}

void Warning::triedAddingSubtypeToVocabulary(const std::string& boundedpredname, const std::string& predname, const std::string& vocname) {
	stringstream ss;
	ss << "Tried to add " << boundedpredname << " to " << vocname << ", instead " << predname << " was added to that vocabulary.";
	warning(ss.str());
}

void Warning::emptySort(const std::string& sortname) {
	stringstream ss;
	ss << "Sort " << sortname << " has an empty interpretation.";
	warning(ss.str());
}

/** Ambiguous partial term **/
void Warning::ambigpartialterm(const string& term, const ParseInfo& pi) {
	stringstream ss;
	ss << "Term " << term << " may lead to an ambiguous meaning of the formula where it occurs.";
	warning(ss.str(), pi);
}

/** Ambiguous statements **/
void Warning::varcouldbeconst(const string& name, const ParseInfo& pi) {
	stringstream ss;
	ss << "'" << name << "' could be a variable or a constant. It is assumed to be a variable.";
	warning(ss.str(), pi);
}

/** Free variables **/
void Warning::freevars(const string& fv, const ParseInfo& pi) {
	stringstream ss;
	if (fv.size() > 1) {
		ss << "Variables" << fv << " are not quantified.";
	} else {
		ss << "Variable" << fv[0] << " is not quantified.";
	}
	warning(ss.str(), pi);
}

/** Unused variables **/
void Warning::unusedvar(const string& uv, const ParseInfo& pi) {
	stringstream ss;
	ss << "Quantified variable " << uv << " is unused.";
	warning(ss.str(), pi);
}

/** Unexpeded type derivation **/
void Warning::derivevarsort(const string& varname, const string& sortname, const ParseInfo& pi) {
	stringstream ss;
	ss << "Derived sort " << sortname << " for variable " << varname << ".";
	warning(ss.str(), pi);
}

/** Introduced variable **/
void Warning::introducedvar(const string&, const string& sortname, const string& term) {
	stringstream ss;
	ss << "Introduced a variable with sort " << sortname << " for term " << term << ".";
	warning(ss.str());
}

/** Autocompletion **/
void Warning::addingeltosort(const string& elname, const string& sortname, const string& structname) {
	stringstream ss;
	ss << "Adding element " << elname << " to the interpretation of sort " << sortname << " in structure " << structname << ".";
	warning(ss.str());
}

/** Reading from stdin **/
void Warning::readingfromstdin() {
	clog << "(Reading from stdin)\n";
}

void Warning::aspQueriesAreParsedAsFacts(){
	stringstream ss;
	ss << "ASP Queries are currently parsed as ASP facts.";
	warning(ss.str());
}
