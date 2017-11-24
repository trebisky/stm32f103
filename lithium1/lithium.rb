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
	    return nil if rv == "Command"
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

l.puts "cal\n"

print "Starting -----------------------------------\n"
logfile.puts "starting calibration"

loop {
    line = l.gets
    break unless line
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
