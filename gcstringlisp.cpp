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

element Eval(element in);

int Q(element s_exp, int offs=0) {
  int V=0;
  int i=offs;
  while (isspace(IntFromBox(string_get_char(s_exp, BoxFromInt(i)))))
    i++;
  int t=i;
  while (V|!(isspace(IntFromBox(string_get_char(s_exp,BoxFromInt(t))))|IntFromBox(string_get_char(s_exp,BoxFromInt(t)))==')'||(IntFromBox(string_get_char(s_exp,BoxFromInt(t)))=='('&&t-i)))
    V+=(IntFromBox(string_get_char(s_exp,BoxFromInt(t)))=='(')-(IntFromBox(string_get_char(s_exp,BoxFromInt(t)))==')'),t++;
  return t-i;
}
element C(element s_exp){ 
	element c1 = string_get_substr(s_exp, BoxFromInt(1));// std::string c1=s_exp.substr(1);
  int Y=Q(c1);
  return string_get_substr(c1, BoxFromInt(0), BoxFromInt(Y));//c1.substr(0,Y);
}

element R(element s_exp) {
  return Eval(C(s_exp));
}
element A(element s_exp){
  element a1=s_exp;
  int a=Q(a1,1)+1;
  while (isspace(IntFromBox(string_get_char(a1, BoxFromInt(a))))) a++;
  element a2=newstr("(");
  a2 = string_append_string(a2, string_get_substr(a1, BoxFromInt(a)));//a2 += a1.substr(a);
  return a2;
}
element Z(element s_exp) {
  return s_exp;
}
element c(element s_exp){
  return C(Eval(C(s_exp)));
}
element q(element s_exp){
  return A(Eval(C(s_exp)));
}
element t(element s_exp){
  element i=Eval(C(s_exp));
  element t=Eval(C(A(s_exp)));
  if (t==newstr("()"))
    return string_append_string(string_append_string(newstr("("),i),newstr(")"));
  //return std::string("(")+i+t.substr(1);
  return string_append_string(string_append_string(newstr("("),i),string_get_substr(t,BoxFromInt(1)));
}
element F(element s_exp){
  return Eval(C(A((Eval(C(s_exp))==newstr("()"))?A(s_exp):s_exp)));
}
bool L(element i, element s){
	std::ostringstream o1; ShowCar(o1, i);
	std::ostringstream o2; ShowCar(o2, s);
	return o1.str()!=o2.str();
  //return isdigit(i[0])?atof(i.c_str())!=atof(s.c_str()):(i!=s);
}
element b(element s_exp){
  return L(Eval(C(s_exp)),Eval(C(A(s_exp))))?newstr("()"):newstr("t");
}
element o(element s_exp){
	element left = Eval(C(s_exp));
	element right = Eval(C(A(s_exp)));
	std::ostringstream o1; ShowCar(o1, left);
	std::ostringstream o2; ShowCar(o2, right);
	return atof(o1.str().c_str())<atof(o2.str().c_str())?newstr("t"):newstr("()");
  //return atof(Eval(C(s_exp)).c_str())<atof(Eval(C(A(s_exp))).c_str())?"t":"()";
}
element f(element s_exp){
	element left = Eval(C(s_exp));
	element right = Eval(C(A(s_exp)));
	std::ostringstream o1; ShowCar(o1, left);
	std::ostringstream o2; ShowCar(o2, right);
  std::ostringstream i;i<<(atof(o1.str().c_str())+ atof(o2.str().c_str()));
  return newstr(i.str().c_str());
}
element g(element s_exp){
	element left = Eval(C(s_exp));
	element right = Eval(C(A(s_exp)));
	std::ostringstream o1; ShowCar(o1, left);
	std::ostringstream o2; ShowCar(o2, right);
  std::ostringstream i;i<<(atof(o1.str().c_str())- atof(o2.str().c_str()));
  return newstr(i.str().c_str());
}
element h(element s_exp){
	element left = Eval(C(s_exp));
	element right = Eval(C(A(s_exp)));
	std::ostringstream o1; ShowCar(o1, left);
	std::ostringstream o2; ShowCar(o2, right);
  std::ostringstream i;i<<(atof(o1.str().c_str())* atof(o2.str().c_str()));
  return newstr(i.str().c_str());
}

