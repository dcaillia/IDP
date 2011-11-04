/************************************
	SimpleLookupGenerator.hpp
	this file belongs to GidL 2.0
	(c) K.U.Leuven
************************************/

#ifndef SIMPLELOOKUPGENERATOR_HPP_
#define SIMPLELOOKUPGENERATOR_HPP_

#include "generators/InstGenerator.hpp"

/**
 * A generator which checks whether a fully instantiated list of variables is a valid tuple for a certain predicate
 */
class LookupGenerator : public InstGenerator {
private:
	const PredTable*						_table;
	std::vector<const DomElemContainer*>	_vars;
	Universe								_universe; // FIXME waarvoor is dit universe nu juist nodig?

	bool _reset;

public:
	LookupGenerator(const PredTable* t, const std::vector<const DomElemContainer*> vars, const Universe& univ)
			:_table(t), _vars(vars), _universe(univ), _reset(true) {
		assert(t->arity()==vars.size());
	}

	void reset(){
		_reset = true;
	}

	void next(){
		if(_reset){
			_reset = false;
			std::vector<const DomainElement*> _currargs;
			for(auto i=_vars.begin(); i<_vars.end(); ++i){
				_currargs.push_back((*i)->get());
			}
			bool allowedvalue = (_table->contains(_currargs) && _universe.contains(_currargs));
			if(not allowedvalue){
				notifyAtEnd();
			}
		}else{
			notifyAtEnd();
		}
	}
};

#endif /* SIMPLELOOKUPGENERATOR_HPP_ */