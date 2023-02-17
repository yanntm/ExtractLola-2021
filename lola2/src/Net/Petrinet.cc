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

/*!
\file
\author Karsten
\status new

\brief All information related to a Petri net
*/

#include <Net/NetState.h>
#include <InputOutput/JSON.h>
#include <Net/Petrinet.h>
#include <Core/Dimensions.h>
#include <Core/Runtime.h>
#include <Frontend/Parser/ParserPTNet.h>
#include <Frontend/SymbolTable/Symbol.h>
#include <Frontend/SymbolTable/PlaceSymbol.h>
#include <Frontend/SymbolTable/TransitionSymbol.h>
#include <Frontend/SymbolTable/SymbolTable.h>
#include <Net/LinearAlgebra.h>
#include <Exploration/Firelist.h>
#include <Exploration/ComputePlacesBounds.h>

// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################

/*
 * function that extracts conflict edges from the relation of a place pl with it's own 'transitions'
 * -for every 'transition[i]' the consumption and production of tokens wrt. to pl is given by 'consumptionOf[i]' and 'productionOf[i]'
 * -we want that there is a (conflict-) edge i->j if triggering i can disable j, hence
 *  -we can assume that both, i and j, are enabled, meaning that pl has at least max{consumptionOf[i], consumptionOf[j]} tokens
 *  -then i is triggered taking consumptionOf[i] tokens from pl and putting productionOf[i] back
 *  -the conflict happens if this can leave less than consumptionOf[j] tokens on pl,
 *  -the condition therefore is: max(consumptionOf[i], consumptionOf[j) - consumptionOf[i] + productionOf[i] < consumptionOf[j]
 *  -this condition can (obviously) be simplified as follows:
 *  -case 1 - consumptionOf[i] <= consumptionOf[j]: then conflict happens under productionOf[i] < consumptionOf[i]
 *  -case 2 - consumptionOf[i] >  consumptionOf[j]: then conflict happens under productionOf[i] < consumptionOf[j]
 *  -taking all conditions together we get that i->j iff productionOf[i] < consumptionOf[i] AND productionOf[i] < consumptionOf[j]
 * -output is given as a set of pairs (O,I) where i->j for all i in O and all j in I
 * -here, such a pair is called a directed clique
 * -every conflict edge inflicted by pl has to be represented by at least one result clique
 * -as explained later, we can represent every clique as a pair of prefix and suffic of two lists
 */
void Petrinet::extractConflictCliques(std::vector<int> &transitions,	 //input: n transitions in a places pl post set
							std::vector<int> &consumptionOf,   //input: consumptions with respect to pl
							std::vector<int> &productionOf,	//input: productions with respect to pl
							std::vector<int> &tsByConsumption, //output: transitions sorted by consumption
							std::vector<int> &tsByProduction,  //output: transitions filtered by productionOf[i] < consumptionOf[i] and then sorted by production
							std::vector<std::pair<int, int> > &cliques)
{   
	//output: pairs (c,p) such that all transitions of the interval [0,p-1] in
	//'tsByProduction' conflict all transitions of the interval [c,n-1] in 'tsByConsumption'
	/*
     * the idee is as follows:
     * -we create two lists:
     *  - (t_0, t_1, ..., t_m), transitions ti with productionOf[i] < consumptionOf[i] sorted ascendingly by production and
     *  - (t'_0, t'_1, ..., t'_n) all transitions sorted ascendingly by consumption
     * -then we know for every 0<=i<m that t_i->t'_j for all j with min_i <= j < n where min_i is the smallest index such that productionOf(t_i) < consumptionOf(t'_{min_i})
     * -also for every 0<=j<n, we have t_i->t'_j for all i with 0 <= i < min_j where min_j is the smallest index such that productionOf(t_{min_j}) >= consumptionOf(t'_j)
     * -hence, every directed clique is given by a pair (c,p) with t_i -> t'_j for all 0<=i<p and all c<=j<n
     * the following therefore creates sorted maps 'mapProductionToTransitions' and 'mapConsumptionToTransitions' to access the two lists by ascending production and consumption
     */
	std::map<int, std::vector<int> > mapProductionToTransitions, mapConsumptionToTransitions;
	for (int i = 0; i < transitions.size(); i++)
	{
		if (productionOf[i] < consumptionOf[i])
		{
			if (mapProductionToTransitions.count(productionOf[i]) == 0)
				mapProductionToTransitions.insert(std::pair<int, std::vector<int> >(productionOf[i], std::vector<int>()));
			mapProductionToTransitions[productionOf[i]].push_back(transitions[i]);
		}

		if (mapConsumptionToTransitions.count(consumptionOf[i]) == 0)
			mapConsumptionToTransitions.insert(std::pair<int, std::vector<int> >(consumptionOf[i], std::vector<int>()));
		mapConsumptionToTransitions[consumptionOf[i]].push_back(transitions[i]);
	}

	// next, we initialize the output
	tsByProduction.clear();
	tsByConsumption.clear();
	cliques.clear();

	/*
     * the following loop flats out the sorted lists and meanwhile creates the clique pairs
     * -we keep a pointer p into the list (t_1,...) and a pointer c into the list (t'_1,...)
     * -basically, one iteration creates one clique
     * -in the beginning of a loop iteration we know that the transitions producing at least as much as p, say x, do not conflict with transitions consuming less then c, say y
     * -we therefore iterate c until we find the first c' where x < y'
     * -then we iterate p until we find the first p' where x' >= y'
     * -after this, we know that all transitions before p' conflict with all transitions after and including c'
     * -hence, this is a clique (c',p'-1)
     * -the only differnce in the following is that p and c are grouped wrt. production and consumption, respectively
     */
	std::map<int, std::vector<int> >::iterator p = mapProductionToTransitions.begin();
	std::map<int, std::vector<int> >::iterator c = mapConsumptionToTransitions.begin();
	while (p != mapProductionToTransitions.end())
	{

		// we iterate c until we find c'
		for (; (c != mapConsumptionToTransitions.end()) && (p->first >= c->first); c++)
			tsByConsumption.insert(tsByConsumption.end(), c->second.begin(), c->second.end());

		// we iterate p until we find p'
		for (; (p != mapProductionToTransitions.end()) && ((c == mapConsumptionToTransitions.end()) || (p->first < c->first)); p++)
			tsByProduction.insert(tsByProduction.end(), p->second.begin(), p->second.end());

		// we add the clique
		if (c != mapConsumptionToTransitions.end())
			cliques.push_back(std::pair<int, int>(tsByConsumption.size(), tsByProduction.size()));
	}
	//fill in everything that has not been filled, yet:
	for (; c != mapConsumptionToTransitions.end(); c++)
		tsByConsumption.insert(tsByConsumption.end(), c->second.begin(), c->second.end());
}

/*
 * function that extracts conflict edges from the relation of a place pl with it's own 'transitions'
 * -for every 'transition[i]' the consumption and production of tokens wrt. to pl is given by 'consumptionOf[i]' and 'productionOf[i]'
 * -we know that there is a (conflict-) edge i->j if 'consumptionOf[i] > productionOf[i]' and consumptionOf[j] > 0
 * -output is given as one pair (O,I) where i->j for all i in O and all j in I
 * -here, such a pair is called a directed clique
 * -as explained later, we can represent the clique as a pair of prefix and suffic of a list
 */
void Petrinet::extractConflictClique(std::vector<int> &transitions,   //input + output: n transitions in a places pl post set
						   std::vector<int> &consumptionOf, //input + output: consumptions with respect to pl
						   std::vector<int> &productionOf,  //input + output: productions with respect to pl
						   std::pair<int, int> &clique)
{   //output: pair (a,b) such that all 'transitions' of the interval [0,b-1] in
	//conflict all'transitions' of the interval [a,n-1]
	int exchange = 0;
#define SWAP(x, y) \
	exchange = x;  \
	x = y;         \
	y = exchange;

	/*
     * the idee is as follows:
     * -we sort transitions such that it becomes transitions = prefix | middle | suffix with
     *  -consumptionOf(i)-productionOf(i) > 0 iff i is part of prefix
     *  -consumptionOf(j) > 0 iff j is part of prefix | middle
     * -then i conflicts j iff i is in prefix and j is in prefix | middle
     */

	// firstly, sorting transitions to become p | s with consumptionOf(i) > 0 iff i is part of p:
	int s = 0, t = transitions.size() - 1;
	while (s < t)
	{
		if (consumptionOf[s] > 0)
			s += 1;
		else if (consumptionOf[t] == 0)
			t -= 1;
		else
		{
			SWAP(transitions[s], transitions[t])
			SWAP(consumptionOf[s], consumptionOf[t])
			SWAP(productionOf[s], productionOf[t])
		}
	}

	// we can already delete s, because they are not in conflict with anyone:
	while (consumptionOf.size() > 0 && consumptionOf.back() == 0)
	{
		transitions.pop_back();
		consumptionOf.pop_back();
		productionOf.pop_back();
	}

	// if there was no transition with consumptionOf(i) > 0, we return the invalid clique
	if (transitions.empty())
	{
		clique.first = -1;
		clique.second = -1;
		return;
	}

	// secondly, sorting p to become prefix | middle:
	s = 0;
	t = transitions.size() - 1;
	while (s < t)
	{
		if (consumptionOf[s] > productionOf[s])
			s += 1;
		else if (consumptionOf[t] <= productionOf[t])
			t -= 1;
		else
		{
			SWAP(transitions[s], transitions[t])
			SWAP(consumptionOf[s], consumptionOf[t])
			SWAP(productionOf[s], productionOf[t])
		}
	}

	// this enables us to set the indexes a,b:
	if (consumptionOf[t] == 0)
		t += 1;
	clique.second = (consumptionOf[s] > productionOf[s]) ? s + 1 : s;
	clique.first = 0;
}

/*
 * function that extracts activation edges from the relation of a place pl with it's own 'transitions'
 * -for every 'transition[i]' the consumption and production of tokens wrt. to pl is given by 'consumptionOf[i]' and 'productionOf[i]'
 * -we know that there is a (activation-) edge i->j if 'productionOf[i] > consumptionOf[i]' and consumptionOf[j] > 0
 * -output is given as one pair (O,I) where i->j for all i in O and all j in I
 * -here, such a pair is called a directed clique
 * -as explained later, we can represent the clique as a pair of prefix and suffic of a list
 */
void Petrinet::extractActivationClique(std::vector<int> &transitions,   //input + output: n transitions in a places pl post set
							 std::vector<int> &consumptionOf, //input + output: consumptions with respect to pl
							 std::vector<int> &productionOf,  //input + output: productions with respect to pl
							 std::pair<int, int> &clique)
{   //output: pair (a,b) such that all 'transitions' of the interval [0,b-1] in
	//activate all'transitions' of the interval [a,n-1] in
	if (transitions.size() == 0)
	{
		clique.first = clique.second = -1;
		return;
	}

	int exchange = 0;
#define SWAP(x, y) \
	exchange = x;  \
	x = y;         \
	y = exchange;

	/*
     * the idee is as follows:
     * -we sort transitions such that it becomes transitions = prefix | middle | suffix | scrap with
     *  -productionOf(i)-consumptionOf(i) > 0 iff i is part of prefix | middle
     *  -consumptionOf(i) > 0 iff i is part of middle | suffix
     * -then i activates j iff i is in prefix | middle and j is in middle | suffix
     */

	// firstly, sorting transitions to become p | c with productionOf(i)-consumptionOf(i) > 0 iff i is part of p:
	int s = 0, t = transitions.size() - 1;
	while (s < t)
	{
		if (productionOf[s] > consumptionOf[s])
			s += 1;
		else if (productionOf[t] <= consumptionOf[t])
			t -= 1;
		else
		{
			SWAP(transitions[s], transitions[t])
			SWAP(consumptionOf[s], consumptionOf[t])
			SWAP(productionOf[s], productionOf[t])
		}
	}

	// if p is empty then there is no activation at all => we return the invalid clique
	if (s == 0 && productionOf[s] <= consumptionOf[s])
	{
		transitions.clear();
		consumptionOf.clear();
		productionOf.clear();
		clique.first = clique.second = -1;
		return;
	}

	// we can already set the index a to the beginning of c:
	clique.second = (productionOf[t] > consumptionOf[t]) ? t + 1 : t;

	// secondly, sorting p to become prefix | middle:
	s = 0;
	t -= 1;
	while (s < t)
	{
		if (consumptionOf[s] == 0)
			s += 1;
		else if (consumptionOf[t] > 0)
			t -= 1;
		else
		{
			SWAP(transitions[s], transitions[t])
			SWAP(consumptionOf[s], consumptionOf[t])
			SWAP(productionOf[s], productionOf[t])
		}
	}

	// this enables us to set the index b to the beginning of middle:
	clique.first = (consumptionOf[t] == 0) ? t + 1 : t;

	// in fact, c should be suffix and scrap should be empty; however:
	// sorting c to become suffix | scrap:
	s = clique.second;
	t = transitions.size() - 1;
	while (s < t)
	{
		if (consumptionOf[s] > 0)
			s += 1;
		else if (consumptionOf[t] == 0)
			t -= 1;
		else
		{
			SWAP(transitions[s], transitions[t])
			SWAP(consumptionOf[s], consumptionOf[t])
			SWAP(productionOf[s], productionOf[t])
		}
	}

	// finally, we delete scrap:
	while (consumptionOf.size() > 0 && consumptionOf.back() == 0)
	{
		transitions.pop_back();
		consumptionOf.pop_back();
		productionOf.pop_back();
	}

	// if there was no transition with consumptionOf(i) > 0, we return the invalid clique
	if (transitions.empty())
		clique.first = clique.second = -1;
}

/*
 * function that is able to add 'vertices' to the directed graph G=(V,E) that are half of a clique (O,I) -- meaning either O or I:
 * in the following, for the sake of simple explanations, we assume that 'vertices' represents O
 * hence, we add all vertices of O that are not yet known to G (i.e., not in V yet) and all edges between O and I' where I' is the subset of I that is already known to G
 */
void Petrinet::addHalfClique(std::vector<int> &vertices, int begin, int end, // input: half clique (in the example: vertices of O)
				   std::vector<int> &mapVertexToClass,			  // update: a map V->H from the vertices of G to the homogenuous sets
				   std::vector<std::vector<int> > &mapCliqueToClasses,   // update: a map CX->H from the half cliques of G to the homogenuous sets (in the example: X=0)
				   std::vector<std::vector<int> > &mapCoCliqueToClasses, // update: a map CY->H from the other half cliques (Y=1-X) of G to the homogenuous sets (in the example: Y=1)
				   std::vector<std::vector<int> > &mapClassToCliques,	// update: a map H->CX from the homogenuous sets of G to the half cliques
				   std::vector<std::vector<int> > &mapClassToCoCliques,  // update: a map H->CY from the homogenuous sets of G to the other half cliques
				   std::vector<ClassMetaData> &mapClassToMeta)
{ 
	// update: a map H->M from the homogenuous sets of G to some meta data

	std::vector<int> knownVertices;		// list of vertices seen before (members of existing classes)
	std::vector<int> intersectedClasses; // list of existing classes intersected by added half clique

	// we iterate all half clique vertices to find all intersected classes:
	std::vector<int>::iterator vertex = vertices.begin() + begin;
	for (int i = begin;
		 i < end;
		 i++, vertex++)
	{
		if (mapVertexToClass[*vertex] < 0)
			// vertex is not a member of known classes (hence, not seen before)
			// we connect new vertex to a new class
			mapVertexToClass[*vertex] = mapClassToMeta.size(); // meaning, we select new class by next free index
		else
		{
			// we memorize the vertex as a known one
			knownVertices.push_back(*vertex);
			// we fetch the class of that known vertex (as it intersects the half clique) and check if we have already seen this class for another vertex
			if (mapClassToMeta[mapVertexToClass[*vertex]].overlapWithClique++ == 0)
				// if the class is new, we memorize it
				intersectedClasses.push_back(mapVertexToClass[*vertex]);
		}
	}

	/*
     * -the current half clique, by intersecting existing classes, splits each of these classes into two classes
     *  -one is a subset of the current half clique and one is simply a reduced old class
     * -moreover, the half clique vertices that are really new to G form an entire new class for themselves
     * -the following initializes a list of all classes contained in the current half clique:
     */
	std::vector<int> classesInClique; // list of classes contained in half clique
	// if there are vertices that are new to G then we have to add the corresponding class
	if (end - begin - knownVertices.size() > 0)
	{
		//in fact, the mapping of these vertices to the new class has already been done above
		//we only have to create the meta object and initialize the given maps
		ClassMetaData newClass;
		newClass.size = end - begin - knownVertices.size();
		newClass.overlapWithClique = 0;
		classesInClique.push_back(mapClassToMeta.size()); //meaning newClass is added to current half clique
		mapClassToMeta.push_back(newClass);
		std::vector<int> cliquesContainingNewClass;
		cliquesContainingNewClass.push_back(mapCliqueToClasses.size()); // meaning current clique is added to newClass
		mapClassToCliques.push_back(cliquesContainingNewClass);
		std::vector<int> coCliquesContainingNewClass;
		mapClassToCoCliques.push_back(coCliquesContainingNewClass);
	}

	/* -as the current half clique, by intersection, splits existing classes, we have to subdivide them
     *  -therefore, we iterate through the intersected classes and make updates:
     */
	for (std::vector<int>::iterator intersectedClass = intersectedClasses.begin();
		 intersectedClass != intersectedClasses.end();
		 intersectedClass++)
	{

		/*
         * -every intersected class X is cut into two disjoint subsets Y and Z, where Y is completely within the current half clique and Z outside
         *  -Y has at least one element, namely the vertex of the half clique that was assigned to X
         *  -Y is subsequently represented by 'containedClass'
         *  -Z can be empty
         * -if Z is empty then the existing class is simply connected (also) to the current half clique
         * -if Z is not empty then the existing class is reduced by the vertices of Y (which has already been done)and we copy this class for Z abd associate it to the half clique as before
         */

		// the variable for set Y
		int containedClass = *intersectedClass; // our guess is that Z is empty and we simply take X for Y

		// we check if there are vertices in X left for Z:
		if (mapClassToMeta[*intersectedClass].overlapWithClique < mapClassToMeta[*intersectedClass].size)
		{
			// here our guess was wrong, Y is a proper subset of class X
			// => we have to split class:
			containedClass = mapClassToMeta.size(); // meaning the index of the newClass yet to be created
			ClassMetaData newClass;
			newClass.size = mapClassToMeta[*intersectedClass].overlapWithClique;
			newClass.overlapWithClique = 0;
			mapClassToMeta.push_back(newClass);
			mapClassToMeta[*intersectedClass].size -= newClass.size;

			// we associate the half cliques (only one side) containg X with the new class:
			std::vector<int> cliquesContainingNewClass;
			for (std::vector<int>::iterator clique = mapClassToCliques[*intersectedClass].begin();
				 clique != mapClassToCliques[*intersectedClass].end();
				 clique++)
			{
				mapCliqueToClasses[*clique].push_back(containedClass);
				cliquesContainingNewClass.push_back(*clique);
			}
			mapClassToCliques.push_back(cliquesContainingNewClass);

			// we associate the other side half cliques containg X with the new class:
			std::vector<int> coCliquesContainingNewClass;
			for (std::vector<int>::iterator coClique = mapClassToCoCliques[*intersectedClass].begin();
				 coClique != mapClassToCoCliques[*intersectedClass].end();
				 coClique++)
			{
				mapCoCliqueToClasses[*coClique].push_back(containedClass);
				coCliquesContainingNewClass.push_back(*coClique);
			}
			mapClassToCoCliques.push_back(coCliquesContainingNewClass);
		}

		// we add Y to the current half clique and vice versa
		mapClassToCliques[containedClass].push_back(mapCliqueToClasses.size());
		classesInClique.push_back(containedClass);

		// we tidy up the meta data
		mapClassToMeta[*intersectedClass].splitOffClass = containedClass;
		mapClassToMeta[*intersectedClass].overlapWithClique = 0;
	}

	/*
     * vertices of the current half clique that have been part of G before are still associate to the old classes that just have been split in two
     * we have to iterate them once again to update the association to their classes:
     */
	for (std::vector<int>::iterator vertex = knownVertices.begin();
		 vertex != knownVertices.end();
		 vertex++)
		mapVertexToClass[*vertex] = mapClassToMeta[mapVertexToClass[*vertex]].splitOffClass;

	// finally, we create the mapping of the current half clique to all its contained classes
	mapCliqueToClasses.push_back(classesInClique);
}

