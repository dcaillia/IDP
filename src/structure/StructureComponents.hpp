/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef STRUCTURECOMPONENTS_HPP
#define STRUCTURECOMPONENTS_HPP

#include <cstdlib>
#include "parseinfo.hpp"
#include "common.hpp"

#include "fwstructure.hpp"

#include "GlobalData.hpp"
#include "utils/NumericLimits.hpp"

/**
 * NAMING CONVENTION
 *		- 'Interpretation' means a possibly three-valued, or even four-valued 
 *			interpretation for a symbol.
 *		- 'Table' means a two-value table
 *		- if a name of a methods begins with 'approx', the method is fast, but 
 *			provides a under approximation of the desired result.
 */

/**********************
 Domain elements
 **********************/

class Compound;
class DomainElementFactory;

std::ostream& operator<<(std::ostream&, const DomainElementType&);

class DomElemContainer {
private:
	mutable const DomainElement* domelem_;
	mutable bool del;
	static std::vector<const DomElemContainer*> containers;

	~DomElemContainer() {
		if (del) {
			delete (domelem_);
		}
	}

public:
	static void deleteAllContainers();
	DomElemContainer() :
			domelem_(new DomainElement()), del(true) {
		containers.push_back(this);
	}

	const DomElemContainer* operator=(const DomElemContainer* container) const {
		if (del) {
			delete (domelem_);
		}
		del = false;
		domelem_ = container->get();
		return this;
	}
	const DomElemContainer* operator=(const DomElemContainer& container) const {
		if (del) {
			delete (domelem_);
		}
		del = false;
		domelem_ = container.get();
		return this;
	}
	const DomElemContainer* operator=(const DomainElement* domelem) const {
		if (del) {
			delete (domelem_);
		}
		del = false;
		domelem_ = domelem;
		return this;
	}

	inline const DomainElement* get() const {
		return domelem_;
	}

	bool operator==(const DomElemContainer& right) const {
		return get() == right.get();
	}
	bool operator<(const DomElemContainer& right) const {
		return get() < right.get();
	}
	bool operator>(const DomElemContainer& right) const {
		return get() > right.get();
	}

	std::ostream& put(std::ostream& stream) const {
		return get()->put(stream);
	}
};

bool operator<(const DomainElement&, const DomainElement&);
inline bool operator>(const DomainElement& d1, const DomainElement& d2) {
	return d2 < d1;
}

inline bool operator==(const DomainElement& d1, const DomainElement& d2) {
	return &d1 == &d2; // FIXME: underapproximation of what is equal: just comparing pointers to get it fast, but NOT exact!
}

inline bool operator!=(const DomainElement& d1, const DomainElement& d2) {
	return not (d1 == d2);
}

inline bool operator<=(const DomainElement& d1, const DomainElement& d2) {
	return d1 == d2 || d1 < d2;
}

inline bool operator>=(const DomainElement& d1, const DomainElement& d2) {
	return d1 == d2 || d1 > d2;
}

std::ostream& operator<<(std::ostream&, const DomainElement&);

std::ostream& operator<<(std::ostream&, const ElementTuple&);

template<class T>
struct Compare {
	bool operator()(const T& t1, const T& t2) const {
		if (t1.size() < t2.size()) {
			return true;
		} else if (t1.size() > t2.size()) {
			return false; //TODO: right?
		}

		for (size_t n = 0; n < t1.size(); ++n) {
			if (*(t1[n]) < *(t2[n])) {
				return true;
			} else if (*(t1[n]) > *(t2[n])) {
				return false;
			}
		}
		return false;
	}
	bool operator()(const T* t1, const T* t2) const {
		return *t1 < *t2;
	}
};

typedef std::set<ElementTuple, Compare<ElementTuple> > SortedElementTable;
typedef std::set<const DomainElement*, Compare<DomainElement> > SortedElementTuple; // TODO naam trekt er niet op?
typedef std::map<ElementTuple, const DomainElement*, Compare<ElementTuple> > Tuple2Elem;

struct FirstNElementsEqual {
	unsigned int _arity;
	FirstNElementsEqual(unsigned int arity) :
			_arity(arity) {
	}
	bool operator()(const ElementTuple&, const ElementTuple&) const;
};

struct StrictWeakNTupleOrdering {
	unsigned int _arity;
	StrictWeakNTupleOrdering(unsigned int arity) :
			_arity(arity) {
	}
	bool operator()(const ElementTuple&, const ElementTuple&) const;
};

bool operator<(const Compound&, const Compound&);
bool operator>(const Compound&, const Compound&);
bool operator==(const Compound&, const Compound&);
bool operator!=(const Compound&, const Compound&);
bool operator<=(const Compound&, const Compound&);
bool operator>=(const Compound&, const Compound&);

std::ostream& operator<<(std::ostream&, const Compound&);

/*******************
 Domain atoms
 *******************/

class DomainAtomFactory;
class PFSymbol;
class Variable;
class Vocabulary;
class AbstractStructure;

class DomainAtom {
private:
	PFSymbol* _symbol;
	ElementTuple _args;

	DomainAtom(PFSymbol* symbol, const ElementTuple& args) :
			_symbol(symbol), _args(args) {
	}

public:
	~DomainAtom() {
	}

	PFSymbol* symbol() const {
		return _symbol;
	}
	const ElementTuple& args() const {
		return _args;
	}

	std::ostream& put(std::ostream&) const;
	std::string toString() const;

	friend class DomainAtomFactory;
};

class DomainAtomFactory {
private:
	static DomainAtomFactory* _instance;
	std::map<PFSymbol*, std::map<ElementTuple, DomainAtom*> > _atoms;
	DomainAtomFactory() {
	}

public:
	~DomainAtomFactory();
	static DomainAtomFactory* instance();
	const DomainAtom* create(PFSymbol*, const ElementTuple&);
};

/****************
 Iterators
 ****************/

class InternalTableIterator;
class InternalSortIterator;

/**
 * Constant iterator over tables for sorts, predicate, and function symbols.
 */
class TableIterator {
private:
	InternalTableIterator* _iterator;
public:
	TableIterator() :
			_iterator(0) {
	}
	TableIterator(const TableIterator&);
	TableIterator(InternalTableIterator* iter) :
			_iterator(iter) {
	}
	TableIterator& operator=(const TableIterator&);
	//bool					hasNext()	const;
	bool isAtEnd() const;
	const ElementTuple& operator*() const;
	void operator++();
	~TableIterator();
	const InternalTableIterator* iterator() const {
		return _iterator;
	}
};

