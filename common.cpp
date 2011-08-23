/************************************
	common.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <string>
#include <iostream>
#include <ostream>
#include <vector>
#include <limits>
#include <cassert>
#include <stdlib.h>
#include <string>
#include <sstream>

#include "common.hpp"

using namespace std;

string libraryname = INTERNALLIBARYNAME;
string lualibraryfilename = INTERNALLIBARYLUA;
string idplibraryfilename = INTERNALLIBARYIDP;
string configfilename = CONFIGFILENAME;
string getLibraryName() { return libraryname; }
string getLuaLibraryFilename() { return lualibraryfilename; }
string getIDPLibraryFilename() { return idplibraryfilename; }
string getConfigFilename() { return configfilename; }

void notyetimplemented(const string& message) {
	cerr << "WARNING or ERROR: The following feature is not yet implemented:\n"
		 << '\t' << message << '\n'
		 << "Please send an e-mail to krr@cs.kuleuven.be if you really need this feature.\n";
}

bool isInt(double d) {
	return (double(int(d)) == d);
}

bool isInt(const string& s) {
	stringstream i(s);
	int n;
	return (i >> n);
}

bool isDouble(const string& s) {
	stringstream i(s);
	double d;
	return (i >> d);
}

int toInt(const string& s) {
	stringstream i(s);
	int n;
	if(!(i >> n)) return 0;
	else return n;
}

double toDouble(const string& s) {
	stringstream i(s);
	double d;
	if(!(i >> d)) return 0;
	else return d;
}

void printtabs(ostream& output, unsigned int tabs) {
	for(unsigned int n = 0; n < tabs; ++n) 
		output << ' ';
}

double applyAgg(const AggFunction& agg, const vector<double>& args) {
	double d;
	switch(agg) {
		case AggFunction::CARD:
			d = double(args.size());
			break;
		case AggFunction::SUM:
			d = 0;
			for(unsigned int n = 0; n < args.size(); ++n) d += args[n];
			break;
		case AggFunction::PROD:
			d = 1;
			for(unsigned int n = 0; n < args.size(); ++n) d = d * args[n];
			break;
		case AggFunction::MIN:
			d = numeric_limits<double>::max();
			for(unsigned int n = 0; n < args.size(); ++n) d = (d <= args[n] ? d : args[n]);
			break;
		case AggFunction::MAX:
			d = numeric_limits<double>::min();
			for(unsigned int n = 0; n < args.size(); ++n) d = (d >= args[n] ? d : args[n]);
			break;
	}
	return d;
}

CompType invertcomp(CompType comp) {
	switch(comp) {
		case CompType::EQ: case CompType::NEQ: return comp;
		case CompType::LT: return CompType::GT;
		case CompType::GT: return CompType::LT;
		case CompType::LEQ: return CompType::GEQ;
		case CompType::GEQ: return CompType::LEQ;
		default:
			assert(false);
			return CompType::EQ;
	}
}

CompType negatecomp(CompType comp) {
	switch(comp) {
		case CompType::EQ: return CompType::NEQ;
		case CompType::NEQ: return CompType::EQ;
		case CompType::LT: return CompType::GEQ;
		case CompType::GT: return CompType::LEQ;
		case CompType::LEQ: return CompType::GT;
		case CompType::GEQ: return CompType::LT;
		default:
			assert(false);
			return CompType::EQ;
	}
}

TsType reverseImplication(TsType type){
	if(type == TsType::IMPL){
		return TsType::RIMPL;
	}
	if(type == TsType::RIMPL){
		return TsType::IMPL;
	}
	return type;
}

// Negate a context
Context swapcontext(Context ct) {
	Context temp;
	switch(ct) {
		case Context::BOTH: 	temp = Context::BOTH; break;
		case Context::POSITIVE:	temp = Context::NEGATIVE; break;
		case Context::NEGATIVE:	temp = Context::POSITIVE; break;
	}
	return temp;
}

bool isPos(SIGN s){
	return s==SIGN::POS;
}
bool isNeg(SIGN s){
	return s==SIGN::NEG;
}

SIGN operator not(SIGN rhs){
	return rhs==SIGN::POS?SIGN::NEG:SIGN::POS;
}

SIGN operator~(SIGN rhs){
	return not rhs;
}

QUANT operator not (QUANT t){
	return t==QUANT::UNIV?QUANT::EXIST:QUANT::UNIV;
}

/*********************
	Shared strings
*********************/

#include <tr1/unordered_map>
typedef std::tr1::unordered_map<std::string,std::string*>	MSSP;
class StringPointers {
	private:
		MSSP	_sharedstrings;		//!< map a string to its shared pointer
	public:
		~StringPointers();
		string*	stringpointer(const std::string&);	//!< get the shared pointer of a string
};

StringPointers::~StringPointers() {
	for(MSSP::iterator it = _sharedstrings.begin(); it != _sharedstrings.end(); ++it) {
		delete(it->second);
	}
}

string* StringPointers::stringpointer(const string& s) {
	MSSP::iterator it = _sharedstrings.find(s);
	if(it != _sharedstrings.end()) return it->second;
	else {
		string* sp = new string(s);
		_sharedstrings[s] = sp;
		return sp;
	}
}

StringPointers sharedstrings;

string* StringPointer(const char* str) {
	return sharedstrings.stringpointer(string(str));
}

string* StringPointer(const string& str) {
	return sharedstrings.stringpointer(str);
}

CompType invertct(CompType ct) {
	switch(ct) {
		case CompType::EQ: case CompType::NEQ: return ct;
		case CompType::LT: return CompType::GT;
		case CompType::GT: return CompType::LT;
		case CompType::LEQ: return CompType::GEQ;
		case CompType::GEQ: return CompType::LEQ;
		default:
			assert(false);
			return CompType::EQ;
	}
}

CompType negatect(CompType ct) {
	switch(ct) {
		case CompType::EQ: return CompType::NEQ;
		case CompType::NEQ: return CompType::EQ;
		case CompType::LT: return CompType::GEQ;
		case CompType::GT: return CompType::LEQ;
		case CompType::LEQ: return CompType::GT;
		case CompType::GEQ: return CompType::LT;
	}
}
