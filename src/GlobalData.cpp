/****************************************************************
 * Copyright 2010-2012 Katholieke Universiteit Leuven
 *  
 * Use of this software is governed by the GNU LGPLv3.0 license
 * 
 * Written by Broes De Cat, Stef De Pooter, Johan Wittocx
 * and Bart Bogaerts, K.U.Leuven, Departement Computerwetenschappen,
 * Celestijnenlaan 200A, B-3001 Leuven, Belgium
 ****************************************************************/

#include "GlobalData.hpp"
#include "IncludeComponents.hpp"
#include "parser/clconst.hpp"
#include "options.hpp"
#include "internalargument.hpp"
#include "insert.hpp"

using namespace std;

GlobalData::GlobalData()
		: 	_globalNamespace(Namespace::createGlobal()),
			_inserter(new Insert(_globalNamespace)),
			_domainelemFactory(DomainElementFactory::createGlobal()),
			_idcounter(1),
			_options(new Options(false)),
			_tabsizestack() {
	shouldTerminate = false;
	_tabsizestack.push(0);
	_stdNamespace = new Namespace("stdspace", _globalNamespace, ParseInfo());
}

GlobalData::~GlobalData() {
	delete (_globalNamespace); // NOTE: also deletes stdnamespace as it is a child
	delete(_inserter);
	delete (_domainelemFactory);
	for (auto i = _openfiles.cbegin(); i != _openfiles.cend(); ++i) {
		fclose(*i);
	}
	for (auto m = _monitors.begin(); m != _monitors.end(); ++m) {
		delete (*m);
	}
	garbageCollectInternalArgumentVectors();
	DomElemContainer::deleteAllContainers();
	// Note: Options are handled by Lua's garbage collection.
}

//setoptions can only be called from the setoptionsinference! Otherwise, we could change the user-defined options
void GlobalData::setOptions(Options* options) {
	Assert(_options!=NULL);
	//TODO should the old options be deleted?
	_options = options;
}

GlobalData* _instance = NULL;
bool GlobalData::shouldTerminate = false;

GlobalData* GlobalData::instance() {
	if (_instance == NULL) {
		_instance = new GlobalData();
	}
	return _instance;
}

DomainElementFactory* GlobalData::getGlobalDomElemFactory() {
	return instance()->getDomElemFactory();
}
Namespace* GlobalData::getGlobalNamespace() {
	return instance()->getNamespace();
}

Namespace* GlobalData::getStdNamespace() {
	return instance()->getStd();
}

void GlobalData::close() {
	Assert(_instance!=NULL);
	delete (_instance);
	_instance = NULL;
}

FILE* GlobalData::openFile(const char* filename, const char* mode) {
	auto f = fopen(filename, mode);
	if (f == NULL) {
		return f;
		//throw IdpException("Could not open file.\n");
	}
	_openfiles.insert(f);
	return f;
}

void GlobalData::closeFile(FILE* filepointer) {
	Assert(filepointer!=NULL);
	_openfiles.erase(filepointer);
	fclose(filepointer);
}

void GlobalData::setTabSize(size_t size) {
	_tabsizestack.push(size);
}
void GlobalData::resetTabSize() {
	Assert(_tabsizestack.size() > 1);
	// Note: always leave the first element
	if (_tabsizestack.size() > 1) {
		_tabsizestack.pop();
	}
}
size_t GlobalData::getTabSize() const {
	Assert(not _tabsizestack.empty());
	return _tabsizestack.top();
}

GlobalData* getGlobal() {
	return GlobalData::instance();
}
