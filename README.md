# FunVM
An implementation of the parser, compiler and virtual machine based on [Robert Nystrom's](https://github.com/munificent) 
["Crafting Interpretes"](https://craftinginterpreters.com), [part III](https://craftinginterpreters.com/a-bytecode-virtual-machine.html).


As author claims this guide through is aimed to introduce a reader with the basic concepts of the realm
of compilers and virtual machines. Though "basic concepts" sounds lighty, one shouldn't underestimate it.
Also the author did his best: he succeed in explaining heavy-weight concepts in straightforward style.

As Mr. Nystrom notes in his book, mastering compilers is ongoing, endless process, and taking into account
this statement, the implementation in this repo goes after the following goals:
1. implementing:
   
1.1. arrays;

1.2. prefix/postfix increment/decrement;

1.3. break;

1.4. continue;

1.5. 'else if(){}' statement ('if(){}else{}' statement, as well as many other concepts, author discussed in details);

2. adding support for:
   
2.1. try-catch;

2.2. multi-threading.

3. Running on STM32F103 AKA "Blue pill" or LPC55:

3.1. TODO

3.N. TODO

3.N - 1. TODO