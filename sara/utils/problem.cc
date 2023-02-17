// -*- C++ -*-

/*!
 * \file    problem.cc
 *
 * \brief   Class Problem
 *
 * \author  Harro Wimmel <harro.wimmel@uni-rostock.de>
 *
 * \since   2009/10/21
 *
 * \date    $Date: 2011-04-08 12:00:00 +0200 (Fr, 08. Apr 2011) $
 *
 * \version $Revision: 1.06 $
 */

#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <map>
#include <string>

#include "verbose.h"

#ifndef PNAPI_PNAPI_H
#include "pnapi/pnapi.h"
#endif
#include "problem.h"
//#include "cmdline.h"
#include "lp_solve_5.5/lp_lib.h"

using std::cerr;
using std::cout;
using std::endl;
using std::vector;
using std::map;
using std::set;
using std::stack;
using std::string;
using std::ifstream;
using std::ofstream;
using std::ostringstream;
using std::setw;

extern vector<Transition*> transitionorder;
extern vector<Place*> placeorder;
extern map<Transition*,int> revtorder;
extern map<Place*,int> revporder;

	/**************************************
	* Implementation of the class Problem *
	**************************************/

/** Standard constructor.
*/
Problem::Problem() : deinit(false),generalcover(false),type(REACHABLE),nettype(LOLA),pn(NULL) {}

/** Destructor.
*/
Problem::~Problem() { clear(); }

/** Deinitialization of a problem.
*/
void Problem::clear() {
	name = "";
	if (deinit) delete pn;
	deinit = false;
	pn = NULL;
	nettype = 0;
	filename = "";
	initial.clear();
	required.clear();
	cover.clear();
	type = 0;
	clhs.clear();
	ccomp.clear();
	crhs.clear();
}

/** Set the name of a problem.
	@param s The name.
*/
void Problem::setName(string s) { name = s; }

/** Set the goal of a problem.
	@param g The goal: REACHABILITY or REALIZABILITY.
*/
void Problem::setGoal(int g) { type = g; }

/** Set the filename for a problem.
	@param fn The filename of the Petri net file.
*/
void Problem::setFilename(string fn) { filename = fn; }

/** Get the filename for a problem.
	@return The filename of the Petri net file.
*/
string Problem::getFilename() const { return filename; }

/** Set the type of Petri net.
	@param nt LOLA or OWFN.
*/
void Problem::setNetType(int nt) { nettype = nt; }

/** Set the initial marking for a problem.
	@param s The name of a place in the Petri net file.
	@param i The number of tokens initially on place s.
*/
void Problem::setInit(string s, int i) { initial[s]=i; }

/** Set the final marking or transition vector for a problem.
	@param s The name of a place or transition in the Petri net file.
	@param f The number of tokens finally on place s or the number of times the transition must fire.
*/
void Problem::setFinal(string s, int f) { required[s]=f; }

/** Set the mode for a place in the final marking.
	@param s The name of a place in the Petri net file.
	@param mode EQ (reach exactly), GE (cover), or LE (bound).
*/
void Problem::setCover(string s, int mode) { cover[s]=mode; }

/** Set the mode for all places unmentioned in the final marking.
	@param cov Whether those places should have >=0 token (true) or ==0 tokens (false)
*/
void Problem::setGeneralCover(bool cov) { generalcover = cov; }

/** Get the name of a problem.
	@return The name.
*/
string Problem::getName() const { return name; }

/** Get the initial marking of a problem.
	@return The initial marking.
*/
Marking Problem::getInitialMarking() {
	if (pn==NULL) getPetriNet();
	if (initial.empty()) return Marking(*pn,false);
	map<const Place*,unsigned int> result;
	map<string,int>::iterator it;
	for(it=initial.begin(); it!=initial.end(); ++it)
	{
		const Place* p(pn->findPlace(it->first));
		if (p) result[p]=(unsigned int)it->second;
	}
	return Marking(result,pn);
}

/** Get the final marking of a problem.
	@return The final marking.
*/
Marking Problem::getFinalMarking() {
	if (pn==NULL) getPetriNet();
	map<const Place*,unsigned int> result;
	map<string,int>::iterator it;
	for(it=required.begin(); it!=required.end(); ++it)
	{
		Place* p(pn->findPlace(it->first));
		if (p) result[p]=(unsigned int)it->second;
	}
	return Marking(result,pn);
}

