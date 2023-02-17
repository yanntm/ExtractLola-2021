#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Exploration/ComputePlacesBounds.h>
#include <Exploration/Constraints.h>

#include "../../libs/lp_solve_5.5/lp_lib.h"

ComputePlacesBounds::ComputePlacesBounds(Petrinet * n, int num_lp_calls)
{
	net = n;
	placesBounds = new capacity_t[net->Card[PL]];
	placesBounds = ComputePlacesBounds::computePlacesBounds(num_lp_calls);
}

capacity_t* ComputePlacesBounds::computePlacesBounds(int num_lp_calls)
{
	//Create_HL_Net chl = Create_HL_Net(net);
	//chl.create_HL_Net();
	//constraints = chl.constraints;
	//elements = chl.elements;
	//
	//map_indices_places = new int[net->Card[PL]]; 
	//map_indices_transitions = new int[net->Card[TR]];
	//
	//place_class = new arrayindex_t[net->Card[PL]];
	//transition_class = new arrayindex_t[net->Card[TR]];
	//num_places_classes = 0;
	//num_transitions_classes = 0;
	//
	//for (int i = 0; i< chl.cardConstraints; i++){
		///* the id of a node in constraints is the real id in PN if it is a place 
		 //* and it is (Card[PL] + real id) if it is a transition */
		///* if the first element is a place then it is a class of places */
		//if (elements[constraints[i].from]->id < net->Card[PL]){  
			//for (int j=constraints[i].from; j<=constraints[i].to; j++){
				//place_class[elements[j]->id] = num_places_classes;
			//}
			//map_indices_places[num_places_classes] = i;
			//p_classes_to_cover.push_back(num_places_classes);
			//num_places_classes++;
		//}
		///* if the first element is a place then it is a class of places */
		//else if (elements[constraints[i].from]->id >= net->Card[PL]){  
			//for (int j=constraints[i].from; j<=constraints[i].to; j++){
				//transition_class[elements[j]->id-net->Card[PL]] = num_transitions_classes;
			//}
			//map_indices_transitions[num_transitions_classes] = i;
			//num_transitions_classes++;
		//}
	//}
	//
	///* compute the incidence matrix constraints for lp_solve */
	//incidence_rows = new REAL*[num_transitions_classes];
	//
	//for (int t_c=0; t_c<num_transitions_classes; t_c++) {
		///* pick some representative of the places class, for instance, the first place in the class 
		//* and iterate over the pre, and post places, in order to find out the pre and post places classes */
		//incidence_rows[t_c] = new REAL[num_places_classes + 1];
		//
		//for(int i=0; i<num_places_classes+1; i++)
			//incidence_rows[t_c][i] = 0;
			//
		//int first_t_id = constraints[map_indices_transitions[t_c]].from;
		 //
		//int t = elements[first_t_id]->id - net->Card[PL];
		//int num_pre_p = net->CardArcs[TR][PRE][t];
		//
		//for (int k=0; k<num_pre_p; k++) {
			//int pre_p = net->Arc[TR][PRE][t][k];
			//int pre_p_class = place_class[pre_p];
			//int mult = net->Mult[TR][PRE][t][k];
			//incidence_rows[t_c][pre_p_class + 1] += -1 * mult;
		//}
		//
		//
		//int num_post_p = net->CardArcs[TR][POST][t];
				//
		//for (int k=0; k<num_post_p; k++) {
			//int post_p = net->Arc[TR][POST][t][k];
			//int post_p_class = place_class[post_p];
			//int mult = net->Mult[TR][POST][t][k];
			//incidence_rows[t_c][post_p_class + 1] += mult;
		//}
	//}
	//
	//
	//capacity_t *bounds = new capacity_t[net->Card[PL]];
	///* number of invariants found */
	//int count_inv = 0;
	///* the block size is the number of places classes for which we compute a covering invariant */
	//int block_length = 1;
	//
	//if (num_places_classes > num_lp_calls){
		//if (num_places_classes % num_lp_calls > 0)
			//block_length = (int) num_places_classes / num_lp_calls;
		//else
			//block_length = num_places_classes / num_lp_calls;
	//}	
	//
	///* the high level PN invariant */
	//int *hl_invariant = new int[num_places_classes]; 
	///* the low level PN invariant */
	//int *ll_invariant = new int[net->Card[PL]]; 
	//while(!p_classes_to_cover.empty()) {
		///* Check if the of remaining places to cover is smaller than the block_size */
		//block_length = std::min(block_length, (int) p_classes_to_cover.size());
		//
		//std::fill(hl_invariant, hl_invariant + num_places_classes, 0);
		//int ret = ComputePlacesBounds::solve_invariant_lp(hl_invariant, block_length);
		//
		//Constraint_HL curr_constraint;
		//
		//if(ret == 0){
			//count_inv++;
			///* ẃhich other places does this invariant cover? store them in p_classes_covered and delete it from p_classes_to_cover*/
			//std::vector<arrayindex_t> p_classes_covered;
			//std::vector<arrayindex_t>::iterator it = p_classes_to_cover.begin();
			//while (it != p_classes_to_cover.end()) {
				//arrayindex_t place = *it;
				//if (hl_invariant[place] > 0) {
					//p_classes_covered.push_back(place);
					//it = p_classes_to_cover.erase(it);
				//} else
					//it++;
			//}
			//std::fill(ll_invariant, ll_invariant + net->Card[PL], 0);
			//
			//for (arrayindex_t j = 0; j<net->Card[PL]; j++){
				//arrayindex_t j_place_class = place_class[j];
				//ll_invariant[j] = hl_invariant[j_place_class];
			//}
			//
			//capacity_t prod_inv_m0 = 0;
			///* compute invariant * init_marking */
			//for (arrayindex_t j = 0; j<net->Card[PL]; j++)
				//prod_inv_m0 += net->Initial[j] * ll_invariant[j];
			//for (int i = 0; i < p_classes_covered.size(); i++) {
				//arrayindex_t p_class = p_classes_covered.at(i);
				//curr_constraint = constraints[map_indices_places[p_class]];
				//for(int k = curr_constraint.from; k <= curr_constraint.to; k++){
					//arrayindex_t place = elements[k]->id;
					//bounds[place] = prod_inv_m0/ll_invariant[place];
				//}
			//}
		//}
		//
		//else{
			//
			//for(int i=0; i < block_length; ++i) {
				//int p_class = p_classes_to_cover.at(i);
				//curr_constraint = constraints[map_indices_places[p_class]];
				//for(int k = curr_constraint.from; k <= curr_constraint.to; k++){
					//arrayindex_t place = elements[k]->id;
					//bounds[place] = MAX_CAPACITY;
				//}
			//}
			//
			///* remove all p_classes in the current block */
			//p_classes_to_cover.erase(p_classes_to_cover.begin(), p_classes_to_cover.begin() + block_length);			
		//}
	//}
	//
	//delete [] hl_invariant;
	//delete [] ll_invariant;
	//delete [] map_indices_places;
	//delete [] map_indices_transitions;
	//delete [] place_class;
	//delete [] transition_class;
	//for (int t_c=0; t_c<num_transitions_classes; t_c++) {
		//delete [] incidence_rows[t_c];
	//}
	//delete [] incidence_rows;
	//
	///* output the bounds */
	///* 
	//for (arrayindex_t place = 0; place<net->Card[PL]; place++)
		//RT::rep->status("place %s bound: %li", net->Name[PL][place], bounds[place]);
		 //* */
	///* output the number of found invariants */
	///* RT::rep->status("Num invariants: %i", count_inv); */
	//
	//return bounds;
	return 0;
}



