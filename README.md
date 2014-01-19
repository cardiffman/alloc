alloc
=====

Functions and data types that support a small garbage-collected runtime system

This module is named after the incredibly imaginative name I gave the header and implementation file.

The interface is based on the element, which is a NaN-boxed dynamic value. It is currently capable of holding a double, a 32-bit signed integer, a reference to a string, or a reference to a cons cell.

Nan-boxing is described well elsewhere. It is embodied in this code as a set of 32-bit enum constants that, when they sit in place of a 64-bit double's most-significant bits, make that double into a NaN (not-a-number). When one of these constants sits in the `type` field of an `element`, the other field, called `tptr`, is either a pointer or if the type is the integer type, the 32-bit signed integer. These two fields are bundled into a struct and that struct is bundled into a union with a double called `num`. When the result of `isnan(anElement.num)` is true, then the `type` and `tptr` give the particular value of the element, otherwise the `num` field is the value.

Here is the code:
```c++
enum Type {
  BOXSTR_LISTTYPE = 0xFFFF0001
  , BOXSTR_FORWARD
  , BOXSTR_STRING
  , BOXSTR_INTEGER
};
union element {
  double num; // use this if !isnan(num)
  struct {
    Type type;
    element* tptr;
  };
  uint16_t ch[4];
};
```
Elements themselves are not automatically in the heap, unless they get referenced by a structure that is in the heap. For example a double doesn't require heap unless it is sitting in half of a cons cell. A reference to a cons cell doesn't necessarily go into the heap unless you want it to join a list.

There are functions to test an element's type and to unbox it.

There is a statically-allocated element called `NIL`. Clients can compare for equality with it if they want to know if an element is `NIL`.

The interface for list handling consists of 
```c++
element car(element list);
element car(element list);
element cons(element car, element cdr);
```

The interface for string handling consists of
```c++
element newstr(); // for an empty string
element newstr(const char* asciz); // to start with a C string
element string_append_char(element string, element character);
element string_append_string(element first, element second);
```

There is also an `operator<<` declared to print an element to a `std::ostream`. If the `element` is a list or string the referenced data is included in the output.

Garbage Collection
------------------

There is a notion, not at this writing fully implemented, of root objects. Root objects don't live in the heap but they refer into it. During a collection, one has to arrange to give the collector the root objects so that the collector can tell you where they were moved.

The implementation of a garbage collector is a slight detour for my work so I have chosen an algorithm with decent qualities but which won't necessarily be a major part of the coding going forward. That choice is a straight-up Cheney copying collector. 

The system operates from one of two spaces using a bump allocator. When the bump allocator hits the end of the space, a garbage collection is called for. At this point the root objects are moved shallowly to the start of the second space, and the root object references are set to the new locations. The shallow copies of the root objects constitute the initial state of a work queue of further objects to move. So after dealing with the root objects, the major copying is done by scanning the copied elements for further references. Each `element` referenced is copied to the end of the work queue. Each `element` so scanned is no longer on the work queue, but it is at its permanent home in the new space. When the scan hits the end of the work queue, then all that remains is paper work. The bump allocator is now set up to avail itself of the free elements in the second space, and preparations are made so that the next collection uses the old space as the new space.

The `element` can assume the role of a forwarding pointer. When an object is copied to the new space, the version left in the old space has its first element replaced with a forwarding pointer that points to the new location. That way, if two elements in the heap refer to a third, then they can be made to refer to the same copy of it in the new space.

References
----------
I made reference to various papers when I implemented the code. The code follows most closely the exhibition of Andrew W. Appel's tutorial "Garbage Collection" from 1990. I also reviewed the original Cheney paper, C. J. Cheney.
A nonrecursive list compacting algorithm. _Communications of the ACM_, 13(11):677{678, 1970.
