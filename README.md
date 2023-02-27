# bright
Change screen brightness and print to stdout.

## Usage
bright [ -+hp ]
- : Decrease screen brightness, and then print to stdout.
+ : Increase screen brightness, and then print to stdout.
h : Print usage
p : Print current brightness to stdout
Without arguments, also print current brightness.

## Configuration
Edit `bright.c` and recompile.

## Installation
```
$ git clone https://github.com/lucas-mior/bright
$ cd bright
$ make
$ sudo make install
```
