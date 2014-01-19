#include "alloc.h"
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <cstdlib>
#include <set>

using std::cout;
using std::endl;
using std::set;

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
const char* typeNames[] = {"undf","LIST", "FRWD", "STRG","NTGR"};

typedef struct cons {
	element car;
	element cdr;
} cons_cell;
typedef struct strhdr {
	element linksize;
	element data[1];
} strhdr;

element NIL;
static const int kSpaceSize = 1024;
element space[2*kSpaceSize];
element* alloc = space;
element* tospace = space+kSpaceSize;
element* endspace;

void ShowString(std::ostream &out, const element &str);
void ShowList(std::ostream &out, const element &cons);
//std::ostream& operator<<(element& elt, std::ostream& out)
std::ostream& operator<<(std::ostream& out, const element& elt)
{
	if (BoxIsDouble(elt))
		out << elt.num;// << '<' << std::hex << elt.type << elt.tptr << std::dec << '>';
	else if (BoxIsString(elt))
		ShowString(out, elt);
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
	std::cout << __FUNCTION__ << " Too many cells requested" << std::endl;
}

void NeedBump(int cells)
{
	if (alloc+cells > endspace)
	{
		if (cells < kSpaceSize)
		{
			std::cout << __FUNCTION__ << " cells requested " << cells <<" but GC needed" << std::endl;
			// Fix it so that the heap is collected
			// and alloc can provide the necessary cells.
			gc_collect();
		}
		else
		{
			BuyRAM();
			std::cout << __FUNCTION__ << " cells requested " << cells <<" but GC needed" << std::endl;
			// Fix it so that the heap is collected
			// and alloc can provide the necessary cells.
			gc_collect();
		}
	}
}

element newstr()
{
	NeedBump(1);
	cout << __FUNCTION__ << " NeedBump returns"<<':'<<__FILE__<<':'<<__LINE__<<endl;
	element r;
	r.type = BOXSTR_STRINGTYPE;
	r.tptr = alloc;
	alloc->type = BOXSTR_INTTYPE;
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
	return (length+3/4);
}

element newstr(const char* asciz)
{
	int chars = strlen(asciz);
	int cells = CellsForChars(chars);
	cout << __FUNCTION__ << " asciz " << asciz << ':'<<__FILE__<<':'<<__LINE__<<endl;
	NeedBump(cells+1);
	cout << __FUNCTION__ << " NeedBump returns"<<':'<<__FILE__<<':'<<__LINE__<<endl;
	element r;
	r.type = BOXSTR_STRINGTYPE;
	r.tptr = alloc;
	alloc[0] = BoxFromInt(chars);
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
	if (new_len_elements != old_len_elements)
	{
		gc_add_root(&str);
		NeedBump(new_len_elements-old_len_elements);
		gc_unroot(&str);
		data = str.tptr;
		data = bigger_mem(data, old_len_elements+1, new_len_elements+1);
	}
	data[0] = BoxFromInt(new_len_chars);
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
	gc_unroot(&stra);
	gc_unroot(&strb);
	cout << __FUNCTION__ << " NeedBump returns "<<alloc<<':'<<__FILE__<<':'<<__LINE__<<endl;
	element* adata = stra.tptr; // Don't do this before bump check
	element* bdata = strb.tptr; // May have shifted
	uint16_t* achars = (uint16_t*)(adata+1);
	uint16_t* bchars = (uint16_t*)(bdata+1);
	element* newpayload = alloc;
	alloc[0] = BoxFromInt(new_len_chars);
	uint16_t* newdata = (uint16_t*)(alloc+1);
	std::copy(achars, achars+a_len_chars, newdata);
	std::copy(bchars, bchars+b_len_chars, newdata+a_len_chars);
	alloc += 1+new_len_elements;
	element result; result.type = BOXSTR_STRINGTYPE; result.tptr = newpayload;
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
	cout << __FUNCTION__ << " str " << str << '['; ShowElement(cout,str); cout << "] first " << first <<':'<<__FILE__<<':'<<__LINE__<<endl;
	gc_add_root(&str); // An example of having a root to know where it moved
						// not just to keep it from being erased.
	NeedBump(1+CellsForChars(srcSize-start));
	gc_unroot(&str);
	cout << __FUNCTION__ << " NeedBump returns "<<alloc<<':'<<__FILE__<<':'<<__LINE__<<endl;
	uint16_t* data = UCharsFromString(str);
	element* newpayload = alloc;
	alloc[0] = BoxFromInt(srcSize-start);
	uint16_t* newdata = (uint16_t*)(alloc+1);
	std::copy(data+start, data+srcSize, newdata);
	alloc += 1+srcSize-start;
	element result; result.type = BOXSTR_STRINGTYPE; result.tptr = newpayload;
	return result;
}
element string_get_substr(element str, element first, element count)
{
	int srcSize = SizeOfString(str);
	int start = IntFromBox(first);
	cout << __FUNCTION__ << " str " << str << '['; ShowElement(cout,str); cout << "] first " << first << " count " << count<<':'<<__FILE__<<':'<<__LINE__<<endl;
	gc_add_root(&str);
	NeedBump(1+CellsForChars(IntFromBox(count)));
	gc_unroot(&str);
	cout << __FUNCTION__ << " NeedBump returns "<<alloc<<':'<<__FILE__<<':'<<__LINE__<<endl;
	uint16_t* data = UCharsFromString(str);
	element* newpayload = alloc;
	alloc[0] = count;
	uint16_t* newdata = (uint16_t*)(alloc+1);
	std::copy(data+start, data+start+IntFromBox(count), newdata);
	alloc += 1+CellsForChars(IntFromBox(count));
	element result; result.type = BOXSTR_STRINGTYPE; result.tptr = newpayload;
	return result;
}
element string_get_size(element str)
{
	return BoxFromInt(SizeOfString(str));
}