class SortIterator {
private:
	InternalSortIterator* _iterator;
public:
	SortIterator(InternalSortIterator* iter) :
			_iterator(iter) {
		Assert(iter!=NULL);
	}
	SortIterator(const SortIterator&);
	SortIterator& operator=(const SortIterator&);
//		bool					hasNext()	const;
	bool isAtEnd() const;
	const DomainElement* operator*() const;
	SortIterator& operator++();
	~SortIterator();
	const InternalSortIterator* iterator() const {
		return _iterator;
	}
};

class InternalTableIterator {
private:
	virtual bool hasNext() const = 0;
	virtual const ElementTuple& operator*() const = 0;
	virtual void operator++() = 0;
public:
	virtual ~InternalTableIterator() {
	}
	virtual InternalTableIterator* clone() const = 0;
	friend class TableIterator;
};

class CartesianInternalTableIterator: public InternalTableIterator {
private:
	std::vector<SortIterator> _iterators;
	std::vector<SortIterator> _lowest;
	mutable ElementTable _deref;
	bool _hasNext;
	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
public:
	CartesianInternalTableIterator(const std::vector<SortIterator>& vsi, const std::vector<SortIterator>& low, bool h = true);
	CartesianInternalTableIterator* clone() const;
};

class InstGenerator;

class GeneratorInternalTableIterator: public InternalTableIterator {
private:
	InstGenerator* _generator;
	std::vector<const DomElemContainer*> _vars;
	bool _hasNext;
	mutable ElementTable _deref;
	bool hasNext() const {
		return _hasNext;
	}
	const ElementTuple& operator*() const;
	void operator++();
public:
	GeneratorInternalTableIterator(InstGenerator* generator, const std::vector<const DomElemContainer*>& vars, bool reset = true, bool h = true);
	GeneratorInternalTableIterator* clone() const {
		return new GeneratorInternalTableIterator(_generator, _vars, false, _hasNext);
	}
};

class SortInternalTableIterator: public InternalTableIterator {
private:
	InternalSortIterator* _iter;
	mutable ElementTable _deref;
	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
public:
	SortInternalTableIterator(InternalSortIterator* isi) :
			_iter(isi) {
	}
	~SortInternalTableIterator();
	SortInternalTableIterator* clone() const;
};

class EnumInternalIterator: public InternalTableIterator {
private:
	SortedElementTable::const_iterator _iter;
	SortedElementTable::const_iterator _end;
	bool hasNext() const {
		return _iter != _end;
	}
	const ElementTuple& operator*() const {
		return *_iter;
	}
	void operator++() {
		++_iter;
	}
public:
	EnumInternalIterator(SortedElementTable::const_iterator it, SortedElementTable::const_iterator end) :
			_iter(it), _end(end) {
	}
	~EnumInternalIterator() {
	}
	EnumInternalIterator* clone() const;
};

class EnumInternalFuncIterator: public InternalTableIterator {
private:
	Tuple2Elem::const_iterator _iter;
	Tuple2Elem::const_iterator _end;
	mutable ElementTable _deref;
	bool hasNext() const {
		return _iter != _end;
	}
	const ElementTuple& operator*() const;
	void operator++() {
		++_iter;
	}
public:
	EnumInternalFuncIterator(Tuple2Elem::const_iterator it, Tuple2Elem::const_iterator end) :
			_iter(it), _end(end) {
	}
	~EnumInternalFuncIterator() {
	}
	EnumInternalFuncIterator* clone() const;
};

class SortTable;
class PredTable;
class InternalPredTable;

class InternalFuncTable;

class InternalFuncIterator: public InternalTableIterator {
private:
	TableIterator _curr;
	mutable ElementTable _deref;
	const InternalFuncTable* _function;
	bool hasNext() const {
		return not _curr.isAtEnd();
	}
	const ElementTuple& operator*() const;
	void operator++();
public:
	InternalFuncIterator(const InternalFuncTable* f, const Universe& univ);
	InternalFuncIterator(const InternalFuncTable* f, const TableIterator& c) :
			_curr(c), _function(f) {
	}
	~InternalFuncIterator() {
	}
	InternalFuncIterator* clone() const {
		return new InternalFuncIterator(_function, _curr);
	}
};

class InternalPredTable;

class ProcInternalTableIterator: public InternalTableIterator {
private:
	TableIterator _curr;
	Universe _univ;
	mutable ElementTable _deref;
	const InternalPredTable* _predicate;
	bool hasNext() const {
		return not _curr.isAtEnd();
	}
	const ElementTuple& operator*() const;
	void operator++();
public:
	ProcInternalTableIterator(const InternalPredTable* p, const Universe& univ);
	ProcInternalTableIterator(const InternalPredTable* p, const TableIterator& c, const Universe& univ) :
			_curr(c), _univ(univ), _predicate(p) {
	}
	~ProcInternalTableIterator() {
	}
	ProcInternalTableIterator* clone() const {
		return new ProcInternalTableIterator(_predicate, _curr, _univ);
	}
};

class UnionInternalIterator: public InternalTableIterator {
private:
	std::vector<TableIterator> _iterators;
	Universe _universe;
	std::vector<InternalPredTable*> _outtables;
	std::vector<TableIterator>::iterator _curriterator;

	bool contains(const ElementTuple&) const;
	void setcurriterator();

	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
public:
	UnionInternalIterator(const std::vector<TableIterator>&, const std::vector<InternalPredTable*>&, const Universe&);
	~UnionInternalIterator() {
	}
	UnionInternalIterator* clone() const;
};

class UNAInternalIterator: public InternalTableIterator {
private:
	std::vector<SortIterator> _curr;
	std::vector<SortIterator> _lowest;
	Function* _function;
	mutable bool _end;
	mutable ElementTuple _currtuple;
	mutable std::vector<ElementTuple> _deref;

	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
	UNAInternalIterator(const std::vector<SortIterator>&, const std::vector<SortIterator>&, Function*, bool);
public:
	UNAInternalIterator(const std::vector<SortIterator>&, Function*);
	~UNAInternalIterator() {
	}
	UNAInternalIterator* clone() const;
};

class InverseInternalIterator: public InternalTableIterator {
private:
	std::vector<SortIterator> _curr;
	std::vector<SortIterator> _lowest;
	Universe _universe;
	InternalPredTable* _outtable;
	mutable bool _end;
	mutable ElementTuple _currtuple;
	mutable ElementTuple _deref;

	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
	InverseInternalIterator(const std::vector<SortIterator>&, const std::vector<SortIterator>&, InternalPredTable*, const Universe&, bool);
public:
	InverseInternalIterator(const std::vector<SortIterator>&, InternalPredTable*, const Universe&);
	~InverseInternalIterator() {
	}
	InverseInternalIterator* clone() const;
};

class EqualInternalIterator: public InternalTableIterator {
private:
	SortIterator _iterator;
	mutable ElementTable _deref;
	bool hasNext() const;
	const ElementTuple& operator*() const;
	void operator++();
public:
	EqualInternalIterator(const SortIterator& iter);
	~EqualInternalIterator() {
	}
	EqualInternalIterator* clone() const;
};

