/************************************
	execute.cpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#include "execute.hpp"
#include "ground.hpp"
#include "ecnf.hpp"
#include "print.hpp"
#include "data.hpp"
#include "lua.hpp"
#include "error.hpp"

/*
	Connection with lua
*/

namespace BuiltinProcs {

	map<string,vector<Inference*> >	_inferences;	// All inference methods

	void initialize() {
		_inferences["print"].push_back(new PrintTheory(false));
		_inferences["print"].push_back(new PrintVocabulary(false));
		_inferences["print"].push_back(new PrintStructure(false));
		_inferences["print"].push_back(new PrintNamespace(false));
		_inferences["print"].push_back(new PrintTheory(true));
		_inferences["print"].push_back(new PrintVocabulary(true));
		_inferences["print"].push_back(new PrintStructure(true));
		_inferences["print"].push_back(new PrintNamespace(true));
		_inferences["push_negations"].push_back(new PushNegations());
		_inferences["remove_equivalences"].push_back(new RemoveEquivalences());
		_inferences["remove_eqchains"].push_back(new RemoveEqchains());
		_inferences["flatten"].push_back(new FlattenFormulas());
		_inferences["ground"].push_back(new GroundingWithResult());
		_inferences["convert_to_theory"].push_back(new StructToTheory());
		_inferences["move_quantifiers"].push_back(new MoveQuantifiers());
		_inferences["tseitin"].push_back(new ApplyTseitin());
		_inferences["reduce"].push_back(new GroundReduce());
		_inferences["move_functions"].push_back(new MoveFunctions());
		_inferences["model_expand"].push_back(new ModelExpansionInference(false));
		_inferences["model_expand"].push_back(new ModelExpansionInference(true));
		_inferences["load_file"].push_back(new LoadFile());
	}

	bool checkintern(lua_State* L, int n, const string& tp) {
		if(lua_type(L,n) == LUA_TUSERDATA) {
			bool ret = luaL_getmetafield(L,n,tp.c_str());
			if(ret) {
				if(lua_isboolean(L,-1)) ret = lua_toboolean(L,-1);
				lua_pop(L,1);
			}
			return ret;
		}
		if(lua_type(L,n) == LUA_TFUNCTION) {
			lua_getglobal(L,"idp_intern");
			lua_pushvalue(L,n);
			lua_getfield(L,-2,tp.c_str());
			int r = lua_pcall(L,1,1,0);
			if(!r) {
				if(lua_type(L,-1) == LUA_TNIL) {
					lua_pop(L,2);
					return false;
				}
				else {
					lua_pop(L,2);
					return true;
				}
			}
			else {
				lua_pop(L,2);
				return false;
			}
		}
		else return false;
	}

	bool checkarg(lua_State* L, int n, InfArgType t) {
		switch(t) {
			case IAT_THEORY:
				return checkintern(L,n,"gettheory");
			case IAT_VOCABULARY:
				return checkintern(L,n,"getvocabulary");
			case IAT_STRUCTURE:
				return checkintern(L,n,"getstructure");
			case IAT_NAMESPACE:
				return checkintern(L,n,"getnamespace");
			case IAT_OPTIONS:
				return checkintern(L,n,"getoptions");
			case IAT_VOID:
				return true;
			case IAT_NIL:
				return lua_type(L,n) == LUA_TNIL;
			case IAT_NUMBER:
				return lua_type(L,n) == LUA_TNUMBER;
			case IAT_BOOLEAN:
				return lua_type(L,n) == LUA_TBOOLEAN;
			case IAT_STRING:
				return lua_type(L,n) == LUA_TSTRING;
			case IAT_TABLE:
				assert(false);
			case IAT_FUNCTION:
				assert(false);
			case IAT_USERDATA:
				assert(false);
			case IAT_THREAD:
				assert(false);
			case IAT_LIGHTUSERDATA:
				assert(false);
			default:
				assert(false);
		}
		return false;
	}


