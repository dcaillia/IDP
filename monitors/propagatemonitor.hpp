#ifndef PROPAGATEMONITOR_HPP_
#define PROPAGATEMONITOR_HPP_

#include "monitors/tracemonitor.hpp"
#include "external/SearchMonitor.hpp"
#include "external/ExternalInterface.hpp"

class PropagateMonitor : public TraceMonitor {
	private:
		std::vector<MinisatID::Literal>	_partialmodel;
		MinisatID::SearchMonitor*		_solvermonitor;
	public:
		PropagateMonitor() {
			cb::Callback2<void, MinisatID::Literal, int> callbackprop(this, &PropagateMonitor::propagate);
			_solvermonitor = new MinisatID::SearchMonitor();
			_solvermonitor->setPropagateCB(callbackprop);
		}
		virtual ~PropagateMonitor() {
			delete(_solvermonitor);
		}

		void backtrack(int )								{ assert(false);							}
		void propagate(MinisatID::Literal lit, int )		{ _partialmodel.push_back(lit);				}
		std::string* index() const							{ assert(false); return 0;					}
		void setTranslator(GroundTranslator* )				{ assert(false);							}
		virtual void setSolver(MinisatID::WrappedPCSolver* solver){ solver->addMonitor(_solvermonitor);		}
		const std::vector<MinisatID::Literal>& model()		{ return _partialmodel;						}
};

#endif /* PROPAGATEMONITOR_HPP_ */
