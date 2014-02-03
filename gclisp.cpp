#include "alloc.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <climits>

using std::cout;
using std::endl;

element Eval(element in);
extern element NIL;

/*element C(element s_exp){ 
	return car(s_exp);
}*/

element interp_function(element s_exp) {
  return Eval(car(s_exp));
}
/*element A(element s_exp){
	return cdr(s_exp);
}*/
element interp_lambda(element s_exp) {
  return s_exp;
}
element interp_car(element s_exp){
	return car(Eval(car(s_exp)));
  //return C(Eval(C(s_exp)));
}
element interp_cdr(element s_exp){
	return cdr(Eval(car(s_exp)));
  //return A(Eval(C(s_exp)));
}
element interp_cons(element s_exp){
  //element i=Eval(C(s_exp));
  //element t=Eval(C(A(s_exp)));
  element i=Eval(car(s_exp));
  element t=Eval(car(cdr(s_exp)));
  return cons(i, t);
}
element interp_if(element s_exp){
  return Eval(car(cdr((Eval(car(s_exp))==NIL)?cdr(s_exp):s_exp)));
}
bool L(element i, element s){
	//cout << "L:" << i << " vs. " << s ;
	if (BoxIsInteger(i) && BoxIsInteger(s))
		return !equal_data(i, s);
	if (BoxIsDouble(i) && BoxIsDouble(s))
		return !equal_data(i, s);
	if (BoxIsDouble(i) && (BoxIsDouble(s) || BoxIsInteger(s))
		|| BoxIsDouble(s) && (BoxIsDouble(i) || BoxIsInteger(i)))
	{
		double id = BoxIsDouble(i) ?i.num : IntFromBox(i);
		double sd = BoxIsDouble(s) ?s.num : IntFromBox(s);
		return id != sd;
	}
	if (BoxIsDouble(i)||BoxIsInteger(i)||BoxIsDouble(s)||BoxIsInteger(s))
	{
		cout << __FUNCTION__ << " NEVER EVER " << i << ' '<< s <<endl;
	}
	bool r = !equal_data(i, s);
	//cout << " -> " << r << endl;
	return r;
}
element interp_equal(element s_exp){
  return L(Eval(car(s_exp)),Eval(car(cdr(s_exp))))?NIL:symbol_create("t");
}
element interp_less(element s_exp){
	element left = Eval(car(s_exp));
	element right = Eval(car(cdr(s_exp)));
	bool lt = false;
	if (BoxIsInteger(left) && BoxIsInteger(right))
		lt = IntFromBox(left) <IntFromBox(right);
	if (BoxIsInteger(left) && !isnan(right.num))
		lt = IntFromBox(left) < right.num;
	else if (!isnan(left.num) && BoxIsInteger(right))
		lt = left.num < IntFromBox(right);
	else if (!isnan(left.num) && !isnan(right.num))
		lt = left.num < right.num;
	return lt ? symbol_create("t"):NIL;
}
element interp_add(element s_exp){
	element left = Eval(car(s_exp));
	element right = Eval(car(cdr(s_exp)));
	if (BoxIsInteger(left) && BoxIsInteger(right))
		return BoxFromInt(IntFromBox(left)+IntFromBox(right));
	double d;
	element r;
	if (BoxIsInteger(left) && !isnan(right.num))
		d = IntFromBox(left) + right.num;
	else if (!isnan(left.num) && BoxIsInteger(right))
		d = left.num + IntFromBox(right);
	else if (!isnan(left.num) && !isnan(right.num))
		d = left.num + right.num;
	else
		return NIL;
	r.num = d;
	return r;
}
element interp_sub(element s_exp){
	element left = Eval(car(s_exp));
	element right = Eval(car(cdr(s_exp)));
	if (BoxIsInteger(left) && BoxIsInteger(right))
		return BoxFromInt(IntFromBox(left)-IntFromBox(right));
	double d;
	element r;
	if (BoxIsInteger(left) && !isnan(right.num))
		d = IntFromBox(left) - right.num;
	else if (!isnan(left.num) && BoxIsInteger(right))
		d = left.num - IntFromBox(right);
	else if (!isnan(left.num) && !isnan(right.num))
		d = left.num - right.num;
	else
		return NIL;
	r.num = d;
	return r;
}
element interp_mul(element s_exp){
	element left = Eval(car(s_exp));
	element right = Eval(car(cdr(s_exp)));
	if (BoxIsInteger(left) && BoxIsInteger(right))
		return BoxFromInt(IntFromBox(left)*IntFromBox(right));
	double d;
	element r;
	if (BoxIsInteger(left) && !isnan(right.num))
		d = IntFromBox(left) * right.num;
	else if (!isnan(left.num) && BoxIsInteger(right))
		d = left.num * IntFromBox(right);
	else if (!isnan(left.num) && !isnan(right.num))
		d = left.num * right.num;
	else
		return NIL;
	r.num = d;
	return r;
}

