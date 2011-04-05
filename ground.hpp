/************************************
	ground.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef GROUND_HPP
#define GROUND_HPP

#include <queue>
#include <stack>
#include <cstdlib> // abs
#include "theory.hpp"
#include "checker.hpp"
#include "generator.hpp"
#include "pcsolver/src/external/ExternalInterface.hpp"

class AbstractGroundTheory;

/**********************************************
	Translate from ground atoms to numbers
**********************************************/

/*
 * Enumeration for the possible ways to define a tseitin atom in terms of the subformula it replaces.
 *		TS_EQ:		tseitin <=> subformula
 *		TS_RULE:	tseitin <- subformula
 *		TS_IMPL:	tseitin => subformula
 *		TS_RIMPL:	tseitin <= subformula
 */
enum TsType { TS_EQ, TS_RULE, TS_IMPL, TS_RIMPL };

/*
 * A complete definition of a tseitin atom
*/
class TsBody {
	protected:
		TsType _type;	// the type of "tseitin definition"
		TsBody(TsType type): _type(type) { }
		virtual ~TsBody() {}
	public:
		TsType type() const { return _type; }
	friend class GroundTranslator;
};

class PCTsBody : public TsBody {
	private:
		vector<int> _body;	// the literals in the subformula replaced by the tseitin
		bool		_conj;	// if true, the replaced subformula is the conjunction of the literals in _body,
							// if false, the replaced subformula is the disjunction of the literals in _body
	public:
		PCTsBody(TsType type, const vector<int>& body, bool conj):
			TsBody(type), _body(body), _conj(conj) { }
		vector<int> 	body() 					const { return _body; 			}
		unsigned int	size()					const { return _body.size();	}
		int				literal(unsigned int n)	const { return _body[n];		}
		bool			conj()					const { return _conj; 			}
	friend class GroundTranslator;
};

class AggTsBody : public TsBody {
	private:
		int		_setnr;
		AggType	_aggtype;
		bool	_lower;
		double	_bound;
	public:
		AggTsBody(TsType type, double bound, bool lower, AggType at, int setnr):
			TsBody(type), _setnr(setnr), _aggtype(at), _lower(lower), _bound(bound) { }
		int		setnr()		const { return _setnr; 		}
		AggType	aggtype()	const { return _aggtype;	}
		bool	lower()		const { return _lower;		}
		double	bound()		const { return _bound;		}
	friend class GroundTranslator;
};

class CPTerm {
	protected:
		virtual ~CPTerm() {	}
	public:
		virtual void accept(Visitor*) const = 0;
};

class CPVarTerm : public CPTerm {
	public:
		unsigned int _varid;
		CPVarTerm(unsigned int varid) : _varid(varid) { }
		void accept(Visitor*) const;
};

class CPSumTerm : public CPTerm {
	public:
		vector<unsigned int> _varids; 
		void accept(Visitor*) const;
};

class CPWSumTerm : public CPTerm {
	public:
		vector<unsigned int> 	_varids; 
		vector<int>				_weights;
		void accept(Visitor*) const;
};

struct CPBound {
	bool _isvarid;
	union { 
		int _bound;
		unsigned int _varid;
	} _value;
	CPBound(bool isvarid, int bound): _isvarid(isvarid) { _value._bound = bound; }
	CPBound(bool isvarid, unsigned int varid): _isvarid(isvarid) { _value._varid = varid; }
};

enum CompType { CT_EQ, CT_NEQ, CT_LEQ, CT_GEQ, CT_LT, CT_GT };

class CPTsBody : public TsBody {
	private:
		CPTerm*		_left;
		CompType	_comp;
		CPBound		_right;
	public:
		CPTsBody(TsType type, CPTerm* left, CompType comp, const CPBound& right) :
			TsBody(type), _left(left), _comp(comp), _right(right) { }
		CPTerm*		left()	const { return _left;	}
		CompType	comp()	const { return _comp;	}
		CPBound		right()	const { return _right;	}
	friend class GroundTranslator;
};

/*
 * Set corresponding to a tseitin
 */ 
