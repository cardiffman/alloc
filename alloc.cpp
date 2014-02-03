#include "alloc.h"
#include <iostream>
#include <iomanip>
#include <stdint.h>
#include <string.h>
#include <cstdlib>
#include <set>
#include <sstream>

using std::cout;
using std::endl;
using std::set;
#if 0
void LOG()
{
	cout << endl;
}
template <typename T> void LOG(const T& t)
{
	cout << t << endl;
}

template <typename First, typename... Rest> void LOG(const First& first, const Rest&... rest)
{
	cout << first;
	LOG(rest...);
}
#else
#define LOG(x...) (void)0
#endif
void XLOG()
{
	cout << endl;
}
template <typename T> void XLOG(const T& t)
{
	cout << t << endl;
}

template <typename First, typename... Rest> void XLOG(const First& first, const Rest&... rest)
{
	cout << first;
	XLOG(rest...);
}


/*
 *
 * Cheney-style copying garbage collection
 *
 * The initial condition is that there are two areas or semispaces of 
 * equal size. One space is known as tospace and one is known as 
 * fromspace. There is 'a' root (having more roots is a trivial 
 * extension). There is a scan pointer and a next pointer. scan and 
 * next start at the beginning of tospace.
 *
 * A subroutine copies the root object from fromspace to where next 
 * points (which is as stated initially the beginning of tospace) so 
 * now the region from scan to next is a queue of live objects to consider.
 *
 * Looking at the current object pointed to by scan, copy each child to
 * where next points, advancing next after copying each child. Then 
 * advance scan past this object.
 *
 * Copying of children is omitted when:
 * a) the object has no children
 * b) the object once had children but has been given a forwarding 
 *   pointer.
 * When do forwarding pointers arise? Any time an object is copied its 
 * old copy is 'turned into' a forwarding pointer which points to its 
 * updated contents at its new location.
 *
 * When an object is being processed as the referent of scan, and its 
 * children are considered, if a child already has been copied it will 
 * be a forwarding pointer and the parent should be updated with the 
 * address the forwarding pointer has.
 */
/*
 *Version one: The list consists of cons pairs
 * and the atoms are doubles.
 * Using nan-boxing a cons cell consists of a pair
 * (car,cdr) and those are of the same type. That
 * type is either a double or a pointer to a cons cell.
 * If the element isnan() then it is really a pointer.
 * An element can be a header for a string.
 * An element can also be a forwarding pointer.
 */
const char* typeNames[] = {"undf","LIST", "FRWD", "STRG","SHDR","ARRY","AHDR","BLTN","SYMB","NTGR"};

typedef struct cons {
	element car;
	element cdr;
} cons_cell;
typedef struct strhdr {
	element linksize;
	element data[1];
} strhdr;

element NIL;
static const int kSpaceSize = 65536;
element space[2*kSpaceSize];
element* alloc = space;
element* tospace = space+kSpaceSize;
element* endspace;

void ShowString(std::ostream &out, const element &str);
void ShowList(std::ostream &out, const element &cons);
void ShowArray(std::ostream &out, const element &array);
//std::ostream& operator<<(element& elt, std::ostream& out)
std::ostream& operator<<(std::ostream& out, const element& elt)
{
	if (BoxIsDouble(elt))
		out << elt.num;// << '<' << std::hex << elt.type << elt.tptr << std::dec << '>';
	else if (BoxIsString(elt) || BoxIsSymbol(elt))
		ShowString(out, elt);
	else if (BoxIsArray(elt))
		ShowArray(out, elt);
	else if (BoxIsList(elt))
		ShowList(out, elt);
	else if (BoxIsInteger(elt))
		out << IntFromBox(elt);
	else
		out << std::hex << typeNames[elt.type-0xFFFF0000] << elt.tptr << std::dec;
	return out;
}

