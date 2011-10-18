/*
 * Copyright 2007-2011 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv3.0 license
 *
 * Written by Broes De Cat and Maarten Mariën, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */
#ifndef GENUTILS_HPP_
#define GENUTILS_HPP_

#include <vector>
#include <map>
#include <exception>
#include <fstream>

#ifdef __GXX_EXPERIMENTAL_CXX0X__
	#include <memory>
	template<class T>
	struct sharedptr{
		typedef std::shared_ptr<T> ptr;
	};
#else
	#include <tr1/memory>
	template<class T>
	struct sharedptr{
		typedef std::tr1::shared_ptr<T> ptr;
	};
#endif

// Support for deleting lists of pointer elements
template<class T>
void deleteList(std::vector<T*>& l){
	for(auto i=l.begin(); i!=l.end(); ++i){
		if(*i!=NULL){
			delete(*i);
		}
	}
	l.clear();
}

template<class T>
void deleteList(std::vector<std::vector<T*> >& l){
	for(auto i=l.begin(); i!=l.end(); ++i){
		deleteList(*i);
	}
	l.clear();
}

template<class T, class K>
void deleteList(std::map<K, T*>& l){
	for(auto i=l.begin(); i!=l.end(); ++i){
		if((*i).second!=NULL){
			delete((*i).second);
		}
	}
	l.clear();
}

template<class T, class K>
void deleteList(std::map<K, std::map<K, std::vector<T*> > >& l){
	for(auto i=l.begin(); i!=l.end(); ++i){
		for(auto j=(*i).second.begin(); j!=(*i).second.end(); ++j){
			for(auto k=(*j).second.begin(); k!=(*j).second.end(); ++k){
				if((*k).second!=NULL){
					delete((*k).second);
				}
			}
		}
	}
	l.clear();
}

template<class List, class Elem>
bool contains(const List& l, const Elem& e){
	return l.find(e)!=l.end();
}

template<class T>
bool fileIsReadable(T* filename) { //quick and dirty
	std::ifstream f(filename, std::ios::in);
	bool exists = f.is_open();
	if(exists){
		f.close();
	}
	return exists;
}

#endif /* GENUTILS_HPP_ */