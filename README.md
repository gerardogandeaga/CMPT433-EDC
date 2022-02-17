# CMPT433-EDC

Starting Out
------------
This project needs to the c++ cross compiler. You can install it on the host with:

`$ sudo apt-get install g++-arm-linux-gnueabihf`

Building
--------
For code organization, we can use the `include/` directory for your `.h` files and `src/` for your `.cpp` files. The Makefile will automatically detect and link files between those 2 folders (Hopefully no need to edit the make for every new file). 

Running "`$ Make`" will create a `bin/` & `obj/` folder autmatically. The executable program will move to the `bin/` folder and copy it to `$(HOME)/cmpt433/public/myApps/` so you can run in on your BBG.