/*
 * function that is able to extract from the used representation of a directed graph all adjacency lists for the homogenuous sets (equivalence class)
 * -we only need one adjacency list for homogenuous set class as all vertices within such a set have the same neighborhood
 *  -this is expressed by the output map H->IBO assigning every class to a list
 * -for every class X we create a IBO-adjacency list which has a prefix I, a middle B, and a suffix O
 *  -I is the list of vertices u that is adjacent to every vertex v in X by an edge uv but not vu (the In-vertices)
 *  -B is the list of vertices u that is adjacent to every vertex v in X by both edges uv and vu (Bidirectional vertices)
 *  -O is the list of vertices u that is adjacent to every vertex v in X by an edge vu but not uV (the Out-vertices)
 *  -the subdivision into the three sublists is given by indices marking the start of and the end of the bidirectional subinterval
 */
void Petrinet::extractBidirectionalAdjacencyLists(std::vector<int> &mapVertexToClass,				  // input: a map V->H from the vertices of G to the homogenuous sets
										std::vector<std::vector<int> > &mapCliqueOToClasses,	 // input: a map C0->H from 0-half cliques of G to the homogenuous sets
										std::vector<std::vector<int> > &mapCliqueIToClasses,	 // input: a map C1->H from 1-half cliques of G to the homogenuous sets
										std::vector<std::vector<int> > &mapClassToCliqueOs,	  // input: a map H->C0 from the homogenuous sets of G to the 0-half cliques
										std::vector<std::vector<int> > &mapClassToCliqueIs,	  // input: a map H->C1 from the homogenuous sets of G to the 1-half cliques
										std::vector<std::vector<int> > &mapClassToAdjacencyList, // output: a map H->IBO from the homogenuous sets of G to the tri-adjacency list
										std::vector<std::pair<int, int> > &mapClassToBiInterval)
{ 
	// output: a map H->N^2 from the homogenuous sets of G to the interval limits of the bidirectional vertices

	// firstly, we create the following reverse map; which has been omitted so far to save necessary updates
	std::vector<std::vector<int> > mapClassToVertices(mapClassToCliqueIs.size(), std::vector<int>());
	for (int vertex = 0; vertex < mapVertexToClass.size(); vertex++)
		if (mapVertexToClass[vertex] >= 0)
			mapClassToVertices[mapVertexToClass[vertex]].push_back(vertex);

	// for initialization, we clear the output
	mapClassToAdjacencyList.clear();
	mapClassToBiInterval.clear();

/*
     * the following loop iteraizes through the homogenuous sets X (by 'clazz') and for every one of them creates the IBO-adjacency list
     * -for every class, we list all 1-half cliques Ii and all 0-half cliques Oj that contain X
     * -we union the classes Oi over all i, which gives us the set of all vertices u that are adjacent to all v in X by an edge uv
     * -we union the classes Ij over all j, which gives us the set of all vertices u that are adjacent to all v in X by an edge vu
     * -in a merging operation we filter out the union of the classes with all vertices u that are adjacent to all v in X by both edges uv und vu
     *  -for efficient union, the following map tracks the references of a particular class ('mapClassToReferenced')
     */
#define CLASS_UNREFERENCED 0
#define CLASS_SEEN 1
#define CLASS_SEEING 2
#define CLASS_BIDIRECTIONAL 3
	std::vector<int> mapClassToReferenced(mapClassToCliqueIs.size(), CLASS_UNREFERENCED);

	for (int clazz = 0; clazz < mapClassToVertices.size(); clazz++)
	{
		/*
         * in the first two loops we iterate the co-classes of all 1-half cliques Ii and all 0-half cliques Oj for the first time to mark ther relation wrt. X
         * -seen classes Y in Ij (that is, where all u in X and all v in Y have an edge uv) are marked CLASS_SEEN
         * -seeing classes Z in Oi (that is, where all u in X and all v in Z have an edge vu) are marked CLASS_SEEING
         * -classes that are both seen and seeing implicitly receive the marking CLASS_BIDIRECTIONAL
         */
		for (std::vector<int>::iterator clique = mapClassToCliqueOs[clazz].begin();
			 clique != mapClassToCliqueOs[clazz].end();
			 clique++)
		{
			for (std::vector<int>::iterator seenClass = mapCliqueIToClasses[*clique].begin();
				 seenClass != mapCliqueIToClasses[*clique].end();
				 seenClass++)
			{
				mapClassToReferenced[*seenClass] = CLASS_SEEN;
			}
		}

		for (std::vector<int>::iterator clique = mapClassToCliqueIs[clazz].begin();
			 clique != mapClassToCliqueIs[clazz].end();
			 clique++)
		{
			for (std::vector<int>::iterator seeingClass = mapCliqueOToClasses[*clique].begin();
				 seeingClass != mapCliqueOToClasses[*clique].end();
				 seeingClass++)
			{
				mapClassToReferenced[*seeingClass] |= CLASS_SEEING;
			}
		}

		/*
         * now we iterate 0-half cliques Oj for the second time to filter out the classes Y in Ij that are not bidirectional
         * -this is recognized by the marking CLASS_SEEN of Y
         * -the filtering result is in 'seenClasses'
         */
		std::vector<int> seenClasses;
		for (std::vector<int>::iterator clique = mapClassToCliqueOs[clazz].begin();
			 clique != mapClassToCliqueOs[clazz].end();
			 clique++)
		{
			for (std::vector<int>::iterator seenClass = mapCliqueIToClasses[*clique].begin();
				 seenClass != mapCliqueIToClasses[*clique].end();
				 seenClass++)
			{
				if (mapClassToReferenced[*seenClass] == CLASS_SEEN)
				{
					seenClasses.push_back(*seenClass);
					mapClassToReferenced[*seenClass] = CLASS_UNREFERENCED; // to tidy up
				}
			}
		}

		/*
         * we also iterate 1-half cliques Ii for the second time to filter out the classes Z in Oi that are either bidirectional or just seeing
         * -this is recognized by the presence or absence of marking CLASS_BIDERECTIONAL
         * -the filtering results are in 'bidirectionalClasses' and 'seeingClasses'
         */
		std::vector<int> seeingClasses, bidirectionalClasses;
		for (std::vector<int>::iterator clique = mapClassToCliqueIs[clazz].begin();
			 clique != mapClassToCliqueIs[clazz].end();
			 clique++)
		{
			for (std::vector<int>::iterator seeingClass = mapCliqueOToClasses[*clique].begin();
				 seeingClass != mapCliqueOToClasses[*clique].end();
				 seeingClass++)
			{
				if (mapClassToReferenced[*seeingClass] == CLASS_BIDIRECTIONAL)
					bidirectionalClasses.push_back(*seeingClass);
				else if (mapClassToReferenced[*seeingClass] != CLASS_UNREFERENCED)
					seeingClasses.push_back(*seeingClass);
				mapClassToReferenced[*seeingClass] = CLASS_UNREFERENCED; // to tidy up
			}
		}

		/*
         * finally we build the IBO adjacency list of X
         * -we first iterate through every seeing class and all their vertices
         * -we secondly iterate through every bidirectional class and all their vertices
         * -we finally iterate through every seen class and all their vertices
         * -the interval limits of the bidirectional subinterval are set meanwhile
         */
		std::vector<int> adjacencyList;
		std::pair<int, int> biInterval(0, 0);
		for (std::vector<int>::iterator seeingClass = seeingClasses.begin();
			 seeingClass != seeingClasses.end();
			 seeingClass++)
		{
			biInterval.first += mapClassToVertices[*seeingClass].size();
			for (std::vector<int>::iterator vertex = mapClassToVertices[*seeingClass].begin();
				 vertex != mapClassToVertices[*seeingClass].end();
				 vertex++)
			{
				adjacencyList.push_back(*vertex);
			}
		}

		biInterval.second = biInterval.first;
		for (std::vector<int>::iterator bidirectionalClass = bidirectionalClasses.begin();
			 bidirectionalClass != bidirectionalClasses.end();
			 bidirectionalClass++)
		{
			biInterval.second += mapClassToVertices[*bidirectionalClass].size();
			for (std::vector<int>::iterator vertex = mapClassToVertices[*bidirectionalClass].begin();
				 vertex != mapClassToVertices[*bidirectionalClass].end();
				 vertex++)
			{
				adjacencyList.push_back(*vertex);
			}
		}

		for (std::vector<int>::iterator seenClass = seenClasses.begin();
			 seenClass != seenClasses.end();
			 seenClass++)
		{
			for (std::vector<int>::iterator vertex = mapClassToVertices[*seenClass].begin();
				 vertex != mapClassToVertices[*seenClass].end();
				 vertex++)
			{
				adjacencyList.push_back(*vertex);
			}
		}

		mapClassToAdjacencyList.push_back(adjacencyList);
		mapClassToBiInterval.push_back(biInterval);
	}
}

/*
 * function that is able to extract from the used representation of a directed graph all adjacency lists for the homogenuous sets (equivalence class)
 * -we only need one adjacency list for homogenuous set class as all vertices within such a set have the same neighborhood
 *  -this is expressed by the output map H->O assigning every class to a list
 * -for every class X we create a O-adjacency list of vertices u that are adjacent to every vertex v in X by an edge vu
 */
void Petrinet::extractSuccessorAdjacencyLists(std::vector<int> &mapVertexToClass,			  // input: a map V->H from the vertices of G to the homogenuous sets
									std::vector<std::vector<int> > &mapCliqueOToClasses, // input: a map C0->H from 0-half cliques of G to the homogenuous sets
									std::vector<std::vector<int> > &mapCliqueIToClasses, // input: a map C1->H from 1-half cliques of G to the homogenuous sets
									std::vector<std::vector<int> > &mapClassToCliqueOs,  // input: a map H->C0 from the homogenuous sets of G to the 0-half cliques
									std::vector<std::vector<int> > &mapClassToCliqueIs,  // input: a map H->C1 from the homogenuous sets of G to the 1-half cliques
									std::vector<std::vector<int> > &mapClassToAdjacencyList)
{ // output: a map H->O from the homogenuous sets of G to the adjacency list

	// firstly, we create the following reverse map; which has been omitted so far to save necessary updates
	std::vector<std::vector<int> > mapClassToVertices(mapClassToCliqueIs.size(), std::vector<int>());
	for (int vertex = 0; vertex < mapVertexToClass.size(); vertex++)
		if (mapVertexToClass[vertex] >= 0)
			mapClassToVertices[mapVertexToClass[vertex]].push_back(vertex);

	// for initialization, we clear the output
	mapClassToAdjacencyList.clear();

	/*
     * the following loop iteraizes through the homogenuous sets X (by 'clazz') and for every one of them creates the IBO-adjacency list
     * -for every class, we list all 1-half cliques Ii and all 0-half cliques Oj that contain X
     * -we union the classes Oi over all i, which gives us the set of all vertices u that are adjacent to all v in X by an edge uv
     * -we union the classes Ij over all j, which gives us the set of all vertices u that are adjacent to all v in X by an edge vu
     * -in a merging operation we filter out the union of the classes with all vertices u that are adjacent to all v in X by both edges uv und vu
     *  -for efficient union, the following map tracks the references of a particular class ('mapClassToReferenced')
     */
	std::vector<int> mapClassToReferenced(mapClassToCliqueIs.size(), 0);

	for (int clazz = 0; clazz < mapClassToVertices.size(); clazz++)
	{
		// in the first loop we iterate the co-classes of all 1-half cliques Ii and all 0-half cliques Oj for the first time to mark the relation wrt. X
		std::vector<int> seenClasses;
		for (std::vector<int>::iterator clique = mapClassToCliqueOs[clazz].begin();
			 clique != mapClassToCliqueOs[clazz].end();
			 clique++)
			for (std::vector<int>::iterator seenClass = mapCliqueIToClasses[*clique].begin();
				 seenClass != mapCliqueIToClasses[*clique].end();
				 seenClass++)
				if (mapClassToReferenced[*seenClass]++ == 0)
					seenClasses.push_back(*seenClass);

		/*
         * finally we build the adjacency list of X
         * -we iterate through every seen class and all their vertices
         */
		std::vector<int> adjacencyList;
		for (std::vector<int>::iterator seenClass = seenClasses.begin();
			 seenClass != seenClasses.end();
			 seenClass++)
		{
			mapClassToReferenced[*seenClass] = 0;
			for (std::vector<int>::iterator vertex = mapClassToVertices[*seenClass].begin();
				 vertex != mapClassToVertices[*seenClass].end();
				 vertex++)
				adjacencyList.push_back(*vertex);
		}
		mapClassToAdjacencyList.push_back(adjacencyList);
	}
}

// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################

Petrinet *Petrinet::InitialNet = NULL;

Petrinet::Petrinet()
{
	preprocessingfinished = false;
	isSkeleton = false;
	name = deconst("the original net");
	Card = new arrayindex_t[2];
	memset(Card, 0, 2 * sizeof(arrayindex_t));
	CardArcs = new arrayindex_t **[2];
	CardArcs[PL] = new arrayindex_t *[2];
	CardArcs[TR] = new arrayindex_t *[2];
	memset(CardArcs[PL], 0, 2 * sizeof(arrayindex_t *));
	memset(CardArcs[TR], 0, 2 * sizeof(arrayindex_t *));
	Arc = new arrayindex_t ***[2];
	Arc[PL] = new arrayindex_t **[2];
	Arc[TR] = new arrayindex_t **[2];
	memset(Arc[PL], 0, 2 * sizeof(arrayindex_t **));
	memset(Arc[TR], 0, 2 * sizeof(arrayindex_t **));
	Mult = new mult_t ***[2];
	Mult[PL] = new mult_t **[2];
	Mult[TR] = new mult_t **[2];
	memset(Mult[PL], 0, 2 * sizeof(mult_t **));
	memset(Mult[TR], 0, 2 * sizeof(mult_t **));
	Name = new const char **[2];
	memset(Name, 0, 2 * sizeof(char **));
	ClusterCardPlaces = NULL;
	ClusterCardTransitions = NULL;
	ClusterTransitions = NULL;
	ClusterPlaces = NULL;
	cardClusters = 0;
	ClusterPlaceIndex = NULL;
	ClusterTransitionIndex = NULL;
	thesymbol = new Symbol ** [2];

	OnlyPre = new arrayindex_t **[2];
	memset(OnlyPre, 0, 2 * sizeof(arrayindex_t **));
	CardOnlyPre = new arrayindex_t *[2];
	memset(CardOnlyPre, 0, sizeof(arrayindex_t *));
	GreaterPre = new arrayindex_t **[2];
	memset(GreaterPre, 0, 2 * sizeof(arrayindex_t **));
	CardGreaterPre = new arrayindex_t *[2];
	memset(CardGreaterPre, 0, sizeof(arrayindex_t *));
	PreEqualPost = new arrayindex_t **[2];
	memset(PreEqualPost, 0, 2 * sizeof(arrayindex_t **));
	CardPreEqualPost = new arrayindex_t *[2];
	memset(CardPreEqualPost, 0, sizeof(arrayindex_t *));
	GreaterPost = new arrayindex_t **[2];
	memset(GreaterPost, 0, 2 * sizeof(arrayindex_t **));
	CardGreaterPost = new arrayindex_t *[2];
	memset(CardGreaterPost, 0, sizeof(arrayindex_t *));
	OnlyPost = new arrayindex_t **[2];
	memset(OnlyPost, 0, 2 * sizeof(arrayindex_t **));
	CardOnlyPost = new arrayindex_t *[2];
	memset(CardOnlyPost, 0, sizeof(arrayindex_t *));

	PlIncreasing = NULL;
	PlDecreasing = NULL;
	PlCardDecreasing = NULL;
	PlCardIncreasing = NULL;
	PlMultIncreasing = NULL;
	PlMultDecreasing = NULL;
	CardSignificant = 0;
	Hash = NULL;
	Capacity = NULL;
	CardBits = NULL;
	SizeOfBitVector = 0;
	Initial = NULL;
	HashInitial = 0;

	Fairness = NULL;
	Enabled = NULL;
	CardEnabled = 0;
	DeltaHash = NULL;
	CardDeltaT = new arrayindex_t *[2];
	memset(CardDeltaT, 0, 2 * sizeof(arrayindex_t *));
	DeltaT = new arrayindex_t **[2];
	memset(DeltaT, 0, 2 * sizeof(arrayindex_t **));
	MultDeltaT = new mult_t **[2];
	memset(MultDeltaT, 0, 2 * sizeof(mult_t **));
	ProgressMeasure = NULL;

	TrCardDecreasing = NULL;
	TrCardDecreased = NULL;
	TrDecreasing = NULL;
	TrDecreased = NULL;
	TrCardIncreased = NULL;
	TrIncreased = NULL;

	StubbornPriority = NULL;
	SingletonClusters = 0;

	cardbuchistates = -1;
}

Petrinet::~Petrinet()
{
	for (int type = PL; type <= TR; ++type)
	{
		for (arrayindex_t i = 0; i < Card[type]; i++)
		{
			delete[] Arc[type][PRE][i];  // Arc[PRE] and Arc[POST] are using a single allocated array
			delete[] Mult[type][PRE][i]; // same holds for Mult
		}
		for (int direction = PRE; direction <= POST; ++direction)
		{
			delete[] CardArcs[type][direction];
			delete[] Arc[type][direction];
			delete[] Mult[type][direction];
		}
		delete[] Name[type];
		delete [] thesymbol[type];
	}
	delete[] Hash;
	delete[] CardBits;
	delete[] Capacity;
	delete[] Initial;
	delete[] Fairness;

	delete[] Enabled;
	delete[] DeltaHash;
	delete[] thesymbol;

	for (int direction = PRE; direction <= POST; direction++)
	{
		for (arrayindex_t i = 0; i < Card[TR]; i++)
		{
			delete[] MultDeltaT[direction][i];
			delete[] DeltaT[direction][i];
		}
		delete[] DeltaT[direction];
		delete[] MultDeltaT[direction];
		delete[] CardDeltaT[direction];
	}

	delete[] ProgressMeasure;
}

/*!
prints the Net
*/
void Petrinet::print()
{
	printf("Net\n===\n\n");

	printf("%u places,  %u  transitions.\n\n", Card[PL], Card[TR]);

	for (arrayindex_t i = 0; i < Card[PL]; i++)
	{
		printf("PLACE %d %s sy %s: %u tokens capacity %u; %d PRE %d POST\n", i, Name[PL][i], thesymbol[PL][i]->getKey(),
			   Initial[i], Capacity[i], CardArcs[PL][PRE][i], CardArcs[PL][POST][i]);
		for(int j = 0; j < CardArcs[PL][PRE][i];j++)	
		{
			printf("\tPRE %s\n", Name[TR][Arc[PL][PRE][i][j]]);
		}
		for(int j = 0; j < CardArcs[PL][POST][i];j++)	
		{
			printf("\tPOST %s\n", Name[TR][Arc[PL][POST][i][j]]);
		}
	}

	for (arrayindex_t i = 0; i < Card[TR]; i++)
	{
		printf("\nTRANSITION %d :%s sy %s\n", i, Name[TR][i], thesymbol[TR][i]->getKey());
		switch (Fairness[i])
		{
		case NO_FAIRNESS:
			printf(" no ");
			break;
		case WEAK_FAIRNESS:
			printf(" weak ");
			break;
		case STRONG_FAIRNESS:
			printf(" strong ");
			break;
		}

		printf("fairness\nCONSUME:\n");
		for (arrayindex_t j = 0; j < CardArcs[TR][PRE][i]; j++)
		{
			printf("%s:%u ", Name[PL][Arc[TR][PRE][i][j]], Mult[TR][PRE][i][j]);
		}
		printf("\n");
		printf("PRODUCE:\n");
		for (arrayindex_t j = 0; j < CardArcs[TR][POST][i]; j++)
		{
			printf("%s:%u ", Name[PL][Arc[TR][POST][i][j]], Mult[TR][POST][i][j]);
		}
		printf("\n");
	}
	printf("done\n");
}

