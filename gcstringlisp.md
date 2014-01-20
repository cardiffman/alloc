GCSTRINGLISP
============

A few words about the string-based lisp interpreter.

There are some notes at the top of gcstringlisp.cpp about how it came to be. I would just like to define its operation,
suggest that it will help debug and gain experience with using garbage collection in a runtime, and show my enthusiasm
for it as a hack.

String-based LISP is more than just a LISP where the only kind of atom is text. It represents the s-expression as the
parenthesized serialization of the s-expression. To an extent, this means that CAR and CDR are parsing operations.

I like the string-based LISP as a GC test case. With a heap semi-space of 1024 64-bit values and 16 bits per character 
in strings it really exercises the collector and the intepreter's use of it. Evaluating (fact 12) causes about three 
collections with those parameters.

I have always liked string-based LISP as a hack. It appeals to me that the s-expression is represented by its string
representation, and it is striking how easily the CAR and CDR of such an s-expression can be computed. To compute the
CAR of a string-based s-expression:
1. Step inside the outer pair of parentheses.
2. Scan over to the first non-blank entity.
3. Record this character index as `start`.
4. If the character is not a '(' continue at step 8
5. If that entity starts with '(' scan over to the balancing ')'
6. Record this character index as `end`.
7. Continue at step 10
8. Scan over to the last non-blank character before a blank or ')'.
9. Record this character index as `end`.
10. The string from `start` to `end`, inclusive, is your CAR.
11. Done.
In a similar fashion it is easy to compute the CDR.
1. Step inside the outer pair of parentheses.
2. Scan over to the first non-blank entity.
3. If the character is not a '(' continue at step 6
4. Scan over to the balancing ')'
5. Continue at step 7
6. Scan over to the last non-blank character before a blank or ')'.
7. Skip over any blank characters.
8. If the scan does not reach a ')' then continue at step 11
9. A NIL CDR should be returned.
10. Continue at step 16
11. Record this character index as `start`
12. Record the position of the original terminating ')' as `end`.
13. Create a string using the substring from `start` to `end`.
14. Insert a '(' in front of the resulting string.
15. This is your CDR.
16. Done.

The methods in gcstringlisp for those two calculations share a routine that finds the position of the end of the CAR,
which is effectively steps 2-6 of the CAR calculation or steps 2-6 of the CDR calculation.
