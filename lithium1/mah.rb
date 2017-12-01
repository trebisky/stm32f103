#!/bin/ruby

# mah.rb
# Calculate mAh rating from calibration data.
#
# Tom Trebisky  11-26-2017
#
# A calibration log may have 25,000 points.

# Only these first two were done with 16.67
#infile = "battery1.log"
#infile = "battery2.log"
#resist = 16.667

#infile = "battery3.log"
#infile = "battery4.log"
#infile = "battery5.log"
#infile = "Data/2017_11_29.dat"
infile = "Data/2017_11_29b.dat"

# Now this script is pretty much obsolete and
# included in the lithium.rb script

resist = 5.0

sph = 60.0 * 60.0

f = File.new infile
time = 0
sum = 0.0
f.each { |line|
    next unless line =~ /^[0-9]/
    w = line.split
    next unless w[2] == "1"
    #puts line
    volts = w[1].to_f / 1000.0
    cur = volts / resist
    sum += cur
    time += 2
}
f.close

factor = 2.0 * 1000.0 / 60.0 / 60.0
mah = sum * factor
print "%.1f mAh\n" % mah

# THE END
