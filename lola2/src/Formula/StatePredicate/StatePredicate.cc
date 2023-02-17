/****************************************************************************
  This file is part of LoLA.

  LoLA is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option)
  any later version.

  LoLA is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
  more details.

  You should have received a copy of the GNU Affero General Public License
  along with LoLA. If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <Core/Dimensions.h>
#include <Formula/StatePredicate/StatePredicate.h>


bool StatePredicate::isOrNode() 
{
    return false;
}

// LCOV_EXCL_START
arrayindex_t StatePredicate::countUnsatisfied() 
{
    return 0;
}
// LCOV_EXCL_STOP

bool StatePredicate::solve_lp(ProtoNet * nnn, lprec * lp)
{
	// return false = INFEASIBLE, true = FEASIBLE or not successful

	// assume lp to be filled with objective functions and constraints
        set_add_rowmode(lp, FALSE); /* rowmode should be turned off again when done building the model */

        /*to show the model in lp format on screen */
  //      write_LP(lp, stdout);
        /* I only want to see important messages on screen while solving */
        set_verbose(lp, IMPORTANT);

        /* Now let lpsolve calculate a solution */
        set_timeout(lp,5);
        set_break_at_first(lp,TRUE); // only interested in feasibility
        //while(true)
        //{
		//print_lp(lp);
                int ret = solve(lp);
                if(ret == INFEASIBLE)
                {
                        // provably no solution exists
                        delete_lp(lp);
                        //resize_lp(lp,lprows, lpcolumns);
//RT::rep->status("INFEASIBLE");
                        return false;

                }
                if(ret != OPTIMAL && ret != SUBOPTIMAL)
                {
                        // no useful solution obtained
                        delete_lp(lp);
                        //resize_lp(lp,lprows, lpcolumns);
//RT::rep->status("NO USEFUL SOLUTION");
                        return true;
                }
//RT::rep->status("CHECK FEASIBLE SOLUTION");
                //// here: have a feasible solution
                //// check for unmarked trap
                //REAL * solution;
                //get_ptr_variables(lp,&solution);
                //bool * trap = new bool[nnn -> cardPL];
                //bool * initial = new bool[nnn -> cardPL];
                //int cardinitial = 0;
                //int cardtrap = 0;
                //for(ProtoPlace * ppp = nnn -> firstplace; ppp; ppp = ppp -> next)
                //{
                        //if(solution[ppp ->index] < 0.5)
                        //{
                                //trap[ppp->index] = true;
                                //cardtrap++;
                                //if(ppp -> marking)
                                //{
                                        //initial[ppp->index] = true;
                                        //cardinitial++;
                                //}
                                //else initial[ppp->index] = false;
                        //}
                        //else trap[ppp->index] = false;
                //}
                //bool somethingchanged;
                //do
                //{
                        //somethingchanged = false;
                        //for(ProtoTransition * ttt = nnn -> firsttransition; ttt; ttt = ttt -> next)
                        //{
                                //Arc * aaa;
                                //for(aaa = ttt -> firstArcPost; aaa; aaa = aaa -> next_tr)
                                //{
                                        //if(trap[aaa -> pl ->index]) break;
                                //}
                                //if(aaa) continue; // t has post-place in trap -> trap property holds for t
                                //for(aaa = ttt -> firstArcPre; aaa; aaa = aaa -> next_tr)
                                //{
                                        //if(trap[aaa->pl->index])
                                        //{
                                                //somethingchanged = true;
                                                //trap[aaa->pl->index] = false;
                                                //cardtrap--;
                                                //if(initial[aaa->pl->index])
                                                //{
                                                        //cardinitial--;
                                                //}
                                        //}
                                        //if(!cardtrap||!cardinitial)
                                        //{
                                                //// there is no empty trap in target marking
                                                //// or empty trap is empty already in initial marking
                                                //// -> cannot add constraint
                                                //// -> cannot prove stability
                                                //delete [] trap;
                                                //delete [] initial;
                                                //resize_lp(lp,lprows, lpcolumns);
////RT::rep->status("SOLUTION CONSISTENT WITH TRAPS");
                                                //return;
                                        //}
                                //}
                        //}
                //}
                //while(somethingchanged);
                //// if not returned yet -> remaining empty trap was initially marked
////RT::rep->status("FOUND TRAP SIZE %d INITIAL %d", cardtrap,cardinitial);
                //// -> add new constraint and continue;
                //REAL * row = new REAL [lpcolumns+1];
                //std::fill(row, row + lpcolumns + 1, 0);
                //for(int i = 0; i < nnn->cardPL;i++)
                //{
                        //if(trap[i])
                        //{
                                //row[i+1] = 1.0;
                        //}
                //}
                //if(!add_constraint(lp,row,GE,1.0))
                //{
                        //resize_lp(lp,lprows,lpcolumns);
                        //delete [] row;
                        //delete [] trap;
                        //delete [] initial;
////RT::rep->status("NO LP3");
                        //return;
                //}
                ////set_row_name(lp,get_Nrows(lp),deconst("TRAP"));
                ////write_LP(lp,stdout);
                //delete [] trap;
                //delete [] initial;
        //}
        //delete_lp(lp);
        //resize_lp(lp,lprows, lpcolumns);
//RT::rep->status("DONE");
        return true;

}