	void converttolua(lua_State* L, InfArg res, InfArgType t) {
		switch(t) {
			case IAT_THEORY:
			{
				AbstractTheory** ptr = (AbstractTheory**) lua_newuserdata(L,sizeof(AbstractTheory*));
				(*ptr) = res._theory;
				luaL_getmetatable (L,"theory");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_VOCABULARY:
			{
				Vocabulary** ptr = (Vocabulary**) lua_newuserdata(L,sizeof(Vocabulary*));
				(*ptr) = res._vocabulary;
				luaL_getmetatable (L,"vocabulary");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_STRUCTURE:
			{
				AbstractStructure** ptr = (AbstractStructure**) lua_newuserdata(L,sizeof(AbstractStructure*));
				(*ptr) = res._structure;
				luaL_getmetatable (L,"structure");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_NAMESPACE:
			{
				Namespace** ptr = (Namespace**) lua_newuserdata(L,sizeof(Namespace*));
				(*ptr) = res._namespace;
				luaL_getmetatable (L,"namespace");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_OPTIONS:
			{
				InfOptions** ptr = (InfOptions**) lua_newuserdata(L,sizeof(InfOptions*));
				(*ptr) = res._options;
				luaL_getmetatable (L,"options");
				lua_setmetatable(L,-2);
				break;
			}
			case IAT_SET_OF_STRUCTURES:
			{
				vector<AbstractStructure*> vs = *(res._setofstructures);
				lua_newtable(L);
				for(unsigned int n = 0; n < vs.size(); ++n) {
					lua_pushinteger(L,n+1);
					InfArg a; a._structure = vs[n];
					converttolua(L,a,IAT_STRUCTURE);
					lua_settable(L,-3);
				}
				break;
			}
			case IAT_VOID:
				break;
			case IAT_NIL:
				lua_pushnil(L);
				break;
			case IAT_NUMBER:
				lua_pushnumber(L,res._number);
				break;
			case IAT_BOOLEAN:
				lua_pushboolean(L,res._boolean);
				break;
			case IAT_STRING:
				lua_pushstring(L,res._string->c_str());
				break;
			case IAT_TABLE:
				assert(false);
			case IAT_FUNCTION:
				assert(false);
			case IAT_USERDATA:
				assert(false);
			case IAT_THREAD:
				assert(false);
			case IAT_LIGHTUSERDATA:
				assert(false);
			default:
				assert(false);
		}
	}

	vector<string> convertintern(lua_State* L,int n, const string& str) {
		lua_getglobal(L,"idp_intern");
		lua_pushvalue(L,n);
		lua_getfield(L,-2,str.c_str());
		lua_call(L,1,1);
		assert(lua_type(L,-1) == LUA_TTABLE);
		lua_getglobal(L,"table");
		lua_getfield(L,-1,"maxn");
		lua_pushvalue(L,-3);
		lua_call(L,1,1);
		int tabsize = int(lua_tonumber(L,-1));
		lua_pop(L,2);
		vector<string> res;
		for(int a = 1; a <= tabsize; ++a) {
			lua_pushinteger(L,a);
			lua_gettable(L,-2);
			res.push_back(string(lua_tostring(L,-1)));
			lua_pop(L,1);
		}
		lua_pop(L,2);
		return res;
	}

	InfArg convertarg(lua_State* L, int n, InfArgType t) {
		InfArg a;
		switch(t) {
			case IAT_THEORY:
			{	
				if(lua_type(L,n) == LUA_TUSERDATA) {
					a._theory = *((AbstractTheory**)lua_touserdata(L,n));
				}
				else {
					vector<string> vs = convertintern(L,n,"gettheory");
					Namespace* ns = Namespace::global();
					for(unsigned int m = 0; m < vs.size() -1; ++m) {
						ns = ns->subspace(vs[m]);
					}
					a._theory = ns->theory(vs[vs.size()-1]);
				}
				break;
			}
			case IAT_VOCABULARY:
			{
				if(lua_type(L,n) == LUA_TUSERDATA) {
					a._theory = *((AbstractTheory**)lua_touserdata(L,n));
				}
				else {
					vector<string> vs = convertintern(L,n,"getvocabulary");
					Namespace* ns = Namespace::global();
					for(unsigned int m = 0; m < vs.size() -1; ++m) {
						ns = ns->subspace(vs[m]);
					}
					a._vocabulary = ns->vocabulary(vs[vs.size()-1]);
				}
				break;
			}
			case IAT_STRUCTURE:
			{
				if(lua_type(L,n) == LUA_TUSERDATA) {
					a._theory = *((AbstractTheory**)lua_touserdata(L,n));
				}
				else {
					vector<string> vs = convertintern(L,n,"getstructure");
					Namespace* ns = Namespace::global();
					for(unsigned int m = 0; m < vs.size() -1; ++m) {
						ns = ns->subspace(vs[m]);
					}
					a._structure = ns->structure(vs[vs.size()-1]);
				}
				break;
			}
			case IAT_NAMESPACE:
			{
				if(lua_type(L,n) == LUA_TUSERDATA) {
					a._theory = *((AbstractTheory**)lua_touserdata(L,n));
				}
				else {
					vector<string> vs = convertintern(L,n,"getnamespace");
					Namespace* ns = Namespace::global();
					for(unsigned int m = 0; m < vs.size() -1; ++m) {
						ns = ns->subspace(vs[m]);
					}
					a._namespace = ns->subspace(vs[vs.size()-1]);
				}
				break;
			}
			case IAT_OPTIONS:
			{
				if(lua_type(L,n) == LUA_TUSERDATA) {
					a._theory = *((AbstractTheory**)lua_touserdata(L,n));
				}
				else {
					vector<string> vs = convertintern(L,n,"getoptions");
					Namespace* ns = Namespace::global();
					for(unsigned int m = 0; m < vs.size() -1; ++m) {
						ns = ns->subspace(vs[m]);
					}
					a._options = ns->option(vs[vs.size()-1]);
				}
				break;
			}
			case IAT_VOID:
				break;
			case IAT_NIL:
				break;
			case IAT_NUMBER:
				a._number = lua_tonumber(L,n);
				break;
			case IAT_BOOLEAN:
				a._boolean = lua_toboolean(L,n);
				break;
			case IAT_STRING:
				a._string = IDPointer(lua_tostring(L,n));
				break;
			default:
				assert(false);
		}
		return a;
	}

}

int idpcall(lua_State* L) {
	// Collect name
	string name = string(lua_tostring(L,1));
	lua_remove(L,1);
	vector<Inference*> pvi = BuiltinProcs::_inferences[name];

	// Try to find correct inference 
	unsigned int nrargs = lua_gettop(L); 
	vector<Inference*> vi;
	for(unsigned int n = 0; n < pvi.size(); ++n) {
		if(nrargs == pvi[n]->arity()) vi.push_back(pvi[n]);
	}
	vector<Inference*> vi2;
	for(unsigned int n = 0; n < vi.size(); ++n) {
		bool ok = true;
		for(unsigned int m = 1; m <= nrargs; ++m) {
			ok = BuiltinProcs::checkarg(L,m,(vi[n]->intypes())[m-1]);
			if(!ok) break;
		}
		if(ok) vi2.push_back(vi[n]);
	}
	if(vi2.empty()) Error::wrongcommandargs(name + '/' + itos(nrargs));
	else if(vi2.size() == 1) {
		vector<InfArg> via;
		for(unsigned int m = 1; m <= nrargs; ++m)
			via.push_back(BuiltinProcs::convertarg(L,m,(vi2[0]->intypes())[m-1]));
		InfArg res = vi2[0]->execute(via);
		if(vi2[0]->reload()) (Namespace::global())->tolua(L);
		if(vi2[0]->outtype() == IAT_VOID) return 0;
		else {
			BuiltinProcs::converttolua(L,res,vi2[0]->outtype());
			return 1;
		}
	}
	else Error::ambigcommand(name + '/' + itos(nrargs));

	return 0;
}



/*
	Built-in procedures
*/

extern void parsefile(const string&);
InfArg LoadFile::execute(const vector<InfArg>& args) const {
	parsefile(*(args[0]._string));
	InfArg a; 
	return a;
}

InfArg PrintTheory::execute(const vector<InfArg>& args) const {
	InfOptions* opts = Namespace::global()->option("DefaultOptions");
	if(args.size() == 2) opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	Theory* t = dynamic_cast<Theory*>(args[0]._theory);
	string str = printer->print(t);
	delete(printer);
	InfArg a; a._string = IDPointer(str);
	return a;
}

InfArg PrintVocabulary::execute(const vector<InfArg>& args) const {
	InfOptions* opts = Namespace::global()->option("DefaultOptions");
	if(args.size() == 2) opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	string str = printer->print(args[0]._vocabulary);
	delete(printer);
	InfArg a; a._string = IDPointer(str);
	return a;
}

InfArg PrintStructure::execute(const vector<InfArg>& args) const {
	InfOptions* opts = Namespace::global()->option("DefaultOptions");
	if(args.size() == 2) opts = args[1]._options;
	Printer* printer = Printer::create(opts);
	Structure* s = dynamic_cast<Structure*>(args[0]._structure);
	string str = printer->print(s);
	delete(printer);
	InfArg a; a._string = IDPointer(str);
	return a;
}

InfArg PrintNamespace::execute(const vector<InfArg>& args) const {
	// TODO
	// string s = (args[0]._namespace)->to_string();
	//cout << s;
	InfArg a; a._string = IDPointer(string(""));
	return a;
}

InfArg PushNegations::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	TheoryUtils::push_negations(args[0]._theory);
	InfArg a;
	return a;
}

InfArg RemoveEquivalences::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	TheoryUtils::remove_equiv(args[0]._theory);
	InfArg a;
	return a;
}

InfArg FlattenFormulas::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	TheoryUtils::flatten(args[0]._theory);
	InfArg a;
	return a;
}

InfArg RemoveEqchains::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	TheoryUtils::remove_eqchains(args[0]._theory);
	InfArg a;
	return a;
}

