/*****************************************************************************\
 LoLA -- a Low Level Petri Net Analyzer

 Copyright (C)  1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007,
                2008, 2009  Karsten Wolf <lola@service-technology.org>

 LoLA is free software: you can redistribute it and/or modify it under the
 terms of the GNU Affero General Public License as published by the Free
 Software Foundation, either version 3 of the License, or (at your option)
 any later version.

 LoLA is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public License for
 more details.

 You should have received a copy of the GNU Affero General Public License
 along with LoLA.  If not, see <http://www.gnu.org/licenses/>.
\*****************************************************************************/


#ifndef UNFOLD_H
#define UNFOLD_H

#include <cstdio>

typedef enum {
    arr, rec, num, boo, enu //,set, lst
} UTypeClass;

const char* getUTypeName(UTypeClass c);

class UValue;

class UType {
    public:
        UTypeClass tag;
        unsigned long int size;
        virtual bool iscompatible(UType*) = 0;
        virtual UValue* make() = 0;
        virtual ~UType() {}
};

class UNumType: public UType {
    public:
        bool iscompatible(UType*);
        int lower;
        int upper;
        UNumType(int, int);
        UValue* make();
};

class UBooType: public UType {
    public:
        UBooType();
        bool iscompatible(UType*);
        UValue* make();
};

class EnSymbol;

class UEnList {
    public:
        EnSymbol* sy;
        UEnList* next;
        UEnList();
        UEnList(EnSymbol*);
};

class UEnuType: public UType {
    public:
        EnSymbol** tags;
        bool iscompatible(UType*);
        UEnuType(UEnList*);
        UValue* make();
};

class UArrType: public UType {
    public:
        UType* index;
        UType* component;
        bool iscompatible(UType*);
        UArrType(UType*, UType*);
        UValue* make();
};

class RcSymbol;

class URcList {
    public:
        RcSymbol* sy;
        UType* ty;
        URcList* next;
        URcList(RcSymbol*, UType*, URcList*);
};

class URecType: public UType {
    public:
        int card;
        RcSymbol** tags;
        UType** component;
        bool iscompatible(UType*);
        URecType(URcList*);
        UValue* make();
};

class UValue {
    public:
        UType* type;
        virtual UValue* copy() = 0;
        virtual void assign(UValue*) = 0;
        virtual bool isfirst() = 0;
//        virtual void print(ostream&) = 0;
        virtual void operator++ (int) = 0;
        virtual bool iseqqual(UValue*) = 0;
        virtual bool islesseqqual(UValue*) = 0;
        virtual UValue* addop(UValue*) = 0;
        virtual UValue* subop(UValue*) = 0;
        virtual UValue* mulop(UValue*) = 0;
        virtual UValue* divop(UValue*) = 0;
        virtual UValue* modop(UValue*) = 0;
        virtual UValue* negop() = 0;
        virtual char* text() = 0;
        UValue(void) {}
        virtual ~UValue(void) {}
};

/*inline ostream& operator<< (ostream& os, UValue* val) {
    val->print(os);
    return os;
}*/

class UNumValue: public UValue {
    public:
        int v;
        UValue* copy();
        void operator++ (int);
        virtual bool isfirst();
//        void print(ostream&);
        void assign(UValue*);
        virtual bool iseqqual(UValue*);
        virtual bool islesseqqual(UValue*);
        UValue* addop(UValue*);
        UValue* subop(UValue*);
        UValue* mulop(UValue*);
        UValue* divop(UValue*);
        UValue* modop(UValue*);
        UValue* negop();
        char* text();
        UNumValue(UType*, int);
};

class UBooValue: public UValue {
    public:
        bool v;
        UValue* copy();
        void operator++ (int);
//        void print(ostream&);
        void assign(UValue*);
        virtual bool iseqqual(UValue*);
        virtual bool isfirst();
        virtual bool islesseqqual(UValue*);
        UValue* addop(UValue*);
        UValue* subop(UValue*);
        UValue* mulop(UValue*);
        UValue* divop(UValue*);
        UValue* modop(UValue*);
        UValue* negop();
        char* text();
        UBooValue(UType*, bool);
};

class UEnuValue: public UValue {
    public:
        int v;
        UValue* copy();
        void operator++ (int);
//        void print(ostream&);
        void assign(UValue*);
        virtual bool iseqqual(UValue*);
        virtual bool isfirst();
        virtual bool islesseqqual(UValue*);
        UValue* addop(UValue*) {
            return NULL;
        }
        UValue* subop(UValue*) {
            return NULL;
        }
        UValue* mulop(UValue*) {
            return NULL;
        }
        UValue* divop(UValue*) {
            return NULL;
        }
        UValue* modop(UValue*) {
            return NULL;
        }
        UValue* negop()        {
            return NULL;
        }
        char* text();
        UEnuValue(UType*, int);
};

class UArrValue : public UValue {
    public:
        UValue** content;
        UValue* copy();
        void operator++ (int);
//        void print(ostream&);
        void assign(UValue*);
        virtual bool iseqqual(UValue*);
        virtual bool isfirst();
        virtual bool islesseqqual(UValue*);
        UValue* addop(UValue*);
        UValue* subop(UValue*);
        UValue* mulop(UValue*);
        UValue* divop(UValue*);
        UValue* modop(UValue*);
        UValue* negop();
        char* text();
};