/** Get the modes of all places for the final marking of a problem.
	@return A map from places to modes (EQ,GE,LE).
*/
map<Place*,int> Problem::getCoverRequirement() {
	if (pn==NULL) getPetriNet();
	map<Place*,int> result;
	map<string,int>::iterator it;
	for(it=cover.begin(); it!=cover.end(); ++it)
	{
		Place* p(pn->findPlace(it->first));
		if (p) result[p]=it->second; 
	}
	if (generalcover)
	{
		set<Place*> pset(pn->getPlaces());
		set<Place*>::iterator pit;
		for(pit=pset.begin(); pit!=pset.end(); ++pit)
			if (required.find((*pit)->getName())==required.end())
				result[*pit]=GE;
	}
	return result;
}

/** Get the transition vector to realize for this problem.
	@return The vector.
*/
map<Transition*,int> Problem::getVectorToRealize() {
	if (pn==NULL) getPetriNet();
	map<Transition*,int> result;
	map<string,int>::iterator it;
	for(it=required.begin(); it!=required.end(); ++it)
	{
		Transition* t(pn->findTransition(it->first));
		if (t) result[t]=it->second;
	}
	return result;
}

/** Get the Petri net of a problem and calculate the global ordering of transitions and places.
	@return The Petri net.
*/
PetriNet* Problem::getPetriNet() {
	if (pn) return pn;

	// try to open file
	ifstream infile(filename.c_str(), ifstream::in);
	if (!infile.is_open()) abort(2,"error: could not read from Petri net file '%s'",filename.c_str());

	pn = new PetriNet();

	// try to parse net
	try {
	    switch (nettype) {
	        case(OWFN): {
				infile >> pnapi::io::owfn >> (*pn);
	            break;
	        }
	        case(LOLA): {
				infile >> pnapi::io::lola >> (*pn);
	            break;
	        }
	        case(PNML): {
				infile >> pnapi::io::pnml >> (*pn);
	            break;
	        }
	    }
	} catch (pnapi::exception::InputError error) {
	    infile.close();
		cerr << "sara: error: " << error << endl;
		abort(4,"error while reading Petri net from file");
	}

	infile.close();
	deinit = true;
//	if (!calcPTOrder() && args_info.verbose_given) status("place or transition ordering is non-deterministic");
	return pn;
}

/** Save the Petri net back to a file.
*/
void Problem::savePetriNet() {
	if (!pn) return;

	// try to open file
	ofstream outfile(filename.c_str(), ofstream::out);
	if (!outfile.is_open()) abort(7,"error: could not write to Petri net file '%s'",filename.c_str());

	// try to parse net
//	try {
	    switch (nettype) {
	        case(OWFN): {
				outfile << pnapi::io::owfn << (*pn);
	            break;
	        }
	        case(LOLA): {
				outfile << pnapi::io::lola << (*pn);
	            break;
	        }
	        case(PNML): {
				outfile << pnapi::io::pnml << (*pn);
	            break;
	        }
	    }
/*
	} catch (pnapi::exception::OutputError error) {
	    outfile.close();
		cerr << "sara: error: " << error << endl;
		abort(8,"error while writing Petri net to file");
	}
*/
	outfile.close();
	return;
}

/** Get the goal of a problem.
	@return REACHABILITY or REALIZABILITY.
*/
int Problem::getGoal() const {
	return type;
}

/** Checks whether this problem needs the same Petri net as the parameter problem.
	If so, advances the Petri net to this problem without reloading.
	@param p A problem with a loaded Petri net.
*/
void Problem::checkForNetReference(Problem& p) {
	if (filename==p.filename) { pn = p.pn; p.deinit = false; deinit = true; }
}