class TsSet {
	private:
		vector<int>		_setlits;		// All literals in the ground set
		vector<double>	_litweights;	// For each literal a corresponding weight
		vector<double>	_trueweights;	// The weights of the true literals in the set
	public:
		vector<int>		literals()				const { return _setlits; 			}
		vector<double>	weights()				const { return _litweights;			}
		vector<double>	trueweights()			const { return _trueweights;		}
		unsigned int	size() 					const { return _setlits.size();		}
		bool			empty()					const { return _setlits.empty();	}
		int				literal(unsigned int n)	const { return _setlits[n];			}
		double			weight(unsigned int n)	const { return _litweights[n];		}
	friend class GroundTranslator;
};

/*
 * Ground translator 
 */
class GroundTranslator {
	private:
		vector<map<vector<domelement>,int> >		_table;			// map atoms to integers
		vector<PFSymbol*>							_symboffsets;	// map integer to symbol
		vector<PFSymbol*>							_backsymbtable;	// map integer to the symbol of its corresponding atom
		vector<vector<domelement> >					_backargstable;	// map integer to the terms of its corresponding atom

		queue<int>									_freenumbers;		// keeps atom numbers that were freed 
																		// and can be used again
		queue<int>									_freesetnumbers;	// keeps set numbers that were freed
																		// and can be used again

		map<int,TsBody*>							_tsbodies;		// keeps mapping between Tseitin numbers and bodies

		vector<TsSet>								_sets;			// keeps mapping between Set numbers and sets

	public:
		GroundTranslator() : _backsymbtable(1), _backargstable(1), _sets(1) { }

		int				translate(unsigned int,const vector<domelement>&);
		int				translate(const vector<int>& cl, bool conj, TsType tp);
		int				translate(double bound, char comp, bool strict, AggType aggtype, int setnr, TsType tstype);
		int				translate(PFSymbol*, const vector<TypedElement>&);
		int				translate(CPTerm*, CompType, const CPBound&, TsType);
		int				nextNumber();
		unsigned int	addSymbol(PFSymbol* pfs);
		int				translateSet(const vector<int>&,const vector<double>&,const vector<double>&);

		PFSymbol*							symbol(int nr)				const	{ return _backsymbtable[abs(nr)];		}
		const vector<domelement>&			args(int nr)				const	{ return _backargstable[abs(nr)];		}
		bool								isTseitin(int l)			const	{ return symbol(l) == 0;				}
		TsBody*								tsbody(int l)				const	{ return _tsbodies.find(abs(l))->second;}
		const TsSet&						groundset(int nr)			const	{ return _sets[nr];						}
		TsSet&								groundset(int nr)					{ return _sets[nr];						}
		unsigned int						nrOffsets()					const	{ return _symboffsets.size();			}
		PFSymbol*							getSymbol(unsigned int n)	const	{ return _symboffsets[n];				}
		const map<vector<domelement>,int>&	getTuples(unsigned int n)	const	{ return _table[n];						}

		string	printAtom(int nr)	const;

};

/*
 * Ground term translator
 */
class GroundTermTranslator {
	private:
		vector<map<vector<domelement>,unsigned int> >	_table;			// map terms to integers
		vector<Function*>								_backfunctable;	// map integer to the symbol of its corresponding term
		vector<vector<domelement> >						_backargstable;	// map integer to the terms of its corresponding term
		
		vector<Function*>				_offset2function;
		map<Function*,unsigned int>		_function2offset;

	public:
		GroundTermTranslator() : _backfunctable(1), _backargstable(1) { }

		unsigned int	translate(unsigned int,const vector<domelement>&);
		unsigned int	translate(Function*,const vector<TypedElement>&);
		unsigned int	nextNumber();
		unsigned int	addFunction(Function*);

		Function*					function(unsigned int nr)		const { return _backfunctable[nr];		}
		const vector<domelement>&	args(unsigned int nr)			const { return _backargstable[nr];		}
		Function*					getFunction(unsigned int nr)	const { return _offset2function[nr]; 		}
		string						printTerm(unsigned int nr)		const;
};


/************************************
	Optimized grounding algorithm
************************************/

