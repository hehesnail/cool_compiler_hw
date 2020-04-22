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
//SymbolTable for identifiers, object type environment
SymbolTable<char*, Entry>* symboltable;
//SymbolTable for classes
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
    ctable->addid(Object->get_string(), Object_class);

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
    ctable->addid(IO->get_string(), IO_class);

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
	class_(Int, 
	       Object,
	       single_Features(attr(val, prim_slot, no_expr())),
	       filename);
    ctable->addid(Int->get_string(), Int_class);

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
	class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())),filename);
    ctable->addid(Bool->get_string(), Bool_class);

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
    ctable->addid(Str->get_string(), Str_class);
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
    bool check_circle();
    bool check_conformace(const Symbol&, const Symbol&);
    Symbol lub(Symbol a, Symbol b);
    void print(); //print the graph for test
    void test_lub(); // only for test
    void test_conformance(); // only for test
} *g;

void inherit_graph::add_Edge(const Symbol &child, const Symbol &parent) {
    if (graph.count(child) > 0) { return; }
    graph[child] = parent;
}

//If there exists a path from one source to itself, thus there is a circle
// Return true if exitst circle; false if NO circle
bool inherit_graph::check_circle() {
    for (std::map<Symbol,Symbol>::iterator sym_iter = graph.begin(); sym_iter != graph.end(); sym_iter++) {
        Symbol child = sym_iter->first;
        Symbol parent = sym_iter->second;
        std::set<Symbol> visited;
        visited.insert(child);

        while (parent != Object) {
            // Already visited means a circle
            if (visited.count(parent) != 0) {
                // std::cout << "Circle on " << parent->get_string() << std::endl;
                return true;
            }

            // std::cout << "Edge: " << child->get_string()<< " -> " << parent->get_string() << std::endl;
            visited.insert(parent); // mask the parent visited.
            child = parent; 
            parent = graph[child];
        }
    }

    return false; // No cirlce, return false
}

