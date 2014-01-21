#ifndef ALLOC_H
#define ALLOC_H
#include <math.h> // isnan
#include <stdint.h>

enum Type { BOXSTR_LISTTYPE=0xFFFF0001, // pointer to a pair of elements. Typically the second in the pair is a list.
			BOXSTR_FORWARDTYPE=0xFFFF0002, // a forwarding pointer
			BOXSTR_STRINGTYPE=0xFFFF0003, // pointer to a STRHDR.
			BOXSTR_STRHDR, // contains a character count and is immediately followed by said characters
			BOXSTR_ARRAY, // pointer to an ARRHDR.
			BOXSTR_ARRHDR, // contains an element count and is immediately followed by said elements
			BOXSTR_BUILTIN, // contains a pointer to a built-in function
			BOXSTR_INTTYPE // contains an integer
		  };
typedef union element {
  double num;
  struct {
#if BIGENDIAN
	  Type type;
	  element* tptr;
#else
	  // This is known to satisfactorily put the pointer in the low
	  // bits of the double and put the Type in the high bits of
	  // the double, with G++ on x86.
	  element* tptr; 
	  Type type;
#endif
  };
  uint16_t ch[4];
} element;
bool equal_data(const element& a, const element& b);
inline bool operator==(const element& a, const element& b)
{
	return equal_data(a,b);
}
inline bool operator!=(const element& a, const element& b)
{
	return !equal_data(a,b);
}

inline bool BoxIsList(const element& b) { return b.type == BOXSTR_LISTTYPE; }
inline bool BoxIsDouble(const element& b) { return !isnan(b.num); }
inline bool BoxIsForward(const element& b) { return b.type==BOXSTR_FORWARDTYPE; }
inline bool BoxIsString(const element& b) {return b.type==BOXSTR_STRINGTYPE; }
inline bool BoxIsInteger(const element& b) { return b.type==BOXSTR_INTTYPE; }
inline bool BoxIsHeader(const element& b) { return b.type==BOXSTR_STRHDR||b.type==BOXSTR_ARRHDR; }
inline bool BoxIsArray(const element& b) { return b.type==BOXSTR_ARRAY; }
inline bool BoxIsBuiltin(const element& b) { return b.type==BOXSTR_BUILTIN; }
inline int IntFromBox(const element& b) { return (int)(b.tptr); }
inline element BoxFromInt(int x) { element e; e.type = BOXSTR_INTTYPE; e.tptr = (element*)x; return e; }
inline element ElementFromInt(Type t, int i) { element e; e.type = t; e.tptr = (element*)i; return e; }
typedef element (*Builtin)(element);
inline element BoxFromBuiltIn(element (*fn)(element)) { element e; e.type = BOXSTR_BUILTIN; e.tptr = (element*)fn; return e;}
inline Builtin BuiltinFromBox(const element& b) { return (Builtin)b.tptr; }
#include <iosfwd>
std::ostream& operator<<(std::ostream& out, const element& elt);

element cons(element car, element cdr);
element car(element pair);
element cdr(element pair);
element newstr();
element newstr(const char* asciz);
element string_append_char(element str, element ch);
element string_append_string(element stra, element strb);
element string_get_char(element str, element index);
element string_get_substr(element str, element first);
element string_get_substr(element str, element first, element count);
element string_get_size(element str);
element array_create();
element array_create(element size);
element array_append_element(element array, element elt);
element array_get_element(element array, element index);
element array_set_element(element array, element index, element elt); // Can extend array
element array_get_size(element array);
element array_set_size(element array, element size); // can extend array

//void ShowCar(std::ostream& out, const element &car);
//void ShowList(std::ostream& out, const element& cons);
void ShowElement(std::ostream& os, element e);
void gc_add_root(element* root);
void gc_unroot(element* root);
void gc_collect();
void init_heap();

// Awkward:
void dump_heap();
// Very Awkward:
void dump_new_heap();

// probably shouldn't be here:
//extern element space[];
//extern element* alloc;
//extern element* tospace;
//extern element* next;

#endif // ALLOC_H
