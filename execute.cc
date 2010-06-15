/************************************
	execute.cc
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "execute.h"
#include "ground.h"
#include "ecnf.h"
#include <iostream>

void PrintTheory::execute(const vector<InfArg>& args, const string& res,Namespace*) const {
	assert(args.size() == 1);
	// TODO
	string s = (args[0]._theory)->to_string();
	cout << s;
}

void PrintVocabulary::execute(const vector<InfArg>& args, const string& res,Namespace*) const {
	assert(args.size() == 1);
	// TODO
	string s = (args[0]._vocabulary)->to_string();
	cout << s;
}

void PrintStructure::execute(const vector<InfArg>& args, const string& res,Namespace*) const {
	assert(args.size() == 1);
	// TODO
	string s = (args[0]._structure)->to_string();
	cout << s;
}

void PrintNamespace::execute(const vector<InfArg>& args, const string& res,Namespace*) const {
	assert(args.size() == 1);
	// TODO
	//string s = (args[0]._namespace)->to_string();
	//cout << s;
}

void PushNegations::execute(const vector<InfArg>& args, const string& res,Namespace*) const {
	assert(args.size() == 1);
	TheoryUtils::push_negations(args[0]._theory);
}

void RemoveEquivalences::execute(const vector<InfArg>& args, const string& res,Namespace*) const {
	assert(args.size() == 1);
	TheoryUtils::remove_equiv(args[0]._theory);
}

void FlattenFormulas::execute(const vector<InfArg>& args, const string& res,Namespace*) const {
	assert(args.size() == 1);
	TheoryUtils::flatten(args[0]._theory);
}

void RemoveEqchains::execute(const vector<InfArg>& args, const string& res,Namespace*) const {
	assert(args.size() == 1);
	TheoryUtils::remove_eqchains(args[0]._theory);
}

GroundingInference::GroundingInference() { 
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_VOID;	
	_description = "Ground the theory and structure and print the grounding";
}

void GroundingInference::execute(const vector<InfArg>& args, const string& res,Namespace*) const {
	assert(args.size() == 2);
	NaiveGrounder ng(args[1]._structure);
	Theory* gr = ng.ground(args[0]._theory);
	NaiveTranslator* nt = new NaiveTranslator();
	EcnfTheory* ecnfgr = TheoryUtils::convert_to_ecnf(gr,nt);
	outputECNF* printer = new outputECNF(stdout); 
	ecnfgr->print(printer);
	gr->recursiveDelete();
	delete(ecnfgr);
	delete(nt);
	delete(printer);
}

GroundingWithResult::GroundingWithResult() { 
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_THEORY;	
	_description = "Ground the theory and structure and store the grounding";
}

void GroundingWithResult::execute(const vector<InfArg>& args, const string& res,Namespace* cn) const {
	assert(args.size() == 2);
	NaiveGrounder ng(args[1]._structure);
	Theory* gr = ng.ground(args[0]._theory);
	gr->name(res);
	cn->add(gr);
}


void StructToTheory::execute(const vector<InfArg>& args, const string& res,Namespace* cn) const {
	assert(args.size() == 1);
	Theory* t = StructUtils::convert_to_theory(args[0]._structure);
	t->name(res);
	cn->add(t);
}