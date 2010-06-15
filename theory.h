/************************************
	theory.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef THEORY_H
#define THEORY_H

#include "structure.h"
#include "term.h"

class GroundTranslator;
class EcnfTheory;

/***************
	Formulas
***************/

/** Abstract base class **/

class Formula {

	protected:

		bool				_sign;	// true iff the formula does not start with a negation
		vector<Variable*>	_fvars;	// free variables of the formula
		ParseInfo*			_pi;	// the place where the formula was parsed (0 for non user-defined formulas)

	public:

		// Constructor
		Formula(bool sign, ParseInfo* pi):   _sign(sign), _pi(pi)  { }

		// Virtual constructors
		virtual	Formula*	clone()									const = 0;	// copy the formula while keeping the free variables
		virtual	Formula*	clone(const map<Variable*,Variable*>&)	const = 0;	// copy the formulas, and replace the free variables
																				// as inidicated by the map

		// Destructor
		virtual void recursiveDelete() = 0;
		virtual ~Formula() { if(_pi) delete(_pi);	}

		// Mutators
		void	setfvars();		// compute the free variables
		void	swapsign()	{ _sign = !_sign;	}

		// Inspectors
				bool			sign()					const { return _sign;			}
				unsigned int	nrFvars()				const { return _fvars.size();	}
				Variable*		fvar(unsigned int n)	const { return _fvars[n];		}
				ParseInfo*		pi()					const { return _pi;				}
		virtual	unsigned int	nrQvars()				const = 0;	// number of variables quantified by the formula
		virtual	unsigned int	nrSubforms()			const = 0;	// number of direct subformulas
		virtual	unsigned int	nrSubterms()			const = 0;  // number of direct subterms
		virtual	Variable*		qvar(unsigned int n)	const = 0;	// the n'th quantified variable
		virtual	Formula*		subform(unsigned int n)	const = 0;	// the n'th direct subformula
		virtual	Term*			subterm(unsigned int n)	const = 0;	// the n'th direct subterm
				bool			contains(Variable*)		const;		// true iff the formula contains the variable

		// Visitor
		virtual void		accept(Visitor* v) = 0;
		virtual Formula*	accept(MutatingVisitor* v) = 0;

		// Debugging
		virtual string to_string()	const = 0;
	
};

/** Atoms **/

class PredForm : public Formula {
	
	private:
		PFSymbol*		_symb;		// the predicate or function
		vector<Term*>	_args;		// the arguments

	public:

		// Constructors
		PredForm(bool sign, PFSymbol* p, const vector<Term*>& a, ParseInfo* pi) : 
			Formula(sign,pi), _symb(p), _args(a) { setfvars(); }

		PredForm*	clone()									const;
		PredForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		void recursiveDelete();

		// Mutators
		void	symb(PFSymbol* s)				{ _symb = s;	}
		void	arg(unsigned int n, Term* t)	{ _args[n] = t;	}

		// Inspectors
		PFSymbol*		symb()					const { return _symb;				}
		unsigned int	nrQvars()				const { return 0;					}
		unsigned int	nrSubforms()			const { return 0;					}
		unsigned int	nrSubterms()			const { return _args.size();		}
		Variable*		qvar(unsigned int n)	const { assert(false); return 0;	}
		Formula*		subform(unsigned int n)	const { assert(false); return 0;	}
		Term*			subterm(unsigned int n)	const { return _args[n];			}
		
		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		string to_string() const;

};


/** Chains of equalities and inequalities **/

class EqChainForm : public Formula {

	private:
		bool			_conj;		// Indicates whether the chain is a conjunction or disjunction of (in)equalties
		vector<Term*>	_terms;		// The consecutive terms in the chain
		vector<char>	_comps;		// The consecutive comparisons ('=', '>' or '<') in the chain
		vector<bool>	_signs;		// The signs of the consecutive comparisons

	public:

		// Constructors
		EqChainForm(bool sign, bool c, Term* t, ParseInfo* pi) : 
			Formula(sign,pi), _conj(c), _terms(1,t), _comps(0), _signs(0) { setfvars(); }
		EqChainForm(bool sign, bool c, const vector<Term*>& vt, const vector<char>& vc, const vector<bool>& vs, ParseInfo* pi) :
			Formula(sign,pi), _conj(c), _terms(vt), _comps(vc), _signs(vs) { setfvars();	}

		EqChainForm*	clone()									const;
		EqChainForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		void recursiveDelete();