class TermGrounder;
class FormulaGrounder;
class SetGrounder;
class RuleGrounder;
class DefinitionGrounder;
struct GroundDefinition;

/** Grounding context **/
enum CompContext { CC_SENTENCE, CC_HEAD, CC_FORMULA };
enum PosContext { PC_POSITIVE, PC_NEGATIVE, PC_BOTH };

struct GroundingContext {
	bool			_truegen;		// Indicates whether the variables are instantiated in order to obtain
									// a ground formula that is possibly true.
	PosContext		_funccontext;	
	PosContext		_monotone;
	CompContext		_component;		// Indicates the context of the visited formula
	TsType			_tseitin;		// Indicates the type of tseitin definition that needs to be used.
	set<PFSymbol*>	_defined;		// Indicates whether the visited rule is recursive.
};


/*** Top level grounders ***/

class TopLevelGrounder {
	protected:
		AbstractGroundTheory*	_grounding;
	public:
		TopLevelGrounder(AbstractGroundTheory* gt) : _grounding(gt) { }

		virtual bool					run()		const = 0;
				AbstractGroundTheory*	grounding()	const { return _grounding;	}
};

class CopyGrounder : public TopLevelGrounder {
	private:
		const GroundTheory*		_original;
	public:
		CopyGrounder(AbstractGroundTheory* gt, const GroundTheory* orig) : TopLevelGrounder(gt), _original(orig) { }
		bool run() const;
};

class TheoryGrounder : public TopLevelGrounder {
	private:
		vector<TopLevelGrounder*>	_grounders;
	public:
		TheoryGrounder(AbstractGroundTheory* gt, const vector<TopLevelGrounder*>& fgs) :
			TopLevelGrounder(gt), _grounders(fgs) { }
		bool run() const;
};

class SentenceGrounder : public TopLevelGrounder {
	private:
		bool				_conj;	
		FormulaGrounder*	_subgrounder;
	public:
		SentenceGrounder(AbstractGroundTheory* gt, FormulaGrounder* sub, bool conj) : 
			TopLevelGrounder(gt), _conj(conj), _subgrounder(sub) { }
		bool run() const;
};

class UnivSentGrounder : public TopLevelGrounder {
	private:
		TopLevelGrounder*	_subgrounder;
		InstGenerator*		_generator;	
	public:
		UnivSentGrounder(AbstractGroundTheory* gt, TopLevelGrounder* sub, InstGenerator* gen) : 
			TopLevelGrounder(gt), _subgrounder(sub), _generator(gen) { }
		bool run() const;
};

/*** Term grounders ***/

class TermGrounder {
	protected:
#ifndef NDEBUG
		const Term*	_origterm;
		map<Variable*,domelement*> _varmap;
		void printorig() const;
#endif
	public:
		TermGrounder() { }
		virtual domelement run() const = 0;
		virtual bool canReturnCPVar() const = 0;
#ifndef NDEBUG
		void setorig(const Term* t, const map<Variable*,domelement*>& mvd); 
#endif
};

class DomTermGrounder : public TermGrounder {
	private:
		domelement	_value;
	public:
		DomTermGrounder(domelement val) : _value(val) { }
		domelement run() const { return _value;	}
		bool canReturnCPVar() const { return false; }
};

class VarTermGrounder : public TermGrounder {
	private:
		domelement*	_value;
	public:
		VarTermGrounder(domelement* a) : _value(a) { }
		domelement run() const; 
		bool canReturnCPVar() const { return false; }
};

class FuncTermGrounder : public TermGrounder {
	private:
		FuncTable*					_function;
		vector<TermGrounder*>		_subtermgrounders;
		mutable vector<domelement>	_args;
	public:
		FuncTermGrounder(const vector<TermGrounder*>& sub, FuncTable* f) :
			_function(f), _subtermgrounders(sub), _args(sub.size()) { }
		domelement run() const;
		bool canReturnCPVar() const { return false; }

		// TODO? Optimisation:
		//			Keep all values of the args + result of the previous call to calc().
		//			If the values of the args did not change, return the result immediately instead of doing the
		//			table lookup
};

