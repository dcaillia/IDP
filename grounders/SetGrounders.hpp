/************************************
 SetGrounders.hpp
 this file belongs to GidL 2.0
 (c) K.U.Leuven
 ************************************/

#ifndef SETGROUNDERS_HPP_
#define SETGROUNDERS_HPP_

#include "ground.hpp"

/*** Set grounders ***/

class SetGrounder {
	protected:
		GroundTranslator*	_translator;
	public:
		SetGrounder(GroundTranslator* gt) : _translator(gt) { }
		virtual ~SetGrounder() { }
		virtual int run() const = 0;
};

class QuantSetGrounder : public SetGrounder {
	private:
		FormulaGrounder*	_subgrounder;
		InstGenerator*		_generator;
		TermGrounder*		_weightgrounder;
	public:
		QuantSetGrounder(GroundTranslator* gt, FormulaGrounder* gr, InstGenerator* ig, TermGrounder* w) :
			SetGrounder(gt), _subgrounder(gr), _generator(ig), _weightgrounder(w) { }
		int run() const;
};

class EnumSetGrounder : public SetGrounder {
	private:
		std::vector<FormulaGrounder*>	_subgrounders;
		std::vector<TermGrounder*>		_subtermgrounders;
	public:
		EnumSetGrounder(GroundTranslator* gt, const std::vector<FormulaGrounder*>& subgr, const std::vector<TermGrounder*>& subtgr) :
			SetGrounder(gt), _subgrounders(subgr), _subtermgrounders(subtgr) { }
		int run() const;
};

#endif /* SETGROUNDERS_HPP_ */