class InternalSortIterator {
public:
	virtual bool hasNext() const = 0; // TODO should become isAtEnd
	virtual const DomainElement* operator*() const = 0;
	virtual void operator++() = 0;
	virtual ~InternalSortIterator() {
	}
	virtual InternalSortIterator* clone() const = 0;
	friend class SortIterator;
	friend class SortInternalTableIterator;
};

class UnionInternalSortIterator: public InternalSortIterator {
private:
	std::vector<SortIterator> _iterators;
	std::vector<SortTable*> _outtables;
	std::vector<SortIterator>::iterator _curriterator;

	bool contains(const DomainElement*) const;
	void setcurriterator();

	bool hasNext() const;
	const DomainElement* operator*() const;
	void operator++();
public:
	UnionInternalSortIterator(const std::vector<SortIterator>&, const std::vector<SortTable*>&);
	~UnionInternalSortIterator() {
	}
	UnionInternalSortIterator* clone() const;
};

class NatInternalSortIterator: public InternalSortIterator {
private:
	int _iter;
	bool hasNext() const {
		return true;
	}
	const DomainElement* operator*() const {
		return createDomElem(_iter);
	}
	void operator++() {
		++_iter;
	}
public:
	NatInternalSortIterator(int iter = 0) :
			_iter(iter) {
	}
	~NatInternalSortIterator() {
	}
	NatInternalSortIterator* clone() const {
		return new NatInternalSortIterator(_iter);
	}
};

class IntInternalSortIterator: public InternalSortIterator {
private:
	int _iter;
	bool hasNext() const {
		return true;
	}
	const DomainElement* operator*() const {
		return createDomElem(_iter);
	}
	void operator++() {
		++_iter;
	}
public:
	IntInternalSortIterator(int iter = getMinElem<int>()) :
			_iter(iter) {
	}
	~IntInternalSortIterator() {
	}
	IntInternalSortIterator* clone() const {
		return new IntInternalSortIterator(_iter);
	}
};

class FloatInternalSortIterator: public InternalSortIterator {
private:
	double _iter;
	bool hasNext() const {
		return true;
	}
	const DomainElement* operator*() const {
		return createDomElem(_iter);
	}
	void operator++() {
		++_iter;
	}
public:
	FloatInternalSortIterator(double iter = getMinElem<double>()) :
			_iter(iter) {
	}
	~FloatInternalSortIterator() {
	}
	FloatInternalSortIterator* clone() const {
		return new FloatInternalSortIterator(_iter);
	}
};

class StringInternalSortIterator: public InternalSortIterator {
private:
	std::string _iter;
	bool hasNext() const {
		return true;
	}
	const DomainElement* operator*() const;
	void operator++();
public:
	StringInternalSortIterator(const std::string& iter = "") :
			_iter(iter) {
	}
	~StringInternalSortIterator() {
	}
	StringInternalSortIterator* clone() const {
		return new StringInternalSortIterator(_iter);
	}
};

class CharInternalSortIterator: public InternalSortIterator {
private:
	char _iter;
	bool _end;
	bool hasNext() const {
		return !_end;
	}
	const DomainElement* operator*() const;
	void operator++();
public:
	CharInternalSortIterator(char iter = getMinElem<char>(), bool end = false) :
			_iter(iter), _end(end) {
	}
	~CharInternalSortIterator() {
	}
	CharInternalSortIterator* clone() const {
		return new CharInternalSortIterator(_iter);
	}
};

// TODO danger of not detecting iterator invalidation when the sortedelementtuple is changed in the associated table
class EnumInternalSortIterator: public InternalSortIterator {
private:
	SortedElementTuple::const_iterator _iter;
	SortedElementTuple::const_iterator _end;
	bool hasNext() const {
		return _iter != _end;
	}
	const DomainElement* operator*() const {
		return *_iter;
	}
	void operator++() {
		++_iter;
	}
public:
	EnumInternalSortIterator(SortedElementTuple::iterator it, SortedElementTuple::iterator end) :
			_iter(it), _end(end) {
	}
	~EnumInternalSortIterator() {
	}
	EnumInternalSortIterator* clone() const {
		return new EnumInternalSortIterator(_iter, _end);
	}
};

class RangeInternalSortIterator: public InternalSortIterator {
private:
	int _current;
	int _last;
	bool hasNext() const {
		return _current <= _last;
	}
	const DomainElement* operator*() const {
		return createDomElem(_current);
	}
	void operator++() {
		++_current;
	}
public:
	RangeInternalSortIterator(int current, int last) :
			_current(current), _last(last) {
	}
	~RangeInternalSortIterator() {
	}
	RangeInternalSortIterator* clone() const {
		return new RangeInternalSortIterator(_current, _last);
	}
};

/********************************************
 Internal tables for predicate symbols
 ********************************************/

class StructureVisitor;

/**
 *	This class implements a concrete two-dimensional table
 */
class InternalPredTable {
protected:
	// Attributes
	unsigned int _nrRefs; //!< The number of references to this table

public:
	// Inspectors
	virtual bool finite(const Universe&) const = 0; //!< Returns true iff the table is finite
	virtual bool empty(const Universe&) const = 0; //!< Returns true iff the table is empty

	virtual bool approxFinite(const Universe&) const = 0;
	//!< Returns false if the table size is infinite. May return true if the table size is finite.
	virtual bool approxEmpty(const Universe&) const = 0;
	//!< Returns false if the table is non-empty. May return true if the table is empty.

	virtual bool contains(const ElementTuple& tuple, const Universe&) const = 0;
	//!< Returns true iff the table contains the tuple.
	virtual tablesize size(const Universe&) const = 0;

	// Mutators
	virtual InternalPredTable* add(const ElementTuple& tuple) = 0; //!< Add a tuple to the table
	virtual InternalPredTable* remove(const ElementTuple& tuple) = 0; //!< Remove a tuple from the table

	void decrementRef(); //!< Delete one reference. Deletes the table if the number of references becomes zero.
	void incrementRef(); //!< Add one reference

	// Iterators
	virtual InternalTableIterator* begin(const Universe&) const = 0;

	InternalPredTable() :
			_nrRefs(0) {
	}

	// Visitor
	virtual void accept(StructureVisitor* v) const = 0;

	friend class PredTable;
	friend class SortTable;

	virtual void put(std::ostream&) const;

protected:
	virtual ~InternalPredTable() {
	}
};

class ProcInternalPredTable: public InternalPredTable {
private:
	std::string* _procedure;
public:
	ProcInternalPredTable(std::string* proc) :
			InternalPredTable(), _procedure(proc) {
	}

