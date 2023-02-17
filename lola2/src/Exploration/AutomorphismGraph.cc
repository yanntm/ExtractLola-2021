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
#include <Exploration/AutomorphismGraph.h>
#include <Core/Runtime.h>
#include <Exploration/Constraints.h>

arrayindex_t Vertex_HL::card = 0;

Vertex_HL::Vertex_HL() : succ(NULL), colorSucc(NULL), pre(NULL), colorPre(NULL)
{
    ++card;
}

/*!
Quicksort

\param arcs  array of nodes
\param color  arc inscriptions
\param from  start index of the area to sort
\param to  end index of the area to sort
*/
void Vertex_HL::sortArcs(Vertex_HL **arcs, arrayindex_t *color, arrayindex_t from, arrayindex_t to)
{
    arrayindex_t blue = from;
    arrayindex_t white = from + 1;
    arrayindex_t red = to;

    arrayindex_t pivot = color[0];

    arrayindex_t tmpcolor;
    Vertex_HL *tmpvertex;
    while (red > white)
    {
        if (color[white] > pivot)
        {
            // swap element at white with element left of red
            tmpcolor = color[white];
            tmpvertex = arcs[white];
            color[white] = color[--red];
            arcs[white] = arcs[red];
            color[red] = tmpcolor;
            arcs[red] = tmpvertex;
        }
        else
        {
            if (color[white] == pivot)
            {
                ++white;
            }
            else
            {
                // swap element at white with element at blue
                tmpcolor = color[white];
                tmpvertex = arcs[white];
                color[white] = color[blue];
                arcs[white++] = arcs[blue];
                color[blue] = tmpcolor;
                arcs[blue++] = tmpvertex;
            }
        }
    }
    if (blue > from)
    {
        sortArcs(arcs, color, from, blue);
    }
    if (red < to)
    {
        sortArcs(arcs, color, red, to);
    }
}
