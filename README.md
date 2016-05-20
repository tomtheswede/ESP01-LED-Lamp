# ESP01-LED-Lamp
Code enabling the control of a 12V led light source over Wifi  (http://espautohome.blogspot.com.au)

Commands available to interact with ESP01-LED-Lamp. Send a UDP message to the IP address of the sensor with the correct device ID.
"LED001,on" - Turns on the LED with fade to the last brightness setting
"LED001,off" or "LED001,0" - Turns off the LED with fade
"LED001,toggle" - Turns on the LED if it is off or turns off if the LED was on
"LED001,hold" - Halts the fading of the LED
"LED001,57" - Fades up/down the LED to 57% (Or any other level between 1 and 100)
"LED001,instant on" - Turns on the LED without fade to the last brightness setting
"LED001,instant off" or "LED001,instant 0" - Turns off the LED without fade
"LED001,instant toggle" - Turns on the LED if it is off or turns off if the LED was on

Prefixes can be used to change fading speeds and set a timer. Examples:
"LED001,fade200 on" - Turns on the LED with a fade with 200 milliseconds between each fade interval
"LED001,timer600 on" - Turns on the LED with a fade for 600 seconds before sending a fade off command
Fade can be used for 'on, off, toggle, 57'
Timer can be used for 'on, toggle, hold, 57, instant on, instant toggle'
