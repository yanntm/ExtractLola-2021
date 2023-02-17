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

\brief Global data for net specific information

*/

#pragma once

#include <Core/Dimensions.h>
#include <InputOutput/JSON.h>

class Symbol;

#include <iostream>
#include <map>
#include <set>
#include <vector>

class Matrix;
class NetState;

/*
 * the theory:
 * -let G = (V, E) be a directed graph
 * -we call V' subset of V an homogenuous set of G if for all u,v in V' have exactly the same neighborhood (predecessors and successors)
 *  -a homogeneous set will also be called (equivalence) class (as all contained vertices are equivalent), but this is inaccurate as they do not have to be maximal
 * -we call a (not necessarily disjoint) pair (O,I) of subsets of V a directed clique if for every x in O and every y in I the edge xy is in E
 *  -for a directed clique (O,I) we call O and I half cliques and I is the 1-half clique and O the 0-half clique
 *  -for a directed clique (O,I) the set O is the co-half clique of I and vice versa
 *
 * -in our application we will be given a sequence of directed cliques C={C1, ..., Ck} representing all vertices and all edges of a directed graph G=(V,E)
 * -if Ci=(Oi,Ii) for all i, then C0 = {O1, ..., Ok} is the set of 0-half cliques and C1 = {I1, ..., Ik} is the set of 1-half cliques
 * -we successively build the following data structure to represent G:
 * 1) a partition H = {H0, H1, ...} of V into homogeneous sets (as a map h: V -> H)
 * 2) maps c0: H -> 2^C0 and c1: H -> 2^C1 assigning to every class Hi all 0-half cliques c0(Hi), respectively 1-half cliques c1(Hi), that contain Hi
 * 3) maps h0: C0 -> 2^H and h1: C1 -> 2^H describing how every 0-half clique Oi decomposes into classes h0(Oi), respectively 1-half cliques Ii into h1(Ii)
 *
 * -as we describe the edges of G implicitly, the data structure (1-3) can be built fairly efficiently when given C; explanation see 'addHalfClique'
 * -having (1-3), we can easily build the adjacency lists of G classwise; explanation see 'extractAdjacencyLists'
 */
struct ClassMetaData
{
	int size;
	int overlapWithClique;
	int splitOffClass;
};

class Petrinet
{
public:
	void checkCons(char *);
	char *name;
	bool isSkeleton;
	// Core information about nodes

	/// number of nodes: Card[PL] = places, Card[TR] = transitions
	arrayindex_t *Card;

	/// names of the nodes
	/// Name[TR][15] is the name of transition 15.
	/// Attention! Names will be shared between several net objects
	/// so do not try to delete them
	const char ***Name;

	/// number of conflict clusters
	arrayindex_t cardClusters;

	// Core information about arcs

	/// CardArcs[PL][PRE][17] is the number of transitions that produce on place 17.
	arrayindex_t ***CardArcs;

	/// For each node, the indices of nodes in pre- reps. post set.
	/// Arc[PL][POST][17][4] is the number of the 5th transition that consumes tokens from place 17.
	arrayindex_t ****Arc;

	/// for each node, the multiplicities of arcs in pre- resp. post set.
	mult_t ****Mult;

	// Core Information on initial marking

	/// initial marking
	capacity_t *Initial;

	/// hash value of initial marking
	hash_t HashInitial;

	// Connection to symbol

	Symbol *** thesymbol;

	// Supplementary information on places

	/// number of signifcant places; significant places are such that comparing markings on the significant places suffices for checking equality
	arrayindex_t CardSignificant;

	/// The hash factor for a place. We compute hash values for places as \f$\sum_{p \in P} m(p)\cdot hash(p)\f$
	hash_t *Hash;

	/// The maximum number of tokens that, according to input specification, can ever be present on this place.
	capacity_t *Capacity;

	/// The number of bits needed for representing the numbers 0 .. Capacity[i]
	cardbit_t *CardBits;

	/// The sum of the CardBits of all places
	arrayindex_t SizeOfBitVector;

	/// place based conflict clusters
	arrayindex_t *ClusterCardPlaces;
	arrayindex_t **ClusterPlaces;
	arrayindex_t *ClusterPlaceIndex;

