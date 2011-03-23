/************************************
	execute.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EXECUTE_HPP
#define EXECUTE_HPP

#include <sstream>
#include <string>
#include <vector>
#include <set>

#include "parseinfo.hpp"
#include "lua.hpp"

class Namespace;
class Vocabulary;
class AbstractTheory;
class AbstractStructure;
class InfOptions;
class Predicate;
class Function;
class Sort;
class PredTable;
class PredInter;
class FuncInter;

/*******************************************
	Argument types for inference methods
*******************************************/

enum InfArgType { 
	IAT_THEORY, 
	IAT_STRUCTURE, 
	IAT_VOCABULARY, 
	IAT_NAMESPACE, 
	IAT_OPTIONS, 
	IAT_NIL, 
	IAT_INT, 
	IAT_DOUBLE,
	IAT_BOOLEAN, 
	IAT_STRING, 
	IAT_TABLE, 
	IAT_PROCEDURE, 
	IAT_OVERLOADED, 
	IAT_SORT,
	IAT_PREDICATE, 
	IAT_FUNCTION,
	IAT_PREDTABLE,
	IAT_PREDINTER,
	IAT_FUNCINTER,
	IAT_TUPLE
};

namespace BuiltinProcs {
	void initialize();
	void cleanup();
}

/** Lua procedures **/

class LuaProcedure {
	protected:
		std::string					_name;		// name without arity
		ParseInfo					_pi;
		std::vector<std::string>	_innames;
		std::stringstream			_code;
		std::string					_registryindex;

	public:
		LuaProcedure() { }
		LuaProcedure(const std::string& name, const ParseInfo& pi) :
			_name(name), _pi(pi), _innames(0), _registryindex("") { }

		// Mutators
		void addarg(const std::string& name)	{ _innames.push_back(name);	}
		void add(char* s)						{ _code << s;				}
		void add(const std::string& s)			{ _code << s;				}
		void compile(lua_State*);
		
		// Inspectors
		const ParseInfo&	pi()			const { return _pi;						}
		const std::string&	name()			const { return _name;					}
		unsigned int		arity()			const { return _innames.size();			}
		virtual std::string	code()			const { return _code.str();				}
		bool				iscompiled()	const { return _registryindex != "";	}
		const std::string&	registryindex()	const { return _registryindex;			}
};

/** class OverloadedObject **/
struct TypedInfArg;

class OverloadedObject {
	private:
		Namespace*			_namespace;
		Vocabulary*			_vocabulary;
		AbstractTheory*		_theory;
		AbstractStructure*	_structure;
		InfOptions*			_options;
		const std::string*	_procedure;

		std::set<Predicate*>*	_predicate;
		std::set<Function*>*	_function;
		std::set<Sort*>*		_sort;

	public:
		// Constructor
		OverloadedObject() : 
			_namespace(0), _vocabulary(0), _theory(0), _structure(0), _options(0), _procedure(0),
			_predicate(0), _function(0), _sort(0) { }

		// Mutators
		void	makenamespace(Namespace* n)			{ _namespace = n;				}
		void	makevocabulary(Vocabulary* v)		{ _vocabulary = v;				}
		void	maketheory(AbstractTheory* t)		{ _theory = t;					}
		void	makestructure(AbstractStructure* s) { _structure = s;				}
		void	makeoptions(InfOptions* o)			{ _options = o;					}
		void	makeprocedure(const std::string* p)	{ _procedure = p;				}
		void	makepredicate(Predicate* p);	
		void	makefunction(Function* f);	
		void	makesort(Sort* s);		

		void	setpredicate(std::set<Predicate*>* s)	{ _predicate = s;	}
		void	setfunction(std::set<Function*>* s)		{ _function = s;	}

		// Inspectors
		bool	isNamespace()	const { return (_namespace != 0);		}
		bool	isVocabulary()	const { return (_vocabulary != 0);		}
		bool	isTheory()		const { return (_theory != 0);			}
		bool	isStructure()	const { return (_structure != 0);		}
		bool	isOptions()		const { return (_options != 0);			}
		bool	isProcedure()	const { return (_procedure != 0);		}
		bool	isPredicate()	const;
		bool	isFunction()	const;
		bool	isSort()		const;

		bool	single()		const;

		Namespace*				getNamespace()		const	{ return _namespace;	}
		Vocabulary*				getVocabulary()		const	{ return _vocabulary;	}
		AbstractTheory*			getTheory()			const	{ return _theory;		}
		AbstractStructure*		getStructure()		const	{ return _structure;	}
		InfOptions*				getOptions()		const	{ return _options;		}
		const std::string*		getProcedure()		const	{ return _procedure;	}
		std::set<Predicate*>*	getPredicate()		const	{ return _predicate;	}
		std::set<Function*>*	getFunction()		const	{ return _function;		}
		std::set<Sort*>*		getSort()			const	{ return _sort;			}
};


/** Possible argument or return value of an execute statement **/
struct TypedInfArg;

struct PredTableTuple {
	PredTable*	_table;
	int			_index;
	PredTableTuple(PredTable* table, int index) : _table(table), _index(index) { }
};

union InfArg {
	Vocabulary*					_vocabulary;
	AbstractStructure*			_structure;
	AbstractTheory*				_theory;
	Namespace*					_namespace;
	double						_double;
	int							_int;
	bool						_boolean;
	std::string*				_string;
	InfOptions*					_options;
	const std::string*			_procedure;		// contains the registry index of a procedure
	OverloadedObject*			_overloaded;
	std::set<Predicate*>*		_predicate;
	std::set<Function*>*		_function;
	std::set<Sort*>*			_sort;
	std::vector<TypedInfArg>*	_table;
	PredInter*					_predinter;
	FuncInter*					_funcinter;
	PredTable*					_predtable;
	PredTableTuple*				_tuple;
};

