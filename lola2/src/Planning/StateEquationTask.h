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

#pragma once
#include <Core/Dimensions.h>
#include <Witness/Path.h>
#include <Planning/Task.h>
#include <Planning/Task.h>
#include <Frontend/Parser/ast-system-k.h>
#include <Frontend/Parser/ast-system-rk.h>
#include <Frontend/Parser/ast-system-unpk.h>

class StateEquationTask : public Task 
{
public:
    static int id;
    int myid;
    kc::tFormula myFormula;
    bool finished;
    ternary_t result;
    StateEquationTask(Petrinet * n, int par, tFormula  f, int fid);
    ~StateEquationTask();

    /// get result from Sara
    virtual ternary_t getResult();

    /// interprete and display the result
    virtual char * interpreteResult(ternary_t r);

    /// return the witness path
    virtual Path getWitnessPath();

    /// return the target marking
    virtual capacity_t *getMarking();

    /// return the statistics of
    virtual statusrecord * getStatistics();

    /// return the status of the analysis every 5 seconds
    statusrecord * getStatus();


    static void buildTask(Petrinet *, int,tFormula , int);
    static pthread_mutex_t stateequation_mutex;
    static void * safetystateequationthread(void *);
    static void * quasilivenessstateequationthread(void *);
    static void * constantplacestateequationthread(void *);
    static ternary_t getSafetyResult(arrayindex_t);
    static void getQuasilivenessResult();
    static void getConstantPlaceResult();
    void derail();
    Task * copy();

private:
    int saraIsRunning;
};
