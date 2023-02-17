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
#include <Core/Runtime.h>
#include <Portfolio/portfoliomanager.h>
#include <InputOutput/InputOutput.h>
#include <Formula/LTL/BuechiAutomata.h>
#include <Formula/StatePredicate/AtomicStatePredicate.h>
#include <Net/NetState.h>


bool BuechiAutomata::isAcceptingState(arrayindex_t state)
{
    return isStateAccepting[state];
}

arrayindex_t BuechiAutomata::getNumberOfStates()
{
    return cardStates;
}

BuechiAutomata::~BuechiAutomata()
{
    for (arrayindex_t i = 0; i < cardStates; i++)
    {
        delete[] nextstate[i];
        delete[] guard[i];
    }

    delete[] nextstate;
    delete[] guard;
    delete[] cardTransitions;
    delete[] isStateAccepting;
}


int current_next_string_index_number = 1;

char *produce_next_string(int *val)
{
    current_next_string_index_number++;
    int length = (int) log10(current_next_string_index_number) + 2;
    char *buf = new char[length]();
    sprintf(buf, "%d", current_next_string_index_number);
    *val = current_next_string_index_number;
    return buf;
}

void BuechiAutomata::writeBuechi()
{
    Output buechifile("Buechi automaton", std::string(RT::args.writeBuechi_arg) + ".buechi");
    fprintf(buechifile, "buechi\n{\n");
    for (int i = 0; i < cardStates; i++)
    {
        fprintf(buechifile, "\tState%d :\n", i);
        for (int j = 0; j < cardTransitions[i]; j++)
        {
            fprintf(buechifile, "\t\t%s => State%d\n", guard[i][j]->toString(), nextstate[i][j]);
        }
        if (i < cardStates - 1)
        {
            fprintf(buechifile, "\t\t,\n");
        }
    }
    fprintf(buechifile, "}\naccept\n{\n");
printf("}\naccept\n{\n");
    bool notfirst = false;
    for (int k = 0; k < cardStates; k++)
    {
        if (isStateAccepting[k])
        {
            if (notfirst)
            {
                fprintf(buechifile, " ,\n");
printf(" ,\n");
            }
            notfirst = true;
            fprintf(buechifile, "\tState%d", k);
printf( "\tState%d", k);
        }
    }
    fprintf(buechifile, "\n};\n");
printf("\n};\n");
}

#define MIN(X,Y) ( ( (X) < (Y) ) ? (X) : (Y))

void BuechiAutomata::setVisible()
{
	visible = new bool * [cardStates];
	int * stack = new int [cardStates];
	int * lowlink = new int [cardStates];
	int * dfs = new int [cardStates];
	int * index = new int [cardStates];
	int * tarjanstack = new int [cardStates];
	bool * seen = new bool [cardStates];
	bool * ontarjan = new bool [cardStates];
	for(int i = 1; i < cardStates;i++) 
	{
		seen[i] = ontarjan[i] = false;
		visible[i] = NULL;
	}
	seen[0] = ontarjan[0] = true;
	visible[0] = NULL;
	int currentstate = 0;
	int dfsstackpointer = 0;
	tarjanstack[0] = 0;
	int tarjanstackpointer = 1;
	int nextdfs = 1;
	int currentlowlink = lowlink[0] = 0;
	int currentdfs = dfs[0] = 0;
	int currentindex = cardTransitions[0];

	while(true)
	{
		// continue exploring current state

		if(currentindex--)
		{
			// current state has unexplored transitions
			// --> fire transition
			int nextst = nextstate[currentstate][currentindex];
			if(seen[nextst])
			{
				// next state already seen
				if(ontarjan[nextst]) currentlowlink = MIN(currentlowlink,lowlink[nextst]);
				continue;
			}
			// next state not yet seen

			// save old configuration
			lowlink[currentstate] = currentlowlink;
			dfs[currentstate] = currentdfs;
			index[currentstate] = currentindex;
			stack[dfsstackpointer++] = currentstate;

			// initialize new configuration
			currentstate = nextst;
			currentdfs = dfs[nextst] = currentlowlink = lowlink[nextst] = nextdfs++;
			seen[nextst] = ontarjan[nextst] = true;
			tarjanstack[tarjanstackpointer++] = nextst;
			currentindex = cardTransitions[nextst];
			continue;
		}
		// current state does not have unexplored transitions

		// check for closed scc

		if(currentdfs == currentlowlink)
		{
			// close scc

			// collect visibility record
			bool * myvisibility = new bool [net->Card[TR]];
			memset(myvisibility,0,net->Card[TR] * sizeof(bool));
			// remove scc from tarjan stack
			int tarjanstate;
			do
			{
				tarjanstate = tarjanstack[--tarjanstackpointer];
				visible[tarjanstate] = myvisibility;
				for(int i = 0; i < cardTransitions[tarjanstate];i++)
				{
					// get visible transitions of guard
					guard[tarjanstate][i]->setVisible(myvisibility);
					int succ = nextstate[tarjanstate][i];
					if(visible[succ])
					{
						for(arrayindex_t j = 0; j < net->Card[TR];j++)
						{
							myvisibility[j] = myvisibility[j] || visible[succ][j];
						}
					}
				}
				ontarjan[tarjanstate] = false;
			}
			while(tarjanstate != currentstate);
		}
		// close state 

		if(!dfsstackpointer) return;

		// return to previous configuration
		currentstate = stack[--dfsstackpointer];
		currentlowlink = MIN(currentlowlink,lowlink[currentstate]);
		currentindex = index[currentstate];
		currentdfs = dfs[currentstate];
		continue;
	}
}

void BuechiAutomata::printVisible()
{
	pthread_mutex_lock(&portfoliomanager::portfolio_mutex);
	std::cout << " ====== " << net -> name << " ======" << std::endl;
	RT::args.writeBuechi_arg = net -> name;
	writeBuechi();
	for(int i = 0; i < cardStates;i++)
	{
		std::cout << i << ": ";
		for(arrayindex_t j = 0; j < net->Card[TR];j++)
		{
			if(visible[i][j]) std::cout << net->Name[TR][j] << " ";
		}
		std::cout << std::endl;
	}
	pthread_mutex_unlock(&portfoliomanager::portfolio_mutex);
}
