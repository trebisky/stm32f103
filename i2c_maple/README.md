I am experimenting with libmaple and an i2c driver

This is my so called "maple-ectomy" of the i2c driver.

At this time, it simply does not work, so I am back
to the drawing board.

9-9-2020

I just finishing running the demo that comes with libmaple
and that uses the MCP4725 i2c DAC chip.
I have built this demo in the standard maple environment
and it works just fine.

The idea here is to move that demo here and extract just the
i2c driver from libmaple.

This began by copying my i2c demo (which is a copy of my old interrupt demo)
