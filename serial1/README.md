The game here is to write a USART driver.
To start, we just want to write characters and poll for status.
Reading and interrupts will come later.

This requires a USB to serial gadget connected to PB9 and PB10
Connect PA9 to RX
Connect PA10 to TX
also connect ground.

This example also has a lot of code cleanup compared to my
earlier blink examples.
