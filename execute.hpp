/************************************
	execute.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef EXECUTE_H
#define EXECUTE_H

#include "namespace.hpp"

/** Possible argument or return value of an execute statement **/
union InfArg {
	Vocabulary*			_vocabulary;
	AbstractStructure*	_structure;
	AbstractTheory*		_theory;
	Namespace*			_namespace;
};

/** An execute statement **/
class Inference {
	protected:
		vector<InfArgType>	_intypes;		// types of the input arguments
		InfArgType			_outtype;		// type of the return value
		string				_description;	// description of the inference
	public:
		virtual ~Inference() { }
		virtual void execute(const vector<InfArg>& args, const string& res,Namespace*) const = 0;	// execute the statement
		const vector<InfArgType>&	intypes()		const { return _intypes;		}
		InfArgType					outtype()		const { return _outtype;		}
		unsigned int				arity()			const { return _intypes.size();	}
		string						description()	const { return _description;	}
};

class PrintTheory : public Inference {
	public:
		PrintTheory() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;
			_description = "Print the theory";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class PrintVocabulary : public Inference {
	public:
		PrintVocabulary() { 
			_intypes = vector<InfArgType>(1,IAT_VOCABULARY); 
			_outtype = IAT_VOID;	
			_description = "Print the vocabulary";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class PrintStructure : public Inference {
	public:
		PrintStructure() { 
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE); 
			_outtype = IAT_VOID;	
			_description = "Print the structure";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class PrintNamespace : public Inference {
	public:
		PrintNamespace() { 
			_intypes = vector<InfArgType>(1,IAT_NAMESPACE); 
			_outtype = IAT_VOID;	
			_description = "Print the namespace";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class PushNegations : public Inference {
	public:
		PushNegations() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Push all negations inside until they are in front of atoms";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class RemoveEquivalences : public Inference {
	public:
		RemoveEquivalences() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Rewrite equivalences into pairs of implications";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class RemoveEqchains : public Inference {
	public:
		RemoveEqchains() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Rewrite chains of (in)equalities to conjunctions of atoms";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;

};

class FlattenFormulas : public Inference {
	public:
		FlattenFormulas() { 
			_intypes = vector<InfArgType>(1,IAT_THEORY); 
			_outtype = IAT_VOID;	
			_description = "Rewrite ((A & B) & C) to (A & B & C), rewrite (! x : ! y : phi(x,y)) to (! x y : phi(x,y)), etc.";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class GroundingInference : public Inference {
	public:
		GroundingInference();
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class GroundingWithResult : public Inference {
	public:
		GroundingWithResult();
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class ModelExpansionInference : public Inference {
	public:
		ModelExpansionInference();
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class StructToTheory : public Inference {
	public:
		StructToTheory() {
			_intypes = vector<InfArgType>(1,IAT_STRUCTURE); 
			_outtype = IAT_THEORY;	
			_description = "Rewrite a structure to a theory";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class MoveQuantifiers : public Inference {
	public:
		MoveQuantifiers() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_VOID;
			_description = "Move universal (existential) quantifiers inside conjunctions (disjunctions)";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class ApplyTseitin : public Inference {
	public:
		ApplyTseitin() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_VOID;
			_description = "Apply the tseitin transformation to a theory";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class GroundReduce : public Inference {
	public:
		GroundReduce();
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};

class MoveFunctions : public Inference {
	public:
		MoveFunctions() {
			_intypes = vector<InfArgType>(1,IAT_THEORY);
			_outtype = IAT_VOID;
			_description = "Move functions until no functions are nested";
		}
		void execute(const vector<InfArg>& args, const string& res,Namespace*) const;
};


#endif