element car(element cons)
{
	return cons.tptr[0];
}
element cdr(element cons)
{
	return cons.tptr[1];
}
void BuyRAM()
{
	LOG(__FUNCTION__," Too many cells requested:",__FILE__,':',__LINE__);
}

void NeedBump(int cells)
{
	if (alloc+cells > endspace)
	{
		if (cells < kSpaceSize)
		{
			LOG(__FUNCTION__," cells requested ",cells," but GC needed:",__FILE__,':',__LINE__);
			// Fix it so that the heap is collected
			// and alloc can provide the necessary cells.
			gc_collect();
			if (alloc+cells > endspace)
			{
				throw "NeedBump has not recovered enough space";
			}
		}
		else
		{
			BuyRAM();
            LOG(__FUNCTION__," cells requested ",cells," but GC needed:",__FILE__,':',__LINE__);
			// Fix it so that the heap is collected
			// and alloc can provide the necessary cells.
			gc_collect();
		}
	}
}

element newstr()
{
	NeedBump(1);
	LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
	element r;
	r.type = BOXSTR_STRING;
	r.tptr = alloc;
	alloc->type = BOXSTR_STRHDR;
	alloc->tptr = (element*)0;
	++alloc;
	return r;
}
/*
 * Only a few cases where this makes sense.
 * Such as appending a character.
 * If copying GC is in effect, NeedBump shall have been
 * called to assure that the space is there.
 */
element* bigger_mem(element* ptr, int oldsize, int newsize)
{
	if (ptr == alloc-oldsize)
	{
		// silly optimization for pointer that was
		// just created.
		alloc = alloc-oldsize+newsize;
		return ptr;
	}
	element* ret = alloc;
	std::copy(ptr, ptr+oldsize, ret);
	alloc += newsize;
	return ret;
}

uint16_t* UCharsFromString(element str)
{
	element* strdata = str.tptr;
	element* espace = strdata+1; // first element is the length;
	return (uint16_t*)espace;
}
int SizeOfString(element str)
{
	element* strdata = str.tptr;
	return IntFromBox(strdata[0]);	
}
inline int CellsForChars(int length)
{
	return (length+3)/4;
}

element newstr(const char* asciz)
{
	int chars = strlen(asciz);
	int cells = CellsForChars(chars);
	NeedBump(cells+1);
	LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
	element r;
	r.type = BOXSTR_STRING;
	r.tptr = alloc;
	alloc[0] = ElementFromInt(BOXSTR_STRHDR, chars);
	uint16_t* pchars = (uint16_t*)(alloc+1);
	for (int i=0; i<chars; ++i)
	{
		pchars[i] = asciz[i];
	}
	alloc += cells+1;
	return r;
}

element string_append_char(element str, element ch)
{
	element* data = str.tptr;
	int old_len_chars = IntFromBox(data[0]);
	int old_len_elements = CellsForChars(old_len_chars); // 2 bytes per ch = 4 bytes per element.
	int new_len_chars = old_len_chars+1;
	int new_len_elements = CellsForChars(new_len_chars);
	// If the string needs the same number of cells it is a special case.
	if (new_len_elements != old_len_elements)
	{
		gc_add_root(&str);
		// Check to see if string can be extended in place or has to move.
		if (data == alloc-old_len_elements)
		{
			// The string is at the end of the heap.
			if (alloc-old_len_elements+new_len_elements < endspace)
			{
				// There is room to extend the string in place.
				gc_unroot(&str);
				data = str.tptr;
				data = bigger_mem(data, old_len_elements+1, new_len_elements+1);
				data[0] = ElementFromInt(BOXSTR_STRHDR, new_len_chars);
				uint16_t* str_ptr = (uint16_t*)(data+1);
				str_ptr[old_len_chars] = (uint16_t)IntFromBox(ch);
				alloc++;
				// str.tptr is still right
				// str.type is still right
				return str;
			}
			// The string cannot be extended in place
		}
		// String has to move. Check for room for new string.
		NeedBump(new_len_elements+1);
		LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
		gc_unroot(&str);
		// Move the existing elements and header
		memcpy(alloc, str.tptr, sizeof(element)*(old_len_elements+1));
		alloc[0] = ElementFromInt(BOXSTR_STRHDR, new_len_chars);
		uint16_t* str_ptr = (uint16_t*)(alloc+1);
		str_ptr[old_len_chars] = (uint16_t)IntFromBox(ch);
		str.tptr = alloc;
		// str.type is still correct.
		alloc += new_len_elements+1;
		return str;
	}
	data[0] = ElementFromInt(BOXSTR_STRHDR, new_len_chars);
	uint16_t* str_ptr = (uint16_t*)(data+1);
	str_ptr[old_len_chars] = (uint16_t)IntFromBox(ch);
	str.tptr = data;
	return str;
}

