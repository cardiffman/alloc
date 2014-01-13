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

/*int Q(element s_exp, int offs=0) {
  int V=0;
  int i=offs;
  while (isspace(IntFromBox(string_get_char(s_exp, BoxFromInt(i)))))
    i++;
  int t=i;
  while (V|!(isspace(IntFromBox(string_get_char(s_exp,BoxFromInt(t))))|IntFromBox(string_get_char(s_exp,BoxFromInt(t)))==')'||(IntFromBox(string_get_char(s_exp,BoxFromInt(t)))=='('&&t-i)))
    V+=(IntFromBox(string_get_char(s_exp,BoxFromInt(t)))=='(')-(IntFromBox(string_get_char(s_exp,BoxFromInt(t)))==')'),t++;
  return t-i;
}*/
element C(element s_exp){ 
	return car(s_exp);
}

element R(element s_exp) {
  return Eval(car(s_exp));
}
element A(element s_exp){
	return cdr(s_exp);
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
  return cons(i, t);
}
element F(element s_exp){
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
element b(element s_exp){
  return L(Eval(car(s_exp)),Eval(car(cdr(s_exp))))?NIL:newstr("t");
}
element o(element s_exp){
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
	return lt ? newstr("t"):NIL;
}
element f(element s_exp){
	element left = Eval(C(s_exp));
	element right = Eval(C(A(s_exp)));
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
element g(element s_exp){
	element left = Eval(C(s_exp));
	element right = Eval(C(A(s_exp)));
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
element h(element s_exp){
	element left = Eval(C(s_exp));
	element right = Eval(C(A(s_exp)));
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

class strfn {
public:
  element get() const { return str; }
  element exec(element ex) { return (*fn)(ex); }
  strfn(element s) { str=s; fn=0; }
  strfn(element (*f)(element)) { fn=f; str=NIL; }
  strfn() { fn=0; str=NIL; }
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
element defun(element s_exp){
  element e=cdr(s_exp);
  element n=car(s_exp);
  std::cout << "Entering e "<<e<<" n "<<n <<std::endl;
  enter(n,e);
  return n;
}
struct bi { const char* name; element (*fn)(element); };
bi builtins[] = {
  "function", R,
  "quote", C,
  "lambda", Z,
  "defun", defun,
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
	//cout << "eval |" << in << '|' << endl;
	if (BoxIsInteger(in))
		return in;
	if (!isnan(in.num))
		return in;
	if (in == NIL)
		return in;
#if 1
	if (BoxIsString(in)) {
		strfn x = find(in);
		if (x.get() != NIL)
			return x.get();
		if (x.func()) {
			return in;
		}
	}
#endif
	//std::cout << "Is List "<< in << std::endl;
	//std::cout << "car: " << car(in) << std::endl;
	//std::cout << "cdr: " << cdr(in) << std::endl;
	
	element op = car(in);
	if (BoxIsList(op))
		op = Eval(op);
	//std::cout << "after refinement "<< op << std::endl;
	
	strfn x = find(op);
	if (x.func())
		return x.exec(A(in));
	
	element lambda = x.get();
	element formals = C(lambda);
	element actuals = A(in);
	//cout << "About to eval to environment" << endl;
	size_t top = table.size();
	while (formals != NIL && actuals != NIL) {
		element formal = C(formals);
		element actual = Eval(C(actuals));
		//cout << "Entering "<< formal << " " << actual << endl;
		enter(formal, actual);
		formals = A(formals);
		actuals = A(actuals);
	}
	//cout <<"Body image: lambda " << lambda << endl; cout <<" cdr(lambda) " << cdr(lambda) <<  endl; cout <<" car(cdr(lambda)) "<< car(cdr(lambda)) << endl;
	element body = car(cdr(lambda));
	//std::cout << "Evaluating "<<  body <<" with the following:"<<std::endl;
	//for (int i=top; i<table.size(); ++i) {
	//  std::cout << table[i].first << " " << table[i].second.get() << std::endl;
	//}
	
	element rv = Eval(body);
	
	
	while (table.size()>top)
		table.pop_back();
	
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
	std::ostringstream os; ShowCar(os, atom);
	const char* ip = os.str().c_str();
	char* tail=0;
	long lval = strtol(ip, &tail, 10);
	if (tail[0]==0 && tail != ip && lval<INT_MAX && lval>INT_MIN)
		return BoxFromInt(lval);
	double dval = strtod(ip, &tail);
	if (tail[0]==0 && tail!= ip)
		atom.num = dval;
	return atom;
}

extern element* alloc;
int main(int, char**)
{
	init_heap();
	NIL.type = BOXSTR_LISTTYPE;
	NIL.tptr = 0;
	while (!feof(stdin))
	{
		cout << '[' << alloc << ']' << "> " << std::flush;
		element e = read_obj(stdin);
		cout << __FUNCTION__ << " The s-expr: " << e << endl;
		//if (BoxIsList(e))
		//	ShowList(cout, e);
		//else
		//	ShowCar(cout, e); 
		//cout << endl;
		element r = Eval(e);
		cout << __FUNCTION__ << " Its value: " << r << endl;
		//if (BoxIsList(r))
		//	ShowList(cout, r);
		//else
		//	ShowCar(cout, r);
		cout << endl;
		dump_heap();
	}
	return 0;
	
}