int ComputePlacesBounds::solve_invariant_lp(int *invariant, int block_length)
{	
	/* create the lp and solve it */
	lprec *lp;
	int Ncol = num_places_classes;
	int *colno = NULL, j, ret = 0;
	REAL *row = NULL;
	lp = make_lp(0, Ncol);
	if(lp == NULL)
		ret = 1; /* couldn't construct a new model... */

	if(ret == 0) {

		/* create space large enough for one row */
		colno = (int *) malloc(Ncol * sizeof(*colno));
		row = (REAL *) malloc((Ncol+1) * sizeof(*row));
		if((colno == NULL) || (row == NULL))
			ret = 2;
	}
	
	if(ret == 0){
		/* set all variables to integer */
		for (int p=1; p<=num_places_classes; p++) {
			set_int(lp, p, TRUE);
		}
	}
	
	/* set the objective function */
	j = 0;

	for (int p = 1; p <= num_places_classes; p++) {
		colno[j] = p;
		row[j++] = 1;
	}

	/* set the objective in lpsolve */
	if(!set_obj_fnex(lp, j, row, colno))
		ret = 4;

	if(ret == 0) {
		/* set the object direction to minimize */
		set_minim(lp);
		set_add_rowmode(lp, TRUE);  /* makes building the model faster if it is done rows by row */
	}

	/* add the constraits row by row*/
	for (int t_c=0; t_c<num_transitions_classes; t_c++) {
		if(!add_constraint(lp, incidence_rows[t_c], EQ, 0)){
			ret = 3;
			break;
		}
	}
	
	if(ret == 0){ 
		for(int it=0; it < block_length; ++it) {
			int place = p_classes_to_cover.at(it);
	        std::fill(row, row + Ncol + 1, 0);
	        row[place + 1] = 1;
			/* there is no > possibility, only >= available. So use >= 1 instead of >0 */
	        if(!add_constraint(lp, row, GE, 1)){ 
				ret = 3;

				break;
			}
	    }
	}
        
	
	if(ret == 0) {
		set_add_rowmode(lp, FALSE); /* rowmode should be turned off again when done building the model */

		/* I only want to see important messages on screen while solving */
		set_verbose(lp, IMPORTANT);
		
		/*to show the model in lp format on screen */
		/*write_LP(lp, stdout);*/

		/* Now let lpsolve calculate a solution */
		set_timeout(lp,10.0);
		ret = solve(lp);
		if(ret == OPTIMAL)
			ret = 0;
	}

	if(ret == 0) {
		double *invariant_double = new double[Ncol];
		get_variables(lp, invariant_double);
		for(j = 0; j < Ncol; j++) {
			invariant[j] =  static_cast<int>(invariant_double[j] + 0.5);
			/* output the invariant */
			/*RT::rep->status("invariant %s: %i\n", get_col_name(lp, j + 1), invariant[j]); */
		}
		delete [] invariant_double;
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
	
	return ret;
}