element string_append_string(element stra, element strb)
{
	int a_len_chars = IntFromBox(stra.tptr[0]);
	int b_len_chars = IntFromBox(strb.tptr[0]);
	int new_len_chars = a_len_chars+b_len_chars;
	int new_len_elements = CellsForChars(new_len_chars); // 2 bytes per ch = 4 bytes per element.
	gc_add_root(&stra);
	gc_add_root(&strb);
	NeedBump(1+new_len_elements);
	LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
	gc_unroot(&stra);
	gc_unroot(&strb);
	element* adata = stra.tptr; // Don't do this before bump check
	element* bdata = strb.tptr; // May have shifted
	uint16_t* achars = (uint16_t*)(adata+1);
	uint16_t* bchars = (uint16_t*)(bdata+1);
	element* newpayload = alloc;
	alloc[0] = ElementFromInt(BOXSTR_STRHDR, new_len_chars);
	uint16_t* newdata = (uint16_t*)(alloc+1);
	std::copy(achars, achars+a_len_chars, newdata);
	std::copy(bchars, bchars+b_len_chars, newdata+a_len_chars);
	alloc += 1+new_len_elements;
	element result; result.type = BOXSTR_STRING; result.tptr = newpayload;
	return result;
}

element string_get_char(element str, element index)
{
	uint16_t* data = UCharsFromString(str);
	int i = IntFromBox(index);
	int ch = i < SizeOfString(str) ? data[i]:0;
	return BoxFromInt(ch);	
}
element string_get_substr(element str, element first)
{
	int srcSize = SizeOfString(str);
	int start = IntFromBox(first);
	gc_add_root(&str); // An example of having a root to know where it moved
						// not just to keep it from being erased.
	NeedBump(1+CellsForChars(srcSize-start));
	LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
	gc_unroot(&str);
	uint16_t* data = UCharsFromString(str);
	element* newpayload = alloc;
	alloc[0] = ElementFromInt(BOXSTR_STRHDR, srcSize-start);
	uint16_t* newdata = (uint16_t*)(alloc+1);
	std::copy(data+start, data+srcSize, newdata);
	alloc += 1+CellsForChars(srcSize-start);
	element result; result.type = BOXSTR_STRING; result.tptr = newpayload;
	return result;
}
element string_get_substr(element str, element first, element count)
{
	int srcSize = SizeOfString(str);
	int start = IntFromBox(first);
	gc_add_root(&str);
	NeedBump(1+CellsForChars(IntFromBox(count)));
	LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
	gc_unroot(&str);
	uint16_t* data = UCharsFromString(str);
	element* newpayload = alloc;
	alloc[0] = ElementFromInt(BOXSTR_STRHDR, IntFromBox(count));
	uint16_t* newdata = (uint16_t*)(alloc+1);
	std::copy(data+start, data+start+IntFromBox(count), newdata);
	alloc += 1+CellsForChars(IntFromBox(count));
	element result; result.type = BOXSTR_STRING; result.tptr = newpayload;
	return result;
}
element string_get_size(element str)
{
	return BoxFromInt(SizeOfString(str));
}
element array_create()
{
	NeedBump(1);
	LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
	element r;
	r.type = BOXSTR_ARRAY;
	r.tptr = alloc;
	alloc[0] = ElementFromInt(BOXSTR_ARRHDR, 0);
	++alloc;
	return r;
}
element array_create(element len)
{
	NeedBump(IntFromBox(len)+1);
	LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
	element r;
	r.type = BOXSTR_ARRAY;
	r.tptr = alloc;
	alloc[0] = ElementFromInt(BOXSTR_ARRHDR, IntFromBox(len));
	alloc += IntFromBox(len)+1;
	return r;
}