element table;
bool builtins_loaded = false;
void enter(element n, element v)
{
	Rooter n_r(n);
	Rooter v_r(v);
	table = array_append_element(table, cons(n, v));
}
void enter(element n, element (*f)(element))
{
	Rooter n_r(n);
	table = array_append_element(table, cons(n, BoxFromBuiltIn(f)));
}
element find(element n)
{
	for (int i=IntFromBox(array_get_size(table)); i>0; --i)
	{
		element pair = array_get_element(table, BoxFromInt(i-1));
		//cout << __FUNCTION__ << " n " << n << " vs. " << pair << " :" <<__FILE__<<':'<<__LINE__<<endl;
		element ifirst = car(pair);
		if (ifirst == n)
			return cdr(pair);
	}
	return NIL;
}
element defun(element s_exp){
  element e=cdr(s_exp);
  element n=car(s_exp);
  std::cout << "Entering e "<<e<<" n "<<n <<std::endl;
  enter(n,e);
  return n;
}
struct bi { const char* name; element (*fn)(element); };
bi builtins[] = {
  "function", interp_function,
  "quote", car,
  "lambda", interp_lambda,
  "defun", defun,
  "if", interp_if,
  "equal", interp_equal,
  "<"  , interp_less,
  "+", interp_add,
  "-", interp_sub,
  "*", interp_mul,
  "car", interp_car,
  "cdr", interp_cdr,
  "cons", interp_cons,
  0,0
};
void setup()
{
	if (!builtins_loaded) {
		builtins_loaded = true;
		table = array_create();
		gc_add_root(&table); // This is permanent by the way.
		for (bi* b=builtins; b->name!=0; ++b) {
			enter(symbol_create(b->name), b->fn);
		}
		enter(symbol_create("t"),symbol_create("t"));
		cout << table << endl;
		cout << __FUNCTION__ << ' ' << "Built-ins installed" << " :" << __FILE__ << ':' << __LINE__ << endl;
	}
}
void check_setup()
{
	for (int i=IntFromBox(array_get_size(table)); i>0; --i)
	{
		element pair = array_get_element(table, BoxFromInt(i-1));
		if (!BoxIsList(pair))
		{
			cout << __FUNCTION__ << " Element " << i << " of the environment is not a pair :" <<__FILE__<<':'<<__LINE__<<endl;
			throw "bad setup";
		}
		//cout << __FUNCTION__ << " n " << n << " vs. " << pair << " :" <<__FILE__<<':'<<__LINE__<<endl;
		element name = car(pair);
		if (!BoxIsSymbol(name))
		{
			cout << __FUNCTION__ << " Element " << i << " car is not a symbol :" <<__FILE__<<':'<<__LINE__<<endl;
			throw "bad setup";
		}
		// The names in the environment cannot be limited to a particular type.
	}
}