//If a conforms b, return true; else return false;
bool inherit_graph::check_conformace(const Symbol &a, const Symbol &b) {
    if (a == b) return 1;
    Symbol temp = a;

    //The type of SELF_TYPE is class type 
    if (temp == SELF_TYPE)  { temp = cur_class->get_name(); }
    //Test whether a is the sub-class of b
    while(temp != Object) {
        if (temp == b) {
            return true;
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
        for (int i = height_a - height_b; i > 0; i--) {
            a = graph[a];
        }
    } else {
        for (int i = height_b - height_a; i > 0; i--) {
            b = graph[b];
        }
    }

    while (a != b) {
        a = graph[a];
        b = graph[b];
    }

    return a;
}


// Only used for test
void inherit_graph::print() {
    for (std::map<Symbol, Symbol>::iterator i = graph.begin(); i != graph.end(); i++) {
        Symbol a = i->first;
        Symbol b = i->second;
        cout << "Edge: "  << a->get_string() << "->" << b->get_string() << endl;
    }
}

// Only used for test
void inherit_graph::test_lub() {
    for (std::map<Symbol, Symbol>::iterator i = graph.begin(); i != graph.end(); i++) {
        for (std::map<Symbol, Symbol>::iterator j = graph.begin(); j != graph.end(); j++) {
            Symbol a = i->first;
            Symbol b = j->first;
            cout << "Lub of "  << a->get_string() << " & " << b->get_string() << " : " << lub(a, b);
            cout << endl;
        }
    }
}


// Only used for test
void inherit_graph::test_conformance() {
    for (std::map<Symbol, Symbol>::iterator i = graph.begin(); i != graph.end(); i++) {
        for (std::map<Symbol, Symbol>::iterator j = graph.begin(); j != graph.end(); j++) {
            Symbol a = i->first;
            Symbol b = j->first;
            if (check_conformace(a, b)) {
                cout << a->get_string() << " confroms " << b->get_string() << endl;
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
bool program_class::check_inheritance_circle() {
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

    // g->print();
    // g->test_lub();
    // g->test_conformance();
    // std::cout << "Check circle" << std::endl;
    return g->check_circle(); // true if a circle, false if no circle
}

/* Check the correctness of classes in program
    1). Circle in inheritance graph 2). Main existence 3). Invalid re-definition
    4). Invalid inheritance 5). Redefine existed class
*/
void program_class::pre_check() {

    //Test if there is circle
    if (check_inheritance_circle()) {
        classtable->semant_error(cur_class) << "Circle in inheritance" << endl;
    }

    bool exist_main = false;
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        cur_class = classes->nth(i);
        char* cur_name = cur_class->get_name()->get_string();
        char* parent_name = cur_class->get_parent()->get_string();

        if (strcmp(cur_name, "Main") == 0) {
            exist_main = true;
        }

        if (strcmp(cur_name, "Object") == 0 || strcmp(cur_name, "Bool") == 0 || 
            strcmp(cur_name, "Int") == 0 || strcmp(cur_name, "String") == 0 || 
            strcmp(cur_name, "IO") == 0 ) {
                classtable->semant_error(cur_class) << "Invalid re-definition" << endl;
                return;
        }

        if (strcmp(parent_name, "Bool") == 0 || strcmp(parent_name, "Int") == 0 ||
            strcmp(parent_name, "String") == 0 || strcmp(parent_name, "SELF_TYPE") == 0) {
                classtable->semant_error(cur_class) << "Invalid inheritance" << endl;
                return;
        }

        if (strcmp(cur_name, "SELF_TYPE") == 0) {
            classtable->semant_error(cur_class) << "Redefine SELF_TYPE" << endl;
            return;
        }

        if (ctable->lookup(cur_name) != NULL) {
            classtable->semant_error(cur_class) << "Redefine class already exists" << endl;
            return;
        }

        ctable->addid(cur_name, cur_class);
    }

    if (!exist_main) {
        classtable->semant_error(cur_class) << "No Main class" << endl;
        return;
    }
}

/*type checking for program, check all classes in program class*/
void program_class::type_check() {
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        cur_class = classes->nth(i);
        if (ctable->lookup(cur_class->get_parent()->get_string()) == NULL) {
            classtable->semant_error(cur_class) << "Inherit from undefined class" << endl;
            return;
        }

        //For each class, traversal the class, check each expr
        //Each class also denotes a new scope
        symboltable->enterscope();
        cur_class->type_check();
        symboltable->exitscope();
    }
}

/*Check the class type rules*/
void class__class::type_check() {
    //check type of all features
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        Feature feat = features->nth(i);
        feat->type_check();
    }
}

/*Get attribute of given symbol name*/
Feature class__class::get_attr(Symbol name) {
    Feature f = NULL;
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        f = features->nth(i);
        if (f->get_formals() == NULL && f->get_feature_name() == name) {
            return f;
        }
    }

    return NULL;
}

/*Get method of given symbol name*/
Feature class__class::get_method(Symbol name) {
    Feature f = NULL; 
    for (int i = features->first(); features->more(i); i = features->next(i)) {
        f = features->nth(i);
        if (f->get_formals() != NULL && f->get_feature_name() == name) {
            return f;
        }
    }

    return NULL;
}

/*Type check for attribute feature*/
void attr_class::type_check() {
    if (type_decl == SELF_TYPE) {
        type_decl = cur_class->get_name();
    }

    if (name == self) {
        classtable->semant_error(cur_class) << "bind self to attribute" << endl;
        return;
    } 

    //check redefinition of inherited attributes
    Class_ p_class = ctable->lookup(cur_class->get_parent()->get_string());
    while(true) {
        if (p_class->get_attr(name) != NULL) {
            classtable->semant_error(cur_class) << "override attribute" << endl;
            return;
            break;
        }

        if (p_class->get_parent() == No_class) {
            break;
        }
        p_class = ctable->lookup(cur_class->get_parent()->get_string());
    }

    init->type_check();
    if (!(init->get_type() == No_type) && !(g->check_conformace(init->get_type(), type_decl))) {
        classtable->semant_error(cur_class) << "Init and type_decl error in attribute" << endl;
        return;
    }
    symboltable->addid(name->get_string(), type_decl);
}

/*Typr check for method*/
void method_class::type_check() {
    symboltable->enterscope();

    if (ctable->lookup(return_type->get_string()) == NULL && return_type != SELF_TYPE) {
        classtable->semant_error(cur_class) << "Invalid return type" << endl;
    }

    //check redefinition of method in inheritance
    Class_ p = ctable->lookup(cur_class->get_parent()->get_string());
    Feature f = NULL;
    while(true) {
        if (p->get_method(name) != NULL) {
            f = p->get_method(name);
        }
        if (p->get_parent() == No_class) {
            break;
        }
        p = ctable->lookup(p->get_parent()->get_string());
    }

    if (f != NULL) {
        //return type;
        Formals formal_params = f->get_formals();
        // check number of args
        if (formals->len() != formal_params->len()) {
            classtable->semant_error(cur_class) << "Override method error: No match len" << endl;
            return;
        }
        //chech types of formals
        for (int i = formals->first(), j = formal_params->first(); 
                 formals->more(i) && formal_params->more(j); 
                 i = formals->next(i), j = formal_params->next(j)) {
            Formal f1 = formals->nth(i), f2 = formal_params->nth(j);

            f1->type_check();
            
            if (f1->get_formal_type() != f2->get_formal_type()) {
                classtable->semant_error(cur_class) << "Types in overriden method don't match" << endl;
                return;
            }
        }
        //check return type
        if (return_type != f->get_type()) {
            classtable->semant_error(cur_class) << "Return type in overriden method does not match" << endl;
            return;
        } 
    } else {
        for (int i = formals->first(); formals->more(i); formals->next(i)) {
            Formal feat = formals->nth(i);
            feat->type_check();
        }
    }

    expr->type_check();
    Symbol test_type;
    if (return_type == SELF_TYPE) {
        test_type = cur_class->get_name();
    } else {
        test_type = return_type;
    }
    if (!g->check_conformace(expr->get_type(), test_type)) {
        classtable->semant_error(cur_class) << "Error in method comformance checking" << endl;
    }

    symboltable->exitscope();
}

/*Type check rule for the formal class*/
void formal_class::type_check() {
    if (symboltable->probe(name->get_string()) != NULL) {
        classtable->semant_error(cur_class) << "Duplicate names in formal" << endl;
        return;
    }

    if (name == self) {
        classtable->semant_error(cur_class) << "Self as parameter name is wrong" << endl;
        return;
    }

    if (type_decl == SELF_TYPE) {
        classtable->semant_error(cur_class) << "SELF_TYPE as parameter type is wrong" << endl;
        return;
    }

    //binding the type with name of parameter, thus change O to O[T/x]
    symboltable->addid(name->get_string(), type_decl);
}

/*Type check for dispatch*/
void dispatch_class::type_check() {
    expr->type_check();
    Symbol t0 = expr->get_type();
    if (t0 == SELF_TYPE) {
        t0 = cur_class->get_name();
    }
    //Get matched method in e0 and beyond
    Class_ p = ctable->lookup(t0->get_string());
    Feature f = p->get_method(name); // feature stores matched method func

    while(true) {
        if (p->get_method(name) != NULL) {
            f = p->get_method(name);
            break;
        }
        if (p->get_parent() == No_class) {
            break;
        }
        p = ctable->lookup(p->get_parent()->get_string());
    }

    if (f == NULL) {
        classtable->semant_error(cur_class) << "No matched method" << endl;
        return;
    }    

    //type check for expressions, conformance checking
    Formals f_params = f->get_formals();
    Symbol f_return_type = f->get_type();
    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        Expression ei = actual->nth(i);
        ei->type_check();
        Formal fi = f_params->nth(i);
        if (!g->check_conformace(ei->get_type(), fi->get_formal_type())) {
            classtable->semant_error(cur_class) << "Parameters types don't match" << endl;
            return;
        }
    }
    if (f_return_type == SELF_TYPE) {
        f_return_type = expr->get_type();
    }
    type = f_return_type;
}

