#!/bin/ruby

# Tom Trebisky  11-18-2017

require 'serialport'

# port is        : /dev/ttyUSB1
# baudrate is    : 115200

usb = "/dev/ttyUSB1"
baud = 115200
parity = SerialPort::NONE

$ser = SerialPort.new usb, baud, 8, 1, parity

# This delay should not be needed, but for now
# it is a workaround for a bug in the remote
# firmware.
def sout ( x )
    $ser.write x
    sleep 0.001
end

#s.puts "abcd\r"
#s.write "abcd\n\r"

# Just this works -- sees \n
# s.write "\r"

# This works too.
# s.write "\n"

sout "a"
sout "b"
sout "c"
sout "d"
sout "\n"

loop {
    c = $ser.read(1)
    putc c
}

loop {
    l = $ser.gets
    puts l
}

puts "done"