element Eval(element in)
{
	//Rooter in_r(in);
	setup();
	//check_setup();
	cout << "eval |" << in << '|' << endl;
	if (BoxIsInteger(in))
		return in;
	if (!isnan(in.num))
		return in;
	if (in == NIL)
		return in;
#if 1
	if (BoxIsSymbol(in)) {
		element x = find(in);
		if (BoxIsBuiltin(x))
			return in;
		if (x == NIL)
			cout << __FUNCTION__ << " Lookup of " << in << " returned "<< x << " " << __FILE__ << ':' << __LINE__ <<endl;
		return x;
	}
#endif
	if (!BoxIsList(in))
		return in;
	//std::cout << "Is List "<< in << std::endl;
	//std::cout << "car: " << car(in) << std::endl;
	//std::cout << "cdr: " << cdr(in) << std::endl;
	
	element op = car(in);
	if (BoxIsList(op))
		op = Eval(op);
	//std::cout << "after refinement "<< op << std::endl;
	
	element x = find(op);
	Rooter x_r(x);
	
	if (BoxIsBuiltin(x))
	{
		Builtin f = BuiltinFromBox(x);
		element r = f(cdr(in));
		cout << __FUNCTION__ << " Result of built-in function " << r << " " << __FILE__ << ':' << __LINE__ <<endl;
		return r;
	}
	
	element lambda = x;
	Rooter lambda_r(lambda);
	element formals = car(lambda);
	Rooter formals_r(formals);
	element actuals = cdr(in);
	Rooter actuals_r(actuals);
	//cout << "About to eval to environment" << endl;
	element top = array_get_size(table);
	while (formals != NIL && actuals != NIL) {
		element formal = car(formals);
		Rooter formal_r(formal);
		element actual = Eval(car(actuals));
		//cout << "Entering "<< formal << " " << actual << endl;
		Rooter actual_r(actual);
		enter(formal, actual);
		formals = cdr(formals);
		actuals = cdr(actuals);
	}
	//cout <<"Body image: lambda " << lambda << endl; cout <<" cdr(lambda) " << cdr(lambda) <<  endl; cout <<" car(cdr(lambda)) "<< car(cdr(lambda)) << endl;
	element body = car(cdr(lambda));
	//std::cout << "Evaluating "<<  body <<" with the following:"<<std::endl;
	//for (int i=top; i<table.size(); ++i) {
	//  std::cout << table[i].first << " " << table[i].second.get() << std::endl;
	//}
	
	element rv = Eval(body);
	
	
	table=array_set_size(table, top);
	
	//cout << "Result is " << rv << endl;
	return rv;
}

