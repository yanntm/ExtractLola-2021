#include <cstdio>
#include <cstdlib>
#include <set>
#include <map>
#include <algorithm>

#ifndef PNAPI_PNAPI_H
#include "pnapi/pnapi.h"
#endif
#include "lp_solve_5.5/lp_lib.h"
#include "verbose.h"

#ifndef LPWRAPPER_H
#include "lpwrapper.h"
#endif

#ifndef PARTIALSOLUTION_H
#include "partialsolution.h"
#endif

#ifndef CONSTRAINT_H
#include "constraint.h"
#endif

using std::cerr;
using std::cout;
using std::endl;
using std::set;
using std::map;
using pnapi::PetriNet;
using pnapi::Place;
using pnapi::Transition;
//using pnapi::Arc;
using pnapi::Marking;

namespace sara {
// global place and transition orderings imported from main.cc
extern vector<Transition*> transitionorder;
extern vector<Place*> placeorder;
extern map<const Transition*,int> revtorder;
extern pthread_mutex_t print_mutex;
}

	/*************************************
	* Class LPWrapper method definitions *
	*************************************/

namespace sara {

/** Constructor for systems with a given number of variables (transitions).
	@param columns The number of transitions in the net (equals the numbers
		of columns in the linear system).
*/
LPWrapper::LPWrapper(unsigned int columns) : verbose(0),cols(columns),basicrows(0) {
	lp = make_lp(0,cols);
	if (!lp) abort(12,"error: could not create LP model");
}

/** Deep copy constructor for equation systems.
	@param lpw The LPWrapper object to copy.
*/
LPWrapper::LPWrapper(const LPWrapper& lpw) {
	verbose = lpw.verbose;
	cols = lpw.cols;
	basicrows = lpw.basicrows;
	lp = copy_lp(lpw.lp);
	if (!lp) abort(12,"error: could not create LP model");
	tpos = lpw.tpos;
	tvector = lpw.tvector;
}

/** Destructor.
*/
LPWrapper::~LPWrapper() {
	delete_lp(lp);
}

/** Create the marking equation for lp_solve.
	@param m1 The initial marking.
	@param m2 The final marking.
	@param cover Information on which token numbers must be reached exactly and which only covered.
	@param pb An instance of the problem to solve, for getting the global constraints.
	@param verbose If TRUE prints information on cout.
	@return The number of equations on success, -1 otherwise.
*/
int LPWrapper::createMEquation(Marking& m1, Marking& m2, map<Place*,int>& cover, Problem* pb, bool verbose) {
	tpos.clear(); // probably not necessary
	tvector.clear();

	// set the column variables in lp_solve according to the global transition ordering
	int colnr=0;
	for(unsigned int o=0; o<transitionorder.size(); ++o,++colnr)
	{
		Transition* t(transitionorder[o]); // get the transitions according to the global ordering 
		set_col_name(lp,colnr+1,const_cast<char*>(t->getName().c_str()));
		set_int(lp,colnr+1,1); // declare all variables to be integer
		tpos[t] = colnr; // remember the ordering of the transitions
	}
	if (colnr!=(int)(cols)) cerr << "sara: LPWrapper error: column number mismatch" << endl;

	//lp_solve objective: minimum firing sequence length
	int *colpoint = new int[cols];
	REAL *mat = new REAL[cols];
	for(unsigned int y=0; y<cols; y++)
	{
		mat[y]=1;
		colpoint[y]=y+1;
	}
	set_add_rowmode(lp,TRUE); // go to rowmode (faster)
	if (!set_obj_fnex(lp,cols,mat,colpoint)) { delete[] colpoint; delete[] mat; return -1; }
	set_minim(lp);

	//create incidence matrix by adding rows to lp_solve
	for(unsigned int k=0; k<placeorder.size(); ++k)
	{
		for(unsigned int y=0; y<cols; ++y) mat[y]=0;
		set<pnapi::Arc*>::iterator ait;
		set<pnapi::Arc*> arcs = placeorder[k]->getPresetArcs(); 
		for(ait=arcs.begin(); ait!=arcs.end(); ++ait)
		{
			Transition* t = &((*ait)->getTransition());
			mat[tpos[t]] += (*ait)->getWeight();
		}
		arcs = placeorder[k]->getPostsetArcs();
		for(ait=arcs.begin(); ait!=arcs.end(); ++ait)
		{
			Transition* t = &((*ait)->getTransition());
			mat[tpos[t]] -= (*ait)->getWeight();
		}
		int mark = m2[*(placeorder[k])]-m1[*(placeorder[k])]; // calculate right hand side
		int comp = (cover.find(placeorder[k])==cover.end() ? EQ : cover[placeorder[k]]); // reach or cover this place in the final marking?
		//initialize the rows
		if (!add_constraintex(lp,cols,mat,colpoint,comp,mark)) { delete[] colpoint; delete[] mat; return -1; }
	}

	//allow only nonnegative solutions
	for(int y=1; y<=(int)(cols); ++y)
		set_lowbo(lp,y,0);

	// now add the global constraints from the problem file, if a problem file is given
	unsigned int cnr(0);
	if (pb) {
		cnr = pb->getNumberOfConstraints();
		for(unsigned int i=0; i<cnr; ++i)
		{
			for(unsigned int y=0; y<cols; ++y) mat[y]=0;
			map<Transition*,int> line;
			int comp, rhs;
			pb->getConstraint(i,line,comp,rhs);
			map<Transition*,int>::iterator lit;
			for(lit=line.begin(); lit!=line.end(); ++lit)
			{
				mat[tpos[lit->first]] = lit->second;	
			}
			if (!add_constraintex(lp,cols,mat,colpoint,comp,rhs)) { delete[] colpoint; delete[] mat; return -1; }
		}
	}

	set_add_rowmode(lp,FALSE);	
//	write_LP(lp,stdout);
	if (verbose) write_LP(lp,stdout);
	else set_verbose(lp,CRITICAL);
	basicrows = placeorder.size()+cnr;
	delete[] colpoint;
	delete[] mat;
	return (int)(basicrows);
}

/** Remove all constraints not belonging to the original marking
	equation as created by createMEquation().
	@return TRUE on success.
*/
bool LPWrapper::stripConstraints() {
	return resize_lp(lp,basicrows,cols);
}

/** Forward a constraint to the internal data structure lprec.
	@param row A constraint.
	@param constr_type LE, EQ oder GE.
	@param rhs The right-hand-side of the constraint.
	@return If adding the constraint succeeded.
*/
bool LPWrapper::addConstraint(REAL* row, int constr_type, REAL rhs) {
	return add_constraint(lp,row,constr_type,rhs);
}

/** Wrapper for solving the system.
	@return The result of solve(lp).
*/
int LPWrapper::solveSystem() {
//	write_LP(lp,stdout);
	return solve(lp);
}

/** Get the name of a transition belonging to a column, wrapper for get_col_name.
	@param col Column number.
	@return The result of get_col_name(lp,col).
*/
char* LPWrapper::getColName(int col) {
	return get_col_name(lp,col);
}

/** Wrapper for get_variables
	@param solution Array of REAL with at least number of transitions elements
		to contain a solution of lp_solve. Will be filled by the method.
	@return TRUE if successful.
*/
unsigned char LPWrapper::getVariables(REAL* solution) {
	return get_variables(lp,solution);
}

/** Add the constraints of a partial solution to the lp model.
	@param ps The partial solution containing the constraints.
	@return TRUE if successful.
*/
bool LPWrapper::addConstraints(PartialSolution& ps) {
	stripConstraints();
	// add new constraints to lp model
	REAL *constraint = new REAL[cols+1];
	for(unsigned int i=0; i<=cols; ++i) constraint[i]=0;
	set<Constraint>::iterator cit;
	for(cit=ps.getConstraints().begin(); cit!=ps.getConstraints().end(); ++cit)
	{
		bool success = cit->calcConstraint(constraint,tpos,cols);
		int rhs = cit->getRHS();
		bool jump = cit->isJump();
		if (verbose>1) {
			pthread_mutex_lock(&print_mutex);
			cerr << "LPWRAPPER: Constraint: ";
			for(unsigned int j=1; j<=cols; ++j) if (constraint[j]>0) cerr << constraint[j] << getColName(j) << " ";
			if (jump) cerr << " <= " << rhs << " for ";
			else      cerr << " >= " << rhs << " for ";
			cit->showConstraint(cerr);
			cerr << endl;
			pthread_mutex_unlock(&print_mutex);
		}
		// add the constraint to the lp model, normal constraints with >=, jumps with <=
		if (success && (jump || rhs>0)) {
			if (!addConstraint(constraint,(jump?LE:GE),rhs)) 
			{ 
			  delete[] constraint; 
			  abort(13,"error: failed to add constraint to LP model");
			  return false; // never reached, prevents memory leak detection
			}
		}
	}
	delete[] constraint;
	return true;
}

/** Return the solution of lp_solve as a map from transitions to multiplicities.
	@param pn The Petri net containing the transitions
	@return The solution map
*/
map<Transition*,int>& LPWrapper::getTVector(PetriNet& pn) {
	REAL *sol = new REAL[cols];
	getVariables(sol);
	for(int y=0; y<(int)(cols); ++y)
		tvector[pn.findTransition(getColName(y+1))] = static_cast<int>(sol[y]);
	delete[] sol;
	return tvector;
}

/** Find the smallest possible additions to initial and final marking such that
	an infeasible marking equation becomes solvable. Print such a solution.
	@return If this worked.
*/
bool LPWrapper::findNearest() {
	if (!stripConstraints()) return false; // first get the naked marking equation
	REAL val;
	for(unsigned int k=1; k<=placeorder.size(); ++k)
	{
		int kcopy = (int)(k);
		val = 1.0; // for each place add a positive
		if (!add_columnex(lp,1,&val,&kcopy)) return false;
		val = -1.0; // and a negative variable
		if (!add_columnex(lp,1,&val,&kcopy)) return false;
	}
	set_add_rowmode(lp,true);
	int *colpoint = new int[2*placeorder.size()];
	REAL *mat = new REAL[2*placeorder.size()];
	for(unsigned int k=0; k<2*placeorder.size(); ++k)
	{
		mat[k]=1;
		colpoint[k]=k+cols+1; // column of a new variable; for the objective (minimizing)
		set_int(lp,colpoint[k],1); // make the new variables integers
	}
	set_add_rowmode(lp,TRUE); // go to rowmode (faster)
	if (!set_obj_fnex(lp,2*placeorder.size(),mat,colpoint)) { delete[] mat; delete[] colpoint; return false; }
	set_minim(lp); // and set the new objective: minimize the sum of all new variables
	delete[] mat;
	delete[] colpoint;

	val = 1.0; // add lower bound of zero for all new variables (set_lowbo doesn't work)
	for(unsigned int k=cols+1; k<cols+1+2*placeorder.size(); ++k)
	{
		int kcopy = (int)(k);
		if (!add_constraintex(lp,1,&val,&kcopy,GE,0)) return false;
	}
	set_add_rowmode(lp,false);
	int ret = solveSystem(); // solve the new equation with the new slack variables
	if (ret==0 || ret==1) // we have found a solution
	{
		REAL *sol = new REAL[cols+1+2*placeorder.size()]; // container for the solution
		getVariables(sol); // get the solution of the new system
		bool in = false; // need to change the initial marking?
		bool out = false; // need to change the final marking?
		for(unsigned int y=cols; y<cols+2*placeorder.size(); y+=2)
		{
			if (sol[y]>0) in = true; // positive variables mean we must increase the initial marking
			if (sol[y+1]>0) out = true; // negative variables mean increasing the final marking
		}
		cout << "You might try to add";
		bool comma = false;
		if (in) for(unsigned int y=cols; y<cols+2*placeorder.size(); y+=2)
			if (sol[y]>0) { // print the change needed for the initial marking
				if (comma) cout << ","; else cout << " ";
				cout << placeorder[(y-cols)/2]->getName() << ":" << sol[y];
				comma = true;
			}
		if (in) cout << " tokens to the initial marking";
		if (in && out) cout << " and";
		comma = false;
		if (out) for(unsigned int y=cols; y<cols+2*placeorder.size(); y+=2)
			if (sol[y+1]>0) { // print the change needed for the final marking
				if (comma) cout << ","; else cout << " ";
				cout << placeorder[(y-cols)/2]->getName() << ":" << sol[y+1];
				comma = true;
			}
		if (out) cout << " tokens to the final marking";
		cout << "." << endl;
		delete[] sol;
		return true; // it worked, we are done.
	} else return false;
}


} // end namespace sara
