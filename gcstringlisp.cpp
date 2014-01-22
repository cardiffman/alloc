/*
 * This small lisp interpreter was based on the
 * 1989 International Obfuscated C Code Championship winner.
 * Many things have been cleaned up. However there are some things to realize 
 * about it:
 * The code from which this is derived was about 1400 characters of
 * obfuscated C code. The code was obfuscated by pointless use of #define
 * a layout that arranged the code in the shape of a valentine heart, and
 * in many cases function names that served the desired look of the output
 * better than they served understandability.
 * Process:
 * The starting point was string-based, using strings whose storage was
 * sometimes obtained by strdup(), but often by sprintf() into the result of
 * "sbrk(199)". 
 * The program was first modified by substituting out the macros so that it
 * was more conventional.
 * Then it was translated to C++ without changing anything that didn't have
 * to change.
 * Then the string usage was changed to <string>
 * The lookup table's idiosyncracies were normalized out and a vector of
 * pairs put in as the basic look-up structure. The original environment
 * maintenance for procedure arguments was based on remembering the lookup
 * table's length before adding the actual parameter definitions to the 
 * table, so that the new symbols could be chopped off when the called
 * procedure returned. That continues to be the way actual parameters are 
 * managed.
 * Then I developed the nuts and bolts of the GC code in alloc.cpp and needed
 * more stressful test code. So a two-phase conversion of this code was done.
 * In the first phase, the code as seen in this file as of Jan 12 '14 was
 * created. The C++ string's were replaced with GC'd string elements. The
 * processing of strings as lists is retained, specifically the routine Q
 * that factors out some brace-counting from the C (car) and A (cdr) routines,
 * and the notion that NIL in this code is 'newstr("()")'.
 * Along the way in all phases some clean-up has been done, but this is really
 * nothing more than a curiosity.
 * The second phase, which also basically completed as of Jan 12 '14, was to
 * use the rest of the data types supported by the GC routines: boxed integers,
 * boxed doubles, and cons-cell-based lists. This code lives in the gclisp.cpp
 * file.
 * In both programs a lot of the original functions still exist and still have
 * the same names. In the gclisp.cpp file occasionally calls to C or A have 
 * been refactored as direct calls to car or cdr.
 * The third phase will be back at this code. This code tears through the heap
 * rather quickly when executing a small program, but I don't believe it really 
 * needs that many live cells. It does create a lot of garbage as it goes, so 
 * it will be the test driver for automatically collecting the garbage.
 */
#include "alloc.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <sstream>

using std::cout;
using std::endl;

element Eval(element in);
static element s_nil;

#if 0
//concept
void foo()
{
for (element i=string_begin(s); i!=string_end(s); i=string_next(i))
{
	int ch = IntFromBox(string_fetch(i));
}
}
#endif

/* 
 * Returns the index within the string of the end of the term.
 * Eg if pointed at the start of "my goodness" returns 2, the
 * index of the space between my and goodness.
 * Eg if pointed at the start of "(billy (goats (gruff))) eat anything"
 * returns 23, the index of the space between the 3rd ')' and eat.
 * Thus if you have a s_exp that represents a list and starts with '(', and give offs==1,
 * the index will be the end of the car of that s_exp. 
 * To get the cdr, first obtain that same index, then scan for the first non-blank,
 * then the cdr is the s_exp suffix that starts there, with a '(' prepended.
 * That is, (a b c) get Q(s_exp, 1) which will be 2. Scan to the b. Your suffix is
 * "b c)". Prefix a '(' to get (b c) and you're done.
 * This function assumes that one of the terminal conditions can happen before
 * the end of the string.
 */
int Q(element s_exp, int offset=0) 
{
	int depth=0;
	int i=offset;
	int k=IntFromBox(string_get_size(s_exp));
	while (i<k && isspace(IntFromBox(string_get_char(s_exp, BoxFromInt(i)))))
		i++;
	int t=i;
	int ch = IntFromBox(string_get_char(s_exp, BoxFromInt(t)));
	while (t<k && (depth|!(isspace(ch)|ch==')'||(ch=='('&&t-i))))
	{
		depth+=(ch=='(')-(ch==')'); // depth increases at '(' and decreases at ')'
		// when depth is not zero, we continue no matter what.
		t++;
		ch = IntFromBox(string_get_char(s_exp, BoxFromInt(t)));
	}
	return t-i;
}
void oldQBody()
{
	int depth;
	element s_exp;
	int t;
	int i;
	while (depth|!
		   (isspace
			(IntFromBox
			 (string_get_char
			  (s_exp,BoxFromInt(t))))
			|IntFromBox
			(string_get_char(
				 s_exp,BoxFromInt(t)))
			==')'
			||(
				IntFromBox(
					string_get_char
					(s_exp,BoxFromInt(t)))
				=='('
				&&t-i)))
		depth+=(IntFromBox
			(string_get_char(s_exp,BoxFromInt(t)))
			=='(')
				-
				(IntFromBox(string_get_char(s_exp,BoxFromInt(t)))
				 ==')'),t++;
	//return t-i;	
}

