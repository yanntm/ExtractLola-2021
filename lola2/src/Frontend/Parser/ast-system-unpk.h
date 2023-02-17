/* translation of file(s)
	"Frontend/Parser/formula_abstract.k"
	"Frontend/Parser/formula_rewrite.k"
	"Frontend/Parser/formula_unparse.k"
 */
/* generated by:
 *  @(#)$Author: Kimwitu++ version 2.3.13 (C) 1998-2008 Humboldt-Universitaet zu Berlin $
 */
#ifndef KC_UNPARSE_HEADER
#define KC_UNPARSE_HEADER

namespace kc { }
using namespace kc;
/* included stuff */
#line 64 "Frontend/Parser/formula_unparse.k"
#include <CoverGraph/CoverGraph.h>
#include <Formula/StatePredicate/DeadlockPredicate.h>
#include <Formula/StatePredicate/FireablePredicate.h>
#include <string>
#include <Portfolio/portfoliomanager.h>
#include <Exploration/Constraints.h>

extern std::string unparsed;

void myprinter(const char *s, kc::uview v);
void stringprinter(const char *s, kc::uview v);

#line  29 "ast-system-unpk.h"
/* end included stuff */


namespace kc {

typedef enum {
    base_uview_enum,
    out_enum,
    relabelformula_enum,
    checkcons_enum,
    checkstableatomic_enum,
    foldformula_enum,
    createskeleton_enum,
    hlunfold_enum,
    reduction_enum,
    markvisible_enum,
    markslicingef_enum,
    markslicingag_enum,
    structural_enum,
    containstemp_enum,
    count_enum,
    elem_enum,
    temporal_enum,
    internal_enum,
    buechi_enum,
    ctl_enum,
    ltl_enum,
    hl_staticanalysis_enum,
    visible_enum,
    copy_enum,
    last_uview
} uview_enum;

struct impl_uviews {
    const char *name;
    uview_class *view;
};
extern impl_uviews uviews[];
class uview_class {
protected:
    // only used in derivations
    uview_class(uview_enum v): m_view(v) { }
    uview_class(c_uview): m_view(base_uview_enum)
	{ /* do not copy m_view */ }
public:
    const char* name() const
	{ return uviews[m_view].name; }
    operator uview_enum() const
	{ return m_view; }
    bool operator==(const uview_class& other) const
	{ return m_view == other.m_view; }
private:
    uview_enum m_view;
};

class printer_functor_class {
public:
    virtual void operator()(const kc_char_t*, uview) { }
    virtual ~printer_functor_class() { }
};

class printer_functor_function_wrapper : public printer_functor_class {
public:
    printer_functor_function_wrapper(const printer_function opf =0): m_old_printer(opf) { }
    virtual ~printer_functor_function_wrapper() { }
    virtual void operator()(const kc_char_t* s, uview v)
	{ if(m_old_printer) m_old_printer(s, v); }
private:
    printer_function m_old_printer;
};

/* Use uviews instead
extern char *kc_view_names[];
*/
struct base_uview_class: uview_class {
    base_uview_class():uview_class(base_uview_enum){}
};
extern base_uview_class base_uview;
struct out_class: uview_class {
    out_class():uview_class(out_enum){}
};
extern out_class out;
struct relabelformula_class: uview_class {
    relabelformula_class():uview_class(relabelformula_enum){}
};
extern relabelformula_class relabelformula;
struct checkcons_class: uview_class {
    checkcons_class():uview_class(checkcons_enum){}
};
extern checkcons_class checkcons;
struct checkstableatomic_class: uview_class {
    checkstableatomic_class():uview_class(checkstableatomic_enum){}
};
extern checkstableatomic_class checkstableatomic;
struct foldformula_class: uview_class {
    foldformula_class():uview_class(foldformula_enum){}
};
extern foldformula_class foldformula;
struct createskeleton_class: uview_class {
    createskeleton_class():uview_class(createskeleton_enum){}
};
extern createskeleton_class createskeleton;
struct hlunfold_class: uview_class {
    hlunfold_class():uview_class(hlunfold_enum){}
};
extern hlunfold_class hlunfold;
struct reduction_class: uview_class {
    reduction_class():uview_class(reduction_enum){}
};
extern reduction_class reduction;
struct markvisible_class: uview_class {
    markvisible_class():uview_class(markvisible_enum){}
};
extern markvisible_class markvisible;
struct markslicingef_class: uview_class {
    markslicingef_class():uview_class(markslicingef_enum){}
};
extern markslicingef_class markslicingef;
struct markslicingag_class: uview_class {
    markslicingag_class():uview_class(markslicingag_enum){}
};
extern markslicingag_class markslicingag;
struct structural_class: uview_class {
    structural_class():uview_class(structural_enum){}
};
extern structural_class structural;
struct containstemp_class: uview_class {
    containstemp_class():uview_class(containstemp_enum){}
};
extern containstemp_class containstemp;
struct count_class: uview_class {
    count_class():uview_class(count_enum){}
};
extern count_class count;
struct elem_class: uview_class {
    elem_class():uview_class(elem_enum){}
};
extern elem_class elem;
struct temporal_class: uview_class {
    temporal_class():uview_class(temporal_enum){}
};
extern temporal_class temporal;
struct internal_class: uview_class {
    internal_class():uview_class(internal_enum){}
};
extern internal_class internal;
struct buechi_class: uview_class {
    buechi_class():uview_class(buechi_enum){}
};
extern buechi_class buechi;
struct ctl_class: uview_class {
    ctl_class():uview_class(ctl_enum){}
};
extern ctl_class ctl;
struct ltl_class: uview_class {
    ltl_class():uview_class(ltl_enum){}
};
extern ltl_class ltl;
struct hl_staticanalysis_class: uview_class {
    hl_staticanalysis_class():uview_class(hl_staticanalysis_enum){}
};
extern hl_staticanalysis_class hl_staticanalysis;
struct visible_class: uview_class {
    visible_class():uview_class(visible_enum){}
};
extern visible_class visible;
struct copy_class: uview_class {
    copy_class():uview_class(copy_enum){}
};
extern copy_class copy;

void unparse(abstract_phylum kc_p, printer_functor kc_printer, uview kc_current_view);
void unparse(void *kc_p, printer_functor kc_printer, uview kc_current_view);
void unparse(int kc_v, printer_functor kc_printer, uview kc_current_view);
void unparse(double kc_v, printer_functor kc_printer, uview kc_current_view);
void unparse(kc_char_t *kc_v, printer_functor kc_printer, uview kc_current_view);
void unparse(kc_string_t kc_v, printer_functor kc_printer, uview kc_current_view);
#define PRINT(string) kc_printer(string,kc_current_view)
#define UNPARSE(node) node->unparse(kc_printer,kc_current_view)

} // namespace kc
#endif // KC_UNPARSE_HEADER