// two places left and right swap their indices. This change must be progated to all data structures that
// rely on place indices.
void Petrinet::swapPlaces(arrayindex_t left, arrayindex_t right)
{
	// 1. Net data structures

	const char *tempname = Name[PL][left];
	Name[PL][left] = Name[PL][right];
	Name[PL][right] = tempname;

	Symbol * tempsymbol = thesymbol[PL][left];
	thesymbol[PL][left] = thesymbol[PL][right];
	thesymbol[PL][right] = tempsymbol;

	// swap the data for left and right in the place based arrays of

	for (int direction = PRE; direction <= POST; ++direction)
	{
		arrayindex_t tempindex = CardArcs[PL][direction][left];
		CardArcs[PL][direction][left] = CardArcs[PL][direction][right];
		CardArcs[PL][direction][right] = tempindex;

		arrayindex_t *tempindexpointer = Arc[PL][direction][left];
		Arc[PL][direction][left] = Arc[PL][direction][right];
		Arc[PL][direction][right] = tempindexpointer;

		mult_t *tempmultpointer = Mult[PL][direction][left];
		Mult[PL][direction][left] = Mult[PL][direction][right];
		Mult[PL][direction][right] = tempmultpointer;
	}
	arrayindex_t tmp = PlCardDecreasing[left];
	PlCardDecreasing[left] = PlCardDecreasing[right];
	PlCardDecreasing[right] = tmp;

	arrayindex_t *ttmp = PlDecreasing[left];
	PlDecreasing[left] = PlDecreasing[right];
	PlDecreasing[right] = ttmp;

	mult_t *tttmp = PlMultDecreasing[left];
	PlMultDecreasing[left] = PlMultDecreasing[right];
	PlMultDecreasing[right] = tttmp;

	tmp = PlCardIncreasing[left];
	PlCardIncreasing[left] = PlCardIncreasing[right];
	PlCardIncreasing[right] = tmp;

	ttmp = PlIncreasing[left];
	PlIncreasing[left] = PlIncreasing[right];
	PlIncreasing[right] = ttmp;

	tttmp = PlMultIncreasing[left];
	PlMultIncreasing[left] = PlMultIncreasing[right];
	PlMultIncreasing[right] = tttmp;

	tmp = CardOnlyPre[PL][left];
	CardOnlyPre[PL][left] = CardOnlyPre[PL][right];
	CardOnlyPre[PL][right] = tmp;

	ttmp = OnlyPre[PL][left];
	OnlyPre[PL][left] = OnlyPre[PL][right];
	OnlyPre[PL][right] = ttmp;

	tmp = CardGreaterPre[PL][left];
	CardGreaterPre[PL][left] = CardGreaterPre[PL][right];
	CardGreaterPre[PL][right] = tmp;

	ttmp = GreaterPre[PL][left];
	GreaterPre[PL][left] = GreaterPre[PL][right];
	GreaterPre[PL][right] = ttmp;

	tmp = CardPreEqualPost[PL][left];
	CardPreEqualPost[PL][left] = CardPreEqualPost[PL][right];
	CardPreEqualPost[PL][right] = tmp;

	ttmp = PreEqualPost[PL][left];
	PreEqualPost[PL][left] = PreEqualPost[PL][right];
	PreEqualPost[PL][right] = ttmp;

	tmp = CardGreaterPost[PL][left];
	CardGreaterPost[PL][left] = CardGreaterPost[PL][right];
	CardGreaterPost[PL][right] = tmp;

	ttmp = GreaterPost[PL][left];
	GreaterPost[PL][left] = GreaterPost[PL][right];
	GreaterPost[PL][right] = ttmp;

	tmp = CardOnlyPost[PL][left];
	CardOnlyPost[PL][left] = CardOnlyPost[PL][right];
	CardOnlyPost[PL][right] = tmp;

	ttmp = OnlyPost[PL][left];
	OnlyPost[PL][left] = OnlyPost[PL][right];
	OnlyPost[PL][right] = ttmp;

	// the tricky part of 1.: references to left and right in transition arc lists

	// It is tricky because both places may refer to the same transition, so we do not
	// trivially know whether a change has already been done. Scanning through all transitions
	// rather than the environments of left and right is too costly, so we chose to go
	// through all transitions in the environment of one transition twice with setting temporary indices in the
	// first run.

	// Part of the trick is that all  information for place left is already at index right

	// first run left
	for (int direction = PRE; direction <= POST; ++direction)
	{
		for (arrayindex_t a = 0; a < CardArcs[PL][direction][right]; ++a)
		{
			direction_t otherdirection = (direction == PRE) ? POST : PRE;
			const arrayindex_t t = Arc[PL][direction][right][a];
			for (arrayindex_t b = 0; b < CardArcs[TR][otherdirection][t]; b++)
			{
				if (Arc[TR][otherdirection][t][b] == left)
				{
					Arc[TR][otherdirection][t][b] = Card[PL];
				}
			}
		}
	}
	// only run right
	for (int direction = PRE; direction <= POST; ++direction)
	{
		for (arrayindex_t a = 0; a < CardArcs[PL][direction][left]; ++a)
		{
			direction_t otherdirection = (direction == PRE) ? POST : PRE;
			const arrayindex_t t = Arc[PL][direction][left][a];
			for (arrayindex_t b = 0; b < CardArcs[TR][otherdirection][t]; b++)
			{
				if (Arc[TR][otherdirection][t][b] == right)
				{
					Arc[TR][otherdirection][t][b] = left;
				}
			}
		}
	}
	// second run left
	for (int direction = PRE; direction <= POST; ++direction)
	{
		for (arrayindex_t a = 0; a < CardArcs[PL][direction][right]; ++a)
		{
			direction_t otherdirection = (direction == PRE) ? POST : PRE;
			const arrayindex_t t = Arc[PL][direction][right][a];
			for (arrayindex_t b = 0; b < CardArcs[TR][otherdirection][t]; b++)
			{
				if (Arc[TR][otherdirection][t][b] == Card[PL])
				{
					Arc[TR][otherdirection][t][b] = right;
				}
			}
		}
	}

	// 2. Place data structures

	hash_t temphash = Hash[left];
	Hash[left] = Hash[right];
	Hash[right] = temphash;

	capacity_t tempcapacity = Capacity[left];
	Capacity[left] = Capacity[right];
	Capacity[right] = tempcapacity;

	cardbit_t tempcardbit = CardBits[left];
	CardBits[left] = CardBits[right];
	CardBits[right] = tempcardbit;
	// 3. Marking data structures

	capacity_t tempmarking = Initial[left];
	Initial[left] = Initial[right];
	Initial[right] = tempmarking;
}

template <typename T>
void Petrinet::sortarcs(arrayindex_t *indexarray, T *multarray, arrayindex_t card)
{
	if (card < 2)
		return;
	arrayindex_t pivot = indexarray[0];
	arrayindex_t blue = 0;
	arrayindex_t white = 1;
	arrayindex_t red = card;

	while (red > white)
	{
		if (indexarray[white] < pivot)
		{
			arrayindex_t tmp = indexarray[white];
			T tmpmult = multarray[white];
			indexarray[white] = indexarray[blue];
			multarray[white++] = multarray[blue];
			indexarray[blue] = tmp;
			multarray[blue++] = tmpmult;
		}
		else
		{
			// indexarray[white > pivot] (assume absence of duplicates in indexarray)
			arrayindex_t tmp = indexarray[white];
			T tmpmult = multarray[white];
			indexarray[white] = indexarray[--red];
			multarray[white] = multarray[red];
			indexarray[red] = tmp;
			multarray[red] = tmpmult;
		}
	}
	sortarcs<T>(indexarray, multarray, blue);
	sortarcs<T>(indexarray + red, multarray + red, card - red);
}
/*!
Creates an equation for the given transition (index) in the provided memory

\param[in] transition the transition for which the equation should be created and saved
\param[in,out] variables a empty array of place indexes, gets set by this function regarding to the places influenced by the given transition
\param[in,out] coefficients a empty array of coefficients, the values getting set by this function regarding the transition multiplicities torwards the corresponding places
\param[in,out] size gets set to the number of elements in the row
\param[in] dual signs if transitions or places should build the rows of the matrix
\post The array variables, the array coefficients and the variable size are set (the rowparts are defined)
 */
void Petrinet::createTransitionEquation(arrayindex_t transition, arrayindex_t *variables,
										long long int *coefficients,
										arrayindex_t &size, bool dual)
{
	// check if we have transition or places rows
	arrayindex_t ND(dual ? PL : TR);
	arrayindex_t mycard;
	arrayindex_t *mynode;
	mult_t *mymult;
	switch (ND)
	{
	case PL:
		// in this case, transition is actually a place index...
		mycard = PlCardIncreasing[transition];
		mynode = PlIncreasing[transition];
		mymult = PlMultIncreasing[transition];
		break;
	case TR:
		mycard = CardDeltaT[PRE][transition];
		mynode = DeltaT[PRE][transition];
		mymult = MultDeltaT[PRE][transition];
	}

	// index in new row
	size = 0;
	// for each place p in the preset of t
	for (arrayindex_t p = 0; p < mycard; ++p)
	{
		// store place index and the it's multiplicity (from p to t)
		variables[size] = mynode[p];
		coefficients[size++] = mymult[p];
	}

	switch (ND)
	{
	case PL:
		//in this case, transition is actually a place index...
		mycard = PlCardDecreasing[transition];
		mynode = PlDecreasing[transition];
		mymult = PlMultDecreasing[transition];
		break;
	case TR:
		mycard = CardDeltaT[POST][transition];
		mynode = DeltaT[POST][transition];
		mymult = MultDeltaT[POST][transition];
	}

	// for each place p in the postset of t
	for (arrayindex_t p = 0; p < mycard; ++p)
	{
		// store place index and the it's multiplicity (from p to t)
		variables[size] = mynode[p];
		coefficients[size++] = -mymult[p];
	}

	sortarcs<long long int>(variables, coefficients, size);
}

/*!
calculates a incidenceMatrix which contains information about the connections between transitions and places
\param line_type the type the columns in the matrix should have (implies the rowtype)
\pre arcs must be sorted
\return a incidence matrix for the net
*/
Matrix Petrinet::getIncidenceMatrix(node_t line_type)
{
	// the row type is the dual to the line type
	const node_t row_type = (line_type == PL) ? TR : PL;

	// get the cardinalities of lines and rows (the dimensions of the matrix)
	const arrayindex_t line_card = Card[line_type];
	const arrayindex_t row_card = Card[row_type];

	// request memory for one full row
	arrayindex_t *newVar = new arrayindex_t[line_card]();
	long long int *newCoef = new long long int[line_card]();
	arrayindex_t newSize;

	// create new matrix
	Matrix m(line_card);

	// load rows into matrix
	// for each transition t
	for (arrayindex_t t = 0; t < row_card; ++t)
	{
		// create equation for current transition
		createTransitionEquation(t, newVar, newCoef, newSize, (line_type == TR));
		// save current arrays as new row
		if (newSize > 0)
			m.addRow(newSize, newVar, newCoef);

		// clear used memory
		memset(newVar, 0, newSize * SIZEOF_ARRAYINDEX_T); // necessary?
		memset(newCoef, 0, newSize * sizeof(int));
	}

	// free memory
	delete[] newVar;
	delete[] newCoef;

	return m;
}

/*!
Calculates all signficant places and sorts them to the front of the place array
\pre the arcs must be sorted
\post the significant places are in the front of the place array
*/
void Petrinet::setSignificantPlaces()
{

	// save number of places
	const arrayindex_t cardPL = Card[PL];

	if (cardPL < 2)
	{
		// nothing to do
		CardSignificant = cardPL;
		return;
	}

	// get incidence matrix (places are lines)
	Matrix m = getIncidenceMatrix(PL);

	// reduce matrix

	if (!m.reduce())
	{
		// failed to calculate significant places (overflow)
		// --> set all places significant
		CardSignificant = cardPL;
		SizeOfBitVector = 0;
		for (arrayindex_t i = 0; i < CardSignificant; i++)
		{
			SizeOfBitVector += CardBits[i];
		}
		return;
	}

	// gather significant places
	CardSignificant = m.getSignificantColCount();
	arrayindex_t lastSignificant = cardPL - 1;
	arrayindex_t p = 0;
	while (p < CardSignificant)
	{
		if (!m.isSignificant(p))
		{
			// p needs to be swapped
			// find first significant place from the right end of all places
			while (!m.isSignificant(lastSignificant))
			{
				lastSignificant--;
			}
			// swap lastSignificant with p
			swapPlaces(p, lastSignificant--);
		}
		p++;
	}

	// adjust SizeOfBitVector
	SizeOfBitVector = 0;
	for (arrayindex_t i = 0; i < CardSignificant; i++)
	{
		SizeOfBitVector += CardBits[i];
	}
}

/*!
Calculates the progress measure for all transitions
\pre arcs must be orderd
\post ProgressMeasure is calculated
*/
void Petrinet::setProgressMeasure()
{
	// arcs must be sorted

	// get incidence matrix (places are lines)
	Matrix m = getIncidenceMatrix(TR);

	// reduce matrix
	m.diagonalise();

	// save number of transitions
	const arrayindex_t cardTR = Card[TR];

	// calculate progress measure
	int *progressMeasure = new int[cardTR]();
	for (arrayindex_t t = 0; t < cardTR; ++t)
		if (m.isSignificant(t))
		{
			const Matrix::Row *curRow = m.getRow(t);
			progressMeasure[curRow->variables[0]] = curRow->coefficients[0];
			for (arrayindex_t v = 1; v < curRow->varCount; ++v)
				if (!m.isSignificant(curRow->variables[v]))
				{
					progressMeasure[curRow->variables[v]] += curRow->coefficients[v];
				}
		}

	// try for a local optimisation (reduce number of transitions with negative progress)
	while (true)
	{
		arrayindex_t tindex(ARRAYINDEX_T_MAX);
		int changes(-1), ctmp;

		for (arrayindex_t t = 0; t < cardTR; ++t)
			if (m.isSignificant(t))
			{
				ctmp = 0;
				const Matrix::Row *curRow = m.getRow(t);
				for (arrayindex_t v = 0; v < curRow->varCount; ++v)
					if (curRow->coefficients[v] < 0 && progressMeasure[curRow->variables[v]] < 0)
					{
						if (2 * curRow->coefficients[v] <= progressMeasure[curRow->variables[v]])
						{
							++ctmp;
						}
					}
					else if (curRow->coefficients[v] > 0 && progressMeasure[curRow->variables[v]] > 0)
						if (2 * curRow->coefficients[v] > progressMeasure[curRow->variables[v]])
						{
							--ctmp;
						}
				if (ctmp > changes)
				{
					if (ctmp > 0 || curRow->coefficients[0] > 0)
					{
						changes = ctmp;
						tindex = t;
					}
				}
			}
		if (tindex == ARRAYINDEX_T_MAX)
		{
			break;
		}

		const Matrix::Row *curRow = m.getRow(tindex);
		for (arrayindex_t v = 0; v < curRow->varCount; ++v)
		{
			progressMeasure[curRow->variables[v]] -= 2 * curRow->coefficients[v];
			curRow->coefficients[v] -= curRow->coefficients[v];
		}
	}

	if (RT::args.sweeplinespread_arg > 1)
	{
		// try for another local optimisation (spread progress values better)
		// first, save the progress measures so far
		int *progressCopy = new int[cardTR];
		memcpy(progressCopy, progressMeasure, cardTR * sizeof(int));

		arrayindex_t threads(RT::args.threads_arg), tries(cardTR), fullbucket(cardTR / threads + 1),
			maxbucket(cardTR);
		std::set<arrayindex_t> done;
		while (--tries > 0)
		{
			std::map<int, arrayindex_t> pbuckets;
			arrayindex_t highbucket(1);
			for (arrayindex_t t = 0; t < cardTR; ++t)
				if (++pbuckets[progressMeasure[t]] > highbucket)
				{
					++highbucket;
				}
			if (fullbucket * fullbucket > cardTR)
			{
				--fullbucket;
			}
			if (highbucket <= maxbucket)
			{
				maxbucket = highbucket - 1;
			}
			if (maxbucket <= fullbucket)
			{
				break;
			}
			std::set<int> tryvalues;
			for (std::map<int, arrayindex_t>::iterator it = pbuckets.begin(); it != pbuckets.end(); ++it)
				if (it->second > maxbucket)
				{
					tryvalues.insert(it->first);
				}
			/*
            		std::cout << "progress values with too many transitions(" << maxbucket << "," << fullbucket << "): ";
            		for(std::set<int>::iterator it=tryvalues.begin(); it!=tryvalues.end(); ++it)
            			std::cout << *it << "(" << pbuckets[*it] << ") ";
            		std::cout << std::endl;
            */
			arrayindex_t t;
			for (t = 0; t < cardTR; ++t)
				if (m.isSignificant(t))
				{
					if (done.find(t) != done.end())
					{
						continue;
					}
					const Matrix::Row *curRow = m.getRow(t);
					if (tryvalues.find(curRow->coefficients[0]) == tryvalues.end())
					{
						continue;
					}

					int mult;
					for (mult = 2; mult <= RT::args.sweeplinespread_arg; ++mult)
					{
						int ctmp = 0;
						for (arrayindex_t v = 0; v < curRow->varCount; ++v)
							if (curRow->coefficients[v] > 0 && progressMeasure[curRow->variables[v]] < 0)
							{
								if (progressMeasure[curRow->variables[v]] + (mult - 1) * curRow->coefficients[v] >= 0)
								{
									++ctmp;
								}
							}
							else if (curRow->coefficients[v] < 0 && progressMeasure[curRow->variables[v]] > 0)
								if (progressMeasure[curRow->variables[v]] + (mult - 1) * curRow->coefficients[v] < 0)
								{
									--ctmp;
								}
						if (ctmp < 0)
						{
							continue;
						}

						int toofull(0);
						for (arrayindex_t v = 0; v < curRow->varCount; ++v)
						{
							if (pbuckets[progressMeasure[curRow->variables[v]]] > fullbucket)
							{
								--toofull;
							}
							if (pbuckets[progressMeasure[curRow->variables[v]] + (mult - 1) * curRow->coefficients[v]] >
								fullbucket)
							{
								++toofull;
							}
						}
						if (toofull >= 0)
						{
							continue;
						}

						done.insert(t);

						for (arrayindex_t v = 0; v < curRow->varCount; ++v)
						{
							progressMeasure[curRow->variables[v]] += (mult - 1) * curRow->coefficients[v];
							curRow->coefficients[v] *= mult;
						}

						break;
					}
					if (mult <= RT::args.sweeplinespread_arg)
					{
						break;
					}
				}
			if (t == cardTR)
			{
				++fullbucket;
			}
		}

		// check if the optimisation uses the buckets in a better way (especially more buckets)
		std::set<int> oldcnt, newcnt;
		for (arrayindex_t t = 0; t < cardTR; ++t)
		{
			oldcnt.insert(progressCopy[t]);
			newcnt.insert(progressMeasure[t]);
		}
		if (newcnt.size() <= oldcnt.size())
		{
			memcpy(progressMeasure, progressCopy, cardTR * sizeof(int));
		}
		else
		{
			float oldrange, newrange, oldsize, newsize;
			oldrange = static_cast<float>(*(oldcnt.rbegin()) - *(oldcnt.begin()) + 1);
			newrange = static_cast<float>(*(newcnt.rbegin()) - *(newcnt.begin()) + 1);
			oldsize = static_cast<float>(oldcnt.size());
			newsize = static_cast<float>(newcnt.size());
			//std::cout << "oldrange=" << oldrange << " newrange=" << newrange << " oldsize=" << oldsize << " newsize=" << newsize << std::endl;
			if (newrange / oldrange >= newsize * newsize / (oldsize * oldsize))
			{
				memcpy(progressMeasure, progressCopy, cardTR * sizeof(int));
			}
			else
			{
				std::cout << "progress adapted" << std::endl;
			}
		}

		delete[] progressCopy;
	}

	// remove gcd from progress values
	int gcd(0);
	for (arrayindex_t t = 0; t < cardTR; ++t)
		if (gcd == 0)
		{
			gcd = progressMeasure[t];
		}
		else if (progressMeasure[t] != 0)
		{
			gcd = ggt(gcd, progressMeasure[t]);
		}
	if (gcd < 0)
	{
		gcd = -gcd;
	}
	if (gcd != 0)
		for (arrayindex_t t = 0; t < cardTR; ++t)
		{
			progressMeasure[t] /= gcd;
		}

	// cast progress measures to 32bit
	ProgressMeasure = new int[cardTR]();
	for (arrayindex_t t = 0; t < cardTR; ++t)
	{
		ProgressMeasure[t] = (int)progressMeasure[t];
	}

	// free memory
	delete[] progressMeasure;
}

