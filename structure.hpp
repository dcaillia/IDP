/************************************
	structure.h
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "vocabulary.hpp"
#include "visitor.hpp"
typedef vector<vector<Element> > VVE;

/*************************************
	Interpretations for predicates
*************************************/

class PredTable {

	public:
		
		// Constructors
		PredTable() { }

		// Destructor
		virtual ~PredTable() { }

		// Mutators
		virtual void	sortunique() = 0;	// Sort and remove duplicates

		// Inspectors
		virtual bool				finite()				const = 0;	// true iff the table is finite
		virtual	bool				empty()					const = 0;	// true iff the table is empty
		virtual	unsigned int		arity()					const = 0;	// the number of columns in the table
		virtual	ElementType			type(unsigned int n)	const = 0;	// the type of elements in the n'th column
				vector<ElementType>	types()					const;		// all types of the table

		// Check if the table contains a given tuple
		//	precondition: the table is sorted and contains no duplicates
		virtual	bool	contains(const vector<Element>&)		const = 0;	// true iff the table contains the tuple
																			// precondition: the given tuple has the same
																			// types as the types of the table
				bool	contains(const vector<TypedElement>&)	const;		// true iff the table contains the tuple
																			// works also if the types of the tuple do not
																			// match the types of the table

		// Inspectors for finite tables
		virtual	unsigned int		size()									const = 0;	// the size of the table
		virtual	vector<Element>		tuple(unsigned int n)					const = 0;	// the n'th tuple
		virtual Element				element(unsigned int r,unsigned int c)	const = 0;	// the element at position (r,c)

		// Debugging
		virtual string to_string(unsigned int spaces = 0)	const = 0;

};

/*****************************************************
	Interpretations for sorts and unary predicates
*****************************************************/

/** Abstract base class **/

class SortTable : public PredTable {
	
	public:

		// Constructors
		SortTable() : PredTable() { }

		// Destructor
		virtual ~SortTable() { }

		// Mutators
		virtual void sortunique() = 0; 

		// Inspectors
		virtual bool			finite()				const = 0;	// Return true iff the size of the table is finite
		virtual bool			empty()					const = 0;	// True iff the sort contains no elements
		virtual ElementType		type()					const = 0;	// return the type of the elements in the table
				unsigned int	arity()					const { return 1;		}
				ElementType		type(unsigned int n)	const { return type();	}

		// Check if the table contains a given element
		//	precondition: the table is sorted and contains no duplicates
				bool	contains(const vector<Element>& ve)			const { return contains(ve[0]);	}
				bool	contains(const vector<TypedElement>& ve)	const { return contains(ve[0]);	}	
		virtual	bool	contains(string*)							const = 0;	// true iff the table contains the string.
		virtual bool	contains(int)								const = 0;	// true iff the table contains the integer
		virtual bool	contains(double)							const = 0;	// true iff the table contains the double
		virtual	bool	contains(compound*)							const = 0;	// true iff the table contains the compound
				bool	contains(Element,ElementType)				const;		// true iff the table contains the element
				bool	contains(Element e)							const { return contains(e,type());				}
				bool	contains(const TypedElement& te)			const { return contains(te._element,te._type);	}


		// Inspectors for finite tables
		virtual unsigned int	size()									const = 0;	// Returns the number of elements.
		virtual Element			element(unsigned int n)					const = 0;	// Return the n'th element
				TypedElement	telement(unsigned int n)				const { TypedElement te(element(n),type()); return te;	}
				vector<Element>	tuple(unsigned int n)					const { return vector<Element>(1,element(n));			}
				Element			element(unsigned int r,unsigned int c)	const { return element(r);								}
		virtual unsigned int	position(Element,ElementType)			const = 0;	// Return the position of the given element
				unsigned int	position(Element e)						const { return position(e,type());					}
				unsigned int	position(TypedElement te)				const { return position(te._element,te._type);		}

		// Debugging
		virtual string to_string(unsigned int spaces = 0) const = 0;

};

/** Abstract class for finite unary tables **/
class FiniteSortTable : public SortTable {

	public:
	
		// Constructors
		FiniteSortTable() : SortTable() { }

		// Destructor
		virtual ~FiniteSortTable() { }

