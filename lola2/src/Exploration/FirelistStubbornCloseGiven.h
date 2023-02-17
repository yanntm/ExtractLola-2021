/*!
\file
\author Markus
\status new

\brief gets a set U of transitions and returns a stubborn set that contains U
*/

#pragma once

#include <Core/Dimensions.h>
#include <Net/Petrinet.h>
#include <Exploration/FirelistStubbornDeadlock.h>

class StatePredicateProperty;

class FirelistStubbornCloseGiven : public Firelist

{
public:

    FirelistStubbornDeadlock *dl;

    arrayindex_t getFirelist(NetState &ns, arrayindex_t **){return 0;}
    arrayindex_t getFirelist(NetState &ns, arrayindex_t **, arrayindex_t *, bool *, arrayindex_t, bool*);

    FirelistStubbornCloseGiven(Petrinet *);
};