GroundingInference::GroundingInference() { 
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_VOID;	
	_description = "Ground the theory and structure and print the grounding";
	_reload = false;
}

InfArg GroundingInference::execute(const vector<InfArg>& args) const {
	assert(args.size() == 2);
	AbstractTheory* t = args[0]._theory;
	AbstractStructure* s = args[1]._structure;
	TheoryUtils::move_functions(t);
	NaiveGrounder ng(s);	
	AbstractTheory* gr = ng.ground(t);
	TheoryUtils::remove_eqchains(gr);
	TheoryUtils::reduce(gr,s);
	TheoryUtils::tseitin(gr);
	EcnfTheory* ecnfgr = TheoryUtils::convert_to_ecnf(gr);
	GroundPrinter* printer = new outputECNF(stdout);
	ecnfgr->print(printer);
	gr->recursiveDelete();
	delete(ecnfgr);
	delete(printer);
	InfArg a;
	return a;
}

GroundingWithResult::GroundingWithResult() { 
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY; 
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_THEORY;	
	_description = "Ground the theory and structure and store the grounding";
	_reload = false;
}

InfArg GroundingWithResult::execute(const vector<InfArg>& args) const {
	assert(args.size() == 2);
	NaiveGrounder ng(args[1]._structure);
	AbstractTheory* gr = ng.ground(args[0]._theory);
	InfArg a; a._theory = gr;
	return a;
}