	~ProcInternalPredTable();

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe& univ) const;

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalPredTable* add(const ElementTuple& tuple); //!< Add a tuple to the table
	InternalPredTable* remove(const ElementTuple& tuple); //!< Remove a tuple from the table

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class FOBDDManager;
class FOBDD;
class AbstractStructure;

class BDDInternalPredTable: public InternalPredTable {
private:
	FOBDDManager* _manager;
	const FOBDD* _bdd;
	std::vector<Variable*> _vars;
	const AbstractStructure* _structure;
public:
	BDDInternalPredTable(const FOBDD*, FOBDDManager*, const std::vector<Variable*>&, const AbstractStructure*);
	~BDDInternalPredTable() {
	}

	FOBDDManager* manager() const {
		return _manager;
	}
	const FOBDD* bdd() const {
		return _bdd;
	}
	const std::vector<Variable*>& vars() const {
		return _vars;
	}
	const AbstractStructure* structure() const {
		return _structure;
	}

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalPredTable* add(const ElementTuple& tuple);
	InternalPredTable* remove(const ElementTuple& tuple);

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class FullInternalPredTable: public InternalPredTable {
private:
public:
	FullInternalPredTable() :
			InternalPredTable() {
	}

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe& univ) const {
		return univ.size();
	}

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalPredTable* add(const ElementTuple& tuple); //!< Add a tuple to the table
	InternalPredTable* remove(const ElementTuple& tuple); //!< Remove a tuple from the table

	InternalTableIterator* begin(const Universe&) const;

	~FullInternalPredTable();

	// Visitor
	void accept(StructureVisitor* v) const;
};

class FuncTable;

class FuncInternalPredTable: public InternalPredTable {
private:
	FuncTable* _table;
	bool _linked;
public:
	FuncInternalPredTable(FuncTable* table, bool linked);

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe& univ) const;

	bool contains(const ElementTuple& tuple, const Universe&) const;
	FuncTable* table() const {
		return _table;
	}

	InternalPredTable* add(const ElementTuple& tuple); //!< Add a tuple to the table
	InternalPredTable* remove(const ElementTuple& tuple); //!< Remove a tuple from the table

	InternalTableIterator* begin(const Universe&) const;

	~FuncInternalPredTable();

	// Visitor
	void accept(StructureVisitor* v) const;
};

class PredTable;

/**
 *	This class implements table that consists of all tuples that belong to the union of a set of tables,
 *	but not to the union of another set of tables.
 *
 *	INVARIANT: the first table of _intables and of _outtables has an enumerated internal table
 */
class UnionInternalPredTable: public InternalPredTable {
private:
	std::vector<InternalPredTable*> _intables;
	//!< a tuple of the table does belong to at least one of the tables in _intables
	std::vector<InternalPredTable*> _outtables;
	//!< a tuple of the table does not belong to any of the tables in _outtables

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

public:
	const std::vector<InternalPredTable*>& inTables() const {
		return _intables;
	}
	const std::vector<InternalPredTable*>& outTables() const {
		return _outtables;
	}

	UnionInternalPredTable();
	UnionInternalPredTable(const std::vector<InternalPredTable*>& in, const std::vector<InternalPredTable*>& out);
	~UnionInternalPredTable();
	void addInTable(InternalPredTable* t) {
		_intables.push_back(t);
		t->incrementRef();
	}
	void addOutTable(InternalPredTable* t) {
		_outtables.push_back(t);
		t->incrementRef();
	}
	InternalPredTable* add(const ElementTuple& tuple);
	InternalPredTable* remove(const ElementTuple& tuple);

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *	This class implements a finite, enumerated InternalPredTable
 */
class EnumeratedInternalPredTable: public InternalPredTable {
private:
	SortedElementTable _table; //!< the actual table

	bool finite(const Universe&) const {
		return true;
	}
	bool empty(const Universe&) const {
		return _table.empty();
	}
	bool approxFinite(const Universe&) const {
		return true;
	}
	bool approxEmpty(const Universe&) const {
		return _table.empty();
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_EXACT, _table.size());
	}

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

public:
	EnumeratedInternalPredTable(const SortedElementTable& tab) :
			InternalPredTable(), _table(tab) {
	}
	EnumeratedInternalPredTable() :
			InternalPredTable() {
	}
	~EnumeratedInternalPredTable() {
	}
	EnumeratedInternalPredTable* add(const ElementTuple& tuple);
	EnumeratedInternalPredTable* remove(const ElementTuple& tuple);

	// Visitor
	void accept(StructureVisitor* v) const;
};

class InternalSortTable;

/**
 *	Abstract base class for implementing InternalPredTable for '=/2', '</2', and '>/2'
 */
class ComparisonInternalPredTable: public InternalPredTable {
protected:
public:
	ComparisonInternalPredTable();
	virtual ~ComparisonInternalPredTable();
	InternalPredTable* add(const ElementTuple& tuple);
	InternalPredTable* remove(const ElementTuple& tuple);
};

/**
 *	Internal table for '=/2'
 */
class EqualInternalPredTable: public ComparisonInternalPredTable {
public:
	EqualInternalPredTable() :
			ComparisonInternalPredTable() {
	}
	~EqualInternalPredTable() {
	}

	bool contains(const ElementTuple&, const Universe&) const;
	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *	Internal table for '</2'
 */
class StrLessInternalPredTable: public ComparisonInternalPredTable {
public:
	StrLessInternalPredTable() :
			ComparisonInternalPredTable() {
	}
	~StrLessInternalPredTable() {
	}

	bool contains(const ElementTuple&, const Universe&) const;
	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *	Internal table for '>/2'
 */
class StrGreaterInternalPredTable: public ComparisonInternalPredTable {
public:
	StrGreaterInternalPredTable() :
			ComparisonInternalPredTable() {
	}
	~StrGreaterInternalPredTable() {
	}

	bool contains(const ElementTuple&, const Universe&) const;
	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;

};

/**
 *	This class implements the complement of an internal predicate table
 */
class InverseInternalPredTable: public InternalPredTable {
private:
	InternalPredTable* _invtable; //!< the inverse of the actual table

public:
	InverseInternalPredTable(InternalPredTable* inv);
	~InverseInternalPredTable();

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;
	InternalPredTable* table() const {
		return _invtable;
	}

	bool contains(const ElementTuple& tuple, const Universe&) const;

	InternalPredTable* add(const ElementTuple& tuple);
	InternalPredTable* remove(const ElementTuple& tuple);

	InternalTableIterator* begin(const Universe&) const;

	void internTable(InternalPredTable*);

	// Visitor
	void accept(StructureVisitor* v) const;
};

/********************************
 Internal tables for sorts
 ********************************/

/**
 *	This class implements a concrete one-dimensional table
 */
class InternalSortTable: public InternalPredTable {
protected:

public:
	InternalSortTable() {
	}

