/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#include "TableSize.hpp"
#include "utils/ArithmeticUtils.hpp"

tablesize tablesize::operator+(const tablesize& rhs) const{
	if(rhs.isInfinite() || isInfinite()){
		return tablesize(TST_INFINITE, 0);
	}
	if(rhs.isUnknown() || isUnknown()){
		return tablesize(TST_UNKNOWN, 0);
	}

	// bool 'safe' indicates whether numerical operation was executed safely
	bool safe = true;
	auto result = _size;
	addition(_size,rhs._size,result,safe);
	if(safe){
		if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
			return tablesize(TableSizeType::TST_APPROXIMATED, result);
		}else{
			return tablesize(TableSizeType::TST_EXACT, result);
		}
	}else{
		return tablesize(TableSizeType::TST_INFINITE,0);
	}
}
tablesize tablesize::operator-(const tablesize& rhs) const{
	Assert(not rhs.isInfinite());

	if(isInfinite()){
		return *this;
	}
	if(rhs.isUnknown() || isUnknown()){
		return tablesize(TST_UNKNOWN, 0);
	}

	Assert(rhs._size <= _size); // result cannot be < 0
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		return tablesize(TableSizeType::TST_APPROXIMATED, _size-rhs._size);
	}else{
		return tablesize(TableSizeType::TST_EXACT, _size-rhs._size);
	}
}
tablesize tablesize::operator*(const tablesize& rhs) const{
	auto t = tablesize(_type, _size);
	t *= rhs;
	return t;
}
void tablesize::operator*=(const tablesize& rhs){
	// In this case the unknown check goes first because in theory
	// the unknown value could also be 0 and infinity * 0 != infinity
	// (This problem does not occur for + or -)
	if(rhs.isUnknown() || isUnknown()){
		_type = TableSizeType::TST_UNKNOWN;
		return;
	}
	if(rhs.isInfinite() || isInfinite()){
		_type = TableSizeType::TST_INFINITE;
		return;
	}

	// bool 'safe' indicates whether numerical operation was executed safely
	bool safe = true;
	auto result = _size;
	multiplication(_size,rhs._size,result,safe);
	if(safe) {
		if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
			_type = TableSizeType::TST_APPROXIMATED;
			_size = result;
		}else{
			_type = TableSizeType::TST_EXACT;
			_size = result;
		}
	}else{
		_type = TableSizeType::TST_INFINITE;
		return;
	}
}
tablesize tablesize::operator/(const tablesize& rhs) const{
	if(rhs.isUnknown()|| isUnknown() || rhs._size==0){
		return tablesize(TST_UNKNOWN, 0);
	}
	if(isInfinite()){
		Assert(not rhs.isInfinite());
		return tablesize(TST_INFINITE, 0);
	}
	if(rhs.isInfinite()){
		return tablesize(TST_EXACT,0);
	}
	if(rhs._type==TableSizeType::TST_APPROXIMATED || _type==TableSizeType::TST_APPROXIMATED){
		return tablesize(TableSizeType::TST_APPROXIMATED, _size/rhs._size);
	}else{
		return tablesize(TableSizeType::TST_EXACT, _size/rhs._size);
	}
}

double toDouble(const tablesize& val){
	if(val.isInfinite()){
		return getMaxElem<double>();
	}
	if(val.isUnknown()){
		return 0;
	}
	return val._size;
}

template<>
std::string toString(const tablesize& obj){
	std::stringstream ss;
	switch(obj._type){
	case TableSizeType::TST_APPROXIMATED:
		ss <<"approximated to " <<obj._size;
		break;
	case TableSizeType::TST_EXACT:
		ss <<obj._size;
		break;
	case TableSizeType::TST_INFINITE:
		ss <<"infinite";
		break;
	case TableSizeType::TST_UNKNOWN:
		ss <<"not exactly known";
		break;
	}
	return ss.str();
}
