#include <stdint.h>
#include <Exploration/Constraints.h>

#include "../../libs/lp_solve_5.5/lp_lib.h"
#include<Net/Petrinet.h>

class ComputePlacesBounds
{
public:
	Petrinet * net;
	ComputePlacesBounds(Petrinet * n, int num_lp_calls);
	capacity_t *placesBounds;
	capacity_t* computePlacesBounds(int num_lp_calls);
	int solve_invariant_lp(int *invariant, int block_length);
	
private:
	Constraint_HL *constraints;
	Vertex_HL **elements;
	
	arrayindex_t *place_class;
	arrayindex_t *transition_class;
	arrayindex_t num_places_classes;
	arrayindex_t num_transitions_classes;
	
	/* maps  the place_classes, transition_classes into the 'contraints' indices 
	 * in 'constraints', transitions and places classes are not separated 
	 * but they have to be separated and structured to write the lp problem
	 * therefore, we map the new classes into the constraints ones to access the information there*/
	int *map_indices_places;
	int *map_indices_transitions;
	
	/* a list with the places classes for which we want to find a covering invariant with i(place)>0 */
	std::vector<arrayindex_t> p_classes_to_cover;
	
	/* a 2d array with the rows for the incidence matrix
	 * it is computed only once and used for the lp constraints for each lp_solve call */
	REAL **incidence_rows;
};
