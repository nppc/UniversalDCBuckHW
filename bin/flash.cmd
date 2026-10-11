cd \avrdude\
avrdude -C \avrdude\avrdude.conf -c usbasp -p t85 -P usb -B 4.0 -U flash:w:C:\src\Tennp\git\UniversalDCBuckHW\bin\UDCBuckHW.hex:a
pause