/*Type check static dispatch*/
void static_dispatch_class::type_check() {
    expr->type_check();
    Symbol e0 = expr->get_type();
    if (!g->check_conformace(e0, type_name)) {
        classtable->semant_error(cur_class) << "Error in static dispatch" << endl;
        return;
    }

    //Get matched method in typename
    Class_ p = ctable->lookup(type_name->get_string());
    Feature f = p->get_method(name);
    while(true) {
        if (p->get_method(name) != NULL) {
            f = p->get_method(name);
            break;
        }
        if (p->get_parent() == No_class) {
            break;
        }
        p = ctable->lookup(p->get_parent()->get_string());
    }
    if (f == NULL) {
        classtable->semant_error(cur_class) << "No match method in static dispatch" << endl;
        return;
    }    

    //type check for expressions, conformance checking
    Formals f_params = f->get_formals();
    Symbol f_return_type = f->get_type();
    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        Expression ei = actual->nth(i);
        ei->type_check();
        Formal fi = f_params->nth(i);
        if (!g->check_conformace(ei->get_type(), fi->get_formal_type())) {
            classtable->semant_error(cur_class) << "Parameters types don't match in static dispatch" << endl;
            return;
        }
    }

    if (f_return_type == SELF_TYPE) {
        f_return_type = expr->get_type();
    }
    type = f_return_type;
}

