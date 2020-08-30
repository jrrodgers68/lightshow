// A basic everyday NeoPixel strip test program.

// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    6

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 150

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)


bool readIncomingCommand();
void ackCommand();
void runLightShow(int);
void processCommands();

#define BUFFER_SIZE 32

char buffer[BUFFER_SIZE];
int offset;
int millisOfLastChar;
int brightness = 5;

// setup() function -- runs once at startup --------------------------------

void setup() {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  Serial.begin(9600);
  Serial1.begin(9600);

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(brightness); // Set BRIGHTNESS to about 1/5 (max = 255)

  //pinMode(A2, INPUT);
  Serial.write("ending setup - getting ready to wait for commands\r\n");
}

// loop() function -- runs repeatedly as long as board is on ---------------
bool run = false;
int step = 0;

bool readIncomingCommand()
{
  bool done = false;
  while(!done)
  {
    while(Serial1.available())
    {
      int val = Serial1.read();
      char x = (char)val;
      millisOfLastChar = millis();
      if(x != '\n')  // end of command marker
      {
        if(offset < BUFFER_SIZE-1)
        {
          buffer[offset] = x;
          offset++;
        }
        else
        {
          Serial.write("buffer limit reached - going to reset and ignore\r\n");
          memset(buffer, 0, BUFFER_SIZE);
          offset = 0;
          return false;
        }
      }
      else
      {
        Serial.write("command received\r\n");
        return true;
      }  
    }
  }
}

void ackCommand(const char* msg)
{
  Serial1.print(msg);    
  Serial1.print("\n");
  Serial1.flush();
}

void loop() 
{
  if(Serial1.available())
  {
    bool result = readIncomingCommand();
    if(result)  // end of command marker
    {
      Serial.write("processing commands\r\n");
      processCommands();
    }
  }

  if(run)
  {
    runLightShow(step);
    step++;
    if(step > 7)
      step = 0;
  }
}


// Some functions of our own for creating animated effects -----------------

void processCommands()
{
    buffer[offset] = 0;
    Serial.write("buffer is: ");
    Serial.write((const char*)buffer);
    Serial.write("\r\n");
    
    if(buffer[0] == 'B')
    {
      ackCommand(buffer);
      
      // make number from rest of string
      buffer[0] = '0';
      brightness = atoi(buffer);
      if((brightness > 0)&& (brightness < 250))
      {
        Serial.write("\nbrightness set to ");
        char temp[4];
        memset(temp, 0, 4);
        sprintf(temp, "%d", brightness);
        Serial.write(temp);
        Serial.write("\r\n");
        strip.setBrightness(brightness);
      }
    }
    else if(strcmp("START", buffer) == 0)
    {
      // start the light show!
      Serial.write("starting light show\r\n");
      ackCommand(buffer);
      run = true;
    }
    else if(strcmp("STOP", buffer) == 0)
    {
      run = false;
      ackCommand(buffer);
      colorWipe(strip.Color(0,   0,   0), 0); // off
      step = 0;
      Serial.write("stopped light show\r\n");
    }
    else if(strcmp("READY", buffer) == 0)
    {
      Serial.write("acking ready request");
      ackCommand("READY");
    }
    else if(strcmp("STATUS", buffer) == 0)
    {
      Serial.write("sending status");
      if(run)
      {
        ackCommand("ON");
        Serial.write("light show is on");
      }
      else
      {
        ackCommand("OFF");
        Serial.write("light show is off");
      }
    }
    else
    {
      // invalid command so send back INVALID
      static const char* INVALID = "INVALID";
      ackCommand(INVALID);
    }
    
    // reset ourselves so ready for next command string
    //memset(buffer, 0, BUFFER_SIZE);
    offset = 0;  
}

void runLightShow(int step)
{
  // Fill along the length of the strip in various colors...
  int delay = 50;
  switch(step)
  {
    case 0:
      colorWipe(strip.Color(255,   0,   0), delay); // Red
      break;
    case 1:
      colorWipe(strip.Color(  0, 255,   0), delay); // Green
      break;
    case 2:
      colorWipe(strip.Color(  0,   0, 255), delay); // Blue
      break;
    case 3:
      theaterChase(strip.Color(127, 127, 127), delay); // White, half brightness
      break;
    case 4:
      theaterChase(strip.Color(127,   0,   0), delay); // Red, half brightness
      break;
    case 5:
      theaterChase(strip.Color(  0,   0, 127), delay); // Blue, half brightness
      break;
    case 6:
      rainbow(10);             // Flowing rainbow cycle along the whole strip
      break;
    case 7:
      theaterChaseRainbow(delay); // Rainbow-enhanced theaterChase variant  
      break;
    default:
      return;
  }
}

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void theaterChase(uint32_t color, int wait) {
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      delay(wait);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