static element atom;
void chartoatom(int ch)
{
	string_append_char(atom, BoxFromInt(ch));
}
int peek_char(FILE* fp)
{
	int ch = getc(fp);
	ungetc(ch, fp);
	return ch;
}
int check_delim(FILE* fp)
{
	int ch = peek_char(fp);
	if (!isspace(ch) && ch!='('&&ch!=')'&&ch!=';'&&ch!='"')
	{
		fprintf(stderr,"Improper delimiter for literal\n");
		return 0;
	}
	return 1;
}
int skip_white(FILE* fp)
{
	int ch;
eat_space:
	for (ch=getc(fp); isspace(ch); ch=getc(fp))
		;
	if (ch==';')
	{
		while (ch != '\n' && ch!=EOF)
			ch = getc(fp);
		goto eat_space;
	}
	return ch;
}
element read_obj(FILE* fp);
element read_pair(FILE* fp)
{
	int ch;
	element car_obj;
	element cdr_obj=NIL;
	ch = skip_white(fp);
	if (ch==')')
		return NIL;
	ungetc(ch, fp);
	car_obj = read_obj(fp);
	ch = skip_white(fp);
	if (ch == '.')
	{
		//printf("read_pair %d\n", __LINE__);
		cdr_obj = read_pair(fp);
		return cons(car_obj,cdr_obj);
	}
	else if (ch == ')')
		return cons(car_obj,cdr_obj);
	ungetc(ch, fp);
	//printf("read_pair  %d\n", __LINE__);
	cdr_obj = read_pair(fp);
	return cons(car_obj,cdr_obj);
}
element read_obj(FILE* fp)
{
	int ch;
	ch = skip_white(fp);
#if 0
	if (ch=='#')
	{
// some sort of literal
		ch = getc(fp);
		if (ch=='(') // vector
		{
			int* elements = 0;
			int nElements = 0;
			int nElementCapy = 0;
			ch = skip_white(fp);
			while (ch != ')')
			{
				ungetc(ch, fp);
				int element = read_obj(fp);
				if (nElements+1 > nElementCapy) {
					if (nElementCapy==0)
						nElementCapy = 8;
					else
						nElementCapy = 6*nElementCapy/5+1;
					elements = (int*)realloc(elements, nElementCapy*sizeof(int));
				}
				elements[nElements++] = element;
				ch = skip_white(fp);
			}
			elements = (int*)realloc(elements, (nElements*4+11)&~7);
			memmove(elements+1,elements,nElements*4);
			elements[0] = nElements;
			return VECTOR+(int)elements;
		}
		if (ch=='\\') // character literal
		{
			ch = getc(fp);
			if (!check_delim(fp))
			{
				fprintf(stderr,"Improper delimiter for literal\n");
				return NIL;
			}
			return ch*256+CHAR_LITERAL;
		}
		if (ch=='t')
			return TRUE_LITERAL;
		if (ch=='f')
			return FALSE_LITERAL;
		return NIL;
	}
#endif
	if (ch=='(') 
	{
		element elt = read_pair(fp);
		cout << "read_pair returned " << elt << std::endl;
		return elt;
	}
	if (ch=='"')
	{
		atom = newstr();
		ch = getc(fp);
		while (ch != '"')
		{
			if (ch == '\\')
				chartoatom(getc(fp));
			else
				chartoatom(ch);
			ch = getc(fp);
		}
		return atom;
	}
	atom = newstr();
	for (; ch!=EOF&&!isspace(ch)&&ch!='('&&ch!=')'&&ch!='#'&&ch!=';'; ch=getc(fp))
	{
		chartoatom(ch);
	}
	ungetc(ch,fp);
	std::ostringstream os; os << atom;
	const char* ip = os.str().c_str();
	char* tail=0;
	long lval = strtol(ip, &tail, 10);
	if (tail[0]==0 && tail != ip && lval<INT_MAX && lval>INT_MIN)
		return BoxFromInt(lval);
	double dval = strtod(ip, &tail);
	if (tail[0]==0 && tail!= ip)
		atom.num = dval;
	return symbol_from_string(atom);
}
#include <cstring>
extern element* alloc;
int main(int argc, char** argv)
{
	init_heap();
	if (argc==2 && strcmp(argv[1], "-t")==0)
	{
		setup();
		extern element symbols;
		cout << symbols << endl;
		cout << table << endl;
		element n = symbol_create("n");
		element nm1 = cons(symbol_create("-"), cons(n,cons(BoxFromInt(1),NIL)));
		element fnm1 = cons(symbol_create("fact"), cons(nm1,NIL));
		element times = cons(symbol_create("*"), cons(n, cons(fnm1,NIL)));
		element one = BoxFromInt(1);
		element test = cons(symbol_create("equal"), cons(n, cons(BoxFromInt(0),NIL)));
		element ifs = cons(symbol_create("if"), cons(test, cons(one, cons(times,NIL))));
		element e = cons(symbol_create("defun"), cons(symbol_create("fact"), cons(cons(n,NIL),cons(ifs,NIL))));
		cout << "Test expr " << e << endl;
		element e2 = cons(symbol_create("fact"), cons(BoxFromDouble(50.0), NIL));
		cout << "Test expr " << e2 << endl;
		cout << Eval(e) << endl;
		cout << Eval(e2) << endl;
		return 0;
	}
	while (!feof(stdin))
	{
		cout << '[' << alloc << ']' << "> " << std::flush;
		element e = read_obj(stdin);
		cout << __FUNCTION__ << " The s-expr: " << e << endl;
		element r = Eval(e);
		cout << __FUNCTION__ << " Its value: " << r << endl;
		cout << endl;
		dump_heap();
	}
	return 0;
	
}