ModelExpansionInference::ModelExpansionInference(bool opts) {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	if(opts) _intypes.push_back(IAT_OPTIONS);
	_outtype = IAT_SET_OF_STRUCTURES;
	_description = "Performs model expansion on the structure given the theory it should satisfy.";
	_reload = false;
}

InfArg ModelExpansionInference::execute(const vector<InfArg>& args) const {
	AbstractTheory* t = args[0]._theory;
	AbstractStructure* s = args[1]._structure;
	InfOptions* opts = Namespace::global()->option("DefaultOptions");
	if(args.size() == 3) opts = args[2]._options;
	TheoryUtils::move_functions(t);
	NaiveGrounder ng(s);	
	AbstractTheory* gr = ng.ground(t);
	TheoryUtils::remove_eqchains(gr);
	TheoryUtils::reduce(gr,s);
	TheoryUtils::tseitin(gr);
	EcnfTheory* ecnfgr = TheoryUtils::convert_to_ecnf(gr);
	MinisatID::SolverOption modes;
	modes.nbmodels = opts->_nrmodels;
	SATSolver* solver = new SATSolver(modes);
	GroundPrinter* printer = new outputToSolver(solver);
	ecnfgr->print(printer);
	gr->recursiveDelete();
	vector<vector<MinisatID::Literal> > models;
	InfArg a; a._setofstructures = new vector<AbstractStructure*>();
	vector<MinisatID::Literal> assumpts;
	MinisatID::Solution* sol = new MinisatID::Solution(false, true, true, modes.nbmodels, assumpts);
	bool sat = solver->solve(sol);

	if(sat){
		for(int i=0; i<sol->getModels().size(); i++){
			AbstractStructure* mod = s->clone();
			mod->forcetwovalued();
			for(int j=0; j<sol->getModels()[i].size(); j++) {
				if(!(sol->getModels()[i][j].hasSign())) {
					PFSymbol* pfs = ecnfgr->translator()->symbol((sol->getModels()[i][j].getAtom().getValue()));
					if(pfs && mod->vocabulary()->contains(pfs)) {
						//cout << pfs->to_string() << '(';
						vector<TypedElement> args = ecnfgr->translator()->args(sol->getModels()[i][j].getAtom().getValue());
						for(unsigned int n = 0; n < args.size(); ++n) {
						//	cout << args[n];
						//	if(n < args.size()-1) cout << ',';
						}
						//cout << "). ";
					}
				}
			}
			a._setofstructures->push_back(mod);
		}
	}
	delete(solver);
	delete(ecnfgr);
	delete(printer);
	return a;
}

InfArg StructToTheory::execute(const vector<InfArg>& args) const {
	assert(args.size() == 1);
	AbstractTheory* t = StructUtils::convert_to_theory(args[0]._structure);
	InfArg a; a._theory = t;
	return a;
}

InfArg MoveQuantifiers::execute(const vector<InfArg>& args) const {
	TheoryUtils::move_quantifiers(args[0]._theory);
	InfArg a;
	return a;
}

InfArg ApplyTseitin::execute(const vector<InfArg>& args) const {
	TheoryUtils::tseitin(args[0]._theory);
	InfArg a;
	return a;
}

GroundReduce::GroundReduce() {
	_intypes = vector<InfArgType>(2);
	_intypes[0] = IAT_THEORY;
	_intypes[1] = IAT_STRUCTURE;
	_outtype = IAT_VOID;
	_description = "Replace ground atoms in the theory by their truth value in the structure";
	_reload = false;
}

InfArg GroundReduce::execute(const vector<InfArg>& args) const {
	TheoryUtils::reduce(args[0]._theory,args[1]._structure);
	InfArg a;
	return a;
}

InfArg MoveFunctions::execute(const vector<InfArg>& args) const {
	TheoryUtils::move_functions(args[0]._theory);
	InfArg a;
	return a;
}