element array_append_element(element array, element elt)
{
	element* data = array.tptr;
	int old_len_elements = IntFromBox(data[0]);
	int new_len_elements = old_len_elements+1;
	gc_add_root(&array);
	gc_add_root(&elt);
	//NeedBump(1);
	// If the array can be appended to in place, append to it.
	// Else move it. NeedBump(1) followed by bigger_mem has the
	// flaw that if it can't be appended in place then a bump
	// for a whole new array is required.
	if (data == alloc-old_len_elements-1)
	{
		// The array is at the end of the heap.
		if (alloc-old_len_elements-1+new_len_elements+1 < endspace)
		{
			// There is room to extend the array.
			gc_unroot(&array);
			gc_unroot(&elt);
			data = array.tptr;
			data = bigger_mem(data, old_len_elements+1, new_len_elements+1);
			data[0] = ElementFromInt(BOXSTR_ARRHDR, new_len_elements);
			element* arr_ptr = data+1;
			arr_ptr[old_len_elements] = elt;
			array.tptr = data;
			return array;
		}
	}
	NeedBump(new_len_elements+1);
	LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
	gc_unroot(&array);
	gc_unroot(&elt);
	//data = array.tptr;
	//data = bigger_mem(data, old_len_elements+1, new_len_elements+1);
	//data[0] = ElementFromInt(BOXSTR_ARRHDR, new_len_elements);
	//element* arr_ptr = data+1;
	//arr_ptr[old_len_elements] = elt;
	//array.tptr = data;
	// The array has to be copied and then extended.
	element* src = array.tptr;
	memcpy(alloc, src, (1+old_len_elements)*sizeof(element));
	alloc[old_len_elements+1] = elt;
	array.tptr = alloc;
	array.tptr[0] = ElementFromInt(BOXSTR_ARRHDR, new_len_elements);
	alloc += 1+new_len_elements;
	return array;
}

element array_get_element(element array, element index)
{
	element* data = array.tptr;
	int i = IntFromBox(index);
	element r = (i < IntFromBox(data[0])) ? data[i+1]:NIL;
	return r;
}
element array_set_element(element array, element index, element elt)
{
	element* data = array.tptr;
	int i = IntFromBox(index);
	int size = IntFromBox(data[0]);
	if (i < size)
	{
		data[i+1] = elt;
	}
	else
	{
		// Out of bounds and we wish to allow it.
		gc_add_root(&array);
		// not the kind of element that moves: gc_add_root(&index);
		gc_add_root(&elt);
		NeedBump(i+1-size);
		LOG(__FUNCTION__," after NeedBump alloc is ",alloc,':',__FILE__,':',__LINE__);
		gc_unroot(&array);
		gc_unroot(&elt);
		data = array.tptr;
		data = bigger_mem(data, size+1, i+1);
		data[i+1] = elt;
	}
	return array;
}
element array_get_size(element array)
{
	return BoxFromInt(IntFromBox(array.tptr[0]));
}

