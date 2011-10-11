/************************************
	options.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include <sstream>
#include <limits>
#include "options.hpp"
#include "error.hpp"
#include <algorithm>
using namespace std;

std::string str(Language choice){
	switch(choice){
		case Language::TXT: return "txt";
		case Language::IDP: return "idp";
		case Language::ECNF: return "ecnf";
		case Language::TPTP: return "tptp";
		case Language::CNF: return "cnf";
		case Language::ASP: return "asp";
		case Language::LATEX: return "latex";
	}
}

std::string str(Format choice){
	switch(choice){
		case Format::TWOVALUED: return "twovalued";
		case Format::THREEVALUED: return "threevalued";
		case Format::ALL: return "all";
	}
}

// TODO add descriptions to options

Options::Options(const string& name, const ParseInfo& pi) : _name(name), _pi(pi) {
	std::set<bool> boolvalues{true, false};
	BoolPol::createOption(BoolType::PRINTTYPES, "printtypes", boolvalues, true, _option2name);
	BoolPol::createOption(BoolType::CPSUPPORT, "cpsupport", boolvalues, false, _option2name);
	BoolPol::createOption(BoolType::TRACE, "trace", boolvalues, false, _option2name);
	BoolPol::createOption(BoolType::AUTOCOMPLETE, "autocomplete", boolvalues, true, _option2name);
	BoolPol::createOption(BoolType::LONGNAMES, "longnames", boolvalues, false, _option2name);
	BoolPol::createOption(BoolType::RELATIVEPROPAGATIONSTEPS, "relativepropsteps", boolvalues, true, _option2name);
	BoolPol::createOption(BoolType::CREATETRANSLATION, "createtranslation", boolvalues, false, _option2name);

	IntPol::createOption(IntType::SATVERBOSITY, "satverbosity", 0,numeric_limits<int>::max(),0, _option2name);
	IntPol::createOption(IntType::GROUNDVERBOSITY, "groundverbosity", 0,numeric_limits<int>::max(),0, _option2name);
	IntPol::createOption(IntType::PROPAGATEVERBOSITY, "propagateverbosity", 0,numeric_limits<int>::max(),0, _option2name);
	IntPol::createOption(IntType::NRMODELS, "nrmodels", 0,numeric_limits<int>::max(),1, _option2name);
	IntPol::createOption(IntType::NRPROPSTEPS, "nrpropsteps", 0,numeric_limits<int>::max(),4, _option2name);
	IntPol::createOption(IntType::LONGESTBRANCH, "longestbranch", 0,numeric_limits<int>::max(),8, _option2name);
	IntPol::createOption(IntType::SYMMETRY, "symmetry", 0,numeric_limits<int>::max(),0, _option2name);
	IntPol::createOption(IntType::PROVERTIMEOUT, "provertimeout", 0,numeric_limits<int>::max(),numeric_limits<int>::max(), _option2name);

	StringPol::createOption(StringType::LANGUAGE,
				"language",
				std::set<std::string>{str(Language::TXT), str(Language::IDP),
									str(Language::LATEX), str(Language::ECNF),
									str(Language::ASP), str(Language::TPTP),
									str(Language::ASP)},
				str(Language::IDP)
				, _option2name);
	StringPol::createOption(StringType::MODELFORMAT,
				"modelformat",
				std::set<std::string>{str(Format::TWOVALUED),str(Format::THREEVALUED),str(Format::ALL)},
				str(Format::THREEVALUED)
				, _option2name);
}

template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::createOption(EnumType type, const std::string& name, const ValueType& lowerbound, const ValueType& upperbound, const ValueType& defaultValue, std::vector<std::string>& option2name){
	_name2type[name] = type;
	auto newoption = new RangeOption<EnumType, ValueType>(type, name, lowerbound, upperbound);
	newoption->setValue(defaultValue);
	std::vector<TypedOption<EnumType, ValueType>*>& options = _options;
	if(options.size()<=type){
		options.resize(type+1, NULL);
		option2name.resize(type+1, "");
		option2name[type] = name;
	}
	options[type] =  newoption;
}
template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::createOption(EnumType type, const std::string& name, const std::set<ValueType>& values, const ValueType& defaultValue, std::vector<std::string>& option2name){
	_name2type[name] = type;
	auto newoption = new EnumeratedOption<EnumType, ValueType>(type, name, values);
	newoption->setValue(defaultValue);
	std::vector<TypedOption<EnumType, ValueType>*>& options = _options;
	if(options.size()<=type){
		options.resize(type+1, NULL);
		option2name.resize(type+1, "");
		option2name[type] = name;
	}
	options[type] = newoption;
}

template<class EnumType, class ValueType>
void OptionPolicy<EnumType, ValueType>::copyValues(Options* opts){
	for(auto i=_options.begin(); i<_options.end(); ++i){
		(*i)->setValue(opts->getValue((*i)->getType()));
	}
}

template<class EnumType, class ConcreteType>
std::string RangeOption<EnumType, ConcreteType>::printOption() const {
	std::stringstream ss; // TODO correct usage
	ss <<"\t" <<TypedOption<EnumType, ConcreteType>::getName() <<" = " <<TypedOption<EnumType, ConcreteType>::getValue();
	ss <<"\n\t\t => between " <<lower() <<" and " <<upper() <<".";
	return ss.str();
}

template<>
std::string EnumeratedOption<BoolType, bool>::printOption() const {
	std::stringstream ss;
	ss <<"\t" <<TypedOption<BoolType, bool>::getName() <<" = " <<(TypedOption<BoolType, bool>::getValue()?"true":"false");

	ss <<"\n\t\t => one of [";
	bool begin = true;
	for(auto i=getAllowedValues().begin(); i!=getAllowedValues().end(); ++i){
		if(not begin){
			ss <<", ";
		}
		begin = false;
		ss <<((*i)?"true":"false");
	}
	ss <<"]";
	return ss.str();
}
template<class EnumType, class ConcreteType>
std::string EnumeratedOption<EnumType, ConcreteType>::printOption() const {
	std::stringstream ss;
	ss <<"\t" <<TypedOption<EnumType, ConcreteType>::getName() <<" = " <<TypedOption<EnumType, ConcreteType>::getValue();

	ss <<"\n\t\t => one of [";
	bool begin = true;
	for(auto i=getAllowedValues().begin(); i!=getAllowedValues().end(); ++i){
		if(not begin){
			ss <<", ";
		}
		begin = false;
		ss <<*i;
	}
	ss <<"]";
	return ss.str();
}

Language Options::language() const {
	const std::string& value = StringPol::getValue(StringType::LANGUAGE);
	if(value.compare(str(Language::TXT))==0){
		return Language::TXT;
	}else if(value.compare("tptp")==0){
		return Language::TPTP;
	}else if(value.compare("idp")==0){
		return Language::IDP;
	}else if(value.compare("ecnf")==0){
		return Language::ECNF;
	}else if(value.compare("asp")==0){
		return Language::ASP;
	}else{
		assert(value.compare("latex")==0);
		return Language::LATEX;
	}
}

std::string Options::printAllowedValues(const std::string& name) const{
	if(isOptionOfType<int>(name)){
		return IntPol::printOption(name);
	}else if(isOptionOfType<std::string>(name)){
		return StringPol::printOption(name);
	}else if(isOptionOfType<double>(name)){
		return DoublePol::printOption(name);
	}else if(isOptionOfType<bool>(name)){
		return BoolPol::printOption(name);
	}else{
		return "";
	}
}

bool Options::isOption(const string& optname) const {
	return isOptionOfType<int>(optname)
			|| isOptionOfType<bool>(optname)
			|| isOptionOfType<double>(optname)
			|| isOptionOfType<std::string>(optname);
}

void Options::copyValues(Options* opts) {
	StringPol::copyValues(opts);
	BoolPol::copyValues(opts);
	IntPol::copyValues(opts);
	DoublePol::copyValues(opts);
}

template<class OptionList, class StringList>
void getStringFromOption(const OptionList& list, StringList& newlist){
	for(auto it = list.begin(); it != list.end(); ++it) {
		stringstream ss;
		ss << (*it)->getName() << " = " << (*it)->getValue();
		newlist.push_back(ss.str());
	}
}

ostream& Options::put(ostream& output) const {
	vector<string> optionslines;
	StringPol::addOptionStrings(optionslines);
	IntPol::addOptionStrings(optionslines);
	BoolPol::addOptionStrings(optionslines);
	DoublePol::addOptionStrings(optionslines);

	sort(optionslines.begin(), optionslines.end());
	for(auto i=optionslines.begin(); i<optionslines.end(); ++i){
		output <<*i <<"\n";
	}

	return output;
}

string Options::toString() const {
	stringstream sstr;
	put(sstr);
	return sstr.str();
}
 