class URecValue : public UValue {
    public:
        UValue** content;
        UValue* copy();
        void operator++ (int);
//        void print(ostream&);
        virtual bool isfirst();
        void assign(UValue*);
        virtual bool iseqqual(UValue*);
        virtual bool islesseqqual(UValue*);
        UValue* addop(UValue*);
        UValue* subop(UValue*);
        UValue* mulop(UValue*);
        UValue* divop(UValue*);
        UValue* modop(UValue*);
        UValue* negop();
        char* text();
};

class UExpression {
    public:
        UType* type;
        virtual UValue* evaluate() = 0;
        virtual ~UExpression() {}
};

class ULVal {
    public:
        UType* type;
        virtual UValue* dereference() = 0;
        virtual ~ULVal() {}
};

class UVar;

class UVarLVal : public ULVal {
    public:
        UVar* var;
        UValue* dereference();
};

class URecordLVal : public ULVal {
    public:
        ULVal* parent;
        unsigned int  offset;
        UValue* dereference();
};

class UArrayLVal : public ULVal {
    public:
        ULVal* parent;
        UExpression* idx;
        UType* indextype;
        UValue* dereference();
};

class UStatement {
    public:
        virtual void execute() = 0;
        virtual ~UStatement() {}
};

class UWhileStatement : public UStatement {
    public:
        UExpression* cond;
        UStatement* body;
        void execute();
};

class UAssignStatement : public UStatement {
    public:
        UExpression* right;
        ULVal* left;
        void execute();
};

class UFunction;

class UReturnStatement : public UStatement {
    public:
        UExpression* exp;
        UFunction* fct;
        void execute();
};

class UConditionalStatement : public UStatement {
    public:
        UExpression* cond;
        UStatement* yes;
        UStatement* no;
        void execute();
};

class UCaseStatement : public UStatement {
    public:
        int card;
        UExpression* exp;
        UExpression** cond;
        UStatement** yes;
        UStatement* def;
        void execute();
};

class UForStatement : public UStatement {
    public:
        UVar* var;
        UExpression* init;
        UExpression* finit;
        UStatement* body;
        void execute();
};

class UExitStatement : public UStatement {
    public:
        UFunction* fct;
        void execute();
};

class USequenceStatement : public UStatement {
    public:
        UStatement* first;
        UStatement* second;
        void execute();
};

class UNopStatement: public UStatement {
    public:
        void execute() {}
};

class UForallStatement : public UStatement {
    public:
        UVar* var;
        UStatement* body;
        void execute();
};

class URepeatStatement : public UStatement {
    public:
        UExpression* cond;
        UStatement* body;
        void execute();
};

class UCallExpression : public UExpression {
    public:
        UFunction* fct;
        UExpression** currentpar;
        UValue* evaluate();
};

class UEqualExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class UGreatereqqualExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class USubExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class UMulExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class UDivExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class UModExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class UNegExpression : public UExpression {
    public:
        UExpression* left;
        UValue* evaluate();
};

class UAddExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class UGreaterthanExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class ULessthanExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class UFalseExpression : public UExpression {
    public:
        UValue* evaluate();
};

class UTrueExpression : public UExpression {
    public:
        UValue* evaluate();
};

class UEnuconstantExpression : public UExpression {
    public:
        unsigned int nu;
        UValue* evaluate();
};

class UIntconstantExpression : public UExpression {
    public:
        unsigned int nu;
        UValue* evaluate();
};

class ULvalExpression : public UExpression {
    public:
        ULVal* lval;
        UValue* evaluate();
};

class URecordExpression : public UExpression {
    public:
        unsigned int card;
        UExpression** cont;
        UValue* evaluate();
};

class UArrayExpression : public UExpression {
    public:
        unsigned int card;
        UExpression** cont;
        UValue* evaluate();
};

class ULesseqqualExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class UUneqqualExpression : public UExpression {
    public:
        UExpression* left;
        UExpression* right;
        UValue* evaluate();
};

class UValueList {
    public:
        UValue* val;
        UValueList* next;
};

class SymbolTab;

class UResultList {
    public:
        UValueList* res;
        UResultList* next;
};

class UFunction {
    public:
        int arity;
        UType* type;
        UValueList* result;
        UResultList* resultstack;
        UVar** formalpar;
        SymbolTab* localsymb;
        UStatement* body;
        void addresult(UValue*);
        void enter();
        void leave();
};

class UVar {
    public:
        UType* type;
        UValue* value;
        UValueList* stack;
        void assign(UValue*);
        UValue* get();
        UVar(UType*);
};

bool deep_compatible(UType*, UType*, UTypeClass);
bool deep_compatible(UType*, UTypeClass);

class UTerm {
    public:
        UType* type;
        virtual UValueList* evaluate() = 0;
        virtual ~UTerm() {}
};

class UVarTerm: public UTerm {
    public:
        UVar* v;
        UValueList* evaluate();
};

class UOpTerm: public UTerm {
    public:
        unsigned int arity;
        UFunction* f;
        UTerm** sub;
        UValueList* evaluate();
};


class UTermList {
    public:
        UTerm* t;
        unsigned int mult;
        UTermList* next;
};

class ExitException {}; //Exit-Anweisung wird als Exception realisiert

class VaSymbol;


/// \todo Warum steht das hier?
class fmode {
    public:
        VaSymbol* v;
        UExpression* t;
        fmode* next;
        fmode();
        fmode(VaSymbol*, UExpression*, fmode*);
};

#endif
