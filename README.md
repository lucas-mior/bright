# bright
Change screen brightness and print to stdout.

## Usage
```
$ bright h
usage: bright [+-hp] <s> 
+ : decrease brightness 
- : increase brightness 
h : show this help message 
p : print current brightness 
if <s> is set, signal dwmblocks. 
```

## Configuration
Edit `bright.c` and recompile.

## Installation
```
$ git clone https://github.com/lucas-mior/bright
$ cd bright
$ make
$ sudo make install
```
