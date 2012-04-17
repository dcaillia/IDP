/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "Universe.hpp"
#include "StructureComponents.hpp"

bool Universe::empty() const {
	for (auto it = _tables.cbegin(); it != _tables.cend(); ++it) {
		if ((*it)->empty()) {
			return true;
		}
	}
	return false;
}

bool Universe::finite() const {
	if (empty()) {
		return true;
	}
	for (auto it = _tables.cbegin(); it != _tables.cend(); ++it) {
		if (not (*it)->finite()) {
			return false;
		}
	}
	return true;
}

bool Universe::approxEmpty() const {
	for (auto it = _tables.cbegin(); it != _tables.cend(); ++it) {
		if ((*it)->approxEmpty()) {
			return true;
		}
	}
	return false;
}

bool Universe::approxFinite() const {
	if (approxEmpty()) {
		return true;
	}
	for (auto it = _tables.cbegin(); it != _tables.cend(); ++it) {
		if (not (*it)->approxFinite()) {
			return false;
		}
	}
	return true;
}

tablesize Universe::size() const {
	size_t currsize = 1;
	TableSizeType tst = TST_EXACT;
	for (auto it = _tables.cbegin(); it != _tables.cend(); ++it) {
		tablesize ts = (*it)->size();
		switch (ts._type) {
		case TST_UNKNOWN:
			return tablesize(TST_UNKNOWN, 0);
		case TST_EXACT:
			currsize = currsize * ts._size;
			break;
		case TST_APPROXIMATED:
			currsize = currsize * ts._size;
			tst = TST_APPROXIMATED;
			break;
		case TST_INFINITE:
			return tablesize(TST_INFINITE, 0);
		}
	}
	return tablesize(tst, currsize);
}

bool Universe::contains(const ElementTuple& tuple) const {
	for (size_t n = 0; n < tuple.size(); ++n) {
		if (not _tables[n]->contains(tuple[n])) {
			return false;
		}
	}
	return true;
}