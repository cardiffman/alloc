LISP implementation notes
=========================

There are two LISP implementations in this code. They are small but they can do quite a bit. I expect them to be convenient test cases for the garbage collector.

gcstringlisp.cpp
----------------

The oldest is gcstringlisp.cpp. It represents a value as a string, regardless of what better choices might exist. I have been playing with versions of this for years. It started as C code that used `sprintf(sbrk(199), ...)` or `strdup` to allocate strings. Then I translated the C to C++ while changing as little as possible. Then I converted the strings to C++ `std::string`. As you see it now it uses `newstr()` and the rest of the string functions defined by alloc.h. The REPL gets strings from `main()` and treats them as s-expressions to evaluate. The reader makes sure that the parens balance.

This LISP implementation cannot fit computing (fact 12) within the 1024-element heap that alloc.cpp currently sets up.

gclisp.cpp
----------
gclisp.cpp is a little more efficient than the other. It uses doubles, integers or strings for atoms and cons pairs for lists. As a result it has no trouble computing fairly high factorial values. But if you go too high you get 'inf' values and when that happens the algorithm breaks.

Common Implementation
---------------------
The gclisp.cpp implementation is just an optimization of the other one. The reader had to be rewritten to make full use of the available data types.

The function names are currently unchanged from the ancient originals. One of my reasons for getting this code into a SCCS is to enable some function renaming and other optimizations, especially in gclisp.cpp but also in gcstringlisp.cpp.
