/************************************
	error.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <iostream>
#include "error.hpp"
#include "options.hpp"
#include "data.hpp"

namespace Error {
	
	/** Data **/
	unsigned int errorcounter = 0;
	unsigned int nr_of_errors()	{ return errorcounter;	}

	/** Global error messages **/

	void error() {
		errorcounter++;
		cerr << "ERROR: ";
	}

	void error(const ParseInfo& p) {
		errorcounter++;
		cerr << "ERROR at line " << p.line() 
			 << ", column " << p.col();
		if(p.file()) cerr << " of file " << *(p.file());
		cerr << ": ";
	}

	/** Command line errors **/

	void constnotset(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Constant '" << s << "' should be set at the command line." << endl;
	}

	void unknoption(const string& s) {
		error();
		cerr << "'" << s << "' is an unknown option." << endl;
	}

	void unknfile(const string& s) {
		error();
		cerr << "'" << s << "' is not a valid file name or not readable." << endl;
	}

	void unknformat(const string& s) {
		error();
		cerr << "'" << s << "' is an unknown format." << endl;
	}

	void constsetexp() {
		error();
		cerr << "Constant assignment expected after '-c'." << endl;
	}

	void stringconsexp(const string& c, const ParseInfo& pi) {
		error(pi);
		cerr << "Command line constant " << c << " should be a string constant.\n";
	}

	void twicestdin(const ParseInfo& pi) {
		error(pi);
		cerr << "stdin can be parsed only once.\n";
	}

	void nrmodelsnegative() {
		error();
		cerr << "Expected a non-negative integer after '-n'" << endl;
	}

	/** File errors **/

	void cyclicinclude(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "File " << s << " includes itself.\n";
	}

	void unexistingfile(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Could not open file " << s << ".\n";
	}

	void unabletoopenfile(const string& s) {
		error();
		cerr << "Could not open file " << s << ".\n";
	}

	/** Invalid ranges **/
	void invalidrange(int n1, int n2, const ParseInfo& pi) {
		error(pi);
		cerr << n1 << ".." << n2 << " is an invalid range." << endl;
	}

	void invalidrange(char c1, char c2, const ParseInfo& pi) {
		error(pi);
		cerr << "'" << c1 << ".." << c2 << "' is an invalid range." << endl;
	}

	/** Invalid tuples **/
	void wrongarity(const ParseInfo& pi) {
		error(pi);
		cerr << "The tuples in this table have different lengths" << endl;
	}

	void wrongpredarity(const string& p, const ParseInfo& pi) {
		error(pi);
		cerr << "Wrong number of terms given to predicate " << p << ".\n";
	}

	void wrongfuncarity(const string& f, const ParseInfo& pi) {
		error(pi);
		cerr << "Wrong number of terms given to function " << f << ".\n";
	}

	/** Invalid interpretations **/
	void expectedutf(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Unexpected '" << s << "', expected 'u', 'ct' or 'cf'\n";
	}

	void multpredinter(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Multiple interpretations for predicate " << s << ".\n";
	}

	void multfuncinter(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Multiple interpretations for function " << s << ".\n";
	}

	void emptyassign(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "I cannot assign the empty interpretation to symbol " << s
			 << " because all symbols with that name do already have an interpretation." << endl;
	}

	void emptyambig(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "I cannot unambiguously assign the empty interpretation to symbol " << s
			 << " because there is more than one symbol with that name and without an interpretation." << endl;
	}

	void multunknpredinter(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Multiple interpretations for predicate " << s << "[u].\n";
	}

	void multunknfuncinter(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Multiple interpretations for function " << s << "[u].\n";
	}

	void multctpredinter(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Multiple interpretations for predicate " << s << "[ct].\n";
	}

	void multcfpredinter(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Multiple interpretations for predicate " << s << "[cf].\n";
	}

	void multctfuncinter(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Multiple interpretations for function " << s << "[ct].\n";
	}

	void multcffuncinter(const string& s, const ParseInfo& pi) {
		error(pi);
		cerr << "Multiple interpretations for function " << s << "[cf].\n";
	}

	void threethreepred(const string& s, const string& str) {
		error();
		cerr << "In structure " << str << ", an interpretation for predicate " 
			 << s << "[ct], " << s << "[cf], and " << s << "[u] is given. \n";
	}

	void threethreefunc(const string& s, const string& str) {
		error();
		cerr << "In structure " << str << ", an interpretation for function " 
			 << s << "[ct], " << s << "[cf], and " << s << "[u] is given. \n";
	}

	void onethreepred(const string& s, const string& str) {
		error();
		cerr << "In structure " << str << ", an interpretation for predicate " << s
			 << "[u] is given, but there is no interpretation for " << s << "[ct] or " << s << "[cf].\n";
	}

	void onethreefunc(const string& s, const string& str) {
		error();
		cerr << "In structure " << str << ", an interpretation for function " << s
			 << "[u] is given, but there is no interpretation for " << s << "[ct] or " << s << "[cf].\n";
	}

	void sortelnotinsort(const string& el, const string& p, const string& s, const string& str) {
		error();
		cerr << "In structure " << str << ", element " << el
			 << " occurs in the domain of sort " << p
			 << " but does not belong to the interpretation of sort " << s << ".\n";
	}

	void predelnotinsort(const string& el, const string& p, const string& s, const string& str) {
		error();
		cerr << "In structure " << str << ", element " << el
			 << " occurs in the interpretation of predicate " << p
			 << " but does not belong to the interpretation of sort " << s << ".\n";
	}

	void funcelnotinsort(const string& el, const string& p, const string& s, const string& str) {
		error();
		cerr << "In structure " << str << ", element " << el
			 << " occurs in the interpretation of function " << p
			 << " but does not belong to the interpretation of sort " << s << ".\n";
	}

	void notfunction(const string& f, const string& str, const vector<string>& el) {
		error();
		cerr << "Tuple (";
		if(el.size()) {
			cerr << el[0];
			for(unsigned int n = 1; n < el.size(); ++n) cerr << "," << el[n];
		}
		cerr << ") has more than one image in the interpretation of function " << f
			 << " in structure " << str << ".\n";
	}

	void nottotal(const string& f, const string& str) {
		error();
		cerr << "The interpretation of function " << f << " in structure " << str << " is non-total.\n";
	}

	/** Multiple incompatible declarations of the same object **/

	void multdeclns(const string& nsname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
		error(thisplace);
		cerr << "Namespace " << nsname << " is already declared in this scope" 
			 << ", namely at line " << prevdeclplace.line() << ", column " << prevdeclplace.col(); 
		if(prevdeclplace.file()) cerr << " of file " << *(prevdeclplace.file());
		cerr << "." << endl;
	}

	void multdeclvoc(const string& vocname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
		error(thisplace);
		cerr << "Vocabulary " << vocname << " is already declared in this scope" 
			 << ", namely at line " << prevdeclplace.line() << ", column " << prevdeclplace.col(); 
		if(prevdeclplace.file()) cerr << " of file " << *(prevdeclplace.file());
		cerr << "." << endl;
	}

	void multdecltheo(const string& thname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
		error(thisplace);
		cerr << "Theory " << thname << " is already declared in this scope" 
			 << ", namely at line " << prevdeclplace.line() << ", column " << prevdeclplace.col(); 
		if(prevdeclplace.file()) cerr << " of file " << *(prevdeclplace.file());
		cerr << "." << endl;
	}

	void multdeclstruct(const string& sname, const ParseInfo& thisplace, const ParseInfo& prevdeclplace) {
		error(thisplace);
		cerr << "Structure " << sname << " is already declared in this scope" 
			 << ", namely at line " << prevdeclplace.line() << ", column " << prevdeclplace.col(); 
		if(prevdeclplace.file()) cerr << " of file " << *(prevdeclplace.file());
		cerr << "." << endl;
	}

	/** Undeclared objects **/

	void undeclvoc(const string& vocname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Vocabulary " << vocname << " is not declared in this scope." << endl;
	}

	void undecltheo(const string& tname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Theory " << tname << " is not declared in this scope." << endl;
	}

	void undeclstruct(const string& sname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Structure " << sname << " is not declared in this scope." << endl;
	}

	void undeclsort(const string& sname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Sort " << sname << " is not declared in this scope." << endl;
	}

	void undeclpred(const string& pname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Predicate " << pname << " is not declared in this scope." << endl;
	}

	void undeclfunc(const string& fname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Function " << fname << " is not declared in this scope." << endl;
	}

	void undeclsymb(const string& name, const ParseInfo& pi) {
		error(pi);
		cerr << "Predicate or function " << name << " is not declared in this scope." << endl;
	}

	/** Unavailable objects **/

	void sortnotintheovoc(const string& sname, const string& tname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Sort " << sname << " is not in the vocabulary of theory " << tname << "." << endl;
	}

	void prednotintheovoc(const string& pname, const string& tname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Predicate " << pname << " is not in the vocabulary of theory " << tname << "." << endl;
	}

	void funcnotintheovoc(const string& fname, const string& tname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Function " << fname << " is not in the vocabulary of theory " << tname << "." << endl;
	}

	void symbnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Symbol " << name << " does not belong to the vocabulary of structrure " << sname << "." << endl;
	}

	void sortnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Sort " << name << " does not belong to the vocabulary of structrure " << sname << "." << endl;
	}

	void prednotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Predicate " << name << " does not belong to the vocabulary of structrure " << sname << "." << endl;
	}

	void funcnotinstructvoc(const string& name, const string& sname, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Function " << name << " does not belong to the vocabulary of structrure " << sname << "." << endl;
	}

	/** Using overlapping symbols **/
	void predorfuncsymbol(const string& name, const ParseInfo& pi) {
		error(pi);
		cerr << name << " could be the predicate " << name << "/1 or the function " << name << "/0 at this place.\n";
	}

	/** Sort hierarchy errors **/

	void notsubsort(const string& c, const string& p, const ParseInfo& pi) {
		error(pi);
		cerr << "Sort " << c << " is not a subsort of sort " << p << ".\n";
	}

	void cyclichierarchy(const string& c, const string& p, const ParseInfo& pi) {
		error(pi);
		cerr << "Cycle introduced between sort " << c << " and sort " << p << ".\n"; 
	}

	/** Type checking **/
	
	void novarsort(const string& name, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Could not derive the sort of variable " << name << ".\n";
	}

	void nodomsort(const string& name, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Could not derive the sort of domain element " << name << ".\n";
	}

	void nopredsort(const string& name, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Could not derive the sorts of predicate " << name << ".\n";
	}

	void nofuncsort(const string& name, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Could not derive the sorts of function " << name << ".\n";
	}

	void wrongsort(const string& termname, const string& termsort, const string& possort, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Term " << termname << " has sort " << termsort << " but occurs in a position with sort " << possort << ".\n";
	}

	/** Unknown commands or options **/

	void unkncommand(const string& name, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Command " << name << " does not exist." << endl;
	}

	void wrongcommandargs(const string& name, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "The arguments given to command " << name << " are either of the wrong type, or do not exist.\n";
	}

	void ambigcommand(const string& name, const ParseInfo& thisplace) {
		error(thisplace);
		cerr << "Ambiguous call to overloaded command " << name << ".\n";
	}
}

