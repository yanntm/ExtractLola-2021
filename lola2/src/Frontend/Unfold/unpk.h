/* translation of file(s)
	"abstract.k"
	"unparse.k"
	"rewrite.k"
 */
/* generated by:
 *  @(#)$Author: Kimwitu++ version 2.3.11 (C) 1998-2007 Humboldt-University of Berlin $
 */
#ifndef KC_UNPARSE_HEADER
#define KC_UNPARSE_HEADER


namespace kc {

typedef enum {
    base_uview_enum,
    typecheck_enum,
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
struct typecheck_class: uview_class {
    typecheck_class():uview_class(typecheck_enum){}
};
extern typecheck_class typecheck;

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