struct TypedInfArg {
	InfArg		_value;
	InfArgType	_type;
};


/** An execute statement **/
class Inference {
	protected:
		std::vector<InfArgType>	_intypes;		// types of the input arguments
		std::string				_description;	// description of the inference
	public:
		virtual ~Inference() { }
		virtual	TypedInfArg 	execute(const std::vector<InfArg>& args, lua_State*) const = 0;	// execute the statement
				const std::vector<InfArgType>&	intypes()		const { return _intypes;		}
				unsigned int					arity()			const { return _intypes.size();	}
				std::string						description()	const { return _description;	}
};

class LoadFile : public Inference {
	public:
		LoadFile() {
			_intypes = std::vector<InfArgType>(1,IAT_STRING);	
			_description = "Load the given file";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class NewOption : public Inference {
	public:
		NewOption() {	
			_intypes = std::vector<InfArgType>(0);
			_description = "Create a option block";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PrintTheory : public Inference {
	public:
		PrintTheory() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the theory";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PrintVocabulary : public Inference {
	public:
		PrintVocabulary() { 
			_intypes = std::vector<InfArgType>(1,IAT_VOCABULARY); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the vocabulary";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PrintStructure : public Inference {
	public:
		PrintStructure() { 
			_intypes = std::vector<InfArgType>(1,IAT_STRUCTURE); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the structure";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PrintNamespace : public Inference {
	public:
		PrintNamespace() { 
			_intypes = std::vector<InfArgType>(1,IAT_NAMESPACE); 
			_intypes.push_back(IAT_OPTIONS);
			_description = "Print the namespace";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class PushNegations : public Inference {
	public:
		PushNegations() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_description = "Push all negations inside until they are in front of atoms";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class RemoveEquivalences : public Inference {
	public:
		RemoveEquivalences() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_description = "Rewrite equivalences into pairs of implications";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class RemoveEqchains : public Inference {
	public:
		RemoveEqchains() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_description = "Rewrite chains of (in)equalities to conjunctions of atoms";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;

};

class FlattenFormulas : public Inference {
	public:
		FlattenFormulas() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY); 
			_description = "Rewrite ((A & B) & C) to (A & B & C), rewrite (! x : ! y : phi(x,y)) to (! x y : phi(x,y)), etc.";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class FastGrounding : public Inference {
	public:
		FastGrounding();
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class FastMXInference : public Inference {
	public:
		FastMXInference();
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class StructToTheory : public Inference {
	public:
		StructToTheory() {
			_intypes = std::vector<InfArgType>(1,IAT_STRUCTURE); 
			_description = "Rewrite a structure to a theory";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class MoveQuantifiers : public Inference {
	public:
		MoveQuantifiers() {
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Move universal (existential) quantifiers inside conjunctions (disjunctions)";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class ApplyTseitin : public Inference {
	public:
		ApplyTseitin() {
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Apply the tseitin transformation to a theory";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class GroundReduce : public Inference {
	public:
		GroundReduce();
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class MoveFunctions : public Inference {
	public:
		MoveFunctions() {
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Move functions until no functions are nested";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class CloneStructure : public Inference {
	public:
		CloneStructure() { 
			_intypes = std::vector<InfArgType>(1,IAT_STRUCTURE);
			_description = "Make a copy of this structure";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class CloneTheory : public Inference {
	public:
		CloneTheory() { 
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Make a copy of this theory";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class ForceTwoValued : public Inference {
	public:
		ForceTwoValued() {
			_intypes = std::vector<InfArgType>(1,IAT_STRUCTURE);
			_description = "Force structure to be two-valued";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class DeleteData : public Inference {
	public:
		DeleteData(InfArgType t) {
			_intypes = std::vector<InfArgType>(1,t);
			_description = "Delete"; 
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class ChangeVoc : public Inference {
	public:
		ChangeVoc();
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class GetIndex : public Inference {
	public:
		GetIndex(InfArgType table, InfArgType index) {
			_intypes = std::vector<InfArgType>(2);
			_intypes[0] = table; _intypes[1] = index;
			_description = "index function"; 
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class SetIndex : public Inference {
	public:
		SetIndex(InfArgType table, InfArgType key, InfArgType value);
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class LenghtOperator : public Inference {
	public:
		LenghtOperator(InfArgType t) {
			_intypes = std::vector<InfArgType>(1,t);
			_description = "# operator";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class CastOperator : public Inference {
	public:
		CastOperator() {
			_intypes = std::vector<InfArgType>(2); _intypes[0] = IAT_OVERLOADED; _intypes[1] = IAT_STRING;
			_description = "disambiguate overloaded object";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class ArityCastOperator : public Inference {
	public:
		ArityCastOperator(InfArgType t) {
			_intypes = std::vector<InfArgType>(2); _intypes[0] = t; _intypes[1] = IAT_INT;
			_description = "disambiguate overloaded object";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

class BDDPrinter : public Inference {
	public:
		BDDPrinter() {
			_intypes = std::vector<InfArgType>(1,IAT_THEORY);
			_description = "Show the given theory as a bdd";
		}
		TypedInfArg execute(const std::vector<InfArg>& args, lua_State*) const;
};

#endif