bool equal_data(const element& a, const element& b)
{
	if (!isnan(a.num))
		return a.num==b.num;
	if (a.type != b.type)
		return false;
	// Users of equal_data might want to compare to string values.
	switch (a.type)
	{
	case BOXSTR_INTTYPE: 
	case BOXSTR_LISTTYPE: 
		return a.tptr == b.tptr;
	case BOXSTR_STRINGTYPE:
		if (SizeOfString(a)==SizeOfString(b))
			return memcmp(UCharsFromString(a), UCharsFromString(b), 2*SizeOfString(a))==0;
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

void ShowCar(std::ostream& out, const element& car)
{
	switch (car.type)
	{
	case BOXSTR_INTTYPE:
		out << IntFromBox(car);
		break;
	case BOXSTR_STRINGTYPE:
		ShowString(out, car);
		break;
	default:
		if (car==NIL)
			out << "()";
		else
			out << car; // displays in technical fashion if it's not a double.
		break;
	}
}

void InnerShowList(std::ostream& out, const element &cons)
{
	if (cons.type == BOXSTR_LISTTYPE && cons.tptr == 0)
	{
		return;
	}
	element ecar = car(cons);
	element ecdr = cdr(cons);
	out << '(' << ecar ;
	while (ecdr != NIL)
	{
		out << ' ';
		ecar = car(ecdr);
		out << ecar;
		ecdr = cdr(ecdr);
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
element* scan;
element* next;
element* fromspace = space;
element appel_forward(element p)
{
	if (p.tptr >= fromspace && p.tptr < fromspace+kSpaceSize)
	{
		if (p.tptr->tptr >= tospace && p.tptr->tptr < tospace+kSpaceSize)
			return p.tptr[0];
		else if (p.type == BOXSTR_LISTTYPE)
		{
			next[0] = p.tptr[0];
			next[1] = p.tptr[1];
			p.tptr[0].tptr = next;
			//p.tptr[0].type = BOXSTR_FORWARDTYPE;
			p.tptr[0].type = BOXSTR_LISTTYPE;
			next += 2;
			return p.tptr[0];
		}
		else if (p.type == BOXSTR_STRINGTYPE)
		{
			next[0] = p.tptr[0];
			int chars = IntFromBox(next[0]);
			int cells = (chars+3)/4;
			if (cells != 0)
				memcpy(next+1, p.tptr+1, cells*sizeof(element));
			p.tptr[0].tptr = next;
			p.tptr[0].type = BOXSTR_STRINGTYPE;
			next += 1 + cells;
			return p.tptr[0];
		}
	}
	else
		return p;
}
void ShowElement(std::ostream& os, element e)
{
	os << e.num;  
	if (isnan(e.num)) 
	{ 
		os << ' ' << std::hex << typeNames[e.type-0xFFFF0000]<< std::dec << ' '<<e.tptr; 
	}
}

std::set<element*> roots;
void gc_add_root(element* root)
{
	element di = (*root);
	if (!BoxIsDouble(di))
	{
	std::cout << __FUNCTION__ << ' ';
	ShowElement(std::cout, di); 
	std::cout << std::endl;
	if (di.tptr >= fromspace && di.tptr < fromspace+kSpaceSize)
		roots.insert(root);
	else
		std::cout << "Root not in from space" << std::endl;
	}
}
void gc_unroot(element* root)
{
	element di = (*root);
	std::cout << __FUNCTION__ << ' ';
	ShowElement(std::cout, di); 
	std::cout << std::endl;
	roots.erase(root);
}

#if 0
void appel_collect(element*& root)
#else
void appel_collect()
#endif
{
	scan = tospace;
	next = tospace;
	std::cout << __FUNCTION__ << ' ' << "at start: from " << fromspace << " scan " << scan << " next " << next << std::endl;
#if 0
	*root = appel_forward(*root);
	//std::cout << __FUNCTION__ << ' ' << "root moved" << std::endl;
	//std::cout << __FUNCTION__ << ' ' << "scan " << scan << " next " << next << std::endl;
#else
	int kRoots = 0;
	for (std::set<element*>::const_iterator i=roots.begin(); i!=roots.end(); ++i)
	{
		std::cout << *i << std::endl;
		element di = (**i);
		std::cout << "before "; ShowElement(std::cout, di);
		if (!BoxIsDouble(di) && di.tptr >= fromspace && di.tptr < fromspace+kSpaceSize)
			std::cout <<' ' << di;
		std::cout << std::endl;
		**i = appel_forward(*(*i));
		di = (**i);
		std::cout << "after  "; ShowElement(std::cout, di);
		if (!BoxIsDouble(di) && di.tptr >= tospace && di.tptr < tospace+kSpaceSize)
			std::cout <<' ' << di;
		std::cout << std::endl;
		++kRoots;
	}
	std::cout << __FUNCTION__ << ' ' << kRoots << " roots moved" << std::endl;
	std::cout << __FUNCTION__ << ' ' << "scan " << scan << " next " << next << std::endl;
#endif
	for (element* look = scan; look != next; ++look)
		std::cout << __FUNCTION__ << ' ' << "item@" << look << ": ["<<*look <<"]" << std::endl;
	while (scan < next)
	{
		//for (int t=0; t<30 && tospace+t!=next; ++t)
		//	std::cout << __FUNCTION__ << ' ' << "cell " << t << ' ' << tospace+t << ':'<< tospace[t] << std::endl;
		element oldscan = scan[0];
		*scan = appel_forward(*scan);
		if (oldscan.type == BOXSTR_LISTTYPE)
			++ scan;
		else if (oldscan.type == BOXSTR_STRINGTYPE)
			++ scan;
		else if (oldscan.type == BOXSTR_INTTYPE)
		{
			std::cout << __FUNCTION__ << ' ' << "string moved" << std::endl;
			scan += 1 + (IntFromBox(oldscan)+3)/4;
		}
		else 
			++ scan;
		//std::cout << __FUNCTION__ << ' ' << "scan " << scan << " next " << next << std::endl;
	}
	alloc = next;	
}

// \brief Make from a forwarding pointer that points where to points
//
void ForwardFrom(element* from, element* to)
{
	from->tptr = to;
	from->type = BOXSTR_FORWARDTYPE;
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
	r.type = BOXSTR_LISTTYPE;
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
	std::cout << "collect completed: fromspace " << fromspace << " alloc " << alloc << " endspace " << endspace << " free cells: " << endspace - alloc << std::endl;
	memset(tospace, 0, sizeof(element)*kSpaceSize);
}

void init_heap()
{
	memset(space, 0, sizeof space);
	endspace = space+kSpaceSize;
	tospace = space+kSpaceSize;
	fromspace = space;
	alloc = space;
	NIL.type = BOXSTR_LISTTYPE; 
	NIL.tptr = 0;
}
void dump_heap()
{
	std::cout << __FUNCTION__ << ' ' << std::hex << "fromspace " << space << std::endl;
	std::cout << __FUNCTION__ << ' ' << std::hex << "alloc " << alloc << std::endl;
	for (int t=0; space+t < alloc; ++t)
		std::cout << __FUNCTION__ << ' ' << "cell " << t << ' ' << space+t << ':' << space[t] << std::endl;
	
}
void dump_new_heap()
{
	std::cout << __FUNCTION__ << ' ' << "alloc " << alloc << ", alloc[-1]=" << alloc[-1] << ' ' << std::endl;
	//for (int t=0; t<5; ++t)
	//	std::cout << __FUNCTION__ << ' ' << "fromspace cell " << t << ' ' << space+t << ": " << space[t] << std::endl;
	for (int t=0; /*t<10;*/tospace+t < alloc; ++t)
		std::cout << __FUNCTION__ << ' ' << "tospace cell " << t << ' ' << tospace+t << ':' << tospace[t] << std::endl;
}
