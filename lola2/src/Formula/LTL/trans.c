/***** ltl2ba : trans.c *****/

/* Written by Denis Oddoux, LIAFA, France                                 */
/* Copyright (c) 2001  Denis Oddoux                                       */
/* Modified by Paul Gastin, LSV, France                                   */
/* Copyright (c) 2007  Paul Gastin                                        */
/*                                                                        */
/* This program is free software; you can redistribute it and/or modify   */
/* it under the terms of the GNU General Public License as published by   */
/* the Free Software Foundation; either version 2 of the License, or      */
/* (at your option) any later version.                                    */
/*                                                                        */
/* This program is distributed in the hope that it will be useful,        */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          */
/* GNU General Public License for more details.                           */
/*                                                                        */
/* You should have received a copy of the GNU General Public License      */
/* along with this program; if not, write to the Free Software            */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA*/
/*                                                                        */
/* Based on the translation algorithm by Gastin and Oddoux,               */
/* presented at the 13th International Conference on Computer Aided       */
/* Verification, CAV 2001, Paris, France.                                 */
/* Proceedings - LNCS 2102, pp. 53-65                                     */
/*                                                                        */
/* Send bug-reports and/or questions to Paul Gastin                       */
/* http://www.lsv.ens-cachan.fr/~gastin                                   */
/*                                                                        */
/* Some of the code in this file was taken from the Spin software         */
/* Written by Gerard J. Holzmann, Bell Laboratories, U.S.A.               */

#include <Formula/LTL/ltl2ba.h>

extern int tl_verbose, tl_terse, tl_errs;
extern FILE	*tl_out;

int	Stack_mx = 0, Max_Red = 0, Total = 0;
static char	dumpbuf[2048];

int
only_nxt(tl_Node *n)
{
    switch (n->ntyp)
    {
    case NEXT:
        return 1;
    case OR:
    case AND:
        return only_nxt(n->rgt) && only_nxt(n->lft);
    default:
        return 0;
    }
}

/* LCOV_EXCL_START */
int
dump_cond(tl_Node *pp, tl_Node *r, int first)
{
    tl_Node *q;
    int frst = first;

    if (!pp)
    {
        return frst;
    }

    q = dupnode(pp);
    q = rewrite(q);

    if (q->ntyp == PREDICATE
            ||  q->ntyp == NOT
#ifndef NXT
            ||  q->ntyp == OR
#endif
            ||  q->ntyp == FFALSE)
    {
        if (!frst)
        {
            fprintf(tl_out, " && ");
        }
        dump(q);
        frst = 0;
    }
    else if (q->ntyp == OR)
    {
        if (!frst)
        {
            fprintf(tl_out, " && ");
        }
        fprintf(tl_out, "((");
        frst = dump_cond(q->lft, r, 1);

        if (!frst)
        {
            fprintf(tl_out, ") || (");
        }
        else
        {
            if (only_nxt(q->lft))
            {
                fprintf(tl_out, "1))");
                return 0;
            }
        }

        frst = dump_cond(q->rgt, r, 1);

        if (frst)
        {
            if (only_nxt(q->rgt))
            {
                fprintf(tl_out, "1");
            }
            else
            {
                fprintf(tl_out, "0");
            }
            frst = 0;
        }

        fprintf(tl_out, "))");
    }
    else  if (q->ntyp == V_OPER
              && !anywhere(AND, q->rgt, r))
    {
        frst = dump_cond(q->rgt, r, frst);
    }
    else  if (q->ntyp == AND)
    {
        frst = dump_cond(q->lft, r, frst);
        frst = dump_cond(q->rgt, r, frst);
    }

    return frst;
}
/* LCOV_EXCL_STOP */

static void
sdump(tl_Node *n)
{
    switch (n->ntyp)
    {
    case PREDICATE:
        strcat(dumpbuf, n->sym->name);
        break;
    case U_OPER:
        strcat(dumpbuf, "U");
        goto common2;
    case V_OPER:
        strcat(dumpbuf, "V");
        goto common2;
    case OR:
        strcat(dumpbuf, "|");
        goto common2;
    case AND:
        strcat(dumpbuf, "&");
common2:
        sdump(n->rgt);
common1:
        sdump(n->lft);
        break;
    case NEXT:
        strcat(dumpbuf, "X");
        goto common1;
    case NOT:
        strcat(dumpbuf, "!");
        goto common1;
    case TTRUE:
        strcat(dumpbuf, "T");
        break;
    case FFALSE:
        strcat(dumpbuf, "F");
        break;
    default:
        strcat(dumpbuf, "?");
        break;
    }
}

tl_Symbol *
DoDump(tl_Node *n)
{
    if (!n)
    {
        return ZS;
    }

    if (n->ntyp == PREDICATE)
    {
        return n->sym;
    }

    dumpbuf[0] = '\0';
    sdump(n);
    return tl_lookup(dumpbuf);
}

void trans(tl_Node *p)
{
    if (!p || tl_errs)
    {
        return;
    }

    if (tl_verbose || tl_terse)
    {
        fprintf(tl_out, "\t/* Normlzd: ");
        dump(p);
        fprintf(tl_out, " */\n");
    }
    if (tl_terse)
    {
        return;
    }

    mk_alternating(p);
    mk_generalized();
    mk_buchi();
}

void init_alternating();
void ttrans(tl_Node *p)
{
Stack_mx = 0; Max_Red = 0; Total = 0;
init_alternating();
    if (!p || tl_errs)
    {
        return;
    }

    if (tl_verbose || tl_terse)
    {
        fprintf(tl_out, "\t/* Normlzd: ");
        dump(p);
        fprintf(tl_out, " */\n");
    }
    if (tl_terse)
    {
        return;
    }

    mk_alternating(p);
    mk_generalized();
    mk_buchi();
}

