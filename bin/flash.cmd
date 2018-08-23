cd \src\Tennp\arduino-1.8.6\hardware\tools\avr\bin\
avrdude -C \src\Tennp\arduino-1.8.6\hardware\tools\avr\etc\avrdude.conf -c usbasp -p t85 -P usb -B 4.0 -U flash:w:C:\src\Tennp\git\UniversalDCBuckHW\bin\UDCBuckHW.hex:a
pause
