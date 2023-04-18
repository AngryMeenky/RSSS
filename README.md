# RSSS

The Really Simple Serial Syncronization library for Arduino.
------------------------------------------------------------

This library provides a wrapper around an
[Arduino](https://www.arduino.cc/)
[Stream](https://www.arduino.cc/reference/en/language/functions/communication/stream/)
instance, typically a
[Serial](https://www.arduino.cc/reference/en/language/functions/communication/serial/)
instance, to provided a synchronized byte stream. This is accomplished by
periodically inserting a multibyte synchronization marker into the byte stream.
These markers will be searched for and removed from the byte stream by the
receiver. Several example host implementations are provided in
[extras](https://github.com/AngryMeenky/RSSS/tree/main/extras).

This library is intended for **binary** communications only, as text streams
have builtin synchronization points ([EOL](https://en.wikipedia.org/wiki/Newline)).
