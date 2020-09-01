#!/bin/ruby

# Tom Trebisky  11-18-2017

# Run this by typing ./lithium1.rb xyz
# This yields the data file Data/xyz.dat
# 
# This just dives in and starts the test,
# it should be worked into a GUI that displays
# initial voltage and has a stop/start button
# and other nice things.
# ** Also should search for serial port and
# probe using some innocuous command, with timeout.
#
# The tester requires two USB cables.
# One powers the device via the USB on the STM
# The other does communication via a USB to serial module.

require 'serialport'

#$port = "/dev/ttyUSB0"
#$port = "/dev/ttyUSB1"
$port = "/dev/ttyUSB2"

$prompt = "Command: "
$datadir = "Data"
resist = 5.0

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

def mk_path ( title )
    return $datadir + "/" + title + ".dat"
end

def mk_png ( title )
    return $datadir + "/" + title + ".png"
end

def gen_title

    # time this very second.
    t = Time.now

    # time at the start of the current day.
    tt = Time.local t.year, t.month, t.day

    ts = tt.strftime "%Y_%m_%d"
    name = mk_path ts

    return ts unless FileTest.exist? name

    # should never have more than 26 files in a day.
    ver = ?b
    loop {
	tsver = ts + ver.chr
	name = mk_path tsver
	return tsver unless FileTest.exist? name
	ver = ver.next
    }

end

def mk_plot ( bat )
    data_path = mk_path bat
    plot_path = mk_png bat
    t = Time.now
    day = t.strftime "%m-%d-%y"
    title = bat + " " + day
    title.tr! "_", "-"

    system "rm -f zplot"
    f = File.new "zplot", "a", 0770
    f.puts "#!/usr/bin/gnuplot -c"
    f.puts "set terminal png"
    f.puts "set output '#{plot_path}'"
    f.puts "plot '#{data_path}' using ($1/3600.0):($2/1000.0) title '#{title}' with lines"
    f.close

    system "gnuplot <zplot"
    system "rm -f zplot"
end

title = nil

if ARGV.size < 1
    puts "usage: lithium title"
    exit
end

if ARGV[0] =~ /^-/
    if ARGV[0] =~ /^-t/
	title = gen_title
    else
	puts "usage: lithium title"
	exit
    end
else
    title = ARGV[0]
end

puts title

# name = File.new("battery.log", "w")
# name = gen_file
name = mk_path title
if FileTest.exist? name
    print "File: #{name} already exists\n"
    exit
end

begin
    logfile = File.new name, "a", 0664
#    File.chmod ( 0664, name )
rescue
    print "Cannot open file: ", name, "\n"
    exit
end

tstamp = `date`.chomp
puts tstamp
logfile.print "# " + tstamp + "\n"

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
logfile.puts "# starting calibration"

# R int = 161 (milliohms)

time = 0
sum = 0.0;
save_rint = nil
loop {
    line = l.gets
    if line =~ /^# R int =/
	save_rint = line
    end
    break if line == $prompt
    puts line
    if line =~ /^[a-zA-Z]/
	logfile.puts "# " + line
	next
    end
    w = line.split
    if w[2] != "1"
	logfile.puts "# " + line
	next
    end

    logfile.puts line

    volts = w[1].to_f / 1000.0
    cur = volts / resist
    sum += cur
    time += 2
    #print "Line: #{line}\n"
    #line.each_byte { |b| print "#{b}\n" }
}

factor = 2.0 * 1000.0 / 60.0 / 60.0
mah = sum * factor
puts "# %.1f mAh" % mah
logfile.puts "# %.1f mAh" % mah
if save_rint
    puts save_rint
    logfile.puts save_rint
end

print "Done -----------------------------------\n"
logfile.puts "# calibration finished"

tstamp = `date`.chomp
puts tstamp
logfile.print "# " + tstamp + "\n"

logfile.close

mk_plot title

# THE END