/*Type check for assign*/
void assign_class::type_check() {
    if(name == self) {
        classtable->semant_error(cur_class) << "Error to assign self" << endl;
        return;
    }

    Symbol searchtype = symboltable->lookup(name->get_string());
    Class_ p = cur_class;
    if (searchtype == NULL) {
        //check attributes
        Feature f = p->get_attr(name);
        while(true) {
            if (f != NULL) {
                searchtype = f->get_type();
                break;
            }
            if (p->get_parent() == No_class) {
                break;
            }
            p = ctable->lookup(p->get_parent()->get_string());
        }
    } 

    if (searchtype == NULL) {
        classtable->semant_error(cur_class) << "Error in finding def in assign" << endl;
        return;
    }

    expr->type_check();
    if (!g->check_conformace(expr->get_type(), searchtype)) {
        classtable->semant_error(cur_class) << "Assign conformance check fail" << endl;
        return;
    }

    type = expr->get_type();
}

/*Type for Int constant is Int*/
void int_const_class::type_check() {
    type = Int;
}

/*Type for bool constant is Bool*/
void bool_const_class::type_check() {
    type = Bool;
}

/*Type for string constant is Str*/
void string_const_class::type_check() {
    type = Str;
}

/*Type for no_expr_class is No_type*/
void no_expr_class::type_check() {
    type = No_type;
}

/*Type check for isvoid*/
void isvoid_class::type_check() {
    e1->type_check();
    type = Bool;
}

/*Type check for new*/
void new__class::type_check() {
    if (type_name == SELF_TYPE) {
        type = cur_class->get_name();
    } else {
        type = type_name;
    }
}

/*Type check for equal*/
void eq_class::type_check() {
    e1->type_check();
    e2->type_check();

    if (e1->get_type() == Int) {
        if (e2->get_type() == Int) {
            type = Bool;
        } else {
            type = Object;
            classtable->semant_error(cur_class) << "Not all Ints in eq_class" << endl;
            return;
         }
    }

    if (e1->get_type() == Str) {
        if (e2->get_type() == Str) {
            type = Bool;
        } else {
            type = Object;
            classtable->semant_error(cur_class) << "Not all Strs in eq_class" << endl;
            return;
        }
    }

    if (e1->get_type() == Bool) {
        if (e2->get_type() == Bool) {
            type = Bool;
        } else {
            type = Object;
            classtable->semant_error(cur_class) << "Not all bools in eq_class" << endl;
            return;
        }
    }

    type = Bool;
}
/*Type check for arithemitic expressions*/
/*Type check for plus*/
void plus_class::type_check() {
    e1->type_check();
    e2->type_check();
    if (e1->get_type() == Int && e2->get_type() == Int) {
        type = Int;
    } else {
        type = Object;
        classtable->semant_error(cur_class) << "Types must be Ints for +" << endl;
        return;
    }
}

/*Type check for sub*/
void sub_class::type_check() {
    e1->type_check();
    e2->type_check();
    if (e1->get_type() == Int && e2->get_type() == Int) {
        type = Int;
    } else {
        type = Object;
        classtable->semant_error(cur_class) << "Types must be Ints for -" << endl;
        return;
    }
}

/*Type check for mul*/
void mul_class::type_check() {
    e1->type_check();
    e2->type_check();
    if (e1->get_type() == Int && e2->get_type() == Int) {
        type = Int;
    } else {
        type = Object;
        classtable->semant_error(cur_class) << "Types must be Ints for *" << endl;
        return;
    }
}

/*Type check for divide*/
void divide_class::type_check() {
    e1->type_check();
    e2->type_check();
    if (e1->get_type() == Int && e2->get_type() == Int) {
        type = Int;
    } else {
        type = Object;
        classtable->semant_error(cur_class) << "Types must be Ints for /" << endl;
        return;
    }
}

/*Type check for neg*/
void neg_class::type_check() {
    e1->type_check();
    if (e1->get_type() == Int) {
        type = Int;
    } else {
        type = Object;
        classtable->semant_error(cur_class) << "Type must be Int for neg" << endl;
        return;
    }
}

/*Type check for comparision expressions*/
/*Type check for Not*/
void comp_class::type_check() {
    e1->type_check();
    if (e1->get_type() == Bool) {
        type = Bool;
    } else {
        type = Object;
        classtable->semant_error(cur_class) << "Type for Not must be Bool" << endl;
        return;
    }
}

/*Type check for <=*/
void leq_class::type_check() {
    e1->type_check();
    e2->type_check();
    if (e1->get_type() == Int && e2->get_type() == Int) {
        type = Bool;
    } else {
        type = Object;
        classtable->semant_error(cur_class) << "Types for <= must be Ints" << endl;
        return;
    }
}