		// Mutators
		void add(char c, bool s, Term* t)		{ _comps.push_back(c); _signs.push_back(s); _terms.push_back(t);	}
		void conj(bool b)						{ _conj = b;														}
		void compsign(unsigned int n, bool b)	{ _signs[n] = b;													}
		void term(unsigned int n, Term* t)		{ _terms[n] = t;													}

		// Inspectors
		bool			conj()						const	{ return _conj;				}
		char			comp(unsigned int n)		const	{ return _comps[n];			}
		bool			compsign(unsigned int n)	const	{ return _signs[n];			}
		unsigned int	nrQvars()					const	{ return 0;					}
		unsigned int	nrSubforms()				const	{ return 0;					}
		unsigned int	nrSubterms()				const	{ return _terms.size();		}
		Variable*		qvar(unsigned int n)		const	{ assert(false); return 0;	}
		Formula*		subform(unsigned int n)		const	{ assert(false); return 0;	}
		Term*			subterm(unsigned int n)		const	{ return _terms[n];			}

		const vector<char>&	comps()		const	{ return _comps;	}
		const vector<bool>& compsigns()	const	{ return _signs;	}

		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		string to_string() const;

};

/** Equivalences **/

class EquivForm : public Formula {
	
	protected:
		Formula*	_left;		// left-hand side formula
		Formula*	_right;		// right-hand side formula

	public:
		
		// Constructors
		EquivForm(bool sign, Formula* lf, Formula* rt, ParseInfo* pi) : 
			Formula(sign,pi), _left(lf), _right(rt) { setfvars(); }

		EquivForm*	clone()									const;
		EquivForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		void recursiveDelete();

		// Mutators
		void left(Formula* f)	{ _left = f;	}
		void right(Formula* f)	{ _right = f;	}

		// Inspectors
		Formula*		left()					const { return _left;					}
		Formula*		right()					const { return _right;					}
		unsigned int	nrQvars()				const { return 0;						}
		unsigned int	nrSubforms()			const { return 2;						}
		unsigned int	nrSubterms()			const { return 0;						}	
		Variable*		qvar(unsigned int n)	const { assert(false); return 0;		}
		Formula*		subform(unsigned int n)	const { return (n ? _left : _right);	}
		Term*			subterm(unsigned int n) const { assert(false); return 0;	 	}

		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

		// Debuging
		string to_string() const;

};


/** Conjunctions and disjunctions **/

class BoolForm : public Formula {
	
	private:
		vector<Formula*>	_subf;	// the direct subformulas
		bool				_conj;	// true (false) if the formula is the conjunction (disjunction) of the 
									// formulas in _subf
									
	public:

		// Constructors
		BoolForm(bool sign, bool c, const vector<Formula*>& sb, ParseInfo* pi) :
			Formula(sign,pi), _subf(sb), _conj(c) { setfvars(); }

		BoolForm*	clone()									const;
		BoolForm*	clone(const map<Variable*,Variable*>&)	const;

	    // Destructor
		void recursiveDelete();

		// Mutators
		void	conj(bool b)						{ _conj = b;	}
		void	subf(unsigned int n, Formula* f)	{ _subf[n] = f;	}
		void	subf(const vector<Formula*>& s)		{ _subf = s;	}

		// Inspectors
		bool			conj()					const	{ return _conj;				}
		Formula*		subf(unsigned int n)	const	{ return _subf[n];			}
		unsigned int	nrQvars()				const	{ return 0;					}
		unsigned int	nrSubforms()			const	{ return _subf.size();		}
		unsigned int	nrSubterms()			const	{ return 0;					}
		Variable*		qvar(unsigned int n)	const	{ assert(false); return 0;	}
		Formula*		subform(unsigned int n)	const	{ return _subf[n];			}
		Term*			subterm(unsigned int n)	const	{ assert(false); return 0; 	}

		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		string to_string()	const;

};


/** Universally and existentially quantified formulas **/

class QuantForm : public Formula {

	private:
		vector<Variable*>	_vars;	// the quantified variables
		Formula*			_subf;	// the direct subformula
		bool				_univ;	// true (false) if the quantifier is universal (existential)

	public:

		// Constructors
		QuantForm(bool sign, bool u, const vector<Variable*>& v, Formula* sf, ParseInfo* pi) : 
			Formula(sign,pi), _vars(v), _subf(sf), _univ(u) { setfvars(); }

