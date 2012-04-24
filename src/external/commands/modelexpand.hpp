/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
****************************************************************/

#ifndef MODELEXPAND_HPP_
#define MODELEXPAND_HPP_

#include <iostream>
#include "commandinterface.hpp"

#include "inferences/modelexpansion/ModelExpansion.hpp"
#include "inferences/modelexpansion/LuaTraceMonitor.hpp"
#include "lua/luaconnection.hpp"

typedef TypedInference<LIST(AbstractTheory*, AbstractStructure*)> ModelExpandInferenceBase;
class ModelExpandInference: public ModelExpandInferenceBase {
public:
	ModelExpandInference()
			: ModelExpandInferenceBase("modelexpand",
					"Return a list containing all 2-valued models of the theory which are more precise than the given structure.", false) {
		setNameSpace(getInternalNamespaceName());
	}

	// TODO trace is returned as the SECOND return value of the lua call
	InternalArgument execute(const std::vector<InternalArgument>& args) const {
		LuaTraceMonitor* tracer = NULL;
		if (getOption(BoolType::TRACE)) {
			tracer = LuaConnection::getLuaTraceMonitor();
		}
		auto models = ModelExpansion::doModelExpansion(get<0>(args), get<1>(args), tracer);

		// Convert to internal arguments
		InternalArgument result;
		result._type = AT_TABLE;
		result._value._table = new std::vector<InternalArgument>();
		addToGarbageCollection(result._value._table);
		for (auto it = models.cbegin(); it != models.cend(); ++it) {
			result._value._table->push_back(InternalArgument(*it));
		}

		if (tracer != NULL) {
			InternalArgument randt;
			randt._type = AT_MULT;
			randt._value._table = new std::vector<InternalArgument>(1, result);
			addToGarbageCollection(randt._value._table);
			InternalArgument trace;
			trace._type = AT_REGISTRY;
			trace._value._string = tracer->index();
			randt._value._table->push_back(trace);
			result = randt;
			delete (tracer);
		}

		return result;
	}
};

#endif
