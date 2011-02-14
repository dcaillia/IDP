#include "error.hpp"
#include "options.hpp"

void setoption(InfOptions* opts, const string& opt, const string& val, ParseInfo* pi) {
	if(InfOptions::isoption(opt)) {
		if(opt == "language") {
			if(val == "txt") opts->_format=OF_TXT;
			else if(val == "idp") opts->_format=OF_IDP;
			else Error::wrongformat(val,pi);
		}
		else if(opt == "modelformat") {
			if(val == "all") opts->_modelformat=MF_ALL;
			else if(val == "twovalued") opts->_modelformat=MF_TWOVAL;
			else if(val == "threevalued") opts->_modelformat=MF_THREEVAL;
			else Error::wrongmodelformat(val,pi);
		}
		else Error::wrongvaluetype(opt,pi);
	}
	else Error::unknopt(opt,pi);
}

void setoption(InfOptions* opts, const string& opt, double, ParseInfo* pi) {
	if(InfOptions::isoption(opt)) {
		Error::wrongvaluetype(opt,pi);
	}
	else Error::unknopt(opt,pi);
}

void setoption(InfOptions* opts, const string& opt, int val, ParseInfo* pi) {
	if(InfOptions::isoption(opt)) {
		if(opt == "nrmodels") {
			if(val >= 0) {
				opts->_nrmodels = val;
			}
			else Error::posintexpected(opt,pi);
		}
		else Error::wrongvaluetype(opt,pi);
	}
	else Error::unknopt(opt,pi);
}