/** Calculate the global ordering of transitions and places for this problem.
	@return If the ordering is deterministic.
*/
bool Problem::calcPTOrder() {
	set<Transition*> tset(pn->getTransitions());
	set<Place*> pset(pn->getPlaces());
	set<Transition*>::iterator tit;
	set<Place*>::iterator pit;
	map<Transition*,int> tval;
	map<Place*,int> pval;
	map<int,set<Transition*> > tord,ttmp;
	map<int,set<Place*> > pord,ptmp;
	for(tit=tset.begin(); tit!=tset.end(); ++tit)
		tval[(*tit)] = 1;
	for(pit=pset.begin(); pit!=pset.end(); ++pit)
		pval[(*pit)] = 1;
	tord[0] = tset;
	pord[0] = pset;
	map<int,set<pnapi::Transition*> >::iterator tmit;
	map<int,set<pnapi::Place*> >::iterator pmit;
	set<pnapi::Arc*>::iterator ait;
	int min;
	for(int i=0; i<6; ++i) // Probably five rounds are enough to make the order deterministic, but we can't be sure!
	{
		min = 0;
		for(tmit=tord.begin(); tmit!=tord.end(); ++tmit)
		{
			int minval = 0;
			for(tit=tmit->second.begin(); tit!=tmit->second.end(); ++tit)
			{
				int val = 0;
				for(ait=(*tit)->getPresetArcs().begin(); ait!=(*tit)->getPresetArcs().end(); ++ait)
					val += 2*pval[&((*ait)->getPlace())]*(*ait)->getWeight();
				for(ait=(*tit)->getPostsetArcs().begin(); ait!=(*tit)->getPostsetArcs().end(); ++ait)
					val -= pval[&((*ait)->getPlace())]*(*ait)->getWeight();
				tval[(*tit)] = val;
				if (val<minval) minval=val;
			}
			for(tit=tmit->second.begin(); tit!=tmit->second.end(); ++tit)
			{
				tval[(*tit)] -= minval;
				ttmp[min+tval[(*tit)]].insert(*tit);
			}
			min = ttmp.rbegin()->first + 1;	
		}
		tord.clear();
		for(tmit=ttmp.begin(),min=1; tmit!=ttmp.end(); ++tmit,++min)
		{
			tord[min]=tmit->second;
			for(tit=tmit->second.begin(); tit!=tmit->second.end(); ++tit)
				tval[(*tit)]=min;
		}
		ttmp.clear();
		min = 0;
		for(pmit=pord.begin(); pmit!=pord.end(); ++pmit)
		{
			int minval = 0;
			for(pit=pmit->second.begin(); pit!=pmit->second.end(); ++pit)
			{
				int val = 0;
				for(ait=(*pit)->getPresetArcs().begin(); ait!=(*pit)->getPresetArcs().end(); ++ait)
					val += tval[&((*ait)->getTransition())]*(*ait)->getWeight();
				for(ait=(*pit)->getPostsetArcs().begin(); ait!=(*pit)->getPostsetArcs().end(); ++ait)
					val -= 2*tval[&((*ait)->getTransition())]*(*ait)->getWeight();
				pval[(*pit)] = val;
				if (val<minval) minval=val;
			}
			for(pit=pmit->second.begin(); pit!=pmit->second.end(); ++pit)
			{
				pval[(*pit)] -= minval;
				ptmp[min+pval[(*pit)]].insert(*pit);
			}
			min = ptmp.rbegin()->first + 1;	
		}
		pord.clear();
		for(pmit=ptmp.begin(),min=1; pmit!=ptmp.end(); ++pmit,++min)
		{
			pord[min]=pmit->second;
			for(pit=pmit->second.begin(); pit!=pmit->second.end(); ++pit)
				pval[(*pit)] = min;
		}
		ptmp.clear();
	}

	// push the result into the global vectors
	bool deterministic(true);
	transitionorder.clear();
	for(tmit=tord.begin(); tmit!=tord.end(); ++tmit)
	{
		if (tmit->second.size()>1) deterministic=false;
		for(tit=tmit->second.begin(); tit!=tmit->second.end(); ++tit)
			transitionorder.push_back(*tit);
	}
	placeorder.clear();
	for(pmit=pord.begin(); pmit!=pord.end(); ++pmit)
	{
		if (pmit->second.size()>1) deterministic=false;
		for(pit=pmit->second.begin(); pit!=pmit->second.end(); ++pit)
			placeorder.push_back(*pit); 
	}
	revtorder.clear();
	for(unsigned int i=0; i<transitionorder.size(); ++i)
		revtorder[transitionorder[i]]=i;
	revporder.clear();
	for(unsigned int i=0; i<placeorder.size(); ++i)
		revporder[placeorder[i]]=i;
	return deterministic;
}

/** Add an external (complex) constraint. This constraint is a weighted sum of places and transitions
	compared to a number.
	@param lhs The weighted sum of places and transitions, mapped by place/transition names.
	@param comp The comparison operator as in lp_solve: GE, EQ, or LE.
	@param rhs The right hand side of the comparison.
*/
void Problem::addConstraint(map<string,int> lhs, int comp, int rhs) {
	clhs.push_back(lhs);
	ccomp.push_back(comp);
	crhs.push_back(rhs);
}