	unsigned int arity() const {
		return 1;
	}

	virtual bool contains(const DomainElement*) const = 0;
	bool contains(const ElementTuple& tuple) const {
		return contains(tuple[0]);
	}
	bool contains(const ElementTuple& tuple, const Universe&) const {
		return contains(tuple[0]);
	}

	virtual bool empty() const = 0;
	virtual bool finite() const = 0;
	virtual bool approxEmpty() const = 0;
	virtual bool approxFinite() const = 0;
	virtual tablesize size() const = 0;
	bool empty(const Universe&) const {
		return empty();
	}
	bool finite(const Universe&) const {
		return finite();
	}
	bool approxEmpty(const Universe&) const {
		return approxEmpty();
	}
	bool approxFinite(const Universe&) const {
		return approxFinite();
	}
	tablesize size(const Universe&) const {
		return size();
	}

	virtual InternalSortTable* add(const DomainElement*) = 0;
	InternalSortTable* add(const ElementTuple& tuple) {
		return add(tuple[0]);
	}
	virtual InternalSortTable* remove(const DomainElement*) = 0;
	InternalSortTable* remove(const ElementTuple& tuple) {
		return remove(tuple[0]);
	}
	virtual InternalSortTable* add(int i1, int i2) = 0;

	virtual InternalSortIterator* sortBegin() const = 0;
	virtual InternalSortIterator* sortIterator(const DomainElement*) const = 0;
	InternalTableIterator* begin() const;
	InternalTableIterator* begin(const Universe&) const {
		return begin();
	}

	// Returns true if non-empty and is a range
	virtual bool isRange() const = 0;
	// Returns NULL if empty or not a range
	virtual const DomainElement* first() const = 0;
	// Returns NULL if empty or not a range
	virtual const DomainElement* last() const = 0;

	// Visitor
	virtual void accept(StructureVisitor* v) const = 0;

protected:
	// SortTable is responsible for ref management and deletion!
	virtual ~InternalSortTable() {
	}

};

class UnionInternalSortTable: public InternalSortTable {
public:
	std::vector<SortTable*> _intables;
	//!< an element of the table does belong to at least one of the tables in _intables
	std::vector<SortTable*> _outtables;
	//!< an element of the table does not belong to any of the tables in _outtables

	bool finite() const;
	bool empty() const;
	bool approxFinite() const;
	bool approxEmpty() const;
	tablesize size() const;

	bool contains(const DomainElement*) const;

	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~UnionInternalSortTable();

public:
	UnionInternalSortTable();
	UnionInternalSortTable(const std::vector<SortTable*>& in, const std::vector<SortTable*>& out) :
			_intables(in), _outtables(out) {
	}
	void addInTable(SortTable* t) {
		_intables.push_back(t);
	}
	void addOutTable(SortTable* t) {
		_outtables.push_back(t);
	}
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	InternalSortTable* add(int i1, int i2);

	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class InfiniteInternalSortTable: public InternalSortTable {
public:
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	bool finite() const {
		return false;
	}
	bool empty() const {
		return false;
	}
	bool approxFinite() const {
		return false;
	}
	bool approxEmpty() const {
		return false;
	}
	tablesize size() const {
		return tablesize(TST_INFINITE, 0);
	}
protected:
	// SortTable is responsible for ref management and deletion!
	virtual ~InfiniteInternalSortTable() {
	}
};

/**
 *	All natural numbers
 */
class AllNaturalNumbers: public InfiniteInternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllNaturalNumbers() {
	}
};

/**
 * All integers
 */
class AllIntegers: public InfiniteInternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllIntegers() {
	}
};

/**
 * All floating point numbers
 */
class AllFloats: public InfiniteInternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllFloats() {
	}
};

/**
 * All strings
 */
class AllStrings: public InfiniteInternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllStrings() {
	}
};

/**
 * All characters
 */
class AllChars: public InternalSortTable {
public:
	bool contains(const DomainElement*) const;
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	InternalSortTable* add(int i1, int i2);

	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	bool finite() const {
		return true;
	}
	bool empty() const {
		return false;
	}
	bool approxFinite() const {
		return true;
	}
	bool approxEmpty() const {
		return false;
	}
	tablesize size() const;

	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const {
		return true;
	}

	// Visitor
	void accept(StructureVisitor* v) const;

protected:
	// SortTable is responsible for ref management and deletion!
	~AllChars() {
	}
};

/**
 *	A finite, enumerated SortTable
 */
class EnumeratedInternalSortTable: public InternalSortTable {
private:
	SortedElementTuple _table;

	bool contains(const DomainElement*) const;

	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	bool finite() const {
		return approxFinite();
	}
	bool empty() const {
		return approxEmpty();
	}
	bool approxFinite() const {
		return true;
	}
	bool approxEmpty() const {
		return _table.empty();
	}
	tablesize size() const {
		return tablesize(TST_EXACT, _table.size());
	}
protected:
	// SortTable is responsible for ref management and deletion!
	~EnumeratedInternalSortTable() {
	}
public:
	EnumeratedInternalSortTable() {
	}
	EnumeratedInternalSortTable(const SortedElementTuple& d) :
			_table(d) {
	}
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;
	bool isRange() const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *		A range of integers
 */
class IntRangeInternalSortTable: public InternalSortTable {
private:
	int _first; //!< first element in the range
	int _last; //!< last element in the range
protected:
	~IntRangeInternalSortTable() {
	}
public:
	// Takes the non-empty interval, regardless of the order
	IntRangeInternalSortTable(int f, int l) :
			_first(f), _last(l) {
		if(_first>_last){
			_last = f;
			_first = l;
		}
	}
	bool finite() const {
		return approxFinite();
	}
	bool empty() const {
		return approxEmpty();
	}
	bool approxFinite() const {
		return true;
	}
	bool approxEmpty() const {
		return _first > _last;
	}
	tablesize size() const {
		return tablesize(TST_EXACT, _last - _first + 1);
	}
	InternalSortTable* add(const DomainElement*);
	InternalSortTable* remove(const DomainElement*);
	InternalSortTable* add(int i1, int i2);
	const DomainElement* first() const;
	const DomainElement* last() const;

	bool contains(const DomainElement*) const;
	bool isRange() const {
		return true;
	}

	InternalSortIterator* sortBegin() const;
	InternalSortIterator* sortIterator(const DomainElement*) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/************************************
 Internal tables for functions
 ************************************/

/**
 *		This class implements a concrete associative array mapping tuples of elements to elements
 */
class InternalFuncTable {
protected:
	unsigned int _nrRefs;
public:
	InternalFuncTable() :
			_nrRefs(0) {
	}
	virtual ~InternalFuncTable() {
	}