/*!
calculates DeltaT and DeltaHash for each transition
\post DeltaT, MultDeltaT and CardDeltaT (PRE and POST) are set for each transition
*/
void Petrinet::preprocess_organizeDeltas()
{
	for (arrayindex_t t = 0; t < Card[TR]; t++)
	{

		// step 2: count duplicates
		//         remember that Arc is just one consecutive array for PRE and POST
		arrayindex_t *aarray = Arc[TR][PRE][t];
		mult_t *oldpre = Mult[TR][PRE][t];
		mult_t *oldpost = Mult[TR][POST][t];

		arrayindex_t ppre = 0;					   // current place in PRE array
		arrayindex_t ppost = CardArcs[TR][PRE][t]; // start of POST  area
		arrayindex_t cardloop = 0;
		while (ppre < CardArcs[TR][PRE][t] && ppost < CardArcs[TR][PRE][t] + CardArcs[TR][POST][t])
		{
			if (aarray[ppre] < aarray[ppost])
			{
				ppre++;
				continue;
			}
			if (aarray[ppre] > aarray[ppost])
			{
				ppost++;
				continue;
			}
			// arriving here: detected a loop place
			cardloop++;
			ppre++;
			ppost++;
		}
		if (cardloop == 0)
		{
			// no complicated re-arrangements are necessary
			DeltaT[PRE][t] = Arc[TR][PRE][t];
			CardDeltaT[PRE][t] = CardArcs[TR][PRE][t];
			MultDeltaT[PRE][t] = Mult[TR][PRE][t];
			DeltaT[POST][t] = Arc[TR][POST][t];
			CardDeltaT[POST][t] = CardArcs[TR][POST][t];
			MultDeltaT[POST][t] = Mult[TR][POST][t];
			OnlyPre[TR][t] = Arc[TR][PRE][t];
			CardOnlyPre[TR][t] = CardArcs[TR][PRE][t];
			GreaterPre[TR][t] = Arc[TR][POST][t];
			PreEqualPost[TR][t] = Arc[TR][POST][t];
			GreaterPost[TR][t] = Arc[TR][POST][t];
			OnlyPost[TR][t] = Arc[TR][POST][t];
			CardGreaterPre[TR][t] = 0;
			CardPreEqualPost[TR][t] = 0;
			CardGreaterPost[TR][t] = 0;
			CardOnlyPost[TR][t] = CardArcs[TR][POST][t];
			continue;
		}
		// step 3: allocate new arrays
		arrayindex_t *newarc = new arrayindex_t[CardArcs[TR][PRE][t] + CardArcs[TR][POST][t] - cardloop];
		mult_t *newpre = new mult_t[CardArcs[TR][PRE][t] + CardArcs[TR][POST][t]];
		mult_t *newpost = newpre + CardArcs[TR][PRE][t];
		// step 4: fill new arrays

		arrayindex_t oldarcpre = 0;
		arrayindex_t oldarcpost = CardArcs[TR][PRE][t];
		arrayindex_t newarcpre = 0;
		arrayindex_t newarcloop = oldarcpost - cardloop;
		arrayindex_t newarcpost = oldarcpost;

		while (oldarcpre < CardArcs[TR][PRE][t] && oldarcpost < CardArcs[TR][PRE][t] + CardArcs[TR][POST][t])
		{
			if (aarray[oldarcpre] < aarray[oldarcpost])
			{
				// pure pre-place
				newarc[newarcpre] = aarray[oldarcpre];
				newpre[newarcpre++] = oldpre[oldarcpre++];
				continue;
			}
			if (aarray[oldarcpre] > aarray[oldarcpost])
			{
				// pure post-place
				newarc[newarcpost] = aarray[oldarcpost];
				newpre[newarcpost++ + cardloop] = oldpre[oldarcpost++];
				continue;
			}
			// arriving here: detected a loop place
			newarc[newarcloop] = aarray[oldarcpre]; // == aarray[oldarcpost]
			newpre[newarcloop] = oldpre[oldarcpre++];
			newpre[newarcloop++ + cardloop] = oldpre[oldarcpost++];
		}

		while (oldarcpre < CardArcs[TR][PRE][t])
		{
			// pure pre-place
			newarc[newarcpre] = aarray[oldarcpre];
			newpre[newarcpre++] = oldpre[oldarcpre++];
		}
		while (oldarcpost < CardArcs[TR][PRE][t] + CardArcs[TR][POST][t])
		{
			// pure post-place
			newarc[newarcpost] = aarray[oldarcpost];
			newpre[newarcpost++ + cardloop] = oldpre[oldarcpost++];
		}

		// step 4: switch to new arrays
		delete[] aarray;
		delete[] oldpre;
		Arc[TR][PRE][t] = newarc;
		Arc[TR][POST][t] = newarc + CardArcs[TR][PRE][t] - cardloop;
		Mult[TR][PRE][t] = newpre;
		Mult[TR][POST][t] = newpost;

		OnlyPre[TR][t] = Arc[TR][PRE][t];
		CardOnlyPre[TR][t] = CardArcs[TR][PRE][t] - cardloop;
		OnlyPost[TR][t] = Arc[TR][PRE][t] + CardArcs[TR][PRE][t];
		CardOnlyPost[TR][t] = CardArcs[TR][POST][t] - cardloop;

		// step 5: separate loop places into three parts:
		// 1. W(p,t) > W(t,p)
		// 2. W(p,t) = W(t,p)
		// 3. W(p,t) < W(t,p)

		aarray = Arc[TR][PRE][t];
		arrayindex_t blue = CardArcs[TR][PRE][t] - cardloop;
		arrayindex_t base = blue;
		arrayindex_t white = blue;
		arrayindex_t red = CardArcs[TR][PRE][t];
		while (white < red)
		{
			int diff = newpre[white] - newpost[white - base];
			if (diff > 0)
			{
				// swap blue and white
				arrayindex_t tmp = aarray[white];
				aarray[white] = aarray[blue];
				aarray[blue] = tmp;
				mult_t tmppre = newpre[white];
				newpre[white] = newpre[blue];
				newpre[blue] = tmppre;
				mult_t tmppost = newpost[white - base];
				newpost[white++ - base] = newpost[blue - base];
				newpost[blue++ - base] = tmppost;
				continue;
			}
			if (diff < 0)
			{
				// swap white and red-1
				arrayindex_t tmp = aarray[white];
				aarray[white] = aarray[--red];
				aarray[red] = tmp;
				mult_t tmppre = newpre[white];
				newpre[white] = newpre[red];
				newpre[red] = tmppre;
				mult_t tmppost = newpost[white - base];
				newpost[white - base] = newpost[red - base];
				newpost[red - base] = tmppost;
				continue;
			}
			// here: diff == 0
			white++;
		}
		GreaterPre[TR][t] = Arc[TR][PRE][t] + base;
		PreEqualPost[TR][t] = Arc[TR][PRE][t] + blue;
		GreaterPost[TR][t] = Arc[TR][PRE][t] + red;
		CardGreaterPre[TR][t] = blue - base;
		CardPreEqualPost[TR][t] = red - blue;
		CardGreaterPost[TR][t] = CardArcs[TR][PRE][t] - red;

		// step 6: extract DeltaT!
		DeltaT[PRE][t] = OnlyPre[TR][t]; // array shared with Arc[TR]
		CardDeltaT[PRE][t] = CardOnlyPre[TR][t] + CardGreaterPre[TR][t];
		DeltaT[POST][t] = GreaterPost[TR][t];
		CardDeltaT[POST][t] = CardGreaterPost[TR][t] + CardOnlyPost[TR][t];
		MultDeltaT[PRE][t] = new mult_t[CardDeltaT[PRE][t] + CardDeltaT[POST][t]];
		MultDeltaT[POST][t] = MultDeltaT[PRE][t] + CardDeltaT[PRE][t];

		// step 7: extract MultDeltaT

		// pre/pure places

		for (arrayindex_t i = 0; i < CardOnlyPre[TR][t]; i++)
		{
			MultDeltaT[PRE][t][i] = newpre[i];
		}
		// pre/loop places
		for (arrayindex_t i = 0; i < CardGreaterPre[TR][t]; i++)
		{
			MultDeltaT[PRE][t][i + CardOnlyPre[TR][t]] = newpre[i + CardOnlyPre[TR][t]] - newpost[i];
		}
		// post/loop places
		for (arrayindex_t i = 0; i < CardGreaterPost[TR][t]; i++)
		{
			MultDeltaT[POST][t][i] = newpost[i + CardGreaterPre[TR][t] + CardPreEqualPost[TR][t]] - newpre[i + CardOnlyPre[TR][t] + CardGreaterPre[TR][t] + CardPreEqualPost[TR][t]];
		}
		// post/pure places
		for (arrayindex_t i = 0; i < CardOnlyPost[TR][t]; i++)
		{
			MultDeltaT[POST][t][i + CardGreaterPost[TR][t]] = newpost[i + CardGreaterPre[TR][t] + CardPreEqualPost[TR][t] + CardGreaterPost[TR][t]];
		}
	}

	/*********************
    * 7b. Set DeltaHash *
    **********************/

	memset(DeltaHash, 0, Card[TR] * sizeof(hash_t));
	for (arrayindex_t t = 0; t < Card[TR]; t++)
	{
		for (arrayindex_t i = 0; i < CardDeltaT[PRE][t]; i++)
		{
			DeltaHash[t] = (DeltaHash[t] - MultDeltaT[PRE][t][i] *
											   Hash[DeltaT[PRE][t][i]]) %
						   SIZEOF_MARKINGTABLE;
		}
		for (arrayindex_t i = 0; i < CardDeltaT[POST][t]; i++)
		{
			DeltaHash[t] = (DeltaHash[t] + MultDeltaT[POST][t][i] *
											   Hash[DeltaT[POST][t][i]]) %
						   SIZEOF_MARKINGTABLE;
		}
	}

	// same for places
	for (arrayindex_t p = 0; p < Card[PL]; p++)
	{

		// step 2: count duplicates
		//         remember that Arc is just one consecutive array for PRE and POST
		arrayindex_t *aarray = Arc[PL][PRE][p];
		mult_t *oldpre = Mult[PL][PRE][p];
		mult_t *oldpost = Mult[PL][POST][p];

		arrayindex_t ppre = 0;					   // current transition in PRE array
		arrayindex_t ppost = CardArcs[PL][PRE][p]; // start of POST  area
		arrayindex_t cardloop = 0;
		while (ppre < CardArcs[PL][PRE][p] && ppost < CardArcs[PL][PRE][p] + CardArcs[PL][POST][p])
		{
			if (aarray[ppre] < aarray[ppost])
			{
				ppre++;
				continue;
			}
			if (aarray[ppre] > aarray[ppost])
			{
				ppost++;
				continue;
			}
			// arriving here: detected a loop trans
			cardloop++;
			ppre++;
			ppost++;
		}

		// if cardloop == 0, Pre = Increasing and Post = Decreasing
		if (cardloop == 0)
		{
			PlIncreasing[p] = Arc[PL][PRE][p];
			PlCardIncreasing[p] = CardArcs[PL][PRE][p];
			PlMultIncreasing[p] = Mult[PL][PRE][p];
			PlDecreasing[p] = Arc[PL][POST][p];
			PlCardDecreasing[p] = CardArcs[PL][POST][p];
			PlMultDecreasing[p] = Mult[PL][POST][p];
			OnlyPre[PL][p] = Arc[PL][PRE][p];
			CardOnlyPre[PL][p] = CardArcs[PL][PRE][p];
			OnlyPost[PL][p] = Arc[PL][POST][p];
			CardOnlyPost[PL][p] = CardArcs[PL][POST][p];
			GreaterPre[PL][p] = Arc[PL][POST][p];
			CardGreaterPre[PL][p] = 0;
			GreaterPost[PL][p] = Arc[PL][POST][p];
			CardGreaterPost[PL][p] = 0;
			PreEqualPost[PL][p] = Arc[PL][POST][p];
			CardPreEqualPost[PL][p] = 0;
			continue;
		}
		// step 3: allocate new arrays
		arrayindex_t *newarc = new arrayindex_t[CardArcs[PL][PRE][p] + CardArcs[PL][POST][p] - cardloop];
		mult_t *newpre = new mult_t[CardArcs[PL][PRE][p] + CardArcs[PL][POST][p]];
		mult_t *newpost = newpre + CardArcs[PL][PRE][p];
		// step 4: fill new arrays

		arrayindex_t oldarcpre = 0;
		arrayindex_t oldarcpost = CardArcs[PL][PRE][p];
		arrayindex_t newarcpre = 0;
		arrayindex_t newarcloop = oldarcpost - cardloop;
		arrayindex_t newarcpost = oldarcpost;

		while (oldarcpre < CardArcs[PL][PRE][p] && oldarcpost < CardArcs[PL][PRE][p] + CardArcs[PL][POST][p])
		{
			if (aarray[oldarcpre] < aarray[oldarcpost])
			{
				// pure pre-trans
				newarc[newarcpre] = aarray[oldarcpre];
				newpre[newarcpre++] = oldpre[oldarcpre++];
				continue;
			}
			if (aarray[oldarcpre] > aarray[oldarcpost])
			{
				// pure post-trans
				newarc[newarcpost] = aarray[oldarcpost];
				newpre[newarcpost++ + cardloop] = oldpre[oldarcpost++];
				continue;
			}
			// arriving here: detected a loop trans
			newarc[newarcloop] = aarray[oldarcpre]; // == aarray[oldarcpost]
			newpre[newarcloop] = oldpre[oldarcpre++];
			newpre[newarcloop++ + cardloop] = oldpre[oldarcpost++];
		}
		while (oldarcpre < CardArcs[PL][PRE][p])
		{
			// pure pre-trans
			newarc[newarcpre] = aarray[oldarcpre];
			newpre[newarcpre++] = oldpre[oldarcpre++];
		}
		while (oldarcpost < CardArcs[PL][PRE][p] + CardArcs[PL][POST][p])
		{
			// pure post-trans
			newarc[newarcpost] = aarray[oldarcpost];
			newpre[newarcpost++ + cardloop] = oldpre[oldarcpost++];
		}

		// step 4: switch to new arrays
		delete[] aarray;
		delete[] oldpre;
		Arc[PL][PRE][p] = newarc;
		Arc[PL][POST][p] = newarc + CardArcs[PL][PRE][p] - cardloop;
		Mult[PL][PRE][p] = newpre;
		Mult[PL][POST][p] = newpost;

		OnlyPre[PL][p] = Arc[PL][PRE][p];
		CardOnlyPre[PL][p] = CardArcs[PL][PRE][p] - cardloop;
		OnlyPost[PL][p] = Arc[PL][POST][p] + cardloop;
		CardOnlyPost[PL][p] = CardArcs[PL][POST][p] - cardloop;

		// step 5: separate loop places into three parts:
		// 1. W(p,t) > W(t,p)
		// 2. W(p,t) = W(t,p)
		// 3. W(p,t) < W(t,p)

		aarray = Arc[PL][PRE][p];
		arrayindex_t blue = CardArcs[PL][PRE][p] - cardloop;
		arrayindex_t base = blue;
		arrayindex_t white = blue;
		arrayindex_t red = CardArcs[PL][PRE][p];
		while (white < red)
		{
			int diff = newpre[white] - newpost[white - base];
			if (diff > 0)
			{
				// swap blue and white
				arrayindex_t tmp = aarray[white];
				aarray[white] = aarray[blue];
				aarray[blue] = tmp;
				mult_t tmppre = newpre[white];
				newpre[white] = newpre[blue];
				newpre[blue] = tmppre;
				mult_t tmppost = newpost[white - base];
				newpost[white++ - base] = newpost[blue - base];
				newpost[blue++ - base] = tmppost;
				continue;
			}
			if (diff < 0)
			{
				// swap white and red-1
				arrayindex_t tmp = aarray[white];
				aarray[white] = aarray[--red];
				aarray[red] = tmp;
				mult_t tmppre = newpre[white];
				newpre[white] = newpre[red];
				newpre[red] = tmppre;
				mult_t tmppost = newpost[white - base];
				newpost[white - base] = newpost[red - base];
				newpost[red - base] = tmppost;
				continue;
			}
			// here: diff == 0
			white++;
		}

		GreaterPre[PL][p] = Arc[PL][PRE][p] + base;
		PreEqualPost[PL][p] = Arc[PL][PRE][p] + blue;
		GreaterPost[PL][p] = Arc[PL][PRE][p] + red;
		CardGreaterPre[PL][p] = blue - base;
		CardPreEqualPost[PL][p] = red - blue;
		CardGreaterPost[PL][p] = CardArcs[PL][PRE][p] - red;

		// step 6: extract Increasing and Decreasing!
		PlIncreasing[p] = OnlyPre[PL][p]; // array shared with Arc[PL]
		PlCardIncreasing[p] = CardOnlyPre[PL][p] + CardGreaterPre[PL][p];
		PlDecreasing[p] = GreaterPost[PL][p];
		PlCardDecreasing[p] = CardGreaterPost[PL][p] + CardOnlyPost[PL][p];
		PlMultIncreasing[p] = new mult_t[PlCardIncreasing[p] + PlCardDecreasing[p]];
		PlMultDecreasing[p] = PlMultIncreasing[p] + PlCardIncreasing[p];
		// step 7: extract Mult...

		// pre/pure place

		for (arrayindex_t i = 0; i < CardOnlyPre[PL][p]; i++)
		{
			PlMultIncreasing[p][i] = newpre[i];
		}
		// pre/loop place
		for (arrayindex_t i = 0; i < CardGreaterPre[PL][p]; i++)
		{
			PlMultIncreasing[p][i] = newpre[i + CardOnlyPre[PL][p]] - newpost[i];
		}
		// post/loop trans
		for (arrayindex_t i = 0; i < CardGreaterPost[PL][p]; i++)
		{
			PlMultDecreasing[p][i] = newpost[i + CardGreaterPre[PL][p] + CardPreEqualPost[PL][p]] - newpre[i + CardOnlyPre[PL][p] + CardGreaterPre[PL][p] + CardPreEqualPost[PL][p]];
		}
		// post/pure trans
		for (arrayindex_t i = 0; i < CardOnlyPost[PL][p]; i++)
		{
			PlMultDecreasing[p][i + CardGreaterPost[PL][p]] = newpost[i + CardGreaterPre[PL][p] + CardPreEqualPost[PL][p] + CardGreaterPost[PL][p]];
		}
	}
}

/*!
 auxiliary comparator object needed for efficient conflict set caching
*/
struct conflictset_comparator
{
	explicit conflictset_comparator(arrayindex_t _len) : len(_len) {}
	arrayindex_t len;
	//TODO comment me
	bool operator()(arrayindex_t *const &a, arrayindex_t *const &b) const
	{
		return (memcmp(a, b, len * SIZEOF_ARRAYINDEX_T) < 0);
	}
};

/*!
 copies all elements in the range [first1,last1), that are also in [first2,last2), to result.
\param first1 a arrayindex_t pointer which marks the begin (included) of the first array to be checked
\param last1 a arrayindex_t pointer which marks the (excluded) end of the second array to be checked
\param first2 a arrayindex_t pointer which marks the begin (included) of the second array to be checked
\param last2 a arrayindex_t pointer which marks the (excluded) end of the second array to be checked
\param[in,out] result a pointer where the elements get stored wich are contained in both arrays
\return returns the number of elements copied.
*/
arrayindex_t Petrinet::set_moveall(arrayindex_t *first1, arrayindex_t *last1,
								   arrayindex_t *first2, arrayindex_t *last2,
								   arrayindex_t *result)
{
	arrayindex_t *res = result;
	arrayindex_t *retain = first1;
	arrayindex_t *lb = first1;
	while (first1 != last1 && first2 != last2)
	{
		lb = std::lower_bound(lb, last1, *first2);
		if (lb == last1)
		{
			break;
		}
		if (*lb == *first2++)
		{
			memmove(retain, first1, (lb - first1) * SIZEOF_ARRAYINDEX_T);
			retain += lb - first1;
			*res++ = *lb++;
			first1 = lb;
		}
	}
	memmove(retain, first1, (last1 - first1) * SIZEOF_ARRAYINDEX_T);
	return res - result;
}

/// sort a duplicate-free array
void qsort(arrayindex_t *feld, arrayindex_t size)
{
	if (size <= 1)
		return;
	arrayindex_t blue = 0;
	arrayindex_t white = 1;
	arrayindex_t red = size;
	arrayindex_t pivot = feld[0];
	while (white < red)
	{
		if (feld[white] < pivot)
		{
			arrayindex_t tmp = feld[white];
			feld[white++] = feld[blue];
			feld[blue++] = tmp;
		}
		else
		{
			// feld[white] > pivot
			arrayindex_t tmp = feld[white];
			feld[white] = feld[--red];
			feld[red] = tmp;
		}
	}
	qsort(feld, blue);
	qsort(feld + red, size - red);
}

typedef enum
{
	DECREASED = 1,
	MUTUAL = 2,
	DECREASING = 3
} tConf;