		// Add elements to a table
		// A pointer to the resulting table is returned. This pointer may point to 'this', but this is not
		// neccessarily the case. No pointers are deleted when calling these methods.
		// The result of add(...) is not necessarily sorted and may contain duplicates.
				FiniteSortTable*	add(Element,ElementType);
		virtual FiniteSortTable*	add(int)		= 0;	// Add an integer to the table.
		virtual FiniteSortTable*	add(double)		= 0;	// Add a floating point number to the table
		virtual FiniteSortTable*	add(string*)	= 0;	// Add a string to the table.
		virtual FiniteSortTable*	add(int,int)	= 0;	// Add a range of integers to the table.
		virtual FiniteSortTable*	add(char,char)	= 0;	// Add a range of characters to the table.
		virtual FiniteSortTable*	add(compound*)	= 0;	// Add a compound element to the table

		// Mutators
		virtual void sortunique() = 0; // Sort the table and remove duplicates.

		// Inspectors
		virtual bool			finite()	const { return true;	}
		virtual bool			empty()		const = 0;	
		virtual ElementType		type()		const = 0;

		// Check if the table contains a given element
		//	precondition: the table is sorted and contains no duplicates
		virtual	bool	contains(string*)					const = 0;
		virtual bool	contains(int)						const = 0;
		virtual bool	contains(double)					const = 0;
		virtual	bool	contains(compound*)					const = 0;
				bool	contains(const TypedElement& te)	const { return SortTable::contains(te);	}

		// Inspectors for finite tables
		virtual unsigned int	size()							const = 0;	// Returns the number of elements.
		virtual Element			element(unsigned int n)			const = 0;	// Return the n'th element
		virtual unsigned int	position(Element,ElementType)	const = 0;	// returns the position of the given element

		// Debugging
		virtual string to_string(unsigned int spaces = 0) const = 0;

};

/** Empty table **/

class EmptySortTable : public FiniteSortTable {

	public:

		// Constructors
		EmptySortTable() : FiniteSortTable() { }

		// Destructor
		virtual ~EmptySortTable() { }

		// Add elements to a table
		FiniteSortTable*	add(int);	
		FiniteSortTable*	add(double);	
		FiniteSortTable*	add(string*);	
		FiniteSortTable*	add(int,int);	
		FiniteSortTable*	add(char,char);	
		FiniteSortTable*	add(compound*);

		// Mutators
		void sortunique() { }

		// Inspectors
		bool			empty()								const { return true;						}	
		ElementType		type()								const { return ElementUtil::leasttype();	}
		bool			contains(string*)					const { return false;						}	
		bool			contains(int)						const { return false;						}	
		bool			contains(double)					const { return false;						}	
		bool			contains(compound*)					const { return false;						}
		bool			contains(const TypedElement& te)	const { return false;						}
		unsigned int	size()								const { return 0;							}	
		Element			element(unsigned int n)				const { assert(false); Element e; return e; } 
		unsigned int	position(Element,ElementType)		const { assert(false); return 0;			}
															
		// Debugging
		string to_string(unsigned int spaces = 0) const { return "";	}

};

/** Domain is an interval of integers **/

class RanSortTable : public FiniteSortTable {
	
	private:
		int _first;		// first element in the range
		int _last;		// last element in the range

	public:

		// Constructors
		RanSortTable(int f, int l) : FiniteSortTable(), _first(f), _last(l) { }

		// Destructor
		~RanSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Mutators
		void			sortunique() { }

		// Inspectors
		bool			empty()								const { return _first > _last;	}
		ElementType		type()								const { return ELINT;			}
		bool			contains(string*)					const;
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _last-_first+1;	}
		Element			element(unsigned int n)				const { Element e; e._int = _first+n; return e;	}
		unsigned int	position(Element,ElementType)		const;

		int	operator[](unsigned int n)	const { return _first+n;	}
		int	first()						const { return _first;		}
		int	last()						const { return _last;		}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/** Domain is a set of integers, but not necessarily an interval **/

class IntSortTable : public FiniteSortTable {
	
	private:
		vector<int> _table;

	public:

		// Constructors
		IntSortTable() : FiniteSortTable(), _table(0) { }

		// Destructor
		~IntSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Mutators
		void	sortunique();

		// Inspectors
		bool			empty()								const { return _table.empty();	}
		ElementType		type()								const { return ELINT;			}
		bool			contains(string*)					const;
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _table.size();	}
		Element			element(unsigned int n)				const { Element e; e._int = _table[n]; return e;	}
		unsigned int	position(Element,ElementType)		const;

		int	operator[](unsigned int n)	const { return _table[n];			}
		int	first()						const { return _table.front();		}
		int	last()						const { return _table.back();		}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