element array_set_size(element array, element size)
{
	element* data = array.tptr;
	int current_size = IntFromBox(data[0]);
	int new_size = IntFromBox(size);
	if (new_size <= current_size)
	{
		data[0] = ElementFromInt(BOXSTR_ARRHDR, new_size);
		// Downsizing seems to be slightly off.
		LOG(__FUNCTION__,' ',"old:",current_size," new:",new_size,':',__FILE__,':',__LINE__);
	}
	else
	{
		gc_add_root(&array);
		NeedBump(new_size-current_size);
		LOG(__FUNCTION__ ," after NeedBump alloc is ",alloc ,':',__FILE__,':',__LINE__);
		gc_unroot(&array);
		data = array.tptr;
		data = bigger_mem(data, current_size+1, new_size+1);
		data[0] = ElementFromInt(BOXSTR_ARRHDR, new_size);
	}
	return array;
}

bool equal_data(const element& a, const element& b)
{
	if (!isnan(a.num))
		return a.num==b.num;
	if (a.type != b.type)
	{
		// It is a question whether this widely-used equality test should
		// compare strings and symbols as if they are interchangeable.
		// Ordinarily if you want to use strings like symbols you could
		// just intern them.
		// For now we will do so.
		if (a.type != BOXSTR_SYMBOL && b.type != BOXSTR_SYMBOL)
			return false; // if neither type is symbol and the types are different they don't match.
		// One of them is a symbol (because the above didn't stop us)
		// One of them is not (because they differ).
		if (a.type == BOXSTR_STRING || b.type == BOXSTR_STRING)
		{
			// If one of them is a string then it must be the symbol-string thing.
			if (SizeOfString(a)==SizeOfString(b))
				return memcmp(UCharsFromString(a), UCharsFromString(b), 2*SizeOfString(a))==0;
			return false;// Both "string-like" so compare as such.
		}
		return false; // None of the exceptions pertain, and the types are different.
	}
	// Users of equal_data might want to compare to string values.
	switch (a.type)
	{
	case BOXSTR_INT: // integer values are encoded in the pointer.
	case BOXSTR_LIST: // lists are different if they are in different places
		return a.tptr == b.tptr;
	case BOXSTR_SYMBOL: // symbols are interned so they should compare in this fashion.
		return a.tptr == b.tptr;
	case BOXSTR_STRING:
		if (SizeOfString(a)==SizeOfString(b))
			return memcmp(UCharsFromString(a), UCharsFromString(b), 2*SizeOfString(a))==0;
		return false;
	case BOXSTR_ARRAY:
		if (IntFromBox(a.tptr[0])==IntFromBox(b.tptr[0]))
		{
			for (int i=0; i<IntFromBox(a.tptr[0]); ++i)
			{
				if (!equal_data(a.tptr[i+1], b.tptr[i+1]))
					return false;
			}
			return true;
		}
		return false;
	}
	return false;
}

void ShowString(std::ostream& out, const element& str)
{
	int s = SizeOfString(str);
	uint16_t* chs = UCharsFromString(str);
	for (int i=0; i<s && i<50; ++i)
		out << (char)chs[i];
}

void ShowArray(std::ostream& out, const element& array)
{
	int s = IntFromBox(array.tptr[0]);
	element* elts = array.tptr+1;
	for (int i=0; i<s && i<50; ++i)
		out << elts[i];
}

void InnerShowList(std::ostream& out, const element &cons)
{
	if (cons.type == BOXSTR_LIST && cons.tptr == 0)
	{
		return;
	}
	element ecar = car(cons);
	element ecdr = cdr(cons);
	out << '(' << ecar ;
	while (ecdr != NIL)
	{
		out << ' ';
		if (BoxIsList(ecdr))
		{
			ecar = car(ecdr);
			out << ecar;
			ecdr = cdr(ecdr);
		}
		else
		{
			out << ". ";
			out << ecdr;
			break;
		}
	}
	out << ')';
	return;
}
void ShowList(std::ostream& out, const element &cons)
{
	if (cons == NIL)
		out << "()";
	else
		InnerShowList(out, cons);
}
/*
forward(p) = IF p points into from-space
				THEN IF mem[p] points into to-space
						THEN RETURN mem[p]
						ELSE mem[next] := mem[p]
							mem[next+1] := mem[p+1]
							mem[p] := next
							next := next+2
							RETURN mem[p]
				ELSE return p


scan := next := beginning of to-space
FOR each root p
	p := forward(p)
WHILE scan < next
	mem[scan] := forward(mem[scan])
	scan := scan+1

*/
void ShowElement(std::ostream& os, element e)
{
	os << e.num;  
	if (isnan(e.num)) 
	{ 
		os << ' ' << std::hex << typeNames[e.type-0xFFFF0000]<< std::dec << ' '<<e.tptr; 
	}
}
std::string ElementDesc(element e)
{
	std::ostringstream os; ShowElement(os, e); return os.str();
}