/// compute, for each t, the set decreasing(t) (set of all t' the may disable t)
/// and the set decreasedBy(t) (set of all t' that can be disabled by t)
/// Both sets are (in general not disjoint) subsets of post(pre(t))
/// The result is a single array with three section:
/// | onlyDecreasedBy(t) | mutually decreasing | onlyDecreasing(t) |
///
/// we assume that conflict clusters have already been computed
void Petrinet::computeConflictingTransitionsOld()
{
	// both sets decreasing(t) and decreasedBy(t) are subsets of the conflict cluster of t
	// Hence, we proceed cluster by cluster (can eventually be parallelised)

	for (arrayindex_t cl = 0; cl < cardClusters; cl++)
	{
		// We proceed place by place for all places in the conflict cluster.
		// Having processed some set Q of places:
		// The main data structure is a set of tuples (Decreased_i, Mutual_i, Decreasing_i,T_i).
		// Its semantics is: for every t in Ti,
		// - Decreased_i is the set of all t' decreased by t but not decreasing t on any place in Q
		// - Mutual_i is the set of all t' decreased by t and decreasing t on any place in Q
		// - Decreasing_i is the set of all t' not decreased by t but decreasing t on any place in Q

		// Processing another place p yields
		// - a set Decr = {t in post(p) | W(p,t) > W(t,p)} and
		// - a set Nodecr = {t in post(p) | W(p,t) <= W(t,p)}
		// Using p, the tuple (Decreased_i, Mutual_i, Decreasing_i,T_i) is split into three new tuples:
		// 1. (Decreased_i, Mutual_i, Decreasing_i,T_i \ post(p))
		//               (for transitions not in post(p), nothing changes)
		// 2. ((Decreased_i \ Decr) cup (Nodecr \ (Mutual_i cup Decreasing_i)),
		//       Mutual_i cup Decr cup (Decreasing_i cap Nodecr),
		//       Decreasing_i \ post(p),
		//       T_i cap Decr)
		//			(the new tuple for transitions that decrease p)
		// 3. (Decreased_i \ Decr,
		//	       Mutual_i cup (Decreased_i cap Decr),
		//              Decreasing_i cup (Decr \ (Mutual_i cup (Decreased_i))),
		//	       T_i cap Nodecr)
		//			(the new tuple for transitions that do not decrease p)
		//
		// Consequently, {T1,....,Tn} is a partition of the transitions in CL
		// Further, for all i, Decreased_i, Mutual_i, and Decreasing_i are disjoint.

		// If, finally, card(T_i) > 1, the transitions in T_i share the same physical
		// array for Decreasing and Decreased.

		// Initially, we start with a single tuple (emptyset,emptyset,emptyset,Transitions(CL)).

		arrayindex_t stackpointer = 1;								  // the number of sets, initially one
		arrayindex_t card = ClusterCardTransitions[cl];				  // max. number of tuples
		arrayindex_t **conflictset = new arrayindex_t *[card + 2];	// conflictset[i] carries the first 3 cmp. of a tuple
		arrayindex_t **transitionset = new arrayindex_t *[card + 2];  // transitionset[i] carries the 4th cmp. of a tuple
		arrayindex_t *cardconflictset = new arrayindex_t[card + 2];   // size of the first three components
		tConf **conflicttype = new tConf *[card + 2];				  // records membership in the three cmp.
		arrayindex_t *cardtransitionset = new arrayindex_t[card + 2]; // size of the 4th cmp.
		arrayindex_t *transitionindex = new arrayindex_t[Card[TR]];   // for each t, the unique i with t in its 4th comp.
		arrayindex_t *cardmutual = new arrayindex_t[card + 2];		  // number of MUTUAL entries in conflict set
		conflictset[0] = NULL;
		conflicttype[0] = NULL;
		cardconflictset[0] = 0;
		cardtransitionset[0] = card;
		cardmutual[0] = 0;

		transitionset[0] = new arrayindex_t[card];
		memcpy(transitionset[0], ClusterTransitions[cl], sizeof(arrayindex_t) * ClusterCardTransitions[cl]);
		memset(transitionindex, 0, sizeof(arrayindex_t) * Card[TR]); // for all t: transitionsindex[t] =0;
		qsort(transitionset[0], card);
		// now, process place by place
		for (arrayindex_t clp = 0; clp < ClusterCardPlaces[cl]; clp++)
		{
			arrayindex_t p = ClusterPlaces[cl][clp];

			// retrieve Decr and Nodecr;
			arrayindex_t cardDecr = PlCardDecreasing[p];
			arrayindex_t cardNodecr = CardArcs[PL][POST][p] - cardDecr;
			arrayindex_t *Nodecr = new arrayindex_t[cardDecr + cardNodecr];
			memcpy(Nodecr, Arc[PL][POST][p], (cardDecr + cardNodecr) * sizeof(arrayindex_t));
			arrayindex_t *Decr = Nodecr + cardNodecr;
			qsort(Decr, cardDecr);
			qsort(Nodecr, cardNodecr);

			arrayindex_t sp = stackpointer; // the number of pairs before starting to split
											// This avoids processing the new pairs obtained
											// in this round of splitting again.
			for (arrayindex_t i = 0; i < sp; i++)
			{
				// fork tuple of type 2

				// compute new 4th component
				arrayindex_t *newconflictset;
				tConf *newconflicttype;
				arrayindex_t *newtransitionset = new arrayindex_t[cardtransitionset[i]];
				arrayindex_t oldtindex = 0; // old transition set
				arrayindex_t oldpindex = 0; // Decr list
				arrayindex_t newpindex = 0; // new forked transition set
				arrayindex_t newtindex = 0; // new original transition set
				while (oldtindex < cardtransitionset[i] && oldpindex < cardDecr)
				{
					if (transitionset[i][oldtindex] < Decr[oldpindex])
					{
						// transition in old set but not in Decr -> remain in old set
						transitionset[i][newtindex++] = transitionset[i][oldtindex++];
						continue;
					}
					if (transitionset[i][oldtindex] > Decr[oldpindex])
					{
						// transition in Decr but not in transition set --> ignore
						oldpindex++;
						continue;
					}
					// transition in old transition set and Decr --> move to forked set
					newtransitionset[newpindex++] = transitionset[i][oldtindex++];
				}
				while (oldtindex < cardtransitionset[i])
				{
					// transition in old set but not in Decr -> remain in old set
					transitionset[i][newtindex++] = transitionset[i][oldtindex++];
				}

				if (newpindex) // there are transitions of this kind...
				{
					// forked transition set is not empty
					// update corresponding conflict set
					arrayindex_t oldindex = 0;
					arrayindex_t newindex = 0;
					arrayindex_t decrindex = 0;
					arrayindex_t nodecrindex = 0;
					newconflictset = new arrayindex_t[card];
					newconflicttype = new tConf[card];
					arrayindex_t newcardmutual = 0;
					while (true)
					{
						if (oldindex >= cardconflictset[i])
						{
							// no remaining elements in old tuple
							if (decrindex >= cardDecr)
							{
								// no remaining elements in old tuple + Decr
								while (nodecrindex < cardNodecr)
								{
									// remaining elements in NoDecr
									newconflictset[newindex] = Nodecr[nodecrindex++];
									newconflicttype[newindex++] = DECREASED;
								}
								// no remaining elements in old tuple + Decr + Nodecr
								break;
							}
							// no remaining elements in old tuple
							// remaining elements in Decr
							if (nodecrindex >= cardNodecr)
							{
								// no remaining elements in old tuple, Nodecr
								// remaining elements in Decr
								do
								{
									newconflictset[newindex] = Decr[decrindex++];
									newconflicttype[newindex++] = MUTUAL;
									newcardmutual++;
								} while (decrindex < cardDecr);
								break;
							}
							// no remaining elements in old tuple
							// remaining elements in Decr + NoDecr
							if (Decr[decrindex] < Nodecr[nodecrindex])
							{
								// next element to be inserted is from Decr
								newconflictset[newindex] = Decr[decrindex++];
								newconflicttype[newindex++] = MUTUAL;
								newcardmutual++;
							}
							else
							{
								// next element to be inserted is from Nodecr
								newconflictset[newindex] = Nodecr[nodecrindex++];
								newconflicttype[newindex++] = DECREASED;
							}
							continue;
						}
						// remaining elements in old tuple
						if (decrindex >= cardDecr)
						{
							// remaining elements in old tuple
							// no remaining elements in Decr
							if (nodecrindex >= cardNodecr)
							{
								// remaining elements in old tuple,
								// no remaining elements in Decr + Nodecr
								do
								{
									newconflictset[newindex] = conflictset[i][oldindex];
									if (conflicttype[i][oldindex] == MUTUAL)
										newcardmutual++;
									newconflicttype[newindex++] = conflicttype[i][oldindex++];

								} while (oldindex < cardconflictset[i]);
								break;
							}
							// remaining elements in old tuple + Nodecr
							// no remaining elements in Decr
							if (conflictset[i][oldindex] < Nodecr[nodecrindex])
							{
								// next element to be inserted in only in old tuple
								newconflictset[newindex] = conflictset[i][oldindex];
								if (conflicttype[i][oldindex] == MUTUAL)
									newcardmutual++;
								newconflicttype[newindex++] = conflicttype[i][oldindex++];
								continue;
							}
							if (conflictset[i][oldindex] > Nodecr[nodecrindex])
							{
								// next element to be inserted is only in Nodecr
								newconflictset[newindex] = Nodecr[nodecrindex++];
								newconflicttype[newindex++] = DECREASED;
								continue;
							}
							// next element to be inserted is in old tuple + nodecr
							newconflictset[newindex] = conflictset[i][oldindex];
						newconflicttype[newindex] = (conflicttype[i][oldindex] == DECREASING ? MUTUAL : conflicttype[i][oldindex]);
							if (newconflicttype[newindex] == MUTUAL)
								newcardmutual++;
							nodecrindex++;
							oldindex++;
							newindex++;
							continue;
						}
						// remaining elements in old tuple + Decr
						if (nodecrindex >= cardNodecr)
						{
							// remaining elements in old tuple + Decr
							// no remaining elements in Nodecr
							if (conflictset[i][oldindex] < Decr[decrindex])
							{
								// next element to be inserted in only in old tuple
								newconflictset[newindex] = conflictset[i][oldindex];
								if (conflicttype[i][oldindex] == MUTUAL)
									newcardmutual++;
								newconflicttype[newindex++] = conflicttype[i][oldindex++];
								continue;
							}
							if (conflictset[i][oldindex] > Decr[decrindex])
							{
								// next element to be inserted is from Decr
								newconflictset[newindex] = Decr[decrindex++];
								newconflicttype[newindex++] = MUTUAL;
								newcardmutual++;
								continue;
							}
							// next element to be inserted is in old tuple + Decr
							newconflictset[newindex] = conflictset[i][oldindex++];
							newconflicttype[newindex++] = MUTUAL;
							newcardmutual++;
							decrindex++;
							continue;
						}
						// remaining elements in old tuple + Decr + Nodecr
						if (conflictset[i][oldindex] < Decr[decrindex])
						{
							if (conflictset[i][oldindex] < Nodecr[nodecrindex])
							{
								// next element to be inserted is in old tuple only
								newconflictset[newindex] = conflictset[i][oldindex];
								if (conflicttype[i][oldindex] == MUTUAL)
									newcardmutual++;
								newconflicttype[newindex++] = conflicttype[i][oldindex++];
								continue;
							}
							if (conflictset[i][oldindex] > Nodecr[nodecrindex])
							{
								// next element to be inserted is only in Nodecr
								newconflictset[newindex] = Nodecr[nodecrindex++];
								newconflicttype[newindex++] = DECREASED;
								continue;
							}
							// next element to be inserted is in old tuple + Nodecr
							newconflictset[newindex] = conflictset[i][oldindex];
							newconflicttype[newindex] = (conflicttype[i][oldindex] == DECREASING ? MUTUAL : conflicttype[i][oldindex]);
							if (newconflicttype[newindex] == MUTUAL)
								newcardmutual++;
							nodecrindex++;
							oldindex++;
							newindex++;
							continue;
						}
						if (conflictset[i][oldindex] == Decr[decrindex])
						{
							if (conflictset[i][oldindex] < Nodecr[nodecrindex])
							{
								// next element to ne inserted is in old tuple and in Decr
								newconflictset[newindex] = conflictset[i][oldindex++];
								newconflicttype[newindex++] = MUTUAL;
								newcardmutual++;
								decrindex++;
								continue;
							}
							// next element to be inserted is in nodecr
							newconflictset[newindex] = Nodecr[nodecrindex++];
							newconflicttype[newindex++] = DECREASED;
							continue;
						}
						// oldtuple > Decr
						if (Decr[decrindex] < Nodecr[nodecrindex])
						{
							// next element to be inserted is only in Decr
							newconflictset[newindex] = Decr[decrindex++];
							newconflicttype[newindex++] = MUTUAL;
							newcardmutual++;
							continue;
						}
						// Nodecr < Decr
						// next element to be inserted is from Nodecr
						newconflictset[newindex] = Nodecr[nodecrindex++];
						newconflicttype[newindex++] = DECREASED;
						continue;
					}

					transitionset[stackpointer] = newtransitionset;
					cardtransitionset[stackpointer] = newpindex;
					cardtransitionset[i] = newtindex;
					conflictset[stackpointer] = newconflictset;
					cardconflictset[stackpointer] = newindex;
					cardmutual[stackpointer] = newcardmutual;
					conflicttype[stackpointer++] = newconflicttype;
				}
				if (cardtransitionset[i])
				{
					// fork tuple of type 3

					// compute new 4th component
					newtransitionset = new arrayindex_t[cardtransitionset[i]];
					oldtindex = 0;
					oldpindex = 0;
					newpindex = 0;
					newtindex = 0;
					while (oldtindex < cardtransitionset[i] && oldpindex < cardNodecr)
					{
						if (transitionset[i][oldtindex] < Nodecr[oldpindex])
						{
							transitionset[i][newtindex++] = transitionset[i][oldtindex++];
							continue;
						}
						if (transitionset[i][oldtindex] > Nodecr[oldpindex])
						{
							oldpindex++;
							continue;
						}
						newtransitionset[newpindex++] = transitionset[i][oldtindex++];
					}
					while (oldtindex < cardtransitionset[i])
					{
						transitionset[i][newtindex++] = transitionset[i][oldtindex++];
					}
					if (newpindex) // there are transitions of this kind
					{
						arrayindex_t oldindex = 0;
						arrayindex_t newindex = 0;
						arrayindex_t decrindex = 0;
						arrayindex_t nodecrindex = 0;
						newconflictset = new arrayindex_t[card];
						newconflicttype = new tConf[card];
						arrayindex_t newcardmutual = 0;
						while (true)
						{
							if (oldindex >= cardconflictset[i])
							{
								// no remaining elements in old tuple
								if (decrindex >= cardDecr)
								{
									// no remaining elements in old tuple + Decr
									break;
								}
								// no remaining elements in old tuple
								// remaining elements in Decr
								do
								{
									newconflictset[newindex] = Decr[decrindex++];
									newconflicttype[newindex++] = DECREASING;
								} while (decrindex < cardDecr);
								break;
							}
							// remaining elements in old tuple
							if (decrindex >= cardDecr)
							{
								// remaining elements in old tuple
								// no remaining elements in Decr
								do
								{
									newconflictset[newindex] = conflictset[i][oldindex];
									if (conflicttype[i][oldindex] == MUTUAL)
										newcardmutual++;
									newconflicttype[newindex++] = conflicttype[i][oldindex++];
								} while (oldindex < cardconflictset[i]);
								break;
							}
							// remaining elements in old tuple + Decr
							if (conflictset[i][oldindex] < Decr[decrindex])
							{
								// next element to be inserted in only in old tuple
								newconflictset[newindex] = conflictset[i][oldindex];
								if (conflicttype[i][oldindex] == MUTUAL)
									newcardmutual++;
								newconflicttype[newindex++] = conflicttype[i][oldindex++];
								continue;
							}
							if (conflictset[i][oldindex] > Decr[decrindex])
							{
								// next element to be inserted is from Decr
								newconflictset[newindex] = Decr[decrindex++];
								newconflicttype[newindex++] = DECREASING;
								continue;
							}
							// next element to be inserted is in old tuple + Decr
							newconflictset[newindex] = conflictset[i][oldindex];
							newconflicttype[newindex] = (conflicttype[i][oldindex] == DECREASED ? MUTUAL : conflicttype[i][oldindex]);
							if (newconflicttype[newindex] == MUTUAL)
								newcardmutual++;
							oldindex++;
							newindex++;
							decrindex++;
							continue;
						}
						transitionset[stackpointer] = newtransitionset;
						cardtransitionset[stackpointer] = newpindex;
						conflictset[stackpointer] = newconflictset;
						cardconflictset[stackpointer] = newindex;
						cardmutual[stackpointer] = newcardmutual;
						conflicttype[stackpointer++] = newconflicttype;
						cardtransitionset[i] = newtindex;
					}
				}
				if (cardtransitionset[i] <= 0)
				{
					delete[] conflictset[i];
					delete[] conflicttype[i];
					delete[] transitionset[i];
					conflictset[i] = conflictset[--stackpointer];
					conflicttype[i] = conflicttype[stackpointer];
					transitionset[i] = transitionset[stackpointer];
					cardconflictset[i] = cardconflictset[stackpointer];
					cardmutual[i] = cardmutual[stackpointer];
					cardtransitionset[i] = cardtransitionset[stackpointer];
				}
			}
			// check whether new set happen to equal some other set
			arrayindex_t j = sp;
			while (j < stackpointer)
			{
				arrayindex_t k;
				for (k = 0; k < j; k++)
				{
					if (
						cardtransitionset[k] != 0 &&
						cardconflictset[k] == cardconflictset[j] &&
						!memcmp(conflictset[j], conflictset[k], cardconflictset[j] * sizeof(arrayindex_t)) &&
						!memcmp(conflicttype[j], conflicttype[k], cardconflictset[j] * sizeof(tConf)))
					{
						arrayindex_t *newts = new arrayindex_t[cardtransitionset[k] + cardtransitionset[j]];
						memcpy(newts, transitionset[k], cardtransitionset[k] * sizeof(arrayindex_t));
						memcpy(newts + cardtransitionset[k], transitionset[j], cardtransitionset[j] * sizeof(arrayindex_t));
						delete[] transitionset[k];
						transitionset[k] = newts;
						cardtransitionset[k] += cardtransitionset[j];
						qsort(transitionset[k], cardtransitionset[k]);
						delete[] conflictset[j];
						delete[] transitionset[j];
						delete[] conflicttype[j];
						transitionset[j] = transitionset[--stackpointer];
						conflictset[j] = conflictset[stackpointer];
						conflicttype[j] = conflicttype[stackpointer];
						cardtransitionset[j] = cardtransitionset[stackpointer];
						cardmutual[j] = cardmutual[stackpointer];
						cardconflictset[j] = cardconflictset[stackpointer];
						break;
					}
				}
				if (k >= j)
					j++;
			}
			// check whether there are saturated clusters
			arrayindex_t i = 0;
			while (i < stackpointer)
			{
				if (cardmutual[i] == card)
				{
					// produce final data structure
					for (arrayindex_t j = 0; j < cardtransitionset[i]; j++)
					{
						arrayindex_t t = transitionset[i][j];
						TrDecreased[t] = TrDecreasing[t] = conflictset[i];
						TrCardDecreasing[t] = TrCardDecreased[t] = card;
					}
					transitionset[i] = transitionset[--stackpointer];
					conflictset[i] = conflictset[stackpointer];
					conflicttype[i] = conflicttype[stackpointer];
					cardtransitionset[i] = cardtransitionset[stackpointer];
					cardmutual[i] = cardmutual[stackpointer];
					cardconflictset[i] = cardconflictset[stackpointer];
				}
				else
				{
					i++;
				}
			}
		}
		// produce final data structures

		for (arrayindex_t sp = 0; sp < stackpointer; sp++)
		{
			// sort conflictset by conflicttype
			arrayindex_t blue = 0;
			arrayindex_t white = 0;
			arrayindex_t red = cardconflictset[sp];

			while (white < red)
			{
				arrayindex_t tmpi;
				tConf tmpt;
				switch (conflicttype[sp][white])
				{
				case DECREASED:
					tmpi = conflictset[sp][white];
					tmpt = conflicttype[sp][white];
					conflictset[sp][white] = conflictset[sp][blue];
					conflicttype[sp][white++] = conflicttype[sp][blue];
					conflictset[sp][blue] = tmpi;
					conflicttype[sp][blue++] = tmpt;
					break;
				case MUTUAL:
					white++;
					break;
				case DECREASING:
					tmpi = conflictset[sp][white];
					tmpt = conflicttype[sp][white];
					conflictset[sp][white] = conflictset[sp][--red];
					conflicttype[sp][white] = conflicttype[sp][red];
					conflictset[sp][red] = tmpi;
					conflicttype[sp][red] = tmpt;
				}
			}
			for (arrayindex_t i = 0; i < cardtransitionset[sp]; i++)
			{
				arrayindex_t t = transitionset[sp][i];
				TrDecreased[t] = conflictset[sp];
				TrCardDecreased[t] = red;
				TrDecreasing[t] = conflictset[sp] + blue;
				TrCardDecreasing[t] = cardconflictset[sp] - blue;
			}
			delete[] conflicttype[sp];
			delete[] transitionset[sp];
		}
		delete[] conflicttype;
		//delete [] conflictset;
		delete[] transitionset;
		delete[] cardtransitionset;
		delete[] cardconflictset;
	}

}