	void decrementRef(); //!< Delete one reference. Deletes the table if the number of references becomes zero.
	void incrementRef(); //!< Add one reference

	virtual bool finite(const Universe&) const = 0; //!< Returns true iff the table is finite
	virtual bool empty(const Universe&) const = 0; //!< Returns true iff the table is empty

	virtual bool approxFinite(const Universe&) const = 0;
	//!< Returns false if the table size is infinite. May return true if the table size is finite.
	virtual bool approxEmpty(const Universe&) const = 0;
	//!< Returns false if the table is non-empty. May return true if the table is empty.
	virtual tablesize size(const Universe&) const = 0;

	bool contains(const ElementTuple& tuple, const Universe&) const;
	virtual const DomainElement* operator[](const ElementTuple& tuple) const = 0;
	//!< Returns the value of the tuple according to the array.

	virtual InternalFuncTable* add(const ElementTuple&) = 0; //!< Add a tuple to the table
	virtual InternalFuncTable* remove(const ElementTuple&) = 0; //!< Remove a tuple from the table

	virtual InternalTableIterator* begin(const Universe&) const = 0;

	// Visitor
	virtual void accept(StructureVisitor* v) const = 0;

	virtual void put(std::ostream& stream) const;
};

class ProcInternalFuncTable: public InternalFuncTable {
private:
	std::string* _procedure;
public:
	ProcInternalFuncTable(std::string* proc) :
			InternalFuncTable(), _procedure(proc) {
	}

