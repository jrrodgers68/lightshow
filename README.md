# Noah's Light Show

This is just a minor modification to Adafruit's strandtest for Neopixels.  

I purchased a 5 meter string of Neopixels (30/meter).  Then used an 
Arduino mega to control the neopixels.  The only real change is to run 
the test when a specific pin (A2) goes high.  If it goes low, once the run 
is done, turn the string dark.

I bring A2 high from a Particle photon which is powered off the Mega's 5V pin.
It subscribes to a MQTT broker and brings the pin high when the "on" payload
is received.  It goes low when the "off" payload is received.  The messages
are published from Home Assistant based on a schedule.