std::set<element*> roots;

element* scan;
element* next;
element* fromspace = space;
void gc_add_root(element* root)
{
	element di = (*root);
	if (!BoxIsDouble(di) && !BoxIsBuiltin(di) && !BoxIsInteger(di) && di != NIL)
	{
		if (di.tptr >= fromspace && di.tptr < fromspace+kSpaceSize)
			roots.insert(root);
		else
			std::cout << "Root "<< root << '['<< ElementDesc(di) <<"] not in from space ("<<fromspace<<':'<<fromspace+kSpaceSize<<")" << ':' << __FILE__ << ':' << __LINE__ <<endl;
	}
}
void gc_unroot(element* root)
{
	roots.erase(root);
}

element appel_forward(element p)
{
	if (p.tptr >= fromspace && p.tptr < fromspace+kSpaceSize)
	{
		if (p.tptr->tptr >= tospace && p.tptr->tptr < tospace+kSpaceSize)
			return p.tptr[0];
		else if (p.type == BOXSTR_LIST)
		{
			next[0] = p.tptr[0];
			next[1] = p.tptr[1];
			element r;
			p.tptr[0].tptr = next;
			//p.tptr[0].type = BOXSTR_FORWARD;
			p.tptr[0].type = p.type;
			r.tptr = next;
			r.type = BOXSTR_LIST;
			next += 2;
			return r;
		}
		else if (p.type == BOXSTR_STRING || p.type == BOXSTR_SYMBOL)
		{
			Type type = p.type;
			next[0] = p.tptr[0];
			int chars = IntFromBox(next[0]);
			int cells = CellsForChars(chars);
			if (cells != 0)
				memcpy(next+1, p.tptr+1, cells*sizeof(element));
			element r;
			//p.tptr = next;
			//p.tptr[0].tptr = next;
			//p.tptr[0].type = BOXSTR_FORWARD;
			p.tptr[0].tptr = next;
			p.tptr[0].type = p.type;
			r.tptr = next;
			r.type = type;
			next += 1 + cells;
			return r;
		}
		else if (p.type == BOXSTR_ARRAY)
		{
			next[0] = p.tptr[0];
			int cells = IntFromBox(next[0]);
			if (cells != 0)
				memcpy(next+1, p.tptr+1, cells*sizeof(element));
			LOG(__FUNCTION__," Moved ",cells,"-element array from ",p.tptr,'-',(p.tptr+1+cells)," to ",next,'-',next+1+cells,' ',':',__FILE__,':',__LINE__);			
			next[1+cells].num = 3.14159;
			element r;
			p.tptr[0].tptr = next;
			p.tptr[0].type = p.type;
			r.tptr = next;
			r.type = BOXSTR_ARRAY;
			next += 1 + cells;
			return r;
		}
		else
			*next++ = p.tptr[0];
	}
	else
		return p;
}
void appel_collect()
{
	scan = tospace;
	next = tospace;
	LOG(__FUNCTION__,' ',"at start: from ",fromspace,'-',fromspace+kSpaceSize-1," scan ",scan," next ",next,':',__FILE__,':',__LINE__);
	int kRoots = 0;
	for (std::set<element*>::const_iterator i=roots.begin(); i!=roots.end(); ++i)
	{
		//if (!BoxIsDouble(**i))
		{
			element di = (**i);
			if (BoxIsDouble(di))
			{
				cout << "F root contains float: var address: " << *i
					 << " var's contents: " << ElementDesc(di) 
					 << std::endl;
				continue;
			}
			if (di == NIL || (!BoxIsForward(di) && !BoxIsForward(di.tptr[0])))
			{
#if 0
			std::cout << *i;// << std::endl;
			element* pdi = *i;
			std::cout << " before "; ShowElement(std::cout, di);
#endif
			// The iterator yields addresses of elements that need to be 
			// retained. To retain them we need to move them to the tospace.
			// To keep from moving the same one a million times we need to
			// mark the old location of the element.
			// The address itself is some variable outside the heap that
			// needs to be updated to the new location. When the element is
			// first moved, it's easy. When the second pointer comes along,
			// the old location will have a forwarding pointer in it.
				
			**i = appel_forward(*(*i));
#if 0
			di = (**i);
			std::cout << ' ' << *i << " after "; ShowElement(std::cout, *pdi);
			std::cout << ':' << __FILE__ << ':' << __LINE__ <<endl;
#endif
			}
			else
			{
				// The variable has been forwarded.
				cout << "F " << *i 
					 << " var's contents " << ElementDesc(di) 
					 << " where var points " << di.tptr 
					 << " what is there: " << ElementDesc(di.tptr[0])
						<< " where that cell points " << (di.tptr[0].tptr)
						<< " what is there " << ElementDesc(di.tptr[0].tptr[0])
					 << std::endl;
				
			}
		}
		++kRoots;
	}
	LOG(__FUNCTION__,' ',kRoots," roots moved",':',__FILE__,':',__LINE__);
	LOG(__FUNCTION__ ,' ',"scan ",scan," next ",next,':',__FILE__,':',__LINE__);
#if 1
	for (element* look = scan; look != next; ++look)
	{
		LOG(__FUNCTION__ ,' ',"item@",look,": [",ElementDesc(*look),'=',*look,"]",':',__FILE__,':',__LINE__);
	}
#endif
	while (scan < next)
	{
		element oldscan = scan[0];
		*scan = appel_forward(*scan);
		LOG(__FUNCTION__,' ',"q oldscan ",scan,'=',ElementDesc(oldscan)," newscan ",ElementDesc(*scan),' ',':',__FILE__,':',__LINE__);
		if (oldscan.type == BOXSTR_LIST)
			++ scan;
		else if (oldscan.type == BOXSTR_STRING || oldscan.type == BOXSTR_SYMBOL)
			++ scan;
		else if (oldscan.type == BOXSTR_STRHDR)
		{
			scan += 1 + CellsForChars(IntFromBox(oldscan));
		}
		/*else if (oldscan.type == BOXSTR_ARRHDR)
		{
			scan += 1 + IntFromBox(oldscan);
		} Don't skip the guts of an array, the elements are boxed */
		else 
			++ scan;
	}
	alloc = next;	
	extern element table;
	int sz = IntFromBox(array_get_size(table));
	for (int s=0; s<sz; ++s)
	{
		element e = array_get_element(table, BoxFromInt(s));
		LOG(__FUNCTION__," TABLE ELEMENT ",s," is ",e," :",__FILE__,':',__LINE__);
	}
}

