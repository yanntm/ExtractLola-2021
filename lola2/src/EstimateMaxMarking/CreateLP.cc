#include <iostream>
#include <fstream>

#include <Core/Runtime.h>
#include <EstimateMaxMarking/CreateLP.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>
#include <InputOutput/InputOutput.h>
#include <Net/Net.h>
#include <Net/LinearAlgebra.h>
#include <Net/Marking.h>
#include <Planning/ComputeBoundTask.h>

#include "../../libs/lp_solve_5.5/lp_lib.h"

using namespace std;

#define num_places_target_fct  2

void CreateLP::createLP()
{
	int coefficients[num_places_target_fct] = {4,3};
	int places_indices[num_places_target_fct] = {1,2};
	std::set<int> used_x_vars;

	ofstream lp_file;
	lp_file.open("EstimateMaxMarking/lp.txt");
	lp_file<<"max: ";
	for (int i=0; i<num_places_target_fct-1; i++){
		lp_file<<coefficients[i]<<"m"<<i<<"+";
	}
	lp_file<<coefficients[num_places_target_fct-1]<<"m"<<num_places_target_fct-1<<";"<<std::endl;
	
	for (int i=0; i<Net::Card[PL]; i++){
		int num_tin = Net::CardArcs[PL][PRE][i];
		lp_file<<"m"<<i<<"=";
		for (int j=0; j<num_tin; j++){
			int tin = Net::Arc[PL][PRE][i][j];
			used_x_vars.insert(tin);
			int mult = Net::Mult[PL][PRE][i][j];
			if (mult == 1)
				lp_file<<"+"<<"x"<<tin;
			else
				lp_file<<"+"<<mult<<"x"<<tin;
		}
		int num_tout = Net::CardArcs[PL][POST][i];
		for (int j=0; j<num_tout; j++){
			int tout = Net::Arc[PL][POST][i][j];
			used_x_vars.insert(tout);
			int mult = Net::Mult[PL][POST][i][j];
			if (mult == 1)
				lp_file<<"-"<<"x"<<tout;
			else
				lp_file<<"-"<<mult<<"x"<<tout;
		}
		lp_file<<"+"<<Marking::Initial[i]<<";"<<endl;
		lp_file<<"m"<<i<<">=0"<<";"<<endl;
	}
	for (std::set<int>::iterator it=used_x_vars.begin(); it!=used_x_vars.end(); ++it)
	    	lp_file<<"int x"<<*it<<";"<<endl;
	lp_file.close();
	
	FILE *fp;
	fp = popen("lp_solve EstimateMaxMarking/lp.txt > EstimateMaxMarking/lpResult.txt", "r");
	pclose(fp);
}


void CreateLP::demo()
{
	
  lprec *lp;
  int Ncol, *colno = NULL, j, ret = 0;
  REAL *row = NULL;

  /* We will build the model row by row
     So we start with creating a model with 0 rows and 2 columns */
  Ncol = 2; /* there are two variables in the model */
  lp = make_lp(0, Ncol);
  if(lp == NULL)
    ret = 1; /* couldn't construct a new model... */

  if(ret == 0) {
    /* let us name our variables. Not required, but can be useful for debugging */
    set_col_name(lp, 1, deconst("x"));
    set_col_name(lp, 2, deconst("y"));

    /* create space large enough for one row */
    colno = (int *) malloc(Ncol * sizeof(*colno));
    row = (REAL *) malloc(Ncol * sizeof(*row));
    if((colno == NULL) || (row == NULL))
      ret = 2;
  }

  if(ret == 0) {
    set_add_rowmode(lp, TRUE);  /* makes building the model faster if it is done rows by row */

    /* construct first row (120 x + 210 y <= 15000) */
    j = 0;

    colno[j] = 1; /* first column */
    row[j++] = 120;

    colno[j] = 2; /* second column */
    row[j++] = 210;

    /* add the row to lpsolve */
    if(!add_constraintex(lp, j, row, colno, LE, 15000))
      ret = 3;
  }

  if(ret == 0) {
    /* construct second row (110 x + 30 y <= 4000) */
    j = 0;

    colno[j] = 1; /* first column */
    row[j++] = 110;

    colno[j] = 2; /* second column */
    row[j++] = 30;

    /* add the row to lpsolve */
    if(!add_constraintex(lp, j, row, colno, LE, 4000))
      ret = 3;
  }

  if(ret == 0) {
    /* construct third row (x + y <= 75) */
    j = 0;

    colno[j] = 1; /* first column */
    row[j++] = 1;

    colno[j] = 2; /* second column */
    row[j++] = 1;

    /* add the row to lpsolve */
    if(!add_constraintex(lp, j, row, colno, LE, 75))
      ret = 3;
  }

  if(ret == 0) {
    set_add_rowmode(lp, FALSE); /* rowmode should be turned off again when done building the model */

    /* set the objective function (143 x + 60 y) */
    j = 0;

    colno[j] = 1; /* first column */
    row[j++] = 143;

    colno[j] = 2; /* second column */
    row[j++] = 60;

    /* set the objective in lpsolve */
    if(!set_obj_fnex(lp, j, row, colno))
      ret = 4;
  }

  if(ret == 0) {
    /* set the object direction to maximize */
    set_maxim(lp);

    /* just out of curioucity, now show the model in lp format on screen */
    /* this only works if this is a console application. If not, use write_lp and a filename */
    write_LP(lp, stdout);
    /* write_lp(lp, "model.lp"); */

    /* I only want to see important messages on screen while solving */
    set_verbose(lp, IMPORTANT);

    /* Now let lpsolve calculate a solution */
    ret = solve(lp);
    if(ret == OPTIMAL)
      ret = 0;
    else
      ret = 5;
  }

  if(ret == 0) {
    /* a solution is calculated, now lets get some results */

    /* objective value */
    printf("Objective value: %f\n", get_objective(lp));

    /* variable values */
    get_variables(lp, row);
    for(j = 0; j < Ncol; j++)
      printf("%s: %f\n", get_col_name(lp, j + 1), row[j]);

    /* we are done now */
  }

  /* free allocated memory */
  if(row != NULL)
    free(row);
  if(colno != NULL)
    free(colno);

  if(lp != NULL) {
    /* clean up such that all used memory by lpsolve is freed */
    delete_lp(lp);
  }
}
