

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "semant.h"
#include "utilities.h"
#include <map>
#include <set>
#include <iostream>


extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////
static Symbol 
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

ClassTable* classtable;
SymbolTable<char*, Entry>* symboltable;
SymbolTable<char*, Class__class>* ctable;
// The cur_class points to the current class of the scope, used for SELF_TYPE
Class_ cur_class;


ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {

    /* Fill this in */
    this->classes = classes->copy_list();
}

void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");
    
    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.
    
    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
	class_(Object, 
	       No_class,
	       append_Features(
			       append_Features(
					       single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
					       single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
			       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
	       filename);

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
	class_(IO, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       single_Features(method(out_string, single_Formals(formal(arg, Str)),
										      SELF_TYPE, no_expr())),
							       single_Features(method(out_int, single_Formals(formal(arg, Int)),
										      SELF_TYPE, no_expr()))),
					       single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
			       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
	       filename);  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
	class_(Str, 
	       Object,
	       append_Features(
			       append_Features(
					       append_Features(
							       append_Features(
									       single_Features(attr(val, Int, no_expr())),
									       single_Features(attr(str_field, prim_slot, no_expr()))),
							       single_Features(method(length, nil_Formals(), Int, no_expr()))),
					       single_Features(method(concat, 
								      single_Formals(formal(arg, Str)),
								      Str, 
								      no_expr()))),
			       single_Features(method(substr, 
						      append_Formals(single_Formals(formal(arg, Int)), 
								     single_Formals(formal(arg2, Int))),
						      Str, 
						      no_expr()))),
	       filename);
}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                                             
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
} 



/*   This is the entry point to the semantic checker.

     Your checker should do the following two things:

     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')

     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */


/*  The inheritance graph class 
    The inheritance graph should:
        1). Store the inheritances between classes 
        2). Determine whether there is a circle 
        3). Determine the conformance(Def 4.1 in Page 6) between two symbols
        4). Obtain least upper bound of two types
*/

class inherit_graph {
private:
    std::map<Symbol, Symbol> graph;

public:
    void add_Edge(const Symbol&, const Symbol&);
    int check_circle();
    int check_conformace(const Symbol&, const Symbol&);
    Symbol lub(Symbol a, Symbol b);
    void print();
    void test_lub();
    void test_conformance();
} *g;

void inherit_graph::add_Edge(const Symbol &child, const Symbol &parent) {
    if (graph.count(child) > 0) { return; }
    graph[child] = parent;
}

//If there exists a path from one source to itself, thus there is a circle
// Return 1 if exitst circle; 0 if NO circle
int inherit_graph::check_circle() {
    for (std::map<Symbol,Symbol>::iterator sym_iter = graph.begin(); sym_iter != graph.end(); sym_iter++) {
        Symbol child = sym_iter->first;
        Symbol parent = sym_iter->second;
        std::set<Symbol> visited;
        visited.insert(child);

        while (parent != Object) {
            // Already visited means a circle
            if (visited.count(parent) != 0) {
                std::cout << "Circle on " << parent->get_string() << std::endl;
                return 1;
            }

            std::cout << "Edge: " << child->get_string()<< " -> " << parent->get_string() << std::endl;
            visited.insert(parent); // mask the parent visited.
            child = parent; 
            parent = graph[child];
        }
    }

    return 0; // No cirlce, return 0
}

//If a conforms b, return 1; else return 0;
int inherit_graph::check_conformace(const Symbol &a, const Symbol &b) {
    if (a == b) return 1;
    Symbol temp = a;

    //The type of SELF_TYPE is class type 
    if (temp == SELF_TYPE)  { temp = cur_class->get_name(); }
    //Test whether a is the sub-class of b
    while(temp != Object) {
        if (temp == b) {
            return 1;
        }
        temp = graph[temp];
    }

    return temp == b;
}

//Return the least common ancestor of two classes based on Def in Page 11
Symbol inherit_graph::lub(Symbol a, Symbol b) {
    if (a == b) return a;
    if (a == SELF_TYPE) a = cur_class->get_name();
    if (b == SELF_TYPE) b = cur_class->get_name();

    int height_a = 0, height_b = 0;
    Symbol temp_a = a, temp_b = b;

    //Calculate the height of a in inheritance graph
    while (temp_a != Object) {
        height_a++;
        temp_a = graph[temp_a];
    }

    //Calculate the height of b in inheritance graph
    while (temp_b != Object) {
        height_b++;
        temp_b = graph[temp_b];
    }
    
    if (height_a >= height_b) {
        for (int i = height_a - height_b; i >= 0; i--) {
            a = graph[a];
        }
    } else {
        for (int i = height_b - height_a; i >= 0; i--) {
            b = graph[b];
        }
    }

    while (a != b) {
        a = graph[a];
        b = graph[b];
    }

    return a;
}

void inherit_graph::print() {
    for (std::map<Symbol, Symbol>::iterator i = graph.begin(); i != graph.end(); i++) {
        Symbol a = i->first;
        Symbol b = i->second;
        std::cout << "Edge: "  << a->get_string() << "->" << b->get_string() << std::endl;
    }
}

void inherit_graph::test_lub() {
    for (std::map<Symbol, Symbol>::iterator i = graph.begin(); i != graph.end(); i++) {
        for (std::map<Symbol, Symbol>::iterator j = graph.begin(); j != graph.end(); j++) {
            Symbol a = i->first;
            Symbol b = j->first;
            std::cout << "Lub of "  << a->get_string() << " & " << b->get_string() << " : " << lub(a, b);
            std::cout << std::endl;
        }
    }
}

void inherit_graph::test_conformance() {
    for (std::map<Symbol, Symbol>::iterator i = graph.begin(); i != graph.end(); i++) {
        for (std::map<Symbol, Symbol>::iterator j = graph.begin(); j != graph.end(); j++) {
            Symbol a = i->first;
            Symbol b = j->first;
            if (check_conformace(a, b)) {
                std::cout << a->get_string() << " confroms " << b->get_string() << std::endl;
            }
        }
    }
}


/*To check the inheritance of classes in program_class
    1). create the inherit graph
    2). add inheritance of basic classes
    3). add inheritance among classes in program_class
    4). check whether there exists a circle in graph
*/
int program_class::check_inheritance() {
    g = new inherit_graph();
    
    g->add_Edge(IO, Object);
    g->add_Edge(Int, Object);
    g->add_Edge(Bool, Object);
    g->add_Edge(Str, Object);

    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        cur_class = classes->nth(i);
        Symbol cur_c = cur_class->get_name(), cur_p = cur_class->get_parent();
        g->add_Edge(cur_c, cur_p);
    }

    g->print();
    g->test_lub();
    g->test_conformance();
    std::cout << "Check circle" << std::endl;
    return g->check_circle(); // 1 if a circle, 0 if no circle
}

void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    ClassTable *classtable = new ClassTable(classes);

    check_inheritance();

    /* some semantic analysis code may go here */

    if (classtable->errors()) {
	cerr << "Compilation halted due to static semantic errors." << endl;
	exit(1);
    }
}


