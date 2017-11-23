#!/bin/ruby

# Tom Trebisky  11-18-2017

require 'serialport'

class Lithium
    def initialize
	@usb = "/dev/ttyUSB2"
	@baud = 115200
	@parity = SerialPort::NONE
	print "Connecting on port #{@usb} at #{@baud} baud\n"
	@ser = SerialPort.new @usb, @baud, 8, 1, @parity
    end
    # We shouldn't need the delay, but for now it is a
    # workaround for some kind of bug in the firmware.
    def sout ( x )
	@ser.write x
	sleep 0.001
	@ser.flush
    end
    def sin
	@ser.read(1)
    end
    def gets
    end
end

l = Lithium.new

#s.puts "abcd\r"
#s.write "abcd\n\r"

# Just this works -- sees \n
# s.write "\r"

# This works too.
# s.write "\n"

l.sout "a"
l.sout "b"
l.sout "c"
l.sout "d"
l.sout "\n"

loop {
    c = l.sin
    putc c
}

loop {
    l = @ser.gets
    puts l
}

puts "done"
