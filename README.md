# UTF-8 JSON String Unescaper Written in C

I had some issues with the
[Kubernetes filter in fluent-bit](https://github.com/fluent/fluent-bit/tree/master/plugins/filter_kubernetes).
And I believed that its `unescape_string` function might be the problem.

Now I wanted to fix it but really don't have any experience in programming C ...

This is my approach for unescaping a
[JSON string](http://www.ietf.org/rfc/rfc4627.txt).


## State Machine
Every transition between two states reads another char from the input
    buffer.  
![state machine](state-machine.png)

## TODO
1. Move it out of the scratch incubator
2. Make a library of it (.h file) and put the main method somewhere else
3. Write test cases
4. Use it together with [JSMN](https://github.com/zserge/jsmn) and write
   something cool.

## DONE
1. Support Unicode on top of the
[Basic multilingual plane](https://en.wikipedia.org/wiki/Plane_%28Unicode%29#Basic_Multilingual_Plane)
2. Escape UTF-8 with bit shifting. Devide and modulo works but the former is easier to read and probably faster. See https://github.com/akheron/jansson/blob/master/src/utf.c
3. Replace camelCase with under_scores in variable names.
4. More code comments
5. Format the code according to how they do it at fluent-bit