	/// place based conflict structures
	arrayindex_t **PlIncreasing;
	arrayindex_t **PlDecreasing;
	arrayindex_t *PlCardDecreasing;
	arrayindex_t *PlCardIncreasing;
	mult_t **PlMultIncreasing;
	mult_t **PlMultDecreasing;

	// Supplementary information on transitions

	/// fairness specification
	fairnessAssumption_t *Fairness;

	/// Activation status
	bool *Enabled;

	/// number of enabled transitions
	arrayindex_t CardEnabled;

	/// When transition i fires from m to m', DeltaHash[i] is hash(m') - hash(m). This value is independent of m and m'
	hash_t *DeltaHash;

	/// That many places need to be changed when transition i fires CardDeltaT[PRE] for places that lose tokens, CardDeltaT[POST] for those that gain tokens
	arrayindex_t **CardDeltaT;

	/// These are the places that change their marking when transition i fires DeltaT[PRE] for places that lose tokens, DeltaT[POST] for those that gain tokens
	arrayindex_t ***DeltaT;

	/// This is the amount of change
	mult_t ***MultDeltaT;

	/// Firing transition i decreases at least one pre-place of the transitions
	/// in Decreased[i]
	arrayindex_t *TrCardDecreased;
	arrayindex_t **TrDecreased;

	/// At least one pre-place of transition i is decreased by firing any of
	/// transitions in Decreasing[i]
	arrayindex_t *TrCardDecreasing;
	arrayindex_t **TrDecreasing;

	/// Firing transition i increases at least one pre-place of transitions in Increased[i]
	arrayindex_t *TrCardIncreased;
	arrayindex_t **TrIncreased;

	/// The progress measure for the sweep line method
	int32_t *ProgressMeasure;

	/// the order in which we look for start transition of stubborn set
	arrayindex_t *StubbornPriority;

	/// the first transtion in StubbornPriority that is not alone in its
	/// conflict cluster
	arrayindex_t SingletonClusters;

	/// transition based conflict clusters
	arrayindex_t *ClusterCardTransitions;
	arrayindex_t **ClusterTransitions;
	arrayindex_t *ClusterTransitionIndex;

	// Supplementary information on nodes (independent of node type)

	/// Physically, for each node x, the arrays Arc[NODETYPE][PRE][x] and Arc[NODETYPE][POST][x] are just
	/// one array that consists of 5 consecutive sections:
	/// 1. nodes in pre(x) \ post(x)
	/// 2. nodes in pre(x) cap post(x) such that mult of incoming arc is greater than mult of outgoing arc
	/// 3. nodes in pre(x) cap post(x) such that mult of incoming and outpoing arc are equal
	/// 4. nodes in pre(x) cap post(x) such that mult of outgoing arc is greater than mult of incoming arc
	/// 5. nodes in post(x) \ pre(x)
	/// pre refers to sections 1-4
	/// post refers to sections 2-5
	/// the following values give individual access to the sections.
	arrayindex_t ***OnlyPre;
	arrayindex_t **CardOnlyPre;
	arrayindex_t ***GreaterPre;
	arrayindex_t **CardGreaterPre;
	arrayindex_t ***PreEqualPost;
	arrayindex_t **CardPreEqualPost;
	arrayindex_t ***GreaterPost;
	arrayindex_t **CardGreaterPost;
	arrayindex_t ***OnlyPost;
	arrayindex_t **CardOnlyPost;

	// conflictcluster of all nodes
	uf_node_t *UnionFind;

	static Petrinet *InitialNet;
	// status information
	bool preprocessingfinished;
	// Methods

	// Just NULL all arrays
	Petrinet();

	// delete all arrays
	~Petrinet();

	/// calculates progress measure for all transitions (used by sweep line method)
	void setProgressMeasure();

	/// sorts a concrete pair of arcs and multiplicities (used as helper method for sortAllArcs)
	void sortArcs(arrayindex_t *arcs, mult_t *mults, const arrayindex_t from,
				  const arrayindex_t to);

	/// swaps two places (does not care about ordering)
	void swapPlaces(arrayindex_t, arrayindex_t);

	/// print the net (using printf)
	void print();

	bool findlow;

	/// assumes that raw net is read and places, transitions and the edges in-between are set properly. Computes additional net information used to speed up the simulation.
	void preprocess0();
	void preprocess1();
	void preprocess2();
	void preprocess3();

	/// calculates the incidence matrix of the net
	Matrix getIncidenceMatrix(node_t line_type = PL);