// \brief Make from a forwarding pointer that points where to points
//
void ForwardFrom(element* from, element* to)
{
	from->tptr = to;
	from->type = BOXSTR_FORWARD;
}

/* \brief cons
 * Put the car and cdr in the heap.
 * Make a typed pointer that points at them.
 * Bump the alloc pointer.
 */
element cons(element car, element cdr)
{
	gc_add_root(&car);
	gc_add_root(&cdr);
	NeedBump(2);
	gc_unroot(&car);
	gc_unroot(&cdr);
	cons_cell& cell = *(cons_cell*)alloc;
	cell.car = car;
	cell.cdr = cdr;
	element r;
	r.tptr = alloc;
	r.type = BOXSTR_LIST;
	alloc += 2;
	return r;
}

void gc_collect()
{
	appel_collect(); // alloc now refers to what's left of tospace.
	element* s = tospace;
	tospace = fromspace;
	fromspace = s;	
	endspace = fromspace + kSpaceSize;
	LOG("collect completed: fromspace ",fromspace," alloc ",alloc," endspace ",endspace," free cells: ",endspace - alloc,':',__FILE__,':',__LINE__);
	if (alloc==endspace)
	{
		LOG("No free space in ",kSpaceSize*sizeof(element)," bytes");
		throw "No space left";
	}
	memset(tospace, 0, sizeof(element)*kSpaceSize);
}