class AggTermGrounder : public TermGrounder {
	private:
		AggType				_type;
		SetGrounder*		_setgrounder;
		GroundTranslator*	_translator;
	public:
		AggTermGrounder(GroundTranslator* gt, AggType tp, SetGrounder* gr):
			_type(tp), _setgrounder(gr), _translator(gt) { }
		domelement run() const;
		bool canReturnCPVar() const { return false; }
};

/*** Three-valued term grounders ***/

class ThreeValuedFuncTermGrounder : public TermGrounder {
	private:
		vector<TermGrounder*>		_subtermgrounders;
		Function*					_function;
		FuncTable*					_functable;
		mutable vector<domelement>	_args;
		vector<SortTable*>			_tables;
	public:
		ThreeValuedFuncTermGrounder(const vector<TermGrounder*>& sub, Function* f, FuncTable* ft, const vector<SortTable*>& vst):
			_subtermgrounders(sub), _function(f), _functable(ft), _args(sub.size()), _tables(vst) { }
		domelement run() const;
		bool canReturnCPVar() const { return true; }
};

class ThreeValuedAggTermGrounder : public TermGrounder {
	private:
		AggType				_type;
		SetGrounder*		_setgrounder;
		GroundTranslator*	_translator;
	public:
		ThreeValuedAggTermGrounder(GroundTranslator* gt, AggType tp, SetGrounder* gr):
			_type(tp), _setgrounder(gr), _translator(gt) { } 
		domelement run() const;
		bool canReturnCPVar() const { return true; }
};

/*** Formula grounders ***/

class FormulaGrounder {
	protected:
#ifndef NDEBUG
		const Formula*				_origform;
		map<Variable*,domelement*>	_varmap;
		void printorig() const;
#endif
		GroundTranslator*	_translator;
		GroundingContext	_context;
	public:
		FormulaGrounder(GroundTranslator* gt, const GroundingContext& ct): _translator(gt), _context(ct) { }
		virtual int		run()				const = 0;
		virtual void	run(vector<int>&)	const = 0;
		virtual bool	conjunctive()		const = 0;
#ifndef NDEBUG
		void setorig(const Formula* f, const map<Variable*,domelement*>& mvd);
#endif
};

class AtomGrounder : public FormulaGrounder {
	private:
		vector<TermGrounder*>		_subtermgrounders;
		InstanceChecker*			_pchecker;
		InstanceChecker*			_cchecker;
		unsigned int				_symbol;
		mutable vector<domelement>	_args;
		vector<SortTable*>			_tables;
		bool						_sign;
		int							_certainvalue;
	public:
		AtomGrounder(GroundTranslator* gt, bool sign, PFSymbol* s,
					const vector<TermGrounder*> sg, InstanceChecker* pic, InstanceChecker* cic,
					const vector<SortTable*>& vst, const GroundingContext&);
		int		run() const;
		void	run(vector<int>&) const;
		bool	conjunctive() const { return true;	}
};

class CPGrounder : public FormulaGrounder {
	private:
		GroundTermTranslator* 	_termtranslator;
		TermGrounder*			_lefttermgrounder;
		TermGrounder*			_righttermgrounder;
		CompType				_comparator;
	public:
		CPGrounder(GroundTranslator* gt, GroundTermTranslator* tt, TermGrounder* left, CompType comp, TermGrounder* right, const GroundingContext& gc):
			FormulaGrounder(gt,gc), _termtranslator(tt), _lefttermgrounder(left), _righttermgrounder(right), _comparator(comp) { } 
		int		run() const;
		void	run(vector<int>&) const;
		bool	conjunctive() const { return true;	}
};

class AggGrounder : public FormulaGrounder {
	private:
		SetGrounder*	_setgrounder;
		TermGrounder*	_boundgrounder;
		AggType			_type;
		char			_comp;
		bool			_sign;
		bool			_doublenegtseitin;
	public:
		AggGrounder(GroundTranslator* tr, GroundingContext gc, AggType tp, SetGrounder* sg, TermGrounder* bg, char c,bool s) :
			FormulaGrounder(tr,gc), _setgrounder(sg), _boundgrounder(bg), _type(tp), _comp(c), _sign(s) { 
				_doublenegtseitin = gc._tseitin == TS_RULE && ((gc._monotone == PC_POSITIVE && !s) || (gc._monotone == PC_NEGATIVE && s));	
			}
		int		run()				const;
		void	run(vector<int>&)	const;
		int		finishCard(double,double,int)	const;
		int		finishSum(double,double,int)	const;
		bool	conjunctive() const { return true;	}
};

