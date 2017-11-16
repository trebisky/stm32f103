adc_serial - write a simple ADC driver

The idea here is to do single conversions when asked for by software.
I don't have working USB yet, so I have enhanced serial.c to support
both read and write (up to now it has been a write only console).