void Petrinet::computeConflictingTransitionsNew()
{
    int nTransitions = 0;

    // cout << "Will read " << Card[PL] << " places." << std::endl;
	// cout << Card[PL] << endl << endl;
    std::vector<std::vector<int> > transitionByConsumptionLists;
    std::vector<std::vector<int> > transitionByProductionLists;
    std::vector<std::vector<std::pair<int,int> > > cliqueLists;

    for (arrayindex_t p = 0; p < Card[PL]; p++) 
	{
        arrayindex_t postTransitions = CardArcs[PL][POST][p];
		// cout << Net::Name[PL][p] << endl;
        // cout << "Read post set of place " << p << " with " << postTransitions << " transitions:" << endl << "{ ";
		// cout << "Read post set of place " << Net::Name[PL][p] << " with " << postTransitions << " transitions:" << endl << "{ ";

        std::vector<int> transitions, consumptions, productions;
        while (postTransitions-- > 0) {
            int transition, consumption, production;
			transition = Arc[PL][POST][p][postTransitions];
			consumption = Mult[PL][POST][p][postTransitions];
			production = 0;
			for (arrayindex_t i = 0; i < CardArcs[PL][PRE][p]; i++)
			{
				if (Arc[PL][PRE][p][i] == transition)
				{
						production = Mult[PL][PRE][p][i];
						break;
				}
			}

            if (transition >= nTransitions)
			{
				nTransitions = transition + 1;
			}
            transitions.push_back(transition);
            consumptions.push_back(consumption);
            productions.push_back(production);
            // cout << "(" 
			// 	//<< transition << ", " 
			// 	<< Net::Name[TR][transition] << ", " 
			// 	<< consumption << ", " 
			// 	<< production << ") "
			// 	;
			// cout << Net::Name[TR][transition] << " " << consumption << " " << production << endl;
        }
        // cout << "}" << endl;
		// cout << endl;

        std::vector<std::pair<int,int> > cliques;
        extractConflictCliques(transitions,
                               consumptions,
                               productions,
                               consumptions,
                               productions,
                               cliques);
        transitionByConsumptionLists.push_back(consumptions);
        transitionByProductionLists.push_back(productions);
        cliqueLists.push_back(cliques);
        // cout << "}" << endl;
    }

    std::vector<int> mapVertexToClass(nTransitions, -1);
    std::vector<std::vector<int> > mapCliqueOToClasses;
    std::vector<std::vector<int> > mapCliqueIToClasses;
    std::vector<std::vector<int> > mapClassToCliqueOs;
    std::vector<std::vector<int> > mapClassToCliqueIs;
    std::vector<ClassMetaData> mapClassToMeta;

    std::vector<std::vector<int> >::iterator transitionsByConsumption = transitionByConsumptionLists.begin();
    std::vector<std::vector<int> >::iterator transitionsByProduction = transitionByProductionLists.begin();
    for (std::vector<std::vector<std::pair<int,int> > >::iterator cliques = cliqueLists.begin();
         cliques != cliqueLists.end();
         cliques++, transitionsByConsumption++, transitionsByProduction++)
        for (std::vector<std::pair<int,int> >::iterator clique = cliques->begin();
            clique != cliques->end();
            clique++) {
            addHalfClique(*transitionsByConsumption, clique->first, transitionsByConsumption->size(),
                            mapVertexToClass,
                            mapCliqueIToClasses,
                            mapCliqueOToClasses,
                            mapClassToCliqueIs,
                            mapClassToCliqueOs,
                            mapClassToMeta);
            addHalfClique(*transitionsByProduction, 0, clique->second,
                          mapVertexToClass,
                          mapCliqueOToClasses,
                          mapCliqueIToClasses,
                          mapClassToCliqueOs,
                          mapClassToCliqueIs,
                          mapClassToMeta);
    }
    
    std::vector<std::vector<int> > mapClassToAdjacencyList;
    std::vector<std::pair<int, int> > mapClassToBiInterval;
    extractBidirectionalAdjacencyLists(mapVertexToClass,
                                       mapCliqueOToClasses,
                                       mapCliqueIToClasses,
                                       mapClassToCliqueOs,
                                       mapClassToCliqueIs,
                                       mapClassToAdjacencyList,
                                       mapClassToBiInterval);


	for(arrayindex_t t =0; t < Card[TR];t++)
	{
		TrCardDecreased[t] = mapClassToAdjacencyList[mapVertexToClass[t]].size();
		TrDecreased[t] = new arrayindex_t[TrCardDecreased[t]];
		int i = 0;
		std::pair<int,int> biInterval = mapClassToBiInterval[mapVertexToClass[t]];
		for (std::vector<int>::iterator neighbor = mapClassToAdjacencyList[mapVertexToClass[t]].begin();
			 neighbor != mapClassToAdjacencyList[mapVertexToClass[t]].end();
			 neighbor ++)
		{
			TrDecreased[t][i++] = *neighbor;
		}
		TrDecreasing[t] = TrDecreased[t] + biInterval.first;
		TrCardDecreasing[t] = TrCardDecreased[t] -  biInterval.first;
		TrCardDecreased[t] = biInterval.second;

	}
	// TODO: map mapClassToAdjacencyList to LoLA data structure
}

/// for every transition t, compute the set of transitions t' such that firing t increases the
/// number of tokens on any pre-place of t' (that is, t potentially enables t')
void Petrinet::computeBackConflictingTransitionsOld()
{
	// We proceed place by place.
	// In every iteration, we update pairs (Ci,Ti) such that
	// every t in Ti increases some pre-place of every transition in Ci
	// In the end, all transitions in Ti share the same array...
	arrayindex_t cardTR = Card[TR];

	arrayindex_t **transitionset = new arrayindex_t *[cardTR ? cardTR : 1];  // carry all the Ti
	arrayindex_t *cardtransitionset = new arrayindex_t[cardTR ? cardTR : 1]; // sizes of the Ti
	arrayindex_t **conflictset = new arrayindex_t *[cardTR ? cardTR : 1];	// carry all the Ci
	arrayindex_t *cardconflictset = new arrayindex_t[cardTR ? cardTR : 1];   // sizes of the Ci
	arrayindex_t stackpointer = 1;								// number of pairs

	//initially there is just one pair (emptyset,T)
	transitionset[0] = new arrayindex_t[cardTR];
	for (arrayindex_t i = 0; i < cardTR; i++)
	{
		transitionset[0][i] = i;
	}
	conflictset[0] = NULL;
	cardtransitionset[0] = cardTR;
	cardconflictset[0] = 0;

	for (arrayindex_t p = 0; p < Card[PL]; p++)
	{
		//// add the effect of p to all pairs

		arrayindex_t cardpostp = CardArcs[PL][POST][p];

		arrayindex_t *postp = new arrayindex_t[cardpostp];
		memcpy(postp, Arc[PL][POST][p], sizeof(arrayindex_t) * cardpostp);
		qsort(postp, cardpostp);
		arrayindex_t cardincr = PlCardIncreasing[p];
		arrayindex_t *incr = new arrayindex_t[cardincr];
		memcpy(incr, PlIncreasing[p], sizeof(arrayindex_t) * cardincr);
		qsort(incr, cardincr);

		//// freeze stackpointer such that fresh pairs are not considered twice
		arrayindex_t sp = stackpointer;
		for (arrayindex_t i = 0; i < sp; i++)
		{
			//// add the effect of p to the pair (Ci,Ti)

			//// step one: separate transition set Ti into
			//// Ti cap increasing(p) and Ti \ increasing(p)
			//// cap --> new pair
			//// \ --> old pair

			arrayindex_t *newtransitionset = new arrayindex_t[cardTR];
			arrayindex_t newcardtransitionset = 0;
			arrayindex_t pindex = 0;	// index in increasing(p)
			arrayindex_t oldtindex = 0; // index for t in Ti to be considered
			arrayindex_t newtindex = 0; // index for t to remain in Ti
			while (pindex < cardincr && oldtindex < cardtransitionset[i])
			{
				if (incr[pindex] < transitionset[i][oldtindex])
				{
					// increasing transition not in Ti --> skip
					pindex++;
					continue;
				}
				if (incr[pindex] > transitionset[i][oldtindex])
				{
					//// increasing transition in Ti \ increasing(p) -->remain
					transitionset[i][newtindex++] = transitionset[i][oldtindex++];
					continue;
				}
				// increasing transition in Ti and increasing(p) -> to new pair
				newtransitionset[newcardtransitionset++] = transitionset[i][oldtindex++];
				pindex++;
			}
			while (oldtindex < cardtransitionset[i])
			{
				transitionset[i][newtindex++] = transitionset[i][oldtindex++];
			}
			// conflict set of old pair remains the same
			// conflict set of new pair: add post(p)
			if (newcardtransitionset)
			{
				arrayindex_t size = cardconflictset[i] + cardpostp;
				if (size > Card[TR])
					size = Card[TR];
				arrayindex_t *newconflictset = new arrayindex_t[size];
				arrayindex_t newcardconflictset = 0;
				pindex = 0; // index in post(p)
				arrayindex_t tindex = 0;
				while (pindex < cardpostp && tindex < cardconflictset[i])
				{
					if (postp[pindex] < conflictset[i][tindex])
					{
						newconflictset[newcardconflictset++] = postp[pindex++];
						continue;
					}
					if (postp[pindex] > conflictset[i][tindex])
					{
						newconflictset[newcardconflictset++] = conflictset[i][tindex++];
						continue;
					}
					newconflictset[newcardconflictset++] = postp[pindex++];
					tindex++;
				}
				while (pindex < cardpostp)
				{
					newconflictset[newcardconflictset++] = postp[pindex++];
				}
				while (tindex < cardconflictset[i])
				{
					newconflictset[newcardconflictset++] = conflictset[i][tindex++];
				}
				if (newcardtransitionset == cardtransitionset[i])
				{
					delete[] conflictset[i];
					delete[] transitionset[i];
					conflictset[i] = newconflictset;
					transitionset[i] = newtransitionset;
					cardconflictset[i] = newcardconflictset;
					// card transitions set untouched
				}
				else
				{
					// create new pair
					transitionset[stackpointer] = newtransitionset;
					conflictset[stackpointer] = newconflictset;
					cardtransitionset[stackpointer] = newcardtransitionset;
					cardconflictset[stackpointer++] = newcardconflictset;
					cardtransitionset[i] -= newcardtransitionset;
				}
			}
			else
			{
				delete[] newtransitionset;
			}
		}
		// check for duplicate Ci and merge them
		arrayindex_t i = sp;
		while (i < stackpointer)
		{
			arrayindex_t j;
			for (j = 0; j < sp; j++)
			{
				arrayindex_t size;
				if ((size = cardconflictset[i]) == cardconflictset[j] && !memcmp(conflictset[i], conflictset[j], size * sizeof(arrayindex_t)))
				{
					//duplicate
					memcpy(transitionset[j] + cardtransitionset[j], transitionset[i], cardtransitionset[i] * sizeof(arrayindex_t));
					cardtransitionset[j] += cardtransitionset[i];
					qsort(transitionset[j], cardtransitionset[j]);
					delete[] transitionset[i];
					delete[] conflictset[i];
					transitionset[i] = transitionset[--stackpointer];
					conflictset[i] = conflictset[stackpointer];
					cardtransitionset[i] = cardtransitionset[stackpointer];
					cardconflictset[i] = cardconflictset[stackpointer];
					break;
				}
			}
			if (j >= sp)
				i++;
		}
		delete[] postp;
		delete[] incr;
	}
	for (arrayindex_t i = 0; i < stackpointer; i++)
	{
		for (arrayindex_t j = 0; j < cardtransitionset[i]; j++)
		{
			arrayindex_t t = transitionset[i][j];
			TrIncreased[t] = conflictset[i];
			TrCardIncreased[t] = cardconflictset[i];
		}
		delete[] transitionset[i];
	}
	delete[] conflictset;
	delete[] cardconflictset;
	delete[] cardtransitionset;
	delete[] transitionset;

}

void Petrinet::computeBackConflictingTransitionsNew()
{

    int nTransitions = 0;

    // cout << "Will read " << Card[PL] << " places." << endl;
	// cout << Card[PL] << endl << endl;
    std::vector<std::vector<int> > transitionLists;
    std::vector<std::pair<int,int> > cliques;

    for (arrayindex_t p = 0; p < Card[PL]; p++) 
	{
        std::vector<int> transitions, consumptions, productions;

        arrayindex_t postTransitions = CardArcs[PL][POST][p];

        while (postTransitions-- > 0) 
		{
            int transition, consumption, production;
			transition = Arc[PL][POST][p][postTransitions];
			consumption = Mult[PL][POST][p][postTransitions];
			production = 0;
			for (arrayindex_t i = 0; i < CardArcs[PL][PRE][p]; i++)
			{
				if (Arc[PL][PRE][p][i] == transition)
				{
						production = Mult[PL][PRE][p][i];
						break;
				}
			}

            if (transition >= nTransitions)
			{
				nTransitions = transition + 1;
			}
            transitions.push_back(transition);
            consumptions.push_back(consumption);
            productions.push_back(production);
            // cout << "(" 
			// 	//<< transition << ", " 
			// 	<< Net::Name[TR][transition] << ", " 
			// 	<< consumption << ", " 
			// 	<< production << ") "
			// 	<< endl;
			// cout << Net::Name[TR][transition] << " " << consumption << " " << production << endl;
        }
        // cout << "}" << endl;


        arrayindex_t preTransitions = CardArcs[PL][PRE][p];
		// cout << Net::Name[PL][p] << endl;
        // cout << "Read pre set of place " << p << " with " << preTransitions << " transitions:" << endl << "{ ";
		// cout << "Read pre set of place " << Net::Name[PL][p] << " with " << preTransitions << " transitions:" << endl << "{ " << endl;

        while (preTransitions-- > 0) 
		{
            int transition, consumption, production;
			transition = Arc[PL][PRE][p][preTransitions];
			production = Mult[PL][PRE][p][preTransitions];
			consumption = 0;
			for (arrayindex_t i = 0; i < CardArcs[PL][POST][p]; i++)
			{
				if (Arc[PL][POST][p][i] == transition)
				{
						consumption = Mult[PL][POST][p][i];
						break;
				}
			}
			if (consumption > 0)
			{
				continue;
			}

            if (transition >= nTransitions)
			{
				nTransitions = transition + 1;
			}
            transitions.push_back(transition);
            consumptions.push_back(consumption);
            productions.push_back(production);
            // cout << "(" 
			// 	//<< transition << ", " 
			// 	<< Net::Name[TR][transition] << ", " 
			// 	<< consumption << ", " 
			// 	<< production << ") "
			// 	<< endl;
			// cout << Net::Name[TR][transition] << " " << consumption << " " << production << endl;
        }
        // cout << "}" << endl;		
       
        std::pair<int,int> clique;
        extractActivationClique(transitions,
                                consumptions,
                                productions,
                                clique);

        if (clique.first >= 0) {
            transitionLists.push_back(transitions);
            cliques.push_back(clique);
        }
        // cout << "}" << endl;
    }

    std::vector<int> mapVertexToClass(nTransitions, -1);
    std::vector<std::vector<int> > mapCliqueOToClasses;
    std::vector<std::vector<int> > mapCliqueIToClasses;
    std::vector<std::vector<int> > mapClassToCliqueOs;
    std::vector<std::vector<int> > mapClassToCliqueIs;
    std::vector<ClassMetaData> mapClassToMeta;
    
    std::vector<std::vector<int> >::iterator  transitions = transitionLists.begin();
    for (std::vector<std::pair<int,int> >::iterator clique = cliques.begin();
         clique != cliques.end();
         clique++, transitions++) {
        addHalfClique(*transitions, clique->first, transitions->size(),
                        mapVertexToClass,
                        mapCliqueIToClasses,
                        mapCliqueOToClasses,
                        mapClassToCliqueIs,
                        mapClassToCliqueOs,
                        mapClassToMeta);
        addHalfClique(*transitions, 0, clique->second,
                        mapVertexToClass,
                        mapCliqueOToClasses,
                        mapCliqueIToClasses,
                        mapClassToCliqueOs,
                        mapClassToCliqueIs,
                        mapClassToMeta);
    }
	std::vector<std::vector<int> > mapClassToAdjacencyList;
    extractSuccessorAdjacencyLists(mapVertexToClass,
                                   mapCliqueOToClasses,
                                   mapCliqueIToClasses,
                                   mapClassToCliqueOs,
                                   mapClassToCliqueIs,
                                   mapClassToAdjacencyList);

     for(arrayindex_t t = 0; t < Card[TR];t++)
     {
	TrCardIncreased[t] = mapClassToAdjacencyList[mapVertexToClass[t]].size();
	TrIncreased[t] = new arrayindex_t[TrCardIncreased[t]];
	arrayindex_t i = 0;
	for (std::vector<int>::iterator neighbor = mapClassToAdjacencyList[mapVertexToClass[t]].begin();
                neighbor != mapClassToAdjacencyList[mapVertexToClass[t]].end();
                neighbor ++)
        {
                TrIncreased[t][i++] = *neighbor ;
        }

     }
}

/*!
This function does the preprocessing for the given net. With finished preprocessing there are additional information available which are used to speed up the simulation.
\pre the raw net is read, places, transitions and edges in-between are set properly.
\post Bitvectorsize, hashindexes, conflicts, significant places, progress measure and enabledness are calculated
*/

void Petrinet::preprocess0()
{
	/************************************
    * 0. compute structural bounds      *
    ************************************/

	if (RT::args.safe_given)
	{
		for (arrayindex_t p = 0; p < Card[PL]; p++)
		{
			Capacity[p] = 1;
		}
	}
}

void Petrinet::preprocess1()
{
	const arrayindex_t cardPL = Card[PL];
	const arrayindex_t cardTR = Card[TR];

	/************************************
    * 1. Compute bits needed for places *
    ************************************/
	CardBits = new cardbit_t[cardPL];
	SizeOfBitVector = 0;
	for (arrayindex_t p = 0; p < cardPL; p++)
	{
		SizeOfBitVector +=
			(CardBits[p] = Capacity2Bits(Capacity[p]));
	}

	/********************
    * 2. Compute Hashes *
    ********************/
	Hash = new hash_t[cardPL];
	HashInitial = 0;
	for (arrayindex_t p = 0; p < cardPL; p++)
	{
		Hash[p] = rand() % MAX_HASH;
		HashInitial = (HashInitial + Hash[p] * Initial[p]) %
					  SIZEOF_MARKINGTABLE;
	}
	// set hash value for initial marking

	/*********************
    * 3. Organize Deltas *
    *********************/

	DeltaHash = new hash_t[cardTR]();
	for (int direction = PRE; direction <= POST; direction++)
	{
		CardDeltaT[direction] = new arrayindex_t[cardTR]();
		DeltaT[direction] = new arrayindex_t *[cardTR];
		MultDeltaT[direction] = new mult_t *[cardTR];
	}
	PlIncreasing = new arrayindex_t *[cardPL]();
	PlDecreasing = new arrayindex_t *[cardPL]();
	PlCardDecreasing = new arrayindex_t[cardPL]();
	PlCardIncreasing = new arrayindex_t[cardPL]();
	PlMultIncreasing = new mult_t *[cardPL]();
	PlMultDecreasing = new mult_t *[cardPL]();
	preprocess_organizeDeltas();

	/****************************
    * 5. Set significant places *
    *****************************/
	// test whether computation actually needed
	if (CardSignificant == UINT_MAX)
	{
		if (RT::args.encoder_arg == encoder_arg_fullcopy)
		{
			CardSignificant = Card[PL];
		}
		else
		{
			setSignificantPlaces();
		}
	}

	/*******************************
    * 7. Initial enabledness check *
    *******************************/

	Enabled = new bool[cardTR];
	CardEnabled = cardTR;
	for (arrayindex_t t = 0; t < cardTR; t++)
	{
		Enabled[t] = true;
	}

	for (arrayindex_t t = 0; t < cardTR; t++)
	{
		checkEnabled_Initial(t);
	}
}

void Petrinet::preprocess2()
{
	arrayindex_t cardTR = Card[TR];
	TrCardDecreasing = new arrayindex_t[cardTR];
	TrCardDecreased = new arrayindex_t[cardTR];
	TrDecreasing = new arrayindex_t *[cardTR];
	TrDecreased = new arrayindex_t *[cardTR];
	TrIncreased = new arrayindex_t *[cardTR];
	TrCardIncreased = new arrayindex_t[cardTR];
	/**************************************
    * 4. Organize conflicting transitions *
    **************************************/
	computeConflictClusters();
	computeConflictingTransitionsOld();
	//computeConflictingTransitionsNew();
}