class ClauseGrounder : public FormulaGrounder {
	protected:
		bool	_sign;
		bool	_conj;
		bool	_doublenegtseitin;
	public:
		ClauseGrounder(GroundTranslator* gt, bool sign, bool conj, const GroundingContext& ct) : 
			FormulaGrounder(gt,ct), _sign(sign), _conj(conj) { 
				_doublenegtseitin = ct._tseitin == TS_RULE && ((ct._monotone == PC_POSITIVE && !sign) || (ct._monotone == PC_NEGATIVE && sign));	
			}
		int		finish(vector<int>&) const;
		bool	check1(int l) const;
		bool	check2(int l) const;
		int		result1() const;
		int		result2() const;
		bool	conjunctive() const { return _conj == _sign;	}
};

class BoolGrounder : public ClauseGrounder {
	private:
		vector<FormulaGrounder*>	_subgrounders;
	public:
		BoolGrounder(GroundTranslator* gt, const vector<FormulaGrounder*> sub, bool sign, bool conj, const GroundingContext& ct):
			ClauseGrounder(gt,sign,conj,ct), _subgrounders(sub) { }
		int	run() const;
		void	run(vector<int>&) const;
};


class QuantGrounder : public ClauseGrounder {
	private:
		FormulaGrounder*	_subgrounder;
		InstGenerator*		_generator;	
	public:
		QuantGrounder(GroundTranslator* gt, FormulaGrounder* sub, bool sign, bool conj, InstGenerator* gen, const GroundingContext& ct):
			ClauseGrounder(gt,sign,conj,ct), _subgrounder(sub), _generator(gen) { }
		int	run() const;
		void	run(vector<int>&) const;
};

class EquivGrounder : public FormulaGrounder {
	private:
		FormulaGrounder*	_leftgrounder;
		FormulaGrounder*	_rightgrounder;
		bool				_sign;
	public:
		EquivGrounder(GroundTranslator* gt, FormulaGrounder* lg, FormulaGrounder* rg, bool sign, const GroundingContext& ct):
			FormulaGrounder(gt,ct), _leftgrounder(lg), _rightgrounder(rg), _sign(sign) { }
		int run() const;
		void	run(vector<int>&) const;
		bool	conjunctive() const { return true;	}
};


/*** Set grounders ***/

class SetGrounder {
	protected:
		GroundTranslator*	_translator;
	public:
		SetGrounder(GroundTranslator* gt) : _translator(gt) { }
		virtual int run() const = 0;
};

class QuantSetGrounder : public SetGrounder {
	private:
		FormulaGrounder*	_subgrounder;
		InstGenerator*		_generator;	
		domelement*			_weight;
	public:
		QuantSetGrounder(GroundTranslator* gt, FormulaGrounder* gr, InstGenerator* ig, domelement* w) :
			SetGrounder(gt), _subgrounder(gr), _generator(ig), _weight(w) { }
		int run() const;
};

class EnumSetGrounder : public SetGrounder {
	private:
		vector<FormulaGrounder*>	_subgrounders;
		vector<TermGrounder*>		_subtermgrounders;
	public:
		EnumSetGrounder(GroundTranslator* gt, const vector<FormulaGrounder*>& subgr, const vector<TermGrounder*>& subtgr) :
			SetGrounder(gt), _subgrounders(subgr), _subtermgrounders(subtgr) { }
		int run() const;
};


/*** Definition grounders ***/