/*
 * Domain is a set of doubles 
 *		Invariant: at least one element in the table is not an integer.
 *
 */
class FloatSortTable : public FiniteSortTable {
	
	private:
		vector<double> _table;

	public:

		// Constructors
		FloatSortTable() : FiniteSortTable(), _table(0) { }

		// Destructor
		~FloatSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Mutators
		void	sortunique();

		// Inspectors
		bool			empty()								const { return _table.empty();		}
		ElementType		type()								const { return ELDOUBLE;			}
		bool			contains(string*)					const;
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _table.size();		}
		Element			element(unsigned int n)				const { Element e; e._double = _table[n]; return e;	}
		unsigned int	position(Element,ElementType)		const;

		double	operator[](unsigned int n)	const { return _table[n];			}
		double	first()						const { return _table.front();		}
		double	last()						const { return _table.back();		}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/*
 * Domain is a set of strings
 *		Invariant: no string in the table is a number (int or double)
 */
class StrSortTable : public FiniteSortTable {
	
	private:
		vector<string*> _table;

	public:

		// Constructors
		StrSortTable() : _table(0) { }

		// Destructor
		~StrSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Cleanup
		void	sortunique();

		// Inspectors
		bool			empty()								const { return _table.empty();		}
		ElementType		type()								const { return ELSTRING;			}
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(string*)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _table.size();		}
		Element			element(unsigned int n)				const { Element e; e._string = _table[n]; return e;	}
		unsigned int	position(Element,ElementType)		const;

		string*	operator[](unsigned int n)	const { return _table[n];			}
		string*	first()						const { return _table.front();		}
		string*	last()						const { return _table.back();		}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

/*
 * Domain contains numbers, strings, and/or compounds
 *		Invariant: there are compounds or both numbers and strings in the table
 */
class MixedSortTable : public FiniteSortTable {
	
	private:
		vector<double>		_numtable;
		vector<string*>		_strtable;
		vector<compound*>	_comtable;

	public:

		// Constructors
		MixedSortTable() : _numtable(0), _strtable(0) { }
		MixedSortTable(const vector<string*>& t) : _numtable(0), _strtable(t)	{ }
		MixedSortTable(const vector<double>& t) : _numtable(t), _strtable(0)	{ }

		// Destructor
		~MixedSortTable() { }

		// Add elements to the table
		FiniteSortTable*	add(int);
		FiniteSortTable*	add(string*);
		FiniteSortTable*	add(double);
		FiniteSortTable*	add(int,int);
		FiniteSortTable*	add(char,char);
		FiniteSortTable*	add(compound*);

		// Mutators
		void			sortunique();

		// Inspectors
		bool			empty()								const { return (_numtable.empty() && _strtable.empty() && _comtable.empty()); }
		ElementType		type()								const;
		bool			contains(int)						const;
		bool			contains(double)					const;
		bool			contains(string*)					const;
		bool			contains(compound*)					const;
		bool			contains(const TypedElement& te)	const { return SortTable::contains(te);	}
		unsigned int	size()								const { return _numtable.size() + _strtable.size() + _comtable.size();	}
		Element			element(unsigned int n)				const; 
		unsigned int	position(Element,ElementType)		const;

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/***********************************************
	Interpretations for non-unary predicates
***********************************************/

/** Finite, enumerated tables  **/

class FinitePredTable : public PredTable {
	
	private:
		vector<ElementType>			_types;		// the types of elements in the columns
		vector<vector<Element> >	_table;		// the actual table
		ElementWeakOrdering			_order;		// less-than-or-equal relation on the tuples of this table
		ElementEquality				_equality;	// equality relation on the tuples of this table

	public:

		// Constructors 
		FinitePredTable(const vector<ElementType>& t) : PredTable(), _types(t), _table(0), _order(t), _equality(t) { }
		FinitePredTable(const FinitePredTable&);
		FinitePredTable(const FiniteSortTable&);

		// Destructor
		~FinitePredTable() { }

		// Mutators
		void	sortunique();

		// Parsing
		void				addRow()								{ _table.push_back(vector<Element>(_types.size()));	}
		void				addRow(const vector<Element>& ve, const vector<ElementType>&);
		void				addColumn(ElementType);
		void				changeElType(unsigned int,ElementType);
		vector<Element>&	operator[](unsigned int n)				{ return _table[n];	}