void Petrinet::preprocess3()
{
	computeBackConflictingTransitionsOld();
	//computeBackConflictingTransitionsNew();
	arrayindex_t cardTR = Card[TR];

	/**************************
    * 6. Set progress measure *
    **************************/

	if (RT::args.search_arg == search_arg_sweepline)
	{
		setProgressMeasure();
	}
}

void unionfindsort(arrayindex_t *cl, int *uf, arrayindex_t card)
{
	int pivot = uf[0];
	arrayindex_t b = 0;	// the first element not larger than pivot
	arrayindex_t w = 1;	// the first element not larger or equal to pivot
	arrayindex_t r = card; // the first element smaller than pivot

	// the elements between w and (including r-1) form the unknwon area
	while (w < r)
	{
		if (uf[w] > pivot)
		{
			// swap into the 0..b area
			arrayindex_t tempc = cl[b];
			int tempu = uf[b];
			cl[b] = cl[w];
			uf[b] = uf[w];
			cl[w] = tempc;
			uf[w] = tempu;
			++w;
			++b;
		}
		else
		{
			if (uf[w] == pivot)
			{
				// append to b+1..w area
				++w;
			}
			else
			{
				//swap into the r..card area
				--r;
				arrayindex_t tempc = cl[w];
				int tempu = uf[w];
				cl[w] = cl[r];
				uf[w] = uf[r];
				cl[r] = tempc;
				uf[r] = tempu;
			}
		}
	}
	// sort 0..b area if necessary
	if (b > 1)
	{
		unionfindsort(cl, uf, b);
	}
	// sort r..card area if necessary
	if ((card - r) > 1)
	{
		unionfindsort(cl + r, uf + r, card - r);
	}
}

void Petrinet::clustersort(arrayindex_t *cl, arrayindex_t fr, arrayindex_t to, arrayindex_t p)
{
	// sort according to p-th pre-place of the transitions in cl between fr and to
	arrayindex_t pivot = Arc[TR][PRE][cl[fr]][p];
	arrayindex_t b = fr;	 // the first element not smaller than pivot
	arrayindex_t w = fr + 1; // the first element not smaller or equal to pivot
	arrayindex_t r = to;	 // the first element larger than pivot

	// the elements between w and (including r-1) form the unknwon area
	while (w < r)
	{
		if (Arc[TR][PRE][cl[w]][p] < pivot)
		{
			// swap into the 0..b area
			arrayindex_t tempc = cl[b];
			cl[b] = cl[w];
			cl[w] = tempc;
			++w;
			++b;
		}
		else
		{
			if (Arc[TR][PRE][cl[w]][p] == pivot)
			{
				// append to b+1..w area
				++w;
			}
			else
			{
				//swap into the r..card area
				--r;
				arrayindex_t tempc = cl[w];
				cl[w] = cl[r];
				cl[r] = tempc;
			}
		}
	}
	// sort 0..b area if necessary
	if (b - fr > 1)
	{
		clustersort(cl, fr, b, p);
	}
	// sort r..card area if necessary
	if ((to - r) > 1)
	{
		clustersort(cl, r, to, p);
	}
}

void Petrinet::backclustersort(arrayindex_t *cl, arrayindex_t fr, arrayindex_t to, arrayindex_t p)
{
	// sort according to p-th pre-place of the transitions in cl between fr and to
	arrayindex_t pivot = Arc[TR][POST][cl[fr]][p];
	arrayindex_t b = fr;	 // the first element not smaller than pivot
	arrayindex_t w = fr + 1; // the first element not smaller or equal to pivot
	arrayindex_t r = to;	 // the first element larger than pivot

	// the elements between w and (including r-1) form the unknwon area
	while (w < r)
	{
		if (Arc[TR][POST][cl[w]][p] < pivot)
		{
			// swap into the 0..b area
			arrayindex_t tempc = cl[b];
			cl[b] = cl[w];
			cl[w] = tempc;
			++w;
			++b;
		}
		else
		{
			if (Arc[TR][POST][cl[w]][p] == pivot)
			{
				// append to b+1..w area
				++w;
			}
			else
			{
				//swap into the r..card area
				--r;
				arrayindex_t tempc = cl[w];
				cl[w] = cl[r];
				cl[r] = tempc;
			}
		}
	}
	// sort 0..b area if necessary
	if (b - fr > 1)
	{
		backclustersort(cl, fr, b, p);
	}
	// sort r..card area if necessary
	if ((to - r) > 1)
	{
		backclustersort(cl, r, to, p);
	}
}

/// Sort a section of the Clusters array (see below) that contains transitions by decreasing
/// number of conflicting transitions
void Petrinet::sortlength(arrayindex_t *a, arrayindex_t card)
{
	if (card < 2)
		return;
	arrayindex_t b = 0;
	arrayindex_t pivot = TrCardDecreasing[a[0]];
	arrayindex_t w = 1;
	arrayindex_t r = card;
	while (w < r)
	{
		if (TrCardDecreasing[a[w]] > pivot)
		{
			// swap into the 0..b area
			arrayindex_t temp = a[b];
			a[b++] = a[w];
			a[w++] = temp;
		}
		else
		{
			if (TrCardDecreasing[a[w]] == pivot)
			{
				// append to b+1..w area
				++w;
			}
			else
			{
				//swap into the r..card area
				arrayindex_t temp = a[w];
				a[w] = a[--r];
				a[r] = temp;
			}
		}
	}
	// sort 0..b area if necessary
	if (b > 1)
	{
		sortlength(a, b);
	}
	// sort r..card area if necessary
	if ((card - r) > 1)
	{
		sortlength(a + r, card - r);
	}
}

arrayindex_t *Clusters;
arrayindex_t cardOriginal;

arrayindex_t runningclusterthreads;
pthread_mutex_t clustermutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clustercond;

// \brief Compute all conflict clusters, fill the array
// StubbornPriority and SingletonClusters
void Petrinet::computeConflictClusters()
{
	// 1. Compute conflict clusters using union/find algorithm

	// the semantics of the data structure:
	// - Each element is included in exactly one set
	// - The name of the set is one of its elements
	// - If element i is the name of a set S, unionfind[i] is -card(S)
	// - If element i is not the name of a set, unionfind[i] is an element
	//   contained in the same set. Seing [i,unionfind[i]] as an edge,
	//   each set is a tree where the root is the element that lends the set
	//   its name
	// - operation find: input: i, output: name of the set containing i
	// - operation union: input i,j, result: union of the sets containing these two
	cardClusters = Card[TR] + Card[PL];
	int *unionfind = new int[cardClusters];

	// initially, all sets are singleton
	for (arrayindex_t i = 0; i < Card[TR]; i++)
	{
		unionfind[i] = -1;
	}
	// place with index i is coded as CardTR + i
	for (arrayindex_t i = Card[TR]; i < Card[TR] + Card[PL]; i++)
	{
		unionfind[i] = -1;
	}

	// we unify sets according to arcs in P x T

	for (arrayindex_t i = 0; i < Card[PL]; i++) // for all places do...
	{
		for (arrayindex_t j = 0; j < CardArcs[PL][POST][i]; j++) // for all post-transitions do...
		{

			// find the set of place i
			arrayindex_t k;
			for (k = i + Card[TR]; unionfind[k] >= 0; k = unionfind[k])
				;

			// find the set of transition j
			arrayindex_t l;
			for (l = Arc[PL][POST][i][j]; unionfind[l] >= 0; l = unionfind[l])
				;

			// perform "union": link the smaller to the larger set

			// remember the new root
			arrayindex_t m;

			if (k == l)
			{
				// already same set, nothing do do
				m = k;
			}
			else
			{
				cardClusters--;
				if (unionfind[k] < unionfind[l])
				{
					// k is in larger set (remember that we compare negative numbers)
					unionfind[k] += unionfind[l]; // new size
					m = unionfind[l] = k;		  // l linked to k
				}
				else
				{
					// l is in larger set
					unionfind[l] += unionfind[k]; // new size
					m = unionfind[k] = l;		  // k linked to l
				}
			}
			// after union, we compress the paths of both original nodes
			// thus leading to shorter paths in the future

			for (k = i + Card[TR]; unionfind[k] >= 0;)
			{
				arrayindex_t n = unionfind[k];
				unionfind[k] = m;
				k = n;
			}
			for (l = Arc[PL][POST][i][j]; unionfind[l] >= 0;)
			{
				arrayindex_t n = unionfind[l];
				unionfind[l] = m;
				l = n;
			}
		}
	}

	// here, the union find structure represents
	// all conflict clusters. For accessing it, we post process the data structure.

	// counts the places in each cluster
	ClusterCardPlaces = new arrayindex_t[cardClusters];
	memset(ClusterCardPlaces,0,cardClusters * sizeof(arrayindex_t));
	// counts the transitions in each cluster
	ClusterCardTransitions = new arrayindex_t[cardClusters];
	memset(ClusterCardTransitions,0,cardClusters * sizeof(arrayindex_t));

	// list the places in each cluster
	ClusterPlaces = new arrayindex_t *[cardClusters];
	// list the transitions in each cluster
	ClusterTransitions = new arrayindex_t *[cardClusters];
	// counts the elements in each cluster
	arrayindex_t *clustersize = new arrayindex_t[cardClusters];
	int cl = 0;

	for (arrayindex_t i = 0; i < Card[TR] + Card[PL]; i++)
	{
		if (unionfind[i] >= 0)
			continue;

		clustersize[cl] = -unionfind[i];
		ClusterTransitions[cl] = new arrayindex_t[clustersize[cl]]; // an array large enough to carry both places and
																	// transitions. ClusterPlaces shall later on point
																	// into the middle of this array
		unionfind[i] = -1 - cl++; // instead of size, root now carries its -1 + (-1)*entry in the cluster data structure
								  // this must start with -1 since itherwise it cannot be distinguished from first index in uf
	}
	// fill ClusterTransitions
	for (arrayindex_t i = 0; i < Card[TR]; i++)
	{
		int k;
		for (k = unionfind[i]; k >= 0; k = unionfind[k])
			;
		cl = -k - 1;
		ClusterTransitions[cl][ClusterCardTransitions[cl]++] = i;
	}
	// fill places into remainder of ClusterTransitions (array is large enough for both)
	for (arrayindex_t i = 0; i < Card[PL]; i++)
	{
		int k;
		for (k = unionfind[i + Card[TR]]; k >= 0; k = unionfind[k])
			;
		cl = -k - 1;
		ClusterTransitions[cl][clustersize[cl] - 1 - ClusterCardPlaces[cl]++] = i;
	}
	// link ClusterPlaces into ClusterTransitions to pretend that it is its own array
	for (cl = 0; cl < cardClusters; cl++)
	{
		ClusterPlaces[cl] = ClusterTransitions[cl] + ClusterCardTransitions[cl];
	}
	ClusterTransitionIndex = new arrayindex_t[Card[TR]]; // maps transitions to index of its cluster
	ClusterPlaceIndex = new arrayindex_t[Card[PL]]; // maps places to index of its cluster

	StubbornPriority = new arrayindex_t[Card[TR]]; // will ultimately be a list of transitions 
							// ordered by size of their cluster
	for (cl = 0; cl < cardClusters; cl++)
	{
//RT::rep->status("CLUSTER %d", cl);
		for (arrayindex_t i = 0; i < ClusterCardTransitions[cl]; i++)
		{
			arrayindex_t t = ClusterTransitions[cl][i];
			ClusterTransitionIndex[t] = cl;
			StubbornPriority[t] = t;
			unionfind[t] = -ClusterCardTransitions[cl];
//RT::rep->status("...CONTAINS TRANSITION %s uf %d", Name[TR][t], unionfind[t]);
		}
		for (arrayindex_t i = 0; i < ClusterCardPlaces[cl]; i++)
		{
//RT::rep->status("...CONTAINS PLACE %s", Name[PL][ClusterPlaces[cl][i]]);
			ClusterPlaceIndex[ClusterPlaces[cl][i]] = cl;
		}
	}

	// Third, we set stubbornPriority
	// sort it according to the unionfind values

	for (arrayindex_t i = 0; i < Card[TR]; i++)
	{
		StubbornPriority[i] = i;
	}
	unionfindsort(StubbornPriority, unionfind, Card[TR]);

	// Fourth: find the position of the first transition that is not alone in its conflict set.

	arrayindex_t i;
	for (i = 0; i < Card[TR]; i++)
	{
		if (unionfind[i] != -1)
		{
//RT::rep->status("BREAKING %d", unionfind[i]);
			break;
		}
	}
//RT::rep->status("SINGLETON CLUSTERS: %d", i);
	SingletonClusters = i;
	delete[] clustersize;
	delete[] unionfind;
}

/*!
  Calculate CardBits based on Capacity
  \return the number of bits needed for the given capacity
 */
cardbit_t Petrinet::Capacity2Bits(capacity_t cap)
{
	cardbit_t k = 0;
	while (cap)
	{
		k++;
		cap = cap >> 1;
	}
	return k;
}

/*!
\brief clean up transitions for valgrind
\post Transition data is deleted
*/

/*!
 Check transition for activation
 1. scan through pre-places for testing enabledness
 \todo swap of values to be done with XOR (ineffektiv(er) bei heutigen Compilern! Außerdem haben wir zwei Swaps verschränkt...)
*/
void Petrinet::checkEnabled(NetState &ns, arrayindex_t t)
{
	// scan through all pre-places
	for (arrayindex_t i = 0; i < CardArcs[TR][PRE][t]; ++i)
	{
		if (ns.Current[Arc[TR][PRE][t][i]] < Mult[TR][PRE][t][i])
		{
			// transition is disabled
			if (ns.Enabled[t])
			{
				// enabled --> disabled
				ns.Enabled[t] = false;
				--ns.CardEnabled;
			}
			return;
		}
	}
	// for loop completed: we did not find an insufficiently marked place
	// => transition enabled
	if (!ns.Enabled[t])
	{
		// disabled-->enabled
		ns.Enabled[t] = true;
		++ns.CardEnabled;
	}
}

/*!
 This function checks the enabledness of a netstate and starts at index t
\param[in] t index where the check for enabledness should start
 */
void Petrinet::checkEnabled_Initial(arrayindex_t t)
{
	NetState ns;
	ns.Current = Initial;
	ns.HashCurrent = HashInitial;
	ns.Enabled = Enabled;
	ns.CardEnabled = CardEnabled;
	checkEnabled(ns, t);
	CardEnabled = ns.CardEnabled;
}

/*!
 fire a transition 
 \param[in,out] ns a netstate on which the transition must be enabled
 \param[in] t index of the to fire transition
 \pre the Transition on place t of the Netstate ns must be enabled
 \post Transition was fired -> Markings(Deltas) and Hashes are updated
 */
void Petrinet::fire(NetState &ns, arrayindex_t t)
{
	// Don't even think about firing a disabled transition!
	assert(ns.Enabled[t]);

	// 1. Update current marking
	for (arrayindex_t i = 0; i < CardDeltaT[PRE][t]; i++)
	{
		// there should be enough tokens to fire this transition
		assert(ns.Current[DeltaT[PRE][t][i]] >= MultDeltaT[PRE][t][i]);
		ns.Current[DeltaT[PRE][t][i]] -= MultDeltaT[PRE][t][i];
	}
	for (arrayindex_t i = 0; i < CardDeltaT[POST][t]; i++)
	{
		ns.Current[DeltaT[POST][t][i]] += MultDeltaT[POST][t][i];
	}
	// 2. update hash value
	ns.HashCurrent += DeltaHash[t];
	ns.HashCurrent %= SIZEOF_MARKINGTABLE;
	while (UNLIKELY(ns.HashCurrent < 0))
	{
		// just safety belt, if % returns negative value
		ns.HashCurrent += SIZEOF_MARKINGTABLE; // LCOV_EXCL_LINE
	}
}

/*!
 check if there is a cycle
 \param[in] t arrayindex_t to use for checking
 \return true if the NetState contains a cycle  else false
 */
bool Petrinet::isCycle(arrayindex_t t)
{
	return (CardDeltaT[PRE][t] == 0 && CardDeltaT[POST][t] == 0);
}

/// update enabledness information after having fired transition t
void Petrinet::updateEnabled(NetState &ns, arrayindex_t t)
{
	// 1. check enabled transitions (tt) if some pre-place was decreased by t
	for (arrayindex_t i = 0; i < TrCardDecreased[t]; i++)
	{
		const arrayindex_t tt = TrDecreased[t][i];
		if (ns.Enabled[tt])
		{
			checkEnabled(ns, tt);
		}
	}

	// 2. check disabled transitions (tt) if some pre-place was increased by t
	for (arrayindex_t i = 0; i < TrCardIncreased[t]; i++)
	{
		const arrayindex_t tt = TrIncreased[t][i];
		if (!ns.Enabled[tt])
		{
			checkEnabled(ns, tt);
		}
	}
}

/// fire a transition in reverse direction (for backtracking) and update enabledness of all transitions
void Petrinet::backfire(NetState &ns, arrayindex_t t)
{
	// 1. Update current marking
	for (arrayindex_t i = 0; i < CardDeltaT[PRE][t]; i++)
	{
		ns.Current[DeltaT[PRE][t][i]] += MultDeltaT[PRE][t][i];
	}
	for (arrayindex_t i = 0; i < CardDeltaT[POST][t]; i++)
	{
		// there should be enough tokens to backfire this transition
		assert(ns.Current[DeltaT[POST][t][i]] >= MultDeltaT[POST][t][i]);

		ns.Current[DeltaT[POST][t][i]] -= MultDeltaT[POST][t][i];
	}
	// 2. update hash value
	ns.HashCurrent -= DeltaHash[t];
	ns.HashCurrent %= SIZEOF_MARKINGTABLE;
	while (ns.HashCurrent < 0)
	{
		ns.HashCurrent += SIZEOF_MARKINGTABLE;
	}
}

/// update enabledness after having backfired a transition t
void Petrinet::revertEnabled(NetState &ns, arrayindex_t t)
{
	// 1. check enabled transitions that got tokens from t
	for (arrayindex_t i = 0; i < TrCardIncreased[t]; i++)
	{
		const arrayindex_t tt = TrIncreased[t][i];
		if (ns.Enabled[tt])
		{
			checkEnabled(ns, tt);
		}
	}

	// 2. check disabled transitions that lost tokens to t
	for (arrayindex_t i = 0; i < TrCardDecreased[t]; i++)
	{
		const arrayindex_t tt = TrDecreased[t][i];
		if (!ns.Enabled[tt])
		{
			checkEnabled(ns, tt);
		}
	}
}