	~ProcInternalFuncTable();

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	const DomainElement* operator[](const ElementTuple& tuple) const;
	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class UNAInternalFuncTable: public InternalFuncTable {
private:
	Function* _function;
public:
	UNAInternalFuncTable(Function* f) :
			InternalFuncTable(), _function(f) {
	}

	~UNAInternalFuncTable() {
	}

	bool finite(const Universe&) const;
	bool empty(const Universe&) const;
	bool approxFinite(const Universe&) const;
	bool approxEmpty(const Universe&) const;
	tablesize size(const Universe&) const;

	const DomainElement* operator[](const ElementTuple& tuple) const;
	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/**
 *		A finite, enumerated InternalFuncTable
 */
class EnumeratedInternalFuncTable: public InternalFuncTable {
private:
	Tuple2Elem _table;
public:
	EnumeratedInternalFuncTable() :
			InternalFuncTable() {
	}
	EnumeratedInternalFuncTable(const Tuple2Elem& tab) :
			InternalFuncTable(), _table(tab) {
	}
	virtual ~EnumeratedInternalFuncTable() {
	}

	bool finite(const Universe&) const {
		return true;
	}
	bool empty(const Universe&) const {
		return _table.empty();
	}
	bool approxFinite(const Universe&) const {
		return true;
	}
	bool approxEmpty(const Universe&) const {
		return _table.empty();
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_EXACT, _table.size());
	}

	const DomainElement* operator[](const ElementTuple& tuple) const;
	EnumeratedInternalFuncTable* add(const ElementTuple&);
	EnumeratedInternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class IntFloatInternalFuncTable: public InternalFuncTable {
protected:
	NumType _type;
public:

	IntFloatInternalFuncTable(bool i) :
			_type(i ? NumType::CERTAINLYINT : NumType::POSSIBLYINT) {
	}

	bool finite(const Universe&) const {
		return false;
	}
	bool empty(const Universe&) const {
		return false;
	}
	bool approxFinite(const Universe&) const {
		return false;
	}
	bool approxEmpty(const Universe&) const {
		return false;
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_INFINITE, 0);
	}

	NumType getType() const {
		return _type;
	}

	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	virtual InternalTableIterator* begin(const Universe&) const = 0;
};

class PlusInternalFuncTable: public IntFloatInternalFuncTable {
public:
	PlusInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class MinusInternalFuncTable: public IntFloatInternalFuncTable {
public:
	MinusInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class TimesInternalFuncTable: public IntFloatInternalFuncTable {
public:
	TimesInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class DivInternalFuncTable: public IntFloatInternalFuncTable {
public:
	DivInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class AbsInternalFuncTable: public IntFloatInternalFuncTable {
public:
	AbsInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class UminInternalFuncTable: public IntFloatInternalFuncTable {
public:
	UminInternalFuncTable(bool isint) :
			IntFloatInternalFuncTable(isint) {
	}
	const DomainElement* operator[](const ElementTuple&) const;
	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

class ExpInternalFuncTable: public InternalFuncTable {
public:
	bool finite(const Universe&) const {
		return false;
	}
	bool empty(const Universe&) const {
		return false;
	}
	bool approxFinite(const Universe&) const {
		return false;
	}
	bool approxEmpty(const Universe&) const {
		return false;
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_INFINITE, 0);
	}
	const DomainElement* operator[](const ElementTuple&) const;

	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	ExpInternalFuncTable() {
	}
	~ExpInternalFuncTable() {
	}

	// Visitor
	void accept(StructureVisitor* v) const;
};

class ModInternalFuncTable: public InternalFuncTable {
public:
	ModInternalFuncTable() {
	}
	~ModInternalFuncTable() {
	}
	bool finite(const Universe&) const {
		return false;
	}
	bool empty(const Universe&) const {
		return false;
	}
	bool approxFinite(const Universe&) const {
		return false;
	}
	bool approxEmpty(const Universe&) const {
		return false;
	}
	tablesize size(const Universe&) const {
		return tablesize(TST_INFINITE, 0);
	}
	const DomainElement* operator[](const ElementTuple&) const;

	InternalFuncTable* add(const ElementTuple&);
	InternalFuncTable* remove(const ElementTuple&);

	InternalTableIterator* begin(const Universe&) const;

	// Visitor
	void accept(StructureVisitor* v) const;
};

/*********************************************************
 Tables for sorts, predicates, and function symbols
 *********************************************************/

/**
 *	This class implements the common functionality of tables for sorts, predicate, and function symbols.
 */
class AbstractTable {
public:
	virtual ~AbstractTable() {
	}

	virtual bool finite() const = 0; //!< Returns true iff the table is finite
	virtual bool empty() const = 0; //!< Returns true iff the table is empty
	virtual unsigned int arity() const = 0; //!< Returns the number of columns in the table

	virtual bool approxFinite() const = 0;
	//!< Returns false if the table size is infinite. May return true if the table size is finite.
	virtual bool approxEmpty() const = 0;
	//!< Returns false if the table is non-empty. May return true if the table is empty.

	virtual bool contains(const ElementTuple& tuple) const = 0;
	//!< Returns true iff the table contains the tuple.

	virtual AbstractTable* materialize() const = 0; //!< Try to replace a symbolic table by an enumerated one.
													//!< Returns 0 in case this is impossible, or the table was
													//!< already enumerated

	virtual void add(const ElementTuple& tuple) = 0; //!< Add a tuple to the table
	virtual void remove(const ElementTuple& tuple) = 0; //!< Remove a tuple from the table

	virtual TableIterator begin() const = 0;

	virtual void put(std::ostream& stream) const = 0;
};

/**
 *	This class implements tables for predicate symbols.
 */
class PredTable: public AbstractTable {
private:
	InternalPredTable* _table; //!< Points to the actual table
	Universe _universe;
public:
	PredTable(InternalPredTable* table, const Universe&);
	~PredTable();

	bool finite() const {
		return _table->finite(_universe);
	}
	bool empty() const {
		return _table->empty(_universe);
	}
	unsigned int arity() const {
		return _universe.arity();
	}
	bool approxFinite() const {
		return _table->approxFinite(_universe);
	}
	bool approxEmpty() const {
		return _table->approxEmpty(_universe);
	}
	bool contains(const ElementTuple& tuple) const {
		return _table->contains(tuple, _universe);
	}
	tablesize size() const {
		return _table->size(_universe);
	}
	void add(const ElementTuple& tuple);
	void remove(const ElementTuple& tuple);

	TableIterator begin() const;

	const Universe& universe() const {
		return _universe;
	}
	PredTable* materialize() const;

	virtual void put(std::ostream& stream) const;

	InternalPredTable* internTable() const {
		return _table;
	}

private:
	void setTable(InternalPredTable* table);
};

/**
 *		This class implements tables for sorts
 */
class SortTable: public AbstractTable {
private:
	InternalSortTable* _table; //!< Points to the actual table
public:
	SortTable(InternalSortTable* table);
	~SortTable();

	void internTable(InternalSortTable*);

	bool finite() const {
		return _table->finite();
	}
	bool empty() const {
		return _table->empty();
	}
	bool approxFinite() const {
		return _table->approxFinite();
	}
	bool approxEmpty() const {
		return _table->approxEmpty();
	}
	unsigned int arity() const {
		return 1;
	}
	tablesize size() const {
		return _table->size();
	}
	bool contains(const ElementTuple& tuple) const {
		return _table->contains(tuple);
	}
	bool contains(const DomainElement* el) const {
		return _table->contains(el);
	}
	void add(const ElementTuple& tuple);
	void add(const DomainElement* el);
	void add(int i1, int i2);
	void remove(const ElementTuple& tuple);
	void remove(const DomainElement* el);
	TableIterator begin() const;
	SortIterator sortBegin() const;
	SortIterator sortIterator(const DomainElement*) const;

	// Returns true if non-empty and a range
	bool isRange() const {
		return _table->isRange();
	}
	// NOTE: first and last are guaranteed NOT NULL if the table is not empty
	const DomainElement* first() const {
		Assert(not empty());
		return _table->first();
	}
	const DomainElement* last() const {
		Assert(not empty());
		return _table->last();
	}

	InternalSortTable* internTable() const {
		return _table;
	}
	SortTable* materialize() const;

	virtual void put(std::ostream& stream) const;
};

/**
 *	This class implements tables for function symbols
 */
class FuncTable: public AbstractTable {
private:
	InternalFuncTable* _table; //!< Points to the actual table
	Universe _universe;
public:
	FuncTable(InternalFuncTable* table, const Universe&);
	~FuncTable();

	bool finite() const {
		return _table->finite(_universe);
	}
	bool empty() const {
		return _table->empty(_universe);
	}
	unsigned int arity() const {
		return _universe.arity() - 1;
	}
	bool approxFinite() const {
		return _table->approxFinite(_universe);
	}
	bool approxEmpty() const {
		return _table->approxEmpty(_universe);
	}
	tablesize size() const {
		return _table->size(_universe);
	}

	// !!! RETURNS NULL iff the given tuple does not map to a domainelement within the range sort
	const DomainElement* operator[](const ElementTuple& tuple) const {
		Assert(tuple.size()==arity());
#ifdef DEBUG
		for (auto i = tuple.cbegin(); i < tuple.cend(); ++i) {
			Assert(*i!=NULL); // TODO this should be allowed (e.g. from a deeper function call, but crashes unexpectedly later in the program).
		}
#endif
		auto result = _table->operator[](tuple);
		if (universe().tables().back()->contains(result)) {
			return result;
		} else {
			return NULL;
		}
	}
	bool contains(const ElementTuple& tuple) const;
	void add(const ElementTuple& tuple);
	void remove(const ElementTuple& tuple);

	TableIterator begin() const;

	InternalFuncTable* internTable() const {
		return _table;
	}

	const Universe& universe() const {
		return _universe;
	}
	FuncTable* materialize() const;

	virtual void put(std::ostream& stream) const;
};

/**********************
 Interpretations
 *********************/

/**
 *	Class to represent a four-valued interpretation for a predicate
 */
class PredInter {
private:
	PredTable* _ct; //!< stores certainly true tuples
	PredTable* _cf; //!< stores certainly false tuples
	PredTable* _pt; //!< stores possibly true tuples
	PredTable* _pf; //!< stores possibly false tuples
	std::set<const ElementTuple*> _inconsistentElements; //!<stores all elements that are inconsistent in this interpretation

	void checkConsistency();

public:
	PredInter(PredTable* ctpf, PredTable* cfpt, bool ct, bool cf);
	PredInter(PredTable* ctpf, bool ct);

	~PredInter();

	// Mutators
	void ct(PredTable*); //!< Replace the certainly true (and possibly false) tuples
	void cf(PredTable*); //!< Replace the certainly false (and possibly true) tuples
	void pt(PredTable*); //!< Replace the possibly true (and certainly false) tuples
	void pf(PredTable*); //!< Replace the possibly false (and certainly true) tuples
	void ctpt(PredTable*); //!< Replace the certainly and possibly true tuples
	void materialize(); //!< Replace symbolic tables by enumerated ones if possible

	void makeTrue(const ElementTuple&); //!< Make the given tuple true
	void makeFalse(const ElementTuple&); //!< Make the given tuple false
	void makeUnknown(const ElementTuple&); //!< Make the given tuple unknown

	// Inspectors
	PredTable* ct() const {
		return _ct;
	}
	PredTable* cf() const {
		return _cf;
	}
	PredTable* pt() const {
		return _pt;
	}
	PredTable* pf() const {
		return _pf;
	}
	bool isTrue(const ElementTuple& tuple) const;
	bool isFalse(const ElementTuple& tuple) const;
	bool isUnknown(const ElementTuple& tuple) const;
	bool isInconsistent(const ElementTuple& tuple) const;
	bool isConsistent() const;
	bool approxTwoValued() const;
	const Universe& universe() const {
		return _ct->universe();
	}
	PredInter* clone(const Universe&) const;

private:
	void moveTupleFromTo(const ElementTuple& tuple, PredTable* from, PredTable* to);
};

std::ostream& operator<<(std::ostream& stream, const PredInter& interpretation);

class AbstractStructure;

class PredInterGenerator {
public:
	virtual PredInter* get(const AbstractStructure* structure) = 0;
	virtual ~PredInterGenerator() {
	}
};

class SinglePredInterGenerator: public PredInterGenerator {
private:
	PredInter* _inter;
public:
	SinglePredInterGenerator(PredInter* inter) :
			_inter(inter) {
	}
	~SinglePredInterGenerator() {
		delete (_inter);
	}
	PredInter* get(const AbstractStructure*) {
		return _inter;
	}
};

class Sort;

class InconsistentPredInterGenerator: public PredInterGenerator {
private:
	Predicate* _predicate;
public:
	InconsistentPredInterGenerator(Predicate* predicate) :
			_predicate(predicate) {
	}
	PredInter* get(const AbstractStructure* structure);
};

class EqualInterGenerator: public PredInterGenerator {
private:
	Sort* _sort;
	std::vector<PredInter*> generatedInters;
public:
	EqualInterGenerator(Sort* sort) :
			_sort(sort) {
	}
	~EqualInterGenerator();
	PredInter* get(const AbstractStructure* structure);
};

class StrLessThanInterGenerator: public PredInterGenerator {
private:
	Sort* _sort;
	std::vector<PredInter*> generatedInters;
public:
	StrLessThanInterGenerator(Sort* sort) :
			_sort(sort) {
	}
	~StrLessThanInterGenerator();
	PredInter* get(const AbstractStructure* structure);
};

class StrGreaterThanInterGenerator: public PredInterGenerator {
private:
	Sort* _sort;
	std::vector<PredInter*> generatedInters;
public:
	StrGreaterThanInterGenerator(Sort* sort) :
			_sort(sort) {
	}
	~StrGreaterThanInterGenerator();
	PredInter* get(const AbstractStructure* structure);
};

class PredInterGeneratorGenerator {
public:
	virtual PredInterGenerator* get(const std::vector<Sort*>&) = 0;
	virtual ~PredInterGeneratorGenerator() {
	}
};

class EqualInterGeneratorGenerator: public PredInterGeneratorGenerator {
public:
	EqualInterGenerator* get(const std::vector<Sort*>&);
};

class StrGreaterThanInterGeneratorGenerator: public PredInterGeneratorGenerator {
public:
	StrGreaterThanInterGenerator* get(const std::vector<Sort*>&);
};

class StrLessThanInterGeneratorGenerator: public PredInterGeneratorGenerator {
public:
	StrLessThanInterGenerator* get(const std::vector<Sort*>&);
};

/**
 *	Class to represent a four-valued interpretation for functions
 */
class FuncInter {
private:
	FuncTable* _functable; //!< the function table (if the interpretation is two-valued, nullpointer otherwise).
	PredInter* _graphinter; //!< the interpretation for the graph of the function

public:
	FuncInter(FuncTable* ft);
	FuncInter(PredInter* pt) :
			_functable(0), _graphinter(pt) {
	}
	~FuncInter();

	void graphInter(PredInter*);
	void funcTable(FuncTable*);

	bool isConsistent() const;
	void materialize();

	PredInter* graphInter() const {
		return _graphinter;
	}
	FuncTable* funcTable() const {
		return _functable;
	}
	bool approxTwoValued() const {
		return _functable != NULL;
	}

	const Universe& universe() const {
		return _graphinter->universe();
	}
	FuncInter* clone(const Universe&) const;
};

class FuncInterGenerator {
public:
	virtual FuncInter* get(const AbstractStructure* structure) = 0;
	virtual ~FuncInterGenerator() {
	}
};

class SingleFuncInterGenerator: public FuncInterGenerator {
private:
	FuncInter* _inter;
public:
	SingleFuncInterGenerator(FuncInter* inter) :
			_inter(inter) {
	}
	~SingleFuncInterGenerator() {
		delete (_inter);
	}
	FuncInter* get(const AbstractStructure*) {
		return _inter;
	}
};

class InconsistentFuncInterGenerator: public FuncInterGenerator {
private:
	Function* _function;
public:
	InconsistentFuncInterGenerator(Function* function) :
			_function(function) {
	}
	FuncInter* get(const AbstractStructure* structure);
};

class OneSortInterGenerator: public FuncInterGenerator {
protected:
	Sort* _sort;
public:
	OneSortInterGenerator(Sort* sort) :
			_sort(sort) {
	}
};

class MinInterGenerator: public OneSortInterGenerator {
public:
	MinInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const AbstractStructure* structure);
};

class MaxInterGenerator: public OneSortInterGenerator {
public:
	MaxInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const AbstractStructure* structure);
};

class SuccInterGenerator: public OneSortInterGenerator {
public:
	SuccInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const AbstractStructure* structure);
};

class InvSuccInterGenerator: public OneSortInterGenerator {
public:
	InvSuccInterGenerator(Sort* sort) :
			OneSortInterGenerator(sort) {
	}
	FuncInter* get(const AbstractStructure* structure);
};

class FuncInterGeneratorGenerator {
public:
	virtual FuncInterGenerator* get(const std::vector<Sort*>&) = 0;
	virtual ~FuncInterGeneratorGenerator() {
	}
};

class MinInterGeneratorGenerator: public FuncInterGeneratorGenerator {
public:
	MinInterGenerator* get(const std::vector<Sort*>&);
};

class MaxInterGeneratorGenerator: public FuncInterGeneratorGenerator {
public:
	MaxInterGenerator* get(const std::vector<Sort*>&);
};

class SuccInterGeneratorGenerator: public FuncInterGeneratorGenerator {
public:
	SuccInterGenerator* get(const std::vector<Sort*>&);
};

class InvSuccInterGeneratorGenerator: public FuncInterGeneratorGenerator {
public:
	InvSuccInterGenerator* get(const std::vector<Sort*>&);
};

// Contents ownership to receiver
std::vector<AbstractStructure*> generateEnoughTwoValuedExtensions(const std::vector<AbstractStructure*>& s);

/*********************
 * Auxiliary methods
 *********************/

namespace TableUtils {
PredInter* leastPredInter(const Universe& univ);
//!< construct a new, least precise predicate interpretation
FuncInter* leastFuncInter(const Universe& univ);
//!< construct a new, least precise function interpretation
Universe fullUniverse(unsigned int arity);

bool approxTotalityCheck(const FuncInter*);
//!< Check whether there is a value for every tuple in the given function interpretation.

bool approxIsInverse(const PredTable* pt1, const PredTable* pt2);

} /* namespace TableUtils */

#endif