/** Grounder for a head of a rule **/
class HeadGrounder {
	private:
		AbstractGroundTheory*		_grounding;
		vector<TermGrounder*>		_subtermgrounders;
		InstanceChecker*			_truechecker;
		InstanceChecker*			_falsechecker;
		unsigned int				_symbol;
		mutable vector<domelement>	_args;
		vector<SortTable*>			_tables;
	public:
		HeadGrounder(AbstractGroundTheory* gt, InstanceChecker* pc, InstanceChecker* cc, PFSymbol* s, 
					const vector<TermGrounder*>&, const vector<SortTable*>&);
		int	run() const;

};

/** Grounder for a single rule **/
class RuleGrounder {
	private:
		GroundDefinition*	_definition;
		HeadGrounder*		_headgrounder;
		FormulaGrounder*	_bodygrounder;
		InstGenerator*		_headgenerator;	
		InstGenerator*		_bodygenerator;	
		GroundingContext	_context;
	public:
		RuleGrounder(GroundDefinition* def, HeadGrounder* hgr, FormulaGrounder* bgr,
					InstGenerator* hig, InstGenerator* big, GroundingContext& ct) :
			_definition(def), _headgrounder(hgr), _bodygrounder(bgr), _headgenerator(hig),
			_bodygenerator(big), _context(ct) { }
		bool run() const;
};

/** Grounder for a definition **/
class DefinitionGrounder : public TopLevelGrounder {
	private:
		GroundDefinition*		_definition;	// The ground definition that will be produced by running the grounder.
		vector<RuleGrounder*>	_subgrounders;	// Grounders for the rules of the definition.
	public:
		DefinitionGrounder(AbstractGroundTheory* gt, GroundDefinition* def, vector<RuleGrounder*> subgr) :
			TopLevelGrounder(gt), _definition(def), _subgrounders(subgr) { }
		bool run() const;
};


/***********************
	Grounder Factory
***********************/

/*
 * Class to produce grounders 
 */
class GrounderFactory : public Visitor {

	private:
		// Data
		AbstractStructure*		_structure;		// The structure that will be used to reduce the grounding
		AbstractGroundTheory*	_grounding;		// The ground theory that will be produced

		// Options
		bool	_usingcp;

		// Context
		GroundingContext		_context;
		stack<GroundingContext>	_contextstack;

		void	InitContext();		// Initialize the context 
		void	AggContext();	
		void	SaveContext();		// Push the current context onto the stack
		void	RestoreContext();	// Set _context to the top of the stack and pop the stack
		void	DeeperContext(bool);

		// Descend in the parse tree while taking care of the context
		void	descend(Formula* f); 
		void	descend(Term* t);
		void	descend(Rule* r);
		void	descend(SetExpr* s);
		
		// Variable mapping
		map<Variable*,domelement*>	_varmapping;	// Maps variables to their counterpart during grounding.
													// That is, the corresponding domelement* acts as a variable+value.

		// Current ground definition
		GroundDefinition*		_definition;	// The ground definition that will be produced by the 
												// currently constructed definition grounder.

		// Return values
		FormulaGrounder*		_formgrounder;
		TermGrounder*			_termgrounder;
		SetGrounder*			_setgrounder;
		TopLevelGrounder*		_toplevelgrounder;
		HeadGrounder*			_headgrounder;
		RuleGrounder*			_rulegrounder;

	public:
		// Constructor
		GrounderFactory(AbstractStructure* structure, bool usingcp): _structure(structure), _usingcp(usingcp) { }

		// Factory method
		TopLevelGrounder* create(const AbstractTheory* theory);
		TopLevelGrounder* create(const AbstractTheory* theory, MinisatID::WrappedPCSolver* solver);

		// Recursive check
		bool recursive(const Formula*);

		// Visitors
		void visit(const GroundTheory*);
		void visit(const Theory*);

		void visit(const PredForm*);
		void visit(const BoolForm*);
		void visit(const QuantForm*);
		void visit(const EquivForm*);
		void visit(const EqChainForm*);
		void visit(const AggForm*);

		void visit(const VarTerm*);
		void visit(const DomainTerm*);
		void visit(const FuncTerm*);
		void visit(const AggTerm*);

		void visit(const EnumSetExpr*);
		void visit(const QuantSetExpr*);

		void visit(const Definition*);
		void visit(const Rule*);

};

#endif
