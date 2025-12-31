Cheap and dirty, sloppy, inefficient implementation of search scripts.
Have to edit the .py files directly to point to directory holding memory dumps which are to be searched.
Have to edit the .py files directly to specify number of data size in bytes, and to specify whether increasing/decreasing search.

Python dependencies were kept at minimum, on purpose, so no need to install/unstand libraries such as numpy.

Scripts require memory be dumped when variable to search for has different values, and that script be prepended with <absolute number | relative number><underscore> to tell which order to search the dumps in.  find_counter_address.py looks for the addresses with the exact prefixed values.  find_decreasing_address looks for decreasing or increasing values based on relative prefixed counter values.

If someone showed an interest in running these scripts, i might write a better explanation of how to run the search scripts.  However, i suspect noone else will ever run these search scripts.
