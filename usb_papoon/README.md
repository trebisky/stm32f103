usb_papoon

This works, but it is just a stepping stone to "usb".

What you really want is "usb"

I wanted to try to use the C++ code from papoon_usb

https://github.com/thanks4opensource/papoon_usb

In particular, the first example.  This is the first real work
I have done in 2023.  I keep the papoon C++ code in a
directory of its own and set up my makefile to deal with
that and make minimal changes to let me call it from my
C framework.

This works!!  It yields /dev/ttyACM0 that echos whatever
you type at it (I use picocom).

I'll move on to usb_papoon2 for further work.
