# bright
Change screen brightness, print to stdout and signal another program.

## Usage
```
usage: bright [+-hp] <s>
+ : decrease brightness
- : increase brightness
h : show this help message
p : print current brightness
if <s> is set, send $BRIGHT signal to <s>.
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