		// Inspectors
		bool				finite()								const { return true;			}
		bool				empty()									const { return _table.empty();	}
		unsigned int		arity()									const { return _types.size();	}	
		ElementType			type(unsigned int n)					const { return _types[n];		}
		unsigned int		size()									const { return _table.size();	}
		vector<Element>		tuple(unsigned int n)					const { return _table[n];		}
		Element				element(unsigned int r,unsigned int c)	const { return _table[r][c];	}

		const vector<vector<Element> >& table()	const { return _table;	}

		// Check if the table contains a given tuple
		bool	contains(const vector<Element>&)	const;

		// Other inspectors
		VVE::const_iterator		begin()									const { return _table.begin();	}
		VVE::const_iterator		end()									const { return _table.end();	}
		const vector<Element>&	operator[](unsigned int n)				const { return _table[n];		}
		
		// Debugging
		string to_string(unsigned int spaces = 0)	const;
};


/********************************************
	Four-valued predicate interpretations
********************************************/

/*
 * Four-valued predicate interpretation, represented by two pointers to tables.
 *	If the two pointers are equal and _ct != _cf, the interpretation is certainly two-valued.
 */ 
class PredInter {
	
	private:
		PredTable*	_ctpf;	// stores certainly true or possibly false tuples
		PredTable*	_cfpt;	// stores certainly false or possibly true tuples
		bool		_ct;	// true iff _ctpf stores certainly true tuples, false iff _ctpf stores possibly false tuples
		bool		_cf;	// ture iff _cfpt stores certainly false tuples, false iff _cfpt stores possibly true tuples

	public:
		
		// Constructors
		PredInter(PredTable* p1,PredTable* p2,bool b1, bool b2) : _ctpf(p1), _cfpt(p2), _ct(b1), _cf(b2) { }
		PredInter(PredTable* p, bool b) : _ctpf(p), _cfpt(p), _ct(b), _cf(!b) { }

		// Destructor
		~PredInter();

		// Mutators
		void replace(PredTable* pt,bool ctpf, bool c);	// If ctpf is true, replace _ctpf by pt and set _ct to c
														// Else, replace cfpt by pt and set _cf to c
		void sortunique()	{ if(_ctpf) _ctpf->sortunique(); if(_cfpt && _ctpf != _cfpt) _cfpt->sortunique();	}

		// Inspectors
		PredTable*	ctpf()									const { return _ctpf;	}
		PredTable*	cfpt()									const { return _cfpt;	}
		bool		ct()									const { return _ct;		}
		bool		cf()									const { return _cf;		}
		bool		istrue(const vector<Element>& vi)		const { return (_ct ? _ctpf->contains(vi) : !(_ctpf->contains(vi)));	}
		bool		isfalse(const vector<Element>& vi)		const { return (_cf ? _cfpt->contains(vi) : !(_cfpt->contains(vi)));	}
		bool		istrue(const vector<TypedElement>& vi)	const;	// return true iff the given tuple is true or inconsistent
		bool		isfalse(const vector<TypedElement>& vi)	const;	// return false iff the given tuple is false or inconsistent

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};


/************************************
	Interpretations for functions 
************************************/

/** abstract base class **/

class FuncTable {

	public:

		// Constructor
		FuncTable() { }

		// Destructor
		virtual ~FuncTable() { }

		// Inspectors
		virtual bool			finite()								const = 0;	// true iff the table is finite
		virtual bool			empty()									const = 0;	// true iff the table is empty
		virtual unsigned int	arity()									const = 0;	// arity of the table - 1
		virtual unsigned int	size()									const = 0;	// size of the table
		virtual ElementType		type(unsigned int n)					const = 0;	// type of the n'th column of the table
		virtual vector<Element>	tuple(unsigned int n)					const = 0;	// return the n'th tuple
		virtual Element			element(unsigned int r,unsigned int c)	const = 0;	// return the element at row r and column c

		virtual	Element	operator[](const vector<Element>& vi)		const = 0;	// return the value of vi according to the function 
																				// NOTE: the type of the n'th element in vi should be
																				// the type of the n'th column in the table. 
				Element	operator[](const vector<TypedElement>& vi)	const;		// return the value of vi according to the function

		// Debugging
		virtual string to_string(unsigned int spaces = 0) const = 0;

};


/** finite, enumerated functions **/
class FiniteFuncTable : public FuncTable {

	private:
		FinitePredTable*	_ftable;	// the actual table
		ElementWeakOrdering	_order;		// less-than-or-equal on a tuple of domain elements with arity n = (_ftable->arity() - 1)
										// and types (_ftable->type(0),...,_ftable->type(n)).
		ElementEquality		_equality;	// equality on the on a tuple of domain elements with arity n = (_ftable->arity() - 1)
										// and types (_ftable->type(0),...,_ftable->type(n)).