element basic_car(element s_exp){ 
	if (equal_data(BoxFromInt(0), string_get_size(s_exp)))
		return s_exp;
	element c1 = string_get_substr(s_exp, BoxFromInt(1));// std::string c1=s_exp.substr(1);
  int Y=Q(c1);
  if (Y>1024)
  {
	  std::cout << __FUNCTION__ << " Y is " << Y << " which is too big " << std::endl; 
	  *(int*)0 = 9;
  }
  return string_get_substr(c1, BoxFromInt(0), BoxFromInt(Y));//c1.substr(0,Y);
}

element interp_function(element s_exp) {
  return Eval(basic_car(s_exp));
}
element basic_cdr(element s_exp){
	if ('('!=IntFromBox(string_get_char(s_exp, BoxFromInt(0))))
	{
		std::cout << __FUNCTION__ << " sexp is " << s_exp << " which is an atom, not a list" << std::endl; 
		*(int*)0 = 9;
	}
	element a1=s_exp;
	int a=Q(a1,1)+1;
	if (a>1024)
	{
		std::cout << __FUNCTION__ << " a is " << a << " which is too big " << std::endl; 
		*(int*)0 = 9;
	}
	while (isspace(IntFromBox(string_get_char(a1, BoxFromInt(a))))) 
		a++;
	gc_add_root(&a1);
	element a2=newstr("(");
	gc_add_root(&a2);
	a2 = string_append_string(a2, string_get_substr(a1, BoxFromInt(a)));//a2 += a1.substr(a);
	gc_unroot(&a2);
	gc_unroot(&a1);
	return a2;
}
element interp_lambda(element s_exp) {
  return s_exp;
}
element interp_car(element s_exp){
  return basic_car(Eval(basic_car(s_exp)));
}
element interp_cdr(element s_exp){
  return basic_cdr(Eval(basic_car(s_exp)));
}
element interp_cons(element s_exp){
  element i=Eval(basic_car(s_exp));
  element t=Eval(basic_car(basic_cdr(s_exp)));
  if (t==s_nil)
    return string_append_string(string_append_string(newstr("("),i),newstr(")"));
  //return std::string("(")+i+t.substr(1);
  return string_append_string(string_append_string(newstr("("),i),string_get_substr(t,BoxFromInt(1)));
}
struct Rooter {
	Rooter(element& elt) : elt(elt) { gc_add_root(&elt); }
	~Rooter() { gc_unroot(&elt); }
	element& elt;
};
struct ExitPrint{
	ExitPrint(const std::string& t) : t(t) {}
	~ExitPrint() { cout << t << " on return." << endl; }
	std::string t;
};
element interp_if(element s_exp){
	Rooter x(s_exp);
	element test = basic_car(s_exp);
	Rooter x0(test);
	element test_result = Eval(test);
	Rooter x1(test_result);
	element cdr_of = test_result==s_nil?basic_cdr(s_exp):s_exp;
	return Eval(basic_car(basic_cdr(cdr_of)));
}
bool not_equal(element i, element s){
	std::ostringstream o1; o1 << i; //ShowCar(o1, i);
	std::ostringstream o2; o2 << s; //ShowCar(o2, s);
	return o1.str()!=o2.str();
  //return isdigit(i[0])?atof(i.c_str())!=atof(s.c_str()):(i!=s);
}