		QuantForm*	clone()									const;
		QuantForm*	clone(const map<Variable*,Variable*>&)	const;

	   // Destructor
	   void recursiveDelete();

		// Mutators
		void	add(Variable* v)	{ _vars.push_back(v);	}
		void	univ(bool b)		{ _univ = b;			}
		void	subf(Formula* f)	{ _subf = f;			}

		// Inspectors
		Formula*		subf()					const { return _subf;				}
		Variable*		vars(unsigned int n)	const { return _vars[n];			}
		bool			univ()					const { return _univ;				}
		unsigned int	nrQvars()				const { return _vars.size();		}
		unsigned int	nrSubforms()			const { return 1;					}
		unsigned int	nrSubterms()			const { return 0;					}
		Variable*		qvar(unsigned int n)	const { return _vars[n];			}
		Formula*		subform(unsigned int n)	const { return	_subf;				}
		Term*			subterm(unsigned int n)	const { assert(false); return 0;	}

		// Visitor
		void		accept(Visitor* v);
		Formula*	accept(MutatingVisitor* v);

		// Debugging
		string to_string()	const;

};

enum TruthValue { TV_TRUE, TV_FALSE, TV_UNKN, TV_INCO, TV_UNDEF };

namespace FormulaUtils {
	
	/*
	 * Evaulate a formula in a structure under the given variable mapping
	 *	Preconditions: 
	 *		- the formula is not allowed to contain subformulas of the class EquivForm or EqChainForm
	 *		- for all subterms, the preconditions of evaluate(Term*,Structure*,const map<Variable*,TypedElement>&) must hold
	 *		- the sort of every quantified variable in the formula should have a finite domain in the given structure
	 */
	TruthValue evaluate(Formula*,Structure*,const map<Variable*,TypedElement>&);	
																				
}


/******************
	Definitions
******************/

class Rule {

	private:
		PredForm*			_head;
		Formula*			_body;
		vector<Variable*>	_vars;	// The universally quantified variables
		ParseInfo*			_pi;

	public:

		// Constructors
		Rule(const vector<Variable*>& vv, PredForm* h, Formula* b, ParseInfo* pi) : 
			_head(h), _body(b), _vars(vv), _pi(pi) { }

		Rule*	clone()									const;

		// Destructor
		~Rule() { if(_pi) delete(_pi);	}
		void recursiveDelete();

		// Mutators
		void	body(Formula* f)	{ _body = f;	}

		// Inspectors
		PredForm*		head()					const { return _head;			}
		Formula*		body()					const { return _body;			}
		ParseInfo*		pi()					const { return _pi;				}
		unsigned int	nrQvars()				const { return _vars.size();	}
		Variable*		qvar(unsigned int n)	const { return _vars[n];		}

		// Visitor
		void	accept(Visitor* v);
		Rule*	accept(MutatingVisitor* v);

		// Debug
		string to_string() const;

};

class Definition {

	private:
		vector<Rule*>		_rules;		// The rules in the definition
		vector<PFSymbol*>	_defsyms;	// Symbols defined by the definition

	public:

		// Constructors
		Definition() : _rules(0), _defsyms(0) { } 

		Definition*	clone()	const;

		// Destructor
		~Definition() { }
		void recursiveDelete();

		// Mutators
		void	add(Rule*);						// add a rule
		void	add(PFSymbol* p);				// set 'p' to be a defined symbol
		void	rule(unsigned int n, Rule* r)	{ _rules[n] = r;	}
		void	defsyms();						// (Re)compute the list of defined symbols

		// Inspectors
		unsigned int	nrrules()				const { return _rules.size();		}
		Rule*			rule(unsigned int n)	const { return _rules[n];			}
		unsigned int	nrdefsyms()				const { return _defsyms.size();		}
		PFSymbol*		defsym(unsigned int n)	const { return _defsyms[n];			}

		// Visitor
		void		accept(Visitor* v);
		Definition*	accept(MutatingVisitor* v);

		// Debug
		string to_string(unsigned int spaces = 0) const;

};

class FixpDef {
	
	private:
		bool				_lfp;		// True iff it is a least fixpoint definition
		vector<FixpDef*>	_defs;		// The direct subdefinitions  of the definition
		vector<Rule*>		_rules;		// The rules of the definition
		vector<PFSymbol*>	_defsyms;	// The predicates in heads of rules in _rules

	public:

