#!/bin/ruby

# Tom Trebisky  11-18-2017

require 'serialport'

$port = "/dev/ttyUSB1"
#$port = "/dev/ttyUSB2"

$prompt = "Command: "

class Lithium
    def initialize
	@usb = $port
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
    def puts ( msg )
	msg.split(//).each { |b| sout b }
    end
    def sin
	@ser.read(1)
    end
    # lines come ending with CR LF
    # chomp gets rid of them both
    def gets
	rv = ""
	loop {
	    x = sin
	    #q = nil
	    #x.each_byte { |b| q = b }
	    #print "Got #{x.size} #{q}\n"
	    rv << x
	    break if x == "\n"
	    return rv if rv == $prompt
	}
	rv.chomp 
    end
end

#logfile = File.new("battery.log", "w")
logfile = File.new("battery.log", "a")

tstamp = `date`
puts tstamp
logfile.puts tstamp

l = Lithium.new

# The idea here is to find out if we actually have a
# connection to the gadet before sending commands.
# I added a "check" command that simply returns "OK"
# but on second thought I could just send a newline and
# see if the echo and prompt come back.

# XXX - This needs a timeout
# without a timeout it just locks up.

l.puts "check\n"
# ignore echo
resp = l.gets
resp = l.gets
if resp != "OK"
    print "No connection via port: #{$port}\n"
    exit
end
# ignore prompt
resp = l.gets

l.puts "cal\n"

print "Starting -----------------------------------\n"
logfile.puts "starting calibration"

loop {
    line = l.gets
    break if line == $prompt
    puts line
    logfile.puts line
    #print "Line: #{line}\n"
    #line.each_byte { |b| print "#{b}\n" }
}

print "Done -----------------------------------\n"
logfile.puts "calibration finished"

tstamp = `date`
puts tstamp
logfile.puts tstamp

logfile.close

# THE END