element interp_equal(element s_exp){
	Rooter x(s_exp);
	element left = Eval(basic_car(s_exp));
	Rooter x0(left);
	element right = Eval(basic_car(basic_cdr(s_exp)));
	Rooter x1(right);
	return not_equal(left, right) ? s_nil:newstr("t");
}
element interp_less(element s_exp){
	element left = Eval(basic_car(s_exp));
	element right = Eval(basic_car(basic_cdr(s_exp)));
	std::ostringstream o1; o1 << left; //ShowCar(o1, left);
	std::ostringstream o2; o2 << right; //ShowCar(o2, right);
	return atof(o1.str().c_str())<atof(o2.str().c_str())?newstr("t"):s_nil;
  //return atof(Eval(C(s_exp)).c_str())<atof(Eval(C(A(s_exp))).c_str())?"t":"()";
}
element interp_add(element s_exp){
	Rooter x(s_exp);
	element left = Eval(basic_car(s_exp));
	Rooter x0(left);
	element right = Eval(basic_car(basic_cdr(s_exp)));
	Rooter x1(right);
	std::ostringstream o1; o1 << left; //ShowCar(o1, left);
	std::ostringstream o2; o2 << right; //ShowCar(o2, right);
  std::ostringstream i;i<<(atof(o1.str().c_str())+ atof(o2.str().c_str()));
  return newstr(i.str().c_str());
}
element interp_sub(element s_exp){
	Rooter x(s_exp);
	element left = Eval(basic_car(s_exp));
	Rooter x0(left);
	element right = Eval(basic_car(basic_cdr(s_exp)));
	Rooter x1(right);
	std::ostringstream o1; o1 << left; //ShowCar(o1, left);
	std::ostringstream o2; o2 << right; //ShowCar(o2, right);
  std::ostringstream i;i<<(atof(o1.str().c_str())- atof(o2.str().c_str()));
  return newstr(i.str().c_str());
}
element interp_mul(element s_exp){
	Rooter x(s_exp);
	element left = Eval(basic_car(s_exp));
	Rooter x0(left);
	element right = Eval(basic_car(basic_cdr(s_exp)));
	Rooter x1(right);
	std::ostringstream o1; o1 << left; //ShowCar(o1, left);
	std::ostringstream o2; o2 << right; //ShowCar(o2, right);
  std::ostringstream i;i<<(atof(o1.str().c_str())* atof(o2.str().c_str()));
  return newstr(i.str().c_str());
}

element table;
bool builtins_loaded = false;
void enter(element n, element v)
{
	table = array_append_element(table, cons(n, v));
}
void enter(element n, element (*f)(element))
{
	table = array_append_element(table, cons(n, BoxFromBuiltIn(f)));
}
void RootEnvironment()
{
	gc_add_root(&table);
}
element find(element n)
{
	for (int i=IntFromBox(array_get_size(table)); i>0; --i)
	{
		element pair = array_get_element(table, BoxFromInt(i-1));
		element ifirst = car(pair);
		if (ifirst == n)
			return cdr(pair);
	}
	return s_nil;
}

