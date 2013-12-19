/*****************************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat, Bart Bogaerts, Stef De Pooter, Johan Wittocx,
 * Jo Devriendt, Joachim Jansen and Pieter Van Hertum
 * K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************************/

#pragma once

#include <unordered_map>

#include "commontypes.hpp"

class PrologTerm;
class DomainElement;
class PFSymbol;
class Sort;

class XSBToIDPTranslator {

private:
	std::unordered_map<std::string, const DomainElement*> _domainels;
	std::unordered_map<std::string, std::string> _termnames;

	std::string transform_into_term_name(std::string);

public:
	bool isoperator(int c);

	std::string to_prolog_term(const PFSymbol*);
	std::string to_idp_pfsymbol(std::string);
	std::string to_prolog_pred_and_arity(const PFSymbol*);
	std::string to_prolog_pred_and_arity(const Sort*);

	std::string to_prolog_term(const DomainElement*);
	const DomainElement* to_idp_domelem(std::string);

	std::string to_prolog_term(CompType);



	std::string to_prolog_varname(std::string);
	std::string to_prolog_sortname(std::string);
//	std::string to_prolog_termname(std::string);
	static std::string to_simple_chars(std::string);

	static std::string get_idp_prefix();
	static std::string get_idp_caps_prefix();
};