	public:
		
		// Constructors
		FiniteFuncTable(FinitePredTable* ft);

		// Destructor
		~FiniteFuncTable() { }

		// Inspectors
		bool				finite()								const { return true;					}
		bool				empty()									const { return _ftable->finite();		}
		unsigned int		arity()									const { return _ftable->arity() - 1;	}
		unsigned int		size()									const { return _ftable->size();			}
		FinitePredTable*	ftable()								const { return _ftable;					}
		ElementType			type(unsigned int n)					const { return _ftable->type(n);		}
		vector<Element>		tuple(unsigned int n)					const { return _ftable->tuple(n);		}
		Element				element(unsigned int r,unsigned int c)	const { return _ftable->element(r,c);	}

		Element		operator[](const vector<Element>& vi)		const;

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

/** predicate table derived from function table **/
class FuncPredTable : public PredTable {

	private:
		FuncTable* _ftable;

	public:

		// Constructors
		FuncPredTable(FuncTable* ft) : PredTable(), _ftable(ft) { }

		// Destructor
		~FuncPredTable() { }

		// Mutators
		void sortunique() { }

		// Inspectors
		bool			finite()				const { return _ftable->finite();		}
		bool			empty()					const { return _ftable->empty();		}
		unsigned int	arity()					const { return _ftable->arity() + 1;	}
		ElementType		type(unsigned int n)	const { return _ftable->type(n);		}

		// Check if the table contains a given tuple
		bool	contains(const vector<Element>&)	const;

		// Inspectors for finite tables
		unsigned int		size()									const { return _ftable->size();			}
		vector<Element>		tuple(unsigned int n)					const { return _ftable->tuple(n);		}
		Element				element(unsigned int r,unsigned int c)	const { return _ftable->element(r,c);	}

		// Debugging
		string to_string(unsigned int spaces = 0)	const { return _ftable->to_string(spaces);	}

};

/** four-valued function interpretations **/
class FuncInter {

	private:
		FuncTable*	_ftable;	// the function (if it is two-valued, nullpointer otherwise).
		PredInter*	_pinter;	// the interpretation for the graph of the function

	public:
		
		// Constructors
		FuncInter(FuncTable* ft, PredInter* pt) : _ftable(ft), _pinter(pt) { }

		// Destructor
		~FuncInter() { if(_ftable) delete(_ftable); delete(_pinter); }

		// Mutators
		void sortunique() { if(_pinter) _pinter->sortunique(); }

		// Inspectors
		PredInter*	predinter()	const { return _pinter;	}
		FuncTable*	functable()	const { return _ftable;	}

		// Debugging
		string to_string(unsigned int spaces = 0) const;

};

/************************
	Auxiliary methods
************************/

namespace TableUtils {

	PredInter*	leastPredInter(unsigned int n);		// construct a new, least precise predicate interpretation with arity n
	FuncInter*	leastFuncInter(unsigned int n);		// construct a new, least precise function interpretation with arity n

	FiniteSortTable*	singletonSort(Element,ElementType);	// construct a sort table containing only the given element
	FiniteSortTable*	singletonSort(TypedElement);		// construct a sort table containing only the given element

	// Return a table containing all tuples (a_i1,...,a_in) such that
	//			vb[ik] is false for every 1 <= k <= n	
	//		AND	(a_1,...,a_m) is a tuple of pt
	//		AND for every 1 <= j <= m, if vb[j] = true, then vet[j] = a_j
	//	Precondition: pt is finite and sorted
	PredTable*	project(PredTable* pt,const vector<TypedElement>& vet, const vector<bool>& vb);
}

/*****************
	Structures
*****************/

/** Abstract base class **/

class AbstractStructure {

	protected:

		string			_name;			// The name of the structure
		ParseInfo		_pi;			// The place where this structure was parsed.
		Vocabulary*		_vocabulary;	// The vocabulary of the structure.

	public:

		// Constructors
		AbstractStructure(string name, const ParseInfo& pi) : _name(name), _pi(pi), _vocabulary(0) { }

		// Destructor
		virtual ~AbstractStructure() { }

		// Mutators
		virtual void	vocabulary(Vocabulary* v) { _vocabulary = v;	}	// set the vocabulary