		// Constructors
		FixpDef(bool lfp) : _lfp(lfp), _defs(0), _rules(0) { }

		FixpDef*	clone()	const;

		// Destructor 
		~FixpDef() { }
		void recursiveDelete();

		// Mutators
		void	add(Rule* r);					// add a rule
		void	add(PFSymbol* p);				// set 'p' to be a defined symbol
		void	add(FixpDef* d)					{ _defs.push_back(d);	}
		void	rule(unsigned int n, Rule* r)	{ _rules[n] = r;		}
		void	def(unsigned int n, FixpDef* d)	{ _defs[n] = d;			}
		void	defsyms();						// (Re)compute the list of defined symbols

		// Inspectors
		bool			lfp()					const { return _lfp;			}
		unsigned int	nrrules()				const { return _rules.size();	}
		unsigned int	nrdefs()				const { return _defs.size();	}
		Rule*			rule(unsigned int n)	const { return _rules[n];		}
		FixpDef*		def(unsigned int n)		const { return _defs[n];		}

		// Visitor
		void		accept(Visitor* v);
		FixpDef*	accept(MutatingVisitor* v);

		// Debug
		string to_string(unsigned int spaces = 0) const;

};


/***************
	Theories
***************/

class Theory {
	
	private:

		string				_name;
		Vocabulary*			_vocabulary;
		Structure*			_structure;
		ParseInfo*			_pi;

		vector<Formula*>	_sentences;
		vector<Definition*>	_definitions;
		vector<FixpDef*>	_fixpdefs;

	public:

		// Constructors 
		Theory(const string& name, ParseInfo* pi) : _name(name), _vocabulary(0), _structure(0), _pi(pi) { }
		Theory(const string& name, Vocabulary* voc, Structure* str, ParseInfo* pi) : _name(name), _vocabulary(voc), _structure(str), _pi(pi) { }

		Theory*	clone()	const;

		// Destructor
		void recursiveDelete();
		~Theory() { if(_pi) delete(_pi); }

		// Mutators
		void	vocabulary(Vocabulary* v)					{ _vocabulary = v;				}
		void	structure(Structure* s)						{ _structure = s;				}
		void	add(Formula* f)								{ _sentences.push_back(f);		}
		void	add(Definition* d)							{ _definitions.push_back(d);	}
		void	add(FixpDef* fd)							{ _fixpdefs.push_back(fd);		}
		void	add(Theory* t);
		void	sentence(unsigned int n, Formula* f)		{ _sentences[n] = f;			}
		void	definition(unsigned int n, Definition* d)	{ _definitions[n] = d;			}
		void	fixpdef(unsigned int n, FixpDef* d)			{ _fixpdefs[n] = d;				}
		void	name(const string& n)						{ _name = n;					}

		// Inspectors
		const string&	name()						const { return _name;				}
		Vocabulary*		vocabulary()				const { return _vocabulary;			}
		Structure*		structure()					const { return _structure;			}
		ParseInfo*		pi()						const { return _pi;					}
		unsigned int	nrSentences()				const { return _sentences.size();	}
		unsigned int	nrDefinitions()				const { return _definitions.size();	}
		unsigned int	nrFixpDefs()				const { return _fixpdefs.size();	}
		Formula*		sentence(unsigned int n)	const { return _sentences[n];		}
		Definition*		definition(unsigned int n)	const { return _definitions[n];		}
		FixpDef*		fixpdef(unsigned int n)		const { return _fixpdefs[n];		}

		// Visitor
		void	accept(Visitor*);
		Theory*	accept(MutatingVisitor*);

		// Debugging
		string to_string() const;

};

namespace TheoryUtils {

	/** Rewriting theories **/
	void push_negations(Theory*);	// Push negations inside
	void remove_equiv(Theory*);		// Rewrite A <=> B to (A => B) & (B => A)
	void flatten(Theory*);			// Rewrite (! x : ! y : phi) to (! x y : phi), rewrite ((A & B) & C) to (A & B & C), etc.
	void remove_eqchains(Theory*);	// Rewrite chains of equalities to a conjunction or disjunction of atoms.
	void tseitin(Theory*);			// Apply the Tseitin transformation
	// TODO  Merge definitions
	
	/** Completion **/
	// TODO  Compute completion of definitions
	
	/** ECNF **/
	EcnfTheory*	convert_to_ecnf(Theory*,GroundTranslator*);		// Convert the theory to ecnf using the given translator
	
}

#endif