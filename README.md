## Two Hash Tables

Two hash tables, including a cuckoo hash. :bird:

### What is a cuckoo hash?

One of the main issues with hash tables is that even if you create a large table to store all the keys in, there's a good chance collisions between keys will occur.

What can we do to resolve this issue? Cuckoo hashing.

A cuckoo hash works in the following way :baby_chick:

If both keys in the table are occupied, you push one key away from its current location (its 'nest') to a different location that is vacant. If it's vacant, happy days. If it isn't, it pushes away the occupying key. This can continue into a cycle of key displacement. If it does, then the table is rehashed.

So, this type of hash table behaves like the world's most evil bird, the cuckoo, which when it hatches pushes all the other eggs out of the nest, despite the fact it is not the offspring of the mother.

:sunny: :hatching_chick:  This helps resolve the issue of hash collisions and finally puts cuckoos to good - ethical - use.  :hatched_chick: :sunny:
