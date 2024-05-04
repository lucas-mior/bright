# bright
Change screen brightness, print to stdout and signal another program.

## Usage
```
$ bright --help
usage: bright COMMAND [program_to_signal] 
Available commands: 
-m | --more   : more brightness 
-l | --less   : less brightness 
-h | --help   : print this help message 
-p | --print  : print current brightness 
```

## Configuration
Edit `bright.c` and recompile.

## Environment variables
- `$BRIGHT` - Which signal number should be send to [program_to_signal]

## Installation
```
$ git clone https://github.com/lucas-mior/bright
$ cd bright
$ ./build.sh
$ sudo ./build.sh install
```