element interp_defun(element s_exp){
  element e=basic_cdr(s_exp);
  element n=basic_car(s_exp);
  //std::cout << "Entering e "<<e<<" n "<<n <<std::endl;
  enter(n,e);
  return n;
}
struct bi { const char* name; element (*fn)(element); };
bi builtins[] = {
  "function", interp_function,
  "quote", basic_car,
  "lambda", interp_lambda,
  "defun", interp_defun,
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
	if (!builtins_loaded) 
	{
		builtins_loaded = true;
		table = array_create();
		for (bi* b=builtins; b->name!=0; ++b) 
		{
			enter(newstr(b->name), b->fn);
		}
		enter(newstr("t"),newstr("t"));
		s_nil = newstr("()");
		gc_add_root(&s_nil);
		gc_add_root(&table);
	}
}
element Eval(element in)
{
	setup();
	cout << __FUNCTION__ << " arg |" << in << "|[";ShowElement(cout,in);cout<<"]:"<<__FILE__<<':'<<__LINE__ << endl;
	if (string_get_size(in)==BoxFromInt(0))
	{
		cout << " bad expr " << endl;
		*(int*)0 = 10;
	}
	std::ostringstream os; os << in; if (os.str().size()==0)
	{
		cout << " bad expr " << endl;
		*(int*)0 = 11;
	}
	cout << __FUNCTION__ << " sizeof visible rep isn't zero? "<<os.str().size() <<':'<<__FILE__<<':'<<__LINE__ << endl;
	if (isdigit(IntFromBox(string_get_char(in, BoxFromInt(0)))))
	{
		cout << __FUNCTION__ << " returning "<< in <<':'<<__FILE__<<':'<<__LINE__ << endl;
		return in;
	}
	if(in==s_nil)
		return in;
#if 1
	if (IntFromBox(string_get_char(in,BoxFromInt(0)))!='(') {
		cout << __FUNCTION__ << " Calling find with "<< in <<':'<<__FILE__<<':'<<__LINE__ << endl;
		element x = find(in);
		cout << "Lookup "<<in<<" result: ";
		if (BoxIsBuiltin(x))
			cout << "a built-in called " << in;
		else
			cout		<< x;
		cout << endl;
		if (!BoxIsBuiltin(x))
		{
			cout << __FUNCTION__ << " returning "<< x <<':'<<__FILE__<<':'<<__LINE__ << endl;
			return x;
		}
		if (BoxIsBuiltin(x))
		{
			cout << __FUNCTION__ << " returning "<< in <<':'<<__FILE__<<':'<<__LINE__ << endl;
			return in;
		}
	}
#endif
	//std::cout << "starts with paren " << in << std::endl;
	//std::cout << "car: " << basic_car(in) << std::endl;
	//std::cout << "cdr: " << basic_cdr(in) << std::endl;
	
	gc_add_root(&in);
	element op = basic_car(in);
	gc_add_root(&op);
	
	std::cout << "line "<< __LINE__ << " in "<< in << " op " << op << std::endl;
	if (IntFromBox(string_get_char(op,BoxFromInt(0)))=='(')
	{
		op=Eval(op);
	}
	std::cout << "line "<< __LINE__ << " after refinement |"<< op <<'|'<< std::endl;
	
	cout << "Calling find " << __LINE__ << endl;
	element x = find(op);
	std::ostringstream ck; ck << x; if (ck.str().size()==0) {
		for (int i=0; i<IntFromBox(array_get_size(table)); ++i)
		{
			 element pair = array_get_element(table, BoxFromInt(i));
			 element name = car(pair);
			 //element func = cdr(pair);
			 //bool isFunc = BoxIsBuiltin(func);
			cout << __FUNCTION__ << " table["<<i<<"] "<< name << '|';ShowElement(cout,name); cout<<'|' << ':'<<__FILE__<<':'<<__LINE__ << endl;
		}
	}
	cout << "Lookup result: "; if (!BoxIsBuiltin(x)) cout << x;
	if (BoxIsBuiltin(x))
		cout << "a built-in called " << op;
	cout << endl;
	gc_unroot(&op);
	if (BoxIsBuiltin(x))
	{
		gc_add_root(&x);
		Builtin f = BuiltinFromBox(x);
		element r = f(basic_cdr(in));
		gc_unroot(&x);
		return r;
	}
	
	element lambda = x;
	gc_add_root(&lambda); std::cout << "Rooted lambda " << lambda.num << ' ' << std::hex << lambda.type << std::dec << ' ' << lambda.tptr << std::endl;
	element formals = basic_car(lambda);
	gc_add_root(&formals);
	cout << __FUNCTION__ << ' ' << "getting actuals from cdr of " << in << std::endl;
	element actuals = basic_cdr(in);
	gc_add_root(&actuals);
	element top = array_get_size(table);
	while (formals != s_nil && actuals != s_nil) {
		cout << __FUNCTION__ << ' ' << "formals: " << formals << " actuals " << actuals << std::endl;
		element formal = basic_car(formals);
		gc_add_root(&formal);
		element actual_expr = basic_car(actuals);
		cout << __FUNCTION__ << ' ' << "formal: " << formal<< " actual expr " << actual_expr << endl;
		gc_add_root(&actual_expr);
		element actual = Eval(actual_expr);
		gc_unroot(&actual_expr);
		cout << __FUNCTION__ << ' ' << "formal: " << formal << " actual " << actual << endl;
		enter(formal, actual);
		formals = basic_cdr(formals);
		actuals = basic_cdr(actuals);
	}
	gc_unroot(&actuals);
	gc_unroot(&formals);
	element body = basic_car(basic_cdr(lambda));
	//std::cout << "Evaluating "<<body<<" with the following:"<<std::endl;
	//for (int i=top; i<table.size(); ++i) {
	//	std::cout << table[i].first << " " << table[i].second.get() << std::endl;
	//}
	
	gc_add_root(&body);
	RootEnvironment();
	element rv = Eval(body);
	gc_unroot(&body);
	gc_unroot(&lambda); std::cout << "Unrooted lambda " << lambda.num << ' ' << std::hex << lambda.type << std::dec << ' ' << lambda.tptr << std::endl;
	

	table = array_set_size(table, top);
	return rv;
}

int main(int, char**)
{
	init_heap();
	setup();// setup the environment.
	RootEnvironment();
	
	element in = newstr();
	element* pin = &in;
	gc_add_root(&in);
	while (true) {
		cout << "> " << std::flush;
		int parens=0;
		int ch;
		while(parens|!isspace(ch=getchar())){
			if (ch==EOF)
				exit(0);
			if (ch=='(')
				parens++;
			else if (ch==')')
				parens--;
			element nin = string_append_char(in, BoxFromInt(ch));
			in = nin;
		}
		cout << Eval(in) << endl;
		in=newstr();
	}
	
}