namespace Warning {

	/** Data **/
	unsigned int warningcounter = 0;

	/** Global error messages **/

	void warning(const ParseInfo& p) {
		warningcounter++;
		cerr << "WARNING at line " << p.line() 
			 << ", column " << p.col();
		if(p.file()) cerr << " of file " << *(p.file());
		cerr << ": ";
	}

	/** Ambiguous statements **/

	void varcouldbeconst(const string& name, const ParseInfo& thisplace) {
		if(_options._warning[WT_VARORCONST]) {
			warning(thisplace);
			cerr << "'" << name << "' could be a variable or a constant. GidL assumes it is a variable.\n";
		}
	}

	/** Free variables **/
	void freevars(const string& fv, const ParseInfo& thisplace) {
		if(_options._warning[WT_FREE_VARS]) {
			warning(thisplace);
			if(fv.size() > 1) cerr << "Variables" << fv << " are not quantified.\n";
			else cerr << "Variable" << fv[0] << " is not quantified.\n";
		}
	}

	/** Unexpeded type derivation **/
	void derivevarsort(const string& varname, const string& sortname, const ParseInfo& thisplace) {
		if(_options._warning[WT_SORTDERIVE]) {
			warning(thisplace);
			cerr << "Derived sort " << sortname << " for variable " << varname << ".\n";
		}
	}

	/** Reading from stdin **/
	void readingfromstdin() {
		if(_options._warning[WT_STDIN]) {
			cerr << "(Reading from stdin)\n";
		}
	}
}

namespace Info {
	
	/** Information **/
	void print(const string& s) {
		if(_options._verbose) {
			cerr << s << endl;
		}
	}
}