class strfn {
public:
  element get() const { return str; }
  element exec(element ex) { return (*fn)(ex); }
  strfn(element s) { str=s; fn=0; }
  strfn(element (*f)(element)) { fn=f; str=newstr(); }
  strfn() { fn=0; str=newstr(); }
  bool func() const { return fn!=0; }
private: 
  element str; 
  element (*fn)(element); 
  
};
std::vector<std::pair<element,strfn> > table;
void enter(element n, element v)
{
  table.push_back(std::make_pair(n,strfn(v)));
}
void enter(element n, element (*f)(element))
{
  table.push_back(std::make_pair(n,strfn(f)));
}
using std::cout;
using std::endl;
strfn find(element n)
{
  for (size_t i=table.size();i>0;--i) {
	  element ifirst = table[i-1].first;
	  //cout << __FUNCTION__ << ' ' << ifirst << " vs " << n << " " << (ifirst==n) << endl;
	  if (ifirst==n)
		  return table[i-1].second;
    if (table[i-1].first==n)
      return table[i-1].second;
  }
  return strfn();
}
element j(element s_exp){
  element e=A(s_exp);
  element n=C(s_exp);
  //std::cout << "Entering e "<<e<<" n "<<n <<std::endl;
  enter(n,e);
  return n;
}
struct bi { const char* name; element (*fn)(element); };
bi builtins[] = {
  "function", R,
  "quote", C,
  "lambda", Z,
  "defun", j,
  "if", F,
  "equal", b,
  "<"  , o,
  "+", f,
  "-", g,
  "*", h,
  "car", c,
  "cdr", q,
  "cons", t,
  0,0
};
void setup()
{
  if (table.size()==0) {
    for (bi* b=builtins; b->name!=0; ++b) {
      enter(newstr(b->name), b->fn);
    }
    enter(newstr("t"),newstr("t"));
  }
}
element Eval(element in)
{
	setup();
	//cout << "eval " << in << endl;
	if (isdigit(IntFromBox(string_get_char(in, BoxFromInt(0)))))
	{
		return in;
	}
	if(in==newstr("()"))
		return in;
#if 1
	if (IntFromBox(string_get_char(in,BoxFromInt(0)))!='(') {
		strfn x = find(in);
		//cout << "Lookup result: " << x.get();
		//if (x.func())
		//	cout << "a built-in called " << in;
		//cout << endl;
		if (IntFromBox(string_get_size(x.get()))!=0)
			return x.get();
		if (x.func()) {
			return in;
		}
	}
#endif
	//std::cout << "starts with paren " << in << std::endl;
	//std::cout << "car: " << C(in) << std::endl;
	//std::cout << "cdr: " << A(in) << std::endl;
	
	element op = C(in);
	//std::cout << "line "<< __LINE__ << " "<< in << std::endl;
	if (IntFromBox(string_get_char(op,BoxFromInt(0)))=='(')
		op=Eval(op);
	//std::cout << "line "<< __LINE__ << " after refinement "<< op << std::endl;
	//if (isdigit(IntFromBox(string_get_char(op, BoxFromInt(0)))))
	//  return op;
	
	strfn x = find(op);
	//cout << "Lookup result: " << x.get();
	//if (x.func())
	//	cout << "a built-in called " << op;
	//cout << endl;
	if (x.func())
		return x.exec(A(in));
	
	element lambda = x.get();
	element formals = C(lambda);
	element actuals = A(in);
	size_t top = table.size();
	while (formals != newstr("()") && actuals != newstr("()")) {
		element formal = C(formals);
		element actual = Eval(C(actuals));
		enter(formal, actual);
		formals = A(formals);
		actuals = A(actuals);
	}
	element body = C(A(lambda));
	//std::cout << "Evaluating "<<body<<" with the following:"<<std::endl;
	//for (int i=top; i<table.size(); ++i) {
	//	std::cout << table[i].first << " " << table[i].second.get() << std::endl;
	//}
	
	element rv = Eval(body);
	
	
	while (table.size()>top)
		table.pop_back();
	
	return rv;
}

int main(int, char**)
{
	init_heap();
	
	element in = newstr();
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
			string_append_char(in, BoxFromInt(ch));
		}
		cout << Eval(in) << endl;
		in=newstr();
	}
	
}
