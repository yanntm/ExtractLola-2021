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
\author Christian Koch
\status new
\ingroup ctl

\brief Definition of the CTL EU operator.
*/

#pragma once

#include <Core/Dimensions.h>
#include <Formula/CTL/DFSFormula.h>
#include <Net/Petrinet.h>

class EUFormula : public DFSFormula
{
public:
    EUFormula(Petrinet *,  CTLFormula *phi, CTLFormula *psi);
    /// returns info on a particular node in the formula tree
    FormulaInfo *getInfo() const;
    /// returns the number of subformulas
    int countSubFormulas() const;
    std::string ctl2string() {return std::string("E(") + phi -> ctl2string() + std::string(" U ") + psi -> ctl2string() + std::string(")");}

private:
    void initAtomics(NetState &ns);
    void updateAtomics(NetState &ns, arrayindex_t t);
    void revertAtomics(NetState &ns, arrayindex_t t);

    /// check whether E(phi U psi) holds
    bool check(Store<void *> &s, NetState &ns, Firelist &firelist, Firelist &sfirelist,
               std::vector<int> &witness);

    bool checkfair(Store<void *> &s, NetState &ns, Firelist &firelist,
               std::vector<int> &witness);

    void DEBUG_print();

    void gatherPayloadInformation(arrayindex_t &numDFS, arrayindex_t &numCachedResults);
    void setPayloadInformation(arrayindex_t cachedResultOffset, size_t payloadSize);

    /// the first inner formula for E(phi U psi)
    CTLFormula *phi;
    /// the second inner formula for E(phi U psi)
    CTLFormula *psi;
    virtual void print();
    void setVisible();
};