	void computeConflictClusters();

	// calculates DeltaT and DeltaHash for each transition
	void preprocess_organizeDeltas();

	/// calculates the set of conflicting transitions for each transition
	void preprocess_organizeConflictingTransitions();

	// moves all elements in the range [first1,last1), that are also in [first2,last2), to result.
	// returns the number of elements moved.
	arrayindex_t set_moveall(arrayindex_t *first1, arrayindex_t *last1,
							 arrayindex_t *first2, arrayindex_t *last2,
							 arrayindex_t *result);

	/// calculates all significant places and changes order of places(!)
	void setSignificantPlaces();

	template <typename T>
	void sortarcs(arrayindex_t *, T *, arrayindex_t);

	/// Compute required nr of bits from max. nr of tokens
	static cardbit_t Capacity2Bits(capacity_t);

	/// Check transition for activation
	void checkEnabled(NetState &ns, arrayindex_t t);

	/// Check transition for activation
	/// wrapper function for the paring phase of the net, when no NetState object has been created yet.
	void checkEnabled_Initial(arrayindex_t t);

	/// fire a transition
	void fire(NetState &ns, arrayindex_t t);

	/// update enabledness of all transitions
	void updateEnabled(NetState &ns, arrayindex_t t);

	/// fire a transition in reverse direction (for backtracking)
	void backfire(NetState &ns, arrayindex_t t);

	/// update enabledness of all transitions after backfiring
	void revertEnabled(NetState &ns, arrayindex_t t);

	bool isCycle(arrayindex_t t);
	void createTransitionEquation(arrayindex_t transition, arrayindex_t *variables,
								  long long int *coefficients,
								  arrayindex_t &size, bool dual = false);
	void clustersort(arrayindex_t *cl, arrayindex_t fr, arrayindex_t to, arrayindex_t p);
	void backclustersort(arrayindex_t *cl, arrayindex_t fr, arrayindex_t to, arrayindex_t p);
	void sortlength(arrayindex_t *a, arrayindex_t card);

	int cardbuchistates;

	/// compute conflict sets
	void computeConflictingTransitionsOld();
	void computeBackConflictingTransitionsOld();
	void computeConflictingTransitionsNew();
	void computeBackConflictingTransitionsNew();

	void extractConflictCliques(std::vector<int> &transitions,
								std::vector<int> &consumptionOf,
								std::vector<int> &productionOf,
								std::vector<int> &tsByConsumption,
								std::vector<int> &tsByProduction,
								std::vector<std::pair<int, int> > &cliques);

	void extractConflictClique(std::vector<int> &transitions,
							   std::vector<int> &consumptionOf,
							   std::vector<int> &productionOf,
							   std::pair<int, int> &clique);

	void extractActivationClique(std::vector<int> &transitions,
								 std::vector<int> &consumptionOf,
								 std::vector<int> &productionOf,
								 std::pair<int, int> &clique);

	void addHalfClique(std::vector<int> &vertices, int begin, int end,
					   std::vector<int> &mapVertexToClass,
					   std::vector<std::vector<int> > &mapCliqueToClasses,
					   std::vector<std::vector<int> > &mapCoCliqueToClasses,
					   std::vector<std::vector<int> > &mapClassToCliques,
					   std::vector<std::vector<int> > &mapClassToCoCliques,
					   std::vector<ClassMetaData> &mapClassToMeta);

	void extractBidirectionalAdjacencyLists(std::vector<int> &mapVertexToClass,
											std::vector<std::vector<int> > &mapCliqueOToClasses,
											std::vector<std::vector<int> > &mapCliqueIToClasses,
											std::vector<std::vector<int> > &mapClassToCliqueOs,
											std::vector<std::vector<int> > &mapClassToCliqueIs,
											std::vector<std::vector<int> > &mapClassToAdjacencyList,
											std::vector<std::pair<int, int> > &mapClassToBiInterval);

	void extractSuccessorAdjacencyLists(std::vector<int> &mapVertexToClass,
										std::vector<std::vector<int> > &mapCliqueOToClasses,
										std::vector<std::vector<int> > &mapCliqueIToClasses,
										std::vector<std::vector<int> > &mapClassToCliqueOs,
										std::vector<std::vector<int> > &mapClassToCliqueIs,
										std::vector<std::vector<int> > &mapClassToAdjacencyList);
};
