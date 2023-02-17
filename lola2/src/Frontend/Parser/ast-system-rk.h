/* translation of file(s)
	"Frontend/Parser/formula_abstract.k"
	"Frontend/Parser/formula_rewrite.k"
	"Frontend/Parser/formula_unparse.k"
 */
/* generated by:
 *  @(#)$Author: Kimwitu++ version 2.3.13 (C) 1998-2008 Humboldt-Universitaet zu Berlin $
 */
#ifndef KC_REWRITE_HEADER
#define KC_REWRITE_HEADER

namespace kc { }
using namespace kc;
/* included stuff */
#line 19 "Frontend/Parser/formula_rewrite.k"
/// variable counting the rule applications in the rewrite rule sets
extern size_t rule_applications;

#line  20 "ast-system-rk.h"
/* end included stuff */


namespace kc {

typedef enum {
    base_rview_enum,
    goodbye_doublearrows_enum,
    goodbye_singlearrows_enum,
    goodbye_xor_enum,
    tautology_enum,
    emptyquantifiers_enum,
    er_to_ag_enum,
    eg_to_ag_enum,
    efagef_to_ag_enum,
    egef_to_ag_enum,
    efeg_to_ag_enum,
    agef_to_ag_enum,
    efagef_to_ef_enum,
    agef_to_ef_enum,
    eu_to_ef_enum,
    exef_to_ef_enum,
    egef_to_ef_enum,
    efeg_to_ef_enum,
    onlynegation_enum,
    removepathquantifiers_enum,
    singletemporal1_enum,
    singletemporal_enum,
    simpleneg_enum,
    booleanlists_enum,
    ctloperators_enum,
    rbuechi_enum,
    last_rview
} rview_enum;

struct impl_rviews {
    const char *name;
    rview_class *view;
};
extern struct impl_rviews rviews[];
class rview_class {
protected:
    // only used in derivations
    rview_class(rview_enum v): m_view(v) { }
    rview_class(const rview_class&): m_view(base_rview_enum)
	{ /* do not copy m_view */ }
public:
    const char* name() const
	{ return rviews[m_view].name; }
    operator rview_enum() const
	{ return m_view; }
    bool operator==(const rview_class& other) const
	{ return m_view == other.m_view; }
private:
    rview_enum m_view;
};

/* Use rviews instead
extern char *kc_rview_names[];
*/
struct base_rview_class: rview_class {
    base_rview_class():rview_class(base_rview_enum){}
};
extern base_rview_class base_rview;
struct goodbye_doublearrows_class: rview_class {
    goodbye_doublearrows_class():rview_class(goodbye_doublearrows_enum){}
};
extern goodbye_doublearrows_class goodbye_doublearrows;
struct goodbye_singlearrows_class: rview_class {
    goodbye_singlearrows_class():rview_class(goodbye_singlearrows_enum){}
};
extern goodbye_singlearrows_class goodbye_singlearrows;
struct goodbye_xor_class: rview_class {
    goodbye_xor_class():rview_class(goodbye_xor_enum){}
};
extern goodbye_xor_class goodbye_xor;
struct tautology_class: rview_class {
    tautology_class():rview_class(tautology_enum){}
};
extern tautology_class tautology;
struct emptyquantifiers_class: rview_class {
    emptyquantifiers_class():rview_class(emptyquantifiers_enum){}
};
extern emptyquantifiers_class emptyquantifiers;
struct er_to_ag_class: rview_class {
    er_to_ag_class():rview_class(er_to_ag_enum){}
};
extern er_to_ag_class er_to_ag;
struct eg_to_ag_class: rview_class {
    eg_to_ag_class():rview_class(eg_to_ag_enum){}
};
extern eg_to_ag_class eg_to_ag;
struct efagef_to_ag_class: rview_class {
    efagef_to_ag_class():rview_class(efagef_to_ag_enum){}
};
extern efagef_to_ag_class efagef_to_ag;
struct egef_to_ag_class: rview_class {
    egef_to_ag_class():rview_class(egef_to_ag_enum){}
};
extern egef_to_ag_class egef_to_ag;
struct efeg_to_ag_class: rview_class {
    efeg_to_ag_class():rview_class(efeg_to_ag_enum){}
};
extern efeg_to_ag_class efeg_to_ag;
struct agef_to_ag_class: rview_class {
    agef_to_ag_class():rview_class(agef_to_ag_enum){}
};
extern agef_to_ag_class agef_to_ag;
struct efagef_to_ef_class: rview_class {
    efagef_to_ef_class():rview_class(efagef_to_ef_enum){}
};
extern efagef_to_ef_class efagef_to_ef;
struct agef_to_ef_class: rview_class {
    agef_to_ef_class():rview_class(agef_to_ef_enum){}
};
extern agef_to_ef_class agef_to_ef;
struct eu_to_ef_class: rview_class {
    eu_to_ef_class():rview_class(eu_to_ef_enum){}
};
extern eu_to_ef_class eu_to_ef;
struct exef_to_ef_class: rview_class {
    exef_to_ef_class():rview_class(exef_to_ef_enum){}
};
extern exef_to_ef_class exef_to_ef;
struct egef_to_ef_class: rview_class {
    egef_to_ef_class():rview_class(egef_to_ef_enum){}
};
extern egef_to_ef_class egef_to_ef;
struct efeg_to_ef_class: rview_class {
    efeg_to_ef_class():rview_class(efeg_to_ef_enum){}
};
extern efeg_to_ef_class efeg_to_ef;
struct onlynegation_class: rview_class {
    onlynegation_class():rview_class(onlynegation_enum){}
};
extern onlynegation_class onlynegation;
struct removepathquantifiers_class: rview_class {
    removepathquantifiers_class():rview_class(removepathquantifiers_enum){}
};
extern removepathquantifiers_class removepathquantifiers;
struct singletemporal1_class: rview_class {
    singletemporal1_class():rview_class(singletemporal1_enum){}
};
extern singletemporal1_class singletemporal1;
struct singletemporal_class: rview_class {
    singletemporal_class():rview_class(singletemporal_enum){}
};
extern singletemporal_class singletemporal;
struct simpleneg_class: rview_class {
    simpleneg_class():rview_class(simpleneg_enum){}
};
extern simpleneg_class simpleneg;
struct booleanlists_class: rview_class {
    booleanlists_class():rview_class(booleanlists_enum){}
};
extern booleanlists_class booleanlists;
struct ctloperators_class: rview_class {
    ctloperators_class():rview_class(ctloperators_enum){}
};
extern ctloperators_class ctloperators;
struct rbuechi_class: rview_class {
    rbuechi_class():rview_class(rbuechi_enum){}
};
extern rbuechi_class rbuechi;

} // namespace kc
#endif // KC_REWRITE_HEADER
