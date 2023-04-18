# RSSS
The Really Simple Serial Syncronization library for Arduino.
============================================================

This library provides a wrapper around an
[Arduino](https://www.arduino.cc/)
[Stream](https://www.arduino.cc/reference/en/language/functions/communication/stream/)
instance, typically a
[Serial](https://www.arduino.cc/reference/en/language/functions/communication/serial/)
instance, to provided a synchronized byte stream. This is accomplished by
periodically inserting a multibyte synchronization marker into the byte stream.