/** Get an external constraint. The constraint is translated into an (in)equation over transitions.
	@param pos The number of the constraint.
	@param lhs The left hand side of the constraint for use with lp_solve.
	@param comp The comparator of the constraint for use with lp_solve.
	@param rhs The right hand side of the constraint for use with lp_solve.
*/
void Problem::getConstraint(unsigned int pos, map<Transition*,int>& lhs, int& comp, int& rhs) {
	if (pn==NULL) getPetriNet();
	lhs.clear();
	if (pos>=clhs.size()) { comp=GE; rhs=0; return; }
	comp = ccomp[pos];
	rhs = crhs[pos];
	// calculate the left hand side
	Marking m0(getInitialMarking());
	map<string,int>::iterator it;
	for(it=clhs[pos].begin(); it!=clhs[pos].end(); ++it)
	{
		Transition* t(pn->findTransition(it->first));
		if (t) lhs[t]+=it->second; // if it's a transition, just add the factor
		Place* p(pn->findPlace(it->first));
		if (p) { // if it's a place we must add a line of the marking equation ...
			set<pnapi::Arc*>::iterator ait;
			set<pnapi::Arc*> arcs = p->getPresetArcs(); 
			for(ait=arcs.begin(); ait!=arcs.end(); ++ait)
			{
				Transition* t = &((*ait)->getTransition());
				lhs[t] += (*ait)->getWeight();
			}
			arcs = p->getPostsetArcs();
			for(ait=arcs.begin(); ait!=arcs.end(); ++ait)
			{
				Transition* t = &((*ait)->getTransition());
				lhs[t] -= (*ait)->getWeight();
			}
			rhs -= m0[*p];
		}
		if (!t && !p) status("warning: global constraint contains unknown object '%s'.",it->first.c_str());
	}
	return;
}

/** Get the overall number of constraints available.
	@return The number of constraints.
*/
unsigned int Problem::getNumberOfConstraints() {
	return clhs.size();
}

/** Print the transition vector to be realized. */
void Problem::showTVector() {
	for(unsigned int i=0; i<transitionorder.size(); ++i)
		if (required.find(transitionorder[i]->getName())!=required.end())
			cout << transitionorder[i]->getName() << ":" << required[transitionorder[i]->getName()] << " ";
}

/** Print the initial marking. */
void Problem::showInitial() {
	for(unsigned int i=0; i<placeorder.size(); ++i)
		if (initial.find(placeorder[i]->getName())!=initial.end())
		{
			cout << placeorder[i]->getName();
			if (initial[placeorder[i]->getName()]!=1) 
				cout << ":" << initial[placeorder[i]->getName()];
			cout << " ";
		}
}

/** Print the final marking. */
void Problem::showFinal() {
	for(unsigned int i=0; i<placeorder.size(); ++i)
		if (required.find(placeorder[i]->getName())!=required.end())
		{
			cout << placeorder[i]->getName(); 
			if (cover.find(placeorder[i]->getName())==cover.end()) 
			{
				if (required[placeorder[i]->getName()]!=1) 
					cout << ":" << required[placeorder[i]->getName()];
				cout << " ";
			} else {
				switch(cover[placeorder[i]->getName()]) {
					case LE: cout << "<="; break;
					case EQ: cout << ":"; break;
					case GE: cout << ">="; break;
				}
				cout << required[placeorder[i]->getName()] << " ";
			}
		}
	if (generalcover) cout << endl << "sara: - Unmentioned places may have any number of tokens in the final marking";
}

/** Print the global constraints. */
void Problem::showConstraints() {
	for(unsigned int i=0; i<clhs.size(); ++i)
	{
		map<Transition*,int> lhs;
		int comp,rhs;
		getConstraint(i,lhs,comp,rhs);
		map<Transition*,int>::iterator mit;
		for(mit=lhs.begin(); mit!=lhs.end(); ++mit)
		{
			if (mit!=lhs.begin() && mit->second>0) cout << "+";
			cout << mit->second << mit->first->getName();
		}
		switch(comp) {
			case LE: cout << "<="; break;
			case EQ: cout << "="; break;
			case GE: cout << ">="; break;
		}
		cout << rhs << " ";
	}
}