element symbol_create(const char* asciz)
{
	element t = newstr(asciz);
	return symbol_from_string(t);
}
element symbols;
element symbol_from_string(element t)
{
	element initial = string_get_char(t, BoxFromInt(0));
	int initialChar = IntFromBox(initial);
	int iBucket = 0;
	if (initialChar >= 'A' && initialChar <= 'Z')
		iBucket = initialChar - 'A';
	else if (initialChar >= 'a' && initialChar <= 'z')
		iBucket = initialChar - 'a';
	else
		iBucket = initialChar % 26;
	element bucket = array_get_element(symbols, BoxFromInt(iBucket));
	element search = bucket;
	while (search != NIL)
	{
		element s = car(search);
		if (s == t)
			return s;
		search = cdr(search);
	}
	Rooter t_r(t);
	Rooter bucket_r(bucket);
	element sym;
	sym.type = BOXSTR_SYMBOL;
	sym.tptr = t.tptr;
	Rooter sym_r(sym);
	bucket = cons(sym, bucket);
	array_set_element(symbols, BoxFromInt(iBucket), bucket);
	return sym;
}

void init_heap()
{
	memset(space, 0, sizeof space);
	endspace = space+kSpaceSize;
	tospace = space+kSpaceSize;
	fromspace = space;
	alloc = space;
	NIL.type = BOXSTR_LIST; 
	NIL.tptr = 0;
	symbols = array_create(BoxFromInt(26));
	for (int i=0; i<26; ++i)
		array_set_element(symbols, BoxFromInt(i), NIL);
	gc_add_root(&symbols);
}
void dump_heap()
{
	std::cout << __FUNCTION__ << ' ' << std::hex << "fromspace " << space << ':' << __FILE__ << ':' << __LINE__ <<endl;
	std::cout << __FUNCTION__ << ' ' << std::hex << "alloc " << alloc << ':' << __FILE__ << ':' << __LINE__ <<endl;
	for (int t=0; space+t < alloc; ++t)
		std::cout << __FUNCTION__ << ' ' << "cell " << t << ' ' << space+t << ':' << space[t] << ':' << __FILE__ << ':' << __LINE__ <<endl;
	
}
void dump_new_heap()
{
	std::cout << __FUNCTION__ << ' ' << "alloc " << alloc << ", alloc[-1]=" << alloc[-1] << ' ' << ':' << __FILE__ << ':' << __LINE__ <<endl;
	//for (int t=0; t<5; ++t)
	//	std::cout << __FUNCTION__ << ' ' << "fromspace cell " << t << ' ' << space+t << ": " << space[t] << ':' << __FILE__ << ':' << __LINE__ <<endl;
	for (int t=0; /*t<10;*/tospace+t < alloc; ++t)
		std::cout << __FUNCTION__ << ' ' << "tospace cell " << t << ' ' << tospace+t << ':' << tospace[t] << ':' << __FILE__ << ':' << __LINE__ <<endl;
}