/*Type check for <*/
void lt_class::type_check() {
    e1->type_check();
    e2->type_check();

    if (e1->get_type() == Int && e2->get_type() == Int) {
        type = Bool;
    } else {
        type = Object;
        classtable->semant_error(cur_class) << "Types for < must be Ints" << endl;
        return;
    }
}

/*Type check for loop*/
void loop_class::type_check() {
    pred->type_check();
    if (pred->get_type() != Bool) {
        classtable->semant_error(cur_class) << "Type for pred in loop must be Bool" << endl;
        return;
    }
    body->type_check();
    type = Object;
}

/*Type check for if*/
void cond_class::type_check() {
    pred->type_check();
    then_exp->type_check();
    else_exp->type_check();

    if (pred->get_type() != Bool) {
        type = Object;
        classtable->semant_error(cur_class) << "Type for pred in if must be Bool" << endl;
        return;
    } else {
        type = g->lub(then_exp->get_type(), else_exp->get_type());
    }
}

/*Type check for block*/ 
void block_class::type_check() {
    Symbol seq_type;
    for (int i = body->first(); body->more(i); i = body->next(i)) {
        Expression e = body->nth(i);
        e->type_check();
        seq_type = e->get_type();
    }

    type = seq_type;
} 

/*Type check for cases*/
/*Type check for branch*/
void branch_class::type_check() {
    if (name == self) {
        classtable->semant_error(cur_class) << "bind self to case" << endl;
        return;
    }
    expr->type_check();
}

/*Type check for case*/
void typcase_class::type_check() {
    expr->type_check();
    Symbol final_type = NULL;

    symboltable->enterscope(); // The object type environment changes
    for(int i = cases->first(); cases->more(i); i = cases->next(i)) {
        Case c = cases->nth(i);
        //Each branch of a case must have distinct types
        if (symboltable->probe(c->get_type_decl()->get_string()) != NULL) {
            classtable->semant_error(cur_class) << "Each branch must have distinct types" << endl;
            return;
        }
        symboltable->addid(c->get_type_decl()->get_string(), c->get_type_decl());
        c->type_check();

        if (!g->check_conformace(c->get_expr_type(), c->get_type_decl())) {
            classtable->semant_error(cur_class) << "In each brank, expr type must conform decl type" << endl;
            return;
        }

        if (!final_type) {
            final_type = c->get_expr_type();
        } else {
            final_type = g->lub(final_type, c->get_expr_type());
        }
    }

    type = final_type;
    symboltable->exitscope();
}

/*Type check for Let expression, note that two cases: let-init, let-no-init*/
void let_class::type_check() {
    if (identifier == self) {
        classtable->semant_error(cur_class) << "bind self in a let" << endl;
        return;
    }

    init->type_check();
    symboltable->enterscope();
    Symbol t0;
    if (type_decl == SELF_TYPE) {
        t0 = cur_class->get_name();
    } else {
        t0 = type_decl;
    }

    if (init->get_type() == No_type) {
        //let-no-init case
        symboltable->addid(identifier->get_string(), t0);
        body->type_check();
        type = body->get_type();
    } else {
        //let-init case
        if (!g->check_conformace(init->get_type(), t0)) {
            classtable->semant_error(cur_class) << "let conformance unsatisfied" << endl;
            return;
        }
        symboltable->addid(identifier->get_string(), t0);
        body->type_check();
        type = body->get_type();
    }
}

/*Type check for object type*/
void object_class::type_check() {
    if (name == self) {
        type = SELF_TYPE;
        return;
    }

    //check declared identifers
    Symbol searchtype = symboltable->lookup(name->get_string());
    if (searchtype == NULL) {
        Class_ p = cur_class;
        //check attributes in parent class
        while(true) {
            Feature f = p->get_attr(name);
            if (f != NULL) {
                searchtype = f->get_type();
                break;
            }

            if (p->get_parent() == No_class) {
                break;
            }

            p = ctable->lookup(p->get_parent()->get_string());
        }
    }

    if (searchtype == NULL) {
        classtable->semant_error(cur_class) << "Can't find object definition" << endl;
        return;
    }

    type = searchtype;
}

void program_class::semant()
{
    initialize_constants();

    /* ClassTable constructor may do some semantic analysis */
    classtable = new ClassTable(classes);
    
    ctable = new SymbolTable<char*, Class__class>();
    ctable->enterscope();
    classtable->install_basic_classes();

    symboltable = new SymbolTable<char*, Entry>();

    pre_check();
    type_check();

    ctable->exitscope(); 

    if (classtable->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl; 
        exit(1);
    }
}