		// Inspectors
				const string&	name()						const { return _name;		}
				ParseInfo		pi()						const { return _pi;			}
				Vocabulary*		vocabulary()				const { return _vocabulary;	}
		virtual SortTable*		inter(Sort* s)				const = 0;	// Return the domain of s.
		virtual PredInter*		inter(Predicate* p)			const = 0;	// Return the interpretation of p.
		virtual FuncInter*		inter(Function* f)			const = 0;	// Return the interpretation of f.
		virtual PredInter*		inter(PFSymbol* s)			const = 0;  // Return the interpretation of s.

		// Visitor
		virtual void accept(Visitor* v)	= 0;

		// Debugging
		virtual string	to_string(unsigned int spaces = 0) const = 0;

};

/** Structures as constructed by the parser **/

class Structure : public AbstractStructure {

	private:

		vector<SortTable*>	_sortinter;		// The domains of the structure. 
											// The domain for sort s is stored in _sortinter[n], 
											// where n is the index of s in _vocabulary.
											// If a sort has no domain, a null-pointer is stored.
		vector<PredInter*>	_predinter;		// The interpretations of the predicate symbols.
											// If a predicate has no interpretation, a null-pointer is stored.
		vector<FuncInter*>	_funcinter;		// The interpretations of the function symbols.
											// If a function has no interpretation, a null-pointer is stored.
	
	public:
		
		// Constructors
		Structure(const string& name, const ParseInfo& pi) : AbstractStructure(name,pi) { }

		// Destructor
		~Structure();

		// Mutators
		void	vocabulary(Vocabulary* v);			// set the vocabulary
		void	inter(Sort* s,SortTable* d);		// set the domain of s to d.
		void	inter(Predicate* p, PredInter* i);	// set the interpretation of p to i.
		void	inter(Function* f, FuncInter* i);	// set the interpretation of f to i.
		void	close();							// set the interpretation of all predicates and functions that 
													// do not yet have an interpretation to the least precise 
													// interpretation.

		// Inspectors
		Vocabulary*		vocabulary()				const { return AbstractStructure::vocabulary();	}
		SortTable*		inter(Sort* s)				const;	// Return the domain of s.
		PredInter*		inter(Predicate* p)			const;	// Return the interpretation of p.
		FuncInter*		inter(Function* f)			const;	// Return the interpretation of f.
		PredInter*		inter(PFSymbol* s)			const;  // Return the interpretation of s.
		SortTable*		sortinter(unsigned int n)	const { return _sortinter[n];	}
		PredInter*		predinter(unsigned int n)	const { return _predinter[n];	}
		FuncInter*		funcinter(unsigned int n)	const { return _funcinter[n];	}
		bool			hasInter(Sort* s)			const;	// True iff s has an interpretation
		bool			hasInter(Predicate* p)		const;	// True iff p has an interpretation
		bool			hasInter(Function* f)		const;	// True iff f has an interpretation
		unsigned int	nrSortInters()				const { return _sortinter.size();	}
		unsigned int	nrPredInters()				const { return _predinter.size();	}
		unsigned int	nrFuncInters()				const { return _funcinter.size();	}

		// Visitor
		void accept(Visitor* v)	{ v->visit(this);	}

		// Debugging
		string	to_string(unsigned int spaces = 0) const;

};

class AbstractTheory;
namespace StructUtils {

	// Make a theory containing all literals that are true according to the given structure
	AbstractTheory*		convert_to_theory(AbstractStructure*);	

	// Compute the complement of the given table in the given structure
	PredTable*	complement(PredTable*,const vector<Sort*>&, AbstractStructure*);

}

/**
 *	Iterate over all elements in the cross product of a tuple of SortTables.
 *		Precondition: all tables in the tuple are finite
 */
class SortTableTupleIterator {
	
	private:
		vector<unsigned int>	_currvalue;
		vector<unsigned int>	_limits;
		vector<SortTable*>		_tables;

	public:
		SortTableTupleIterator(const vector<SortTable*>&);
		SortTableTupleIterator(const vector<Variable*>&, AbstractStructure*);	// tuple of tables are the tables
																				// for the sorts of the given variables
																				// in the given structure
		bool nextvalue();							// go to the next value. Returns false iff there is no next value.
		bool empty()						const;	// true iff the cross product is empty
		bool singleton()					const;  // true iff the cross product is a singleton
		ElementType type(unsigned int n)	const;	// Get the type of the values of the n'th table in the tuple
		Element		value(unsigned int n)	const;	// Get the n'th element in the current tuple

};

#endif