void Petrinet::checkCons(char * opening)
{
	pthread_mutex_lock(&kimwitu_mutex);
	RT::rep->status("@ %s: CHECKING CONSISTENCY OF NET %s SKEL %d #PL %d #TR %d", opening, name, isSkeleton, Card[PL],Card[TR]);
	for(arrayindex_t i = 0; i < Card[PL];i++)
	{
		if(!Name[PL][i]) RT::rep->status("NET %s SKEL %d NO PL NAME %d",name, isSkeleton, i);
		if(CardArcs[PL][PRE][i] > Card[TR]) RT::rep->status("NET %s SKEL %d CARDPRE CORRUPT %s",name, isSkeleton, Name[PL][i]);
		if(CardArcs[PL][POST][i] > Card[TR]) RT::rep->status("NET %s SKEL %d CARDPOST CORRUPT %s",name, isSkeleton, Name[PL][i]);
	}
	for(arrayindex_t i = 0; i < Card[TR];i++)
	{
		if(!Name[TR][i]) RT::rep->status("NET %s SKEL %d NO TR NAME %d",name, isSkeleton, i);
		if(CardArcs[TR][PRE][i] > Card[PL]) RT::rep->status("NET %s SKEL %d CARDPRE CORRUPT %s",name, isSkeleton, Name[TR][i]);
		if(CardArcs[TR][POST][i] > Card[PL]) RT::rep->status("NET %s SKEL %d CARDPOST CORRUPT %s",name, isSkeleton, Name[TR][i]);
	}
	for(arrayindex_t i = 0; i < Card[PL];i++)
	{
		for(int j = 0; j < CardArcs[PL][PRE][i];j++)
		{
			if(Arc[PL][PRE][i][j] >= Card[TR]) RT::rep->status("NET %s SKEL %d PRE CORRUPT %s %d",name, isSkeleton, Name[PL][i],j);
			if(Mult[PL][PRE][i][j] == 0) RT::rep->status("NET %s SKEL %d PRE CORRUPT MULT %s %d",name, isSkeleton, Name[PL][i],j);
		}
		for(int j = 0; j < CardArcs[PL][POST][i];j++)
		{
			if(Arc[PL][POST][i][j] >= Card[TR]) RT::rep->status("NET %s SKEL %d POST CORRUPT %s %d",name, isSkeleton, Name[PL][i],j);
			if(Mult[PL][POST][i][j] == 0) RT::rep->status("NET %s SKEL %d POST CORRUPT MULT %s %d",name, isSkeleton, Name[PL][i],j);
		}
	}
	for(arrayindex_t i = 0; i < Card[TR];i++)
	{
		for(int j = 0; j < CardArcs[TR][PRE][i];j++)
		{
			if(Arc[TR][PRE][i][j] >= Card[PL]) RT::rep->status("NET %s SKEL %d PRE CORRUPT %s %d",name, isSkeleton, Name[TR][i],j);
			if(Mult[TR][PRE][i][j] == 0) RT::rep->status("NET %s SKEL %d PRE CORRUPT MULT%s %d",name, isSkeleton, Name[TR][i],j);
		}
		for(int j = 0; j < CardArcs[TR][POST][i];j++)
		{
			if(Arc[TR][POST][i][j] >= Card[PL]) RT::rep->status("NET %s SKEL %d POST CORRUPT %s %d",name, isSkeleton, Name[TR][i],j);
			if(Mult[TR][POST][i][j] == 0) RT::rep->status("NET %s SKEL %d POST CORRUPT MULT%s %d",name, isSkeleton, Name[TR][i],j);
		}
	}
	for(arrayindex_t p = 0; p < Card[PL]; p++)
	{
		for(arrayindex_t a = 0; a < CardArcs[PL][PRE][p];a++)
		{
			arrayindex_t t = Arc[PL][PRE][p][a];
			arrayindex_t b;
			for(b = 0; b < CardArcs[TR][POST][t];b++)
			{
				if(Arc[TR][POST][t][b] == p) break;
			}
			if(b >= CardArcs[TR][POST][t]) RT::rep->status("NET %s SKEL %d PRE TR %s of PL %s DOES NOT MATCH POST PL OF TR", name,isSkeleton, Name[TR][t], Name[PL][p]);
			if(Mult[PL][PRE][p][a] != Mult[TR][POST][t][b]) RT::rep->status("NET %s SKEL %d PRE TR %s of PL %s HAS DIFF MULT THAN PL OF TR", name,isSkeleton, Name[TR][t], Name[PL][p]);
		}
	}
	for(arrayindex_t p = 0; p < Card[PL]; p++)
	{
		for(arrayindex_t a = 0; a < CardArcs[PL][POST][p];a++)
		{
			arrayindex_t t = Arc[PL][POST][p][a];
			arrayindex_t b;
			for(b = 0; b < CardArcs[TR][PRE][t];b++)
			{
				if(Arc[TR][PRE][t][b] == p) break;
			}
			if(b >= CardArcs[TR][PRE][t]) RT::rep->status("NET %s SKEL %d POST TR %s of PL %s DOES NOT MATCH PRE PL OF TR", name,isSkeleton, Name[TR][t], Name[PL][p]);
			if(Mult[PL][POST][p][a] != Mult[TR][PRE][t][b]) RT::rep->status("NET %s SKEL %d POST TR %s of PL %s HAS DIFF MULT THAN PL OF TR", name,isSkeleton, Name[TR][t], Name[PL][p]);
		}
	}
	for(arrayindex_t t = 0; t < Card[TR]; t++)
	{
		for(arrayindex_t a = 0; a < CardArcs[TR][PRE][t];a++)
		{
			arrayindex_t p = Arc[TR][PRE][t][a];
			arrayindex_t b;
			for(b = 0; b < CardArcs[PL][POST][p];b++)
			{
				if(Arc[PL][POST][p][b] == t) break;
			}
			if(b >= CardArcs[PL][POST][p]) RT::rep->status("NET %s SKEL %d PRE PL %s of TR %s DOES NOT MATCH POST TR OF PL", name,isSkeleton, Name[PL][p], Name[TR][t]);
			if(Mult[TR][PRE][t][a] != Mult[PL][POST][p][b]) RT::rep->status("NET %s SKEL %d PRE PL %s of TR %s HAS DIFF MULT THAN TR OF PL", name,isSkeleton, Name[PL][p], Name[TR][t]);
		}
	}
	for(arrayindex_t t = 0; t < Card[TR]; t++)
	{
		for(arrayindex_t a = 0; a < CardArcs[TR][POST][t];a++)
		{
			arrayindex_t p = Arc[TR][POST][t][a];
			arrayindex_t b;
			for(b = 0; b < CardArcs[PL][PRE][p];b++)
			{
				if(Arc[PL][PRE][p][b] == t) break;
			}
			if(b >= CardArcs[PL][PRE][p]) RT::rep->status("NET %s SKEL %d POST PL %s of TR %s DOES NOT MATCH PRE TR OF PL", name,isSkeleton, Name[PL][p], Name[TR][t]);
			if(Mult[TR][POST][t][a] != Mult[PL][PRE][p][b]) RT::rep->status("NET %s SKEL %d POST PL %s of TR %s HAS DIFF MULT THAN TR OF PL", name,isSkeleton, Name[PL][p], Name[TR][t]);
		}
	}
	for(arrayindex_t p = 0; p < Card[PL];p++)
	{
		for(arrayindex_t i = 0; i < PlCardIncreasing[p]; i++)
		{
			arrayindex_t t = PlIncreasing[p][i];
			if(t >= Card[PL]) RT::rep->status("NET %s SKEL %d PL %s HAS CORRUPT INCREASING TR", name,isSkeleton, Name[PL][p]);
			arrayindex_t a;
			for(a = 0; a < CardArcs[TR][POST][t];a++)
			{
				if(Arc[TR][POST][t][a] == p) break;
			}
			if(a >= CardArcs[TR][POST][t]) RT::rep->status("NET %s SKEL %d PL %s HAS WRONG INCREASING TR", name,isSkeleton, Name[PL][p]);
			arrayindex_t b;
			for(b = 0; b < CardArcs[TR][PRE][t];b++)
			{
				if(Arc[TR][PRE][t][b] == p) break;
			}
			int m1 = Mult[TR][POST][t][a];
			if(b < CardArcs[TR][PRE][t])
			{
				int m2 = Mult[TR][PRE][t][b];
				if(m2 >= m1) RT::rep->status("NET %s SKEL %d PL %s HAS NOT REALLY INCREASING TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
				m1 -= m2;
			}
			if(m1 != PlMultIncreasing[p][i]) RT::rep->status("NET %s SKEL %d PL %s HAS DIFFERENTLY INCREASING TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
		}
		for(arrayindex_t i = 0; i < PlCardDecreasing[p]; i++)
		{
			arrayindex_t t = PlDecreasing[p][i];
			if(t >= Card[PL]) RT::rep->status("NET %s SKEL %d PL %s HAS CORRUPT DECREASING TR", name,isSkeleton, Name[PL][p]);
			arrayindex_t a;
			for(a = 0; a < CardArcs[TR][PRE][t];a++)
			{
				if(Arc[TR][PRE][t][a] == p) break;
			}
			if(a >= CardArcs[TR][PRE][t]) RT::rep->status("NET %s SKEL %d PL %s HAS WRONG DECREASING TR %s", name,isSkeleton, Name[PL][p],Name[TR][t]);
			arrayindex_t b;
			for(b = 0; b < CardArcs[TR][POST][t];b++)
			{
				if(Arc[TR][POST][t][b] == p) break;
			}
			int m1 = Mult[TR][PRE][t][a];
			if(b < CardArcs[TR][POST][t])
			{
				int m2 = Mult[TR][POST][t][b];
				if(m2 >= m1) RT::rep->status("NET %s SKEL %d PL %s HAS NOT REALLY DECREASING TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
				m1 -= m2;
			}
			if(m1 != PlMultDecreasing[p][i]) RT::rep->status("NET %s SKEL %d PL %s HAS DIFFERENTLY DECREASING TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
		}
	}	
	for(arrayindex_t t = 0; t < Card[TR];t++)
	{
		for(int i = 0; i < CardDeltaT[PRE][t];i++)
		{
			arrayindex_t p = DeltaT[PRE][t][i];
			if(p >= Card[PL]) RT::rep->status("NET %s SKEL %d PL %s IS CORRUPT IN DELTA T PRE OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
			if(MultDeltaT[PRE][t][i] == 0) RT::rep->status("NET %s SKEL %d PL %s HAS ZERO MULT IN DELTA T PRE OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
			arrayindex_t a;
			for(a = 0; a < CardArcs[PL][POST][p];a++)
			{
				if(Arc[PL][POST][p][a] == t) break;
			}
			if(a >= CardArcs[PL][POST][p]) RT::rep->status("NET %s SKEL %d PL %s HAS SHOULD NOT BELONG TO DELTA T PRE OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
			int m1 = Mult[PL][POST][p][a];
			arrayindex_t b;
			for(b = 0; b < CardArcs[PL][PRE][p];b++)
			{
				if(Arc[PL][PRE][p][b] == t) break;
			}
			if(b < CardArcs[PL][PRE][p])
			{
				int m2 = Mult[PL][PRE][p][b];
				if(m2 >= m1) RT::rep->status("NET %s SKEL %d PL %s SHOULD NOT BELONG TO DELTA T PRE OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
				m1 -= m2;
			}
			if(m1 != MultDeltaT[PRE][t][i]) RT::rep->status("NET %s SKEL %d PL %s HAS WRONG DELTA T PRE OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
			
		}
		for(int i = 0; i < CardDeltaT[POST][t];i++)
		{
			arrayindex_t p = DeltaT[POST][t][i];
			if(p >= Card[PL]) RT::rep->status("NET %s SKEL %d PL %s IS CORRUPT IN DELTA T POST OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
			if(MultDeltaT[POST][t][i] == 0) RT::rep->status("NET %s SKEL %d PL %s HAS ZERO MULT IN DELTA T POST OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
			arrayindex_t a;
			for(a = 0; a < CardArcs[PL][PRE][p];a++)
			{
				if(Arc[PL][PRE][p][a] == t) break;
			}
			if(a >= CardArcs[PL][PRE][p]) RT::rep->status("NET %s SKEL %d PL %s HAS SHOULD NOT BELONG TO DELTA T POST OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
			int m1 = Mult[PL][PRE][p][a];
			arrayindex_t b;
			for(b = 0; b < CardArcs[PL][POST][p];b++)
			{
				if(Arc[PL][POST][p][b] == t) break;
			}
			if(b < CardArcs[PL][POST][p])
			{
				int m2 = Mult[PL][POST][p][b];
				if(m2 >= m1) RT::rep->status("NET %s SKEL %d PL %s SHOULD NOT BELONG TO DELTA T POST OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
				m1 -= m2;
			}
			if(m1 != MultDeltaT[POST][t][i]) RT::rep->status("NET %s SKEL %d PL %s HAS WRONG DELTA T POST OF TR %s", name,isSkeleton, Name[PL][p], Name[TR][t]);
			
		}
		if(TrCardIncreased)
		for(int i = 0; i < TrCardIncreased[t];i++)
		{
			arrayindex_t tt = TrIncreased[t][i];
			if(tt >= Card[TR]) RT::rep->status("NET %s SKEL %d TRINCREASED CORRUPT %s", name,isSkeleton, Name[TR][t]);
		}
		if(TrCardDecreased)
		for(int i = 0; i < TrCardDecreased[t];i++)
		{
			arrayindex_t tt = TrDecreased[t][i];
			if(tt >= Card[TR]) RT::rep->status("NET %s SKEL %d TRDECREASED CORRUPT %s", name,isSkeleton, Name[TR][t]);
		}
		if(TrCardDecreasing)
		for(int i = 0; i < TrCardDecreasing[t];i++)
		{
			arrayindex_t tt = TrDecreasing[t][i];
			if(tt >= Card[TR]) RT::rep->status("NET %s SKEL %d TRDECREASING CORRUPT %s", name,isSkeleton, Name[TR][t]);
		}
		
	}

	for(arrayindex_t p = 0; p < Card[PL];p++)
	{
		int * pr = new int [Card[TR]];
		memset(pr,0,Card[TR] * sizeof(int));
		int * po = new int [Card[TR]];
		memset(po,0,Card[TR] * sizeof(int));
		for(arrayindex_t a = 0; a < CardArcs[PL][PRE][p];a++)
		{
			pr[Arc[PL][PRE][p][a]] = Mult[PL][PRE][p][a];
		}
		for(arrayindex_t a = 0; a < CardArcs[PL][POST][p];a++)
		{
			po[Arc[PL][POST][p][a]] = Mult[PL][POST][p][a];
		}
		for(int i = 0; i < CardOnlyPre[PL][p];i++)
		{
			arrayindex_t t = OnlyPre[PL][p][i];
			if(t >= Card[TR]) RT::rep->status("NET %s SKEL %d PL ONLYPRE CORRUPT %s TR # %d", name,isSkeleton, Name[PL][p],t);
			if(pr[t] <= 0) RT::rep->status("NET %s SKEL %d PL ONLYPRE CORRUPT II %s", name,isSkeleton, Name[PL][p]);
			if(po[t] != 0) RT::rep->status("NET %s SKEL %d PL ONLYPRE CORRUPT III %s", name,isSkeleton, Name[PL][p]);
			pr[t] = 0;
		}
		for(int i = 0; i < CardGreaterPre[PL][p];i++)
		{
			arrayindex_t t = GreaterPre[PL][p][i];
			if(t >= Card[TR]) RT::rep->status("NET %s SKEL %d PL GREATERPRE CORRUPT %s", name,isSkeleton, Name[PL][p]);
			if(pr[t] <= 0) RT::rep->status("NET %s SKEL %d PL GREATERPRE CORRUPT II %s", name,isSkeleton, Name[PL][p]);
			if(po[t] <= 0) RT::rep->status("NET %s SKEL %d PL GREATERPRE CORRUPT III %s", name,isSkeleton, Name[PL][p]);
			if(po[t] >= pr[t]) RT::rep->status("NET %s SKEL %d PL GREATERPRE CORRUPT IIII %s", name,isSkeleton, Name[PL][p]);
			pr[t] = po[t] = 0;
		}
		for(int i = 0; i < CardPreEqualPost[PL][p];i++)
		{
			arrayindex_t t = PreEqualPost[PL][p][i];
			if(t >= Card[TR]) RT::rep->status("NET %s SKEL %d PL PREEQUALPOST CORRUPT %s", name,isSkeleton, Name[PL][p]);
			if(pr[t] <= 0) RT::rep->status("NET %s SKEL %d PL PREEQUALPOST CORRUPT II %s", name,isSkeleton, Name[PL][p]);
			if(po[t] != pr[t]) RT::rep->status("NET %s SKEL %d PL PREEQUALPOST CORRUPT III %s", name,isSkeleton, Name[PL][p]);
			pr[t] = po[t] = 0;
		}
		for(int i = 0; i < CardGreaterPost[PL][p];i++)
		{
			arrayindex_t t = GreaterPost[PL][p][i];
			if(t >= Card[TR]) RT::rep->status("NET %s SKEL %d PL GREATERPOST CORRUPT %s", name,isSkeleton, Name[PL][p]);
			if(po[t] <= 0) RT::rep->status("NET %s SKEL %d PL GREATERPOST CORRUPT II %s", name,isSkeleton, Name[PL][p]);
			if(pr[t] <= 0) RT::rep->status("NET %s SKEL %d PL GREATERPOST CORRUPT III %s", name,isSkeleton, Name[PL][p]);
			if(pr[t] >= po[t]) RT::rep->status("NET %s SKEL %d PL GREATEROST CORRUPT IIII %s", name,isSkeleton, Name[PL][p]);
			pr[t] = po[t] = 0;
		}
		for(int i = 0; i < CardOnlyPost[PL][p];i++)
		{
			arrayindex_t t = OnlyPost[PL][p][i];
			if(t >= Card[TR]) RT::rep->status("NET %s SKEL %d PL ONLYPOST CORRUPT %s", name,isSkeleton, Name[PL][p]);
			if(po[t] <= 0) RT::rep->status("NET %s SKEL %d PL ONLYPOST CORRUPT II %s", name,isSkeleton, Name[PL][p]);
			if(pr[t] != 0) RT::rep->status("NET %s SKEL %d PL ONLYPOST CORRUPT III %s", name,isSkeleton, Name[PL][p]);
			po[t] = 0;
		}
		delete [] pr;
		delete [] po;
	}
	for(arrayindex_t t = 0; t < Card[TR]; t++)	
	{
		int * pr = new int [Card[PL]];
		memset(pr,0,Card[PL] * sizeof(int));
		int * po = new int [Card[PL]];
		memset(po,0,Card[PL] * sizeof(int));
		for(arrayindex_t a = 0; a < CardArcs[TR][PRE][t];a++)
		{
			pr[Arc[TR][PRE][t][a]] = Mult[TR][PRE][t][a];
		}
		for(arrayindex_t a = 0; a < CardArcs[TR][POST][t];a++)
		{
			po[Arc[TR][POST][t][a]] = Mult[TR][POST][t][a];
		}
		for(int i = 0; i < CardOnlyPre[TR][t];i++)
		{
			arrayindex_t p = OnlyPre[TR][t][i];
			if(p >= Card[PL]) RT::rep->status("NET %s SKEL %d TR ONLYPRE CORRUPT %s PL # %d", name,isSkeleton, Name[TR][t],p);
			if(pr[p] <= 0) RT::rep->status("NET %s SKEL %d TR ONLYPRE CORRUPT II %s", name,isSkeleton, Name[TR][t]);
			if(po[p] != 0) RT::rep->status("NET %s SKEL %d TR ONLYPRE CORRUPT III %s", name,isSkeleton, Name[TR][t]);
			pr[p] = po[p] = 0;
		}
		for(int i = 0; i < CardGreaterPre[TR][t];i++)
		{
			arrayindex_t p = GreaterPre[TR][t][i];
			if(p >= Card[PL]) RT::rep->status("NET %s SKEL %d TR GREATERPRE CORRUPT %s", name,isSkeleton, Name[TR][t]);
			if(pr[p] <= 0) RT::rep->status("NET %s SKEL %d TR GREATERPRE CORRUPT II %s", name,isSkeleton, Name[TR][t]);
			if(po[p] <= 0) RT::rep->status("NET %s SKEL %d TR GREATERPRE CORRUPT III %s", name,isSkeleton, Name[TR][t]);
			if(po[p] >=  pr[p]) RT::rep->status("NET %s SKEL %d TR GREATERPRE CORRUPT IIII %s", name,isSkeleton, Name[TR][t]);
			pr[p] = po[p] = 0;
		}
		for(int i = 0; i < CardPreEqualPost[TR][t];i++)
		{
			arrayindex_t p = PreEqualPost[TR][t][i];
			if(p >= Card[PL]) RT::rep->status("NET %s SKEL %d TR PREEQUALPOST CORRUPT %s", name,isSkeleton, Name[TR][t]);
			if(pr[p] <= 0) RT::rep->status("NET %s SKEL %d TR PREEQUALPOST CORRUPT II %s", name,isSkeleton, Name[TR][t]);
			if(po[p] !=  pr[p]) RT::rep->status("NET %s SKEL %d TR PREEQUALPOST CORRUPT IIII %s", name,isSkeleton, Name[TR][t]);
			pr[p] = po[p] = 0;
		}
		for(int i = 0; i < CardGreaterPost[TR][t];i++)
		{
			arrayindex_t p = GreaterPost[TR][t][i];
			if(p >= Card[PL]) RT::rep->status("NET %s SKEL %d TR GREATEROST CORRUPT %s", name,isSkeleton, Name[TR][t]);
			if(po[p] <= 0) RT::rep->status("NET %s SKEL %d TR GREATERPOST CORRUPT II %s", name,isSkeleton, Name[TR][t]);
			if(pr[p] <= 0) RT::rep->status("NET %s SKEL %d TR GREATERPOST CORRUPT III %s", name,isSkeleton, Name[TR][t]);
			if(pr[p] >=  po[p]) RT::rep->status("NET %s SKEL %d TR GREATERPOST CORRUPT IIII %s", name,isSkeleton, Name[TR][t]);
			pr[p] = po[p] = 0;
		}
		for(int i = 0; i < CardOnlyPost[TR][t];i++)
		{
			arrayindex_t p = OnlyPost[TR][t][i];
			if(p >= Card[PL]) RT::rep->status("NET %s SKEL %d TR ONLYPOST CORRUPT %s", name,isSkeleton, Name[TR][t]);
			if(po[p] <= 0) RT::rep->status("NET %s SKEL %d TR ONLYPOST CORRUPT II %s", name,isSkeleton, Name[TR][t]);
			if(pr[p] != 0) RT::rep->status("NET %s SKEL %d TR ONLYPOST CORRUPT III %s", name,isSkeleton, Name[TR][t]);
			pr[p] = po[p] = 0;
		}
	}
		
	
	pthread_mutex_unlock(&kimwitu_mutex);
}
