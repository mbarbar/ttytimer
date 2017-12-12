# ttytimer

ttytimer is a fork of tty-clock by xorg62 which modifies the
original project to be a timer, as the name suggests.

## Usage

```
usage : %s [-xbvih] [-C color] hh:mm:ss
        -x            Show box
        -C color      Set the clock color
           color  ==  black | red | green
                      | yellow | blue | magenta
                      | cyan | white
        -b            Use bold colors
        -v            Show ttytimer version
        -h            Show this page
```

### At runtime
```
[qQ] : quit
```

## Installation

### Dependencies

* `make`
* ncurses 5
* C compiler
* `toot` (optional - for alarm on 00:00:00)

### Instructions

```
make install
```

To install without `toot`,

```
make install TOOT=no
```

## TODO
1. Option to count up rather than just down.
1. Man page - including detailed description of time format.

