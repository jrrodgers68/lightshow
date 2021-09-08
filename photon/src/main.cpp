
#include <MQTT.h>

static const char* deviceTopic = "home1/noahsbedroom/device/lightshow";

static const char* START = "START";
static const char* STOP = "STOP";
static const char* READY = "READY";
static const char* STATUS = "STATUS";
static const char* ON = "ON";
static const char* OFF = "OFF";

// Constants
const size_t READ_BUF_SIZE = 64;
const unsigned long CHAR_TIMEOUT = 10000;

// Global variables
char readBuf[READ_BUF_SIZE];
size_t readBufOffset = 0;
unsigned long lastCharTime = 0;
unsigned long lastReadySend = 0;

int lastDSTCheckDay = 0;

int defaultBrightness = 5;

struct LightShowState
{
    bool isRunning;
    int  brightness;

    LightShowState()
    {
        isRunning = false;
        brightness = 0;
    }

    bool isSame(LightShowState& rhs)
    {
        return (this->brightness == rhs.brightness) && (this->isRunning == rhs.isRunning);
    }
};

LightShowState currentState;
LightShowState changeState;

bool waitingForResponse = false;
bool lightShowReady = false;
const char* lastCommand = NULL;
char brightness[8];
unsigned long lastPublish = 0;

void callback(char* topic, byte* payload, unsigned int length);
MQTT client("192.168.2.245", 1883, callback);

// function prototypes
void connect();
bool isDST(int day, int month, int dow);
void handleDst();
void sendCommand(String command);
void startLightShow();
void stopLightShow();
void updateBrightness();
void processResponse();
void handleStateChange();
void publish(const char* msg);
bool hasStateChanged();

///////////////////////////////////////////////////////////////////////////////

// recieve message
void callback(char* topic, byte* payload, unsigned int length)
{
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;

    String reboot("reboot");
    String on("on");
    String off("off");
    String bright("b=");

    String ps(p);
    if(reboot.equalsIgnoreCase(ps))
    {
        publish("Reboot request received");
        System.reset();
    }
    else if(on.equalsIgnoreCase(ps))
    {
        publish("Received light show start");
        changeState.brightness = defaultBrightness;
        changeState.isRunning = true;
    }
    else if(off.equalsIgnoreCase(ps))
    {
        publish("Received light show stop");
        changeState.isRunning = false;
    }
    else if(ps.startsWith(bright))
    {
        String bLevel = ps.substring(2);
        defaultBrightness = bLevel.toInt();
        changeState.brightness = defaultBrightness;
    }
}

bool isDST(int day, int month, int dow)
{
    // inputs are assumed to be 1 based for month & day.  DOW is ZERO based!

    // jan, feb, and dec are out
    if(month < 3 || month > 11)
    {
        return false;
    }

    // april - oct is DST
    if(month > 3 && month < 11)
    {
        return true;
    }

    // in March, we are DST if our previous Sunday was on or after the 8th
    int previousSunday = day - dow;
    if(month == 3)
    {
        return previousSunday >= 8;
    }

    // for November, must be before Sunday to be DST ::  so previous Sunday must be before the 1st
    return previousSunday <= 0;
}

void handleDst()
{
    int offset = -5;
    Time.zone(offset);
    if(isDST(Time.day(), Time.month(), Time.weekday()-1))
    {
        Time.beginDST();
    }
    else
    {
        Time.endDST();
    }

    lastDSTCheckDay = Time.day();
}

bool waitForResponse(const char* text)
{
    String s = Serial1.readStringUntil('\n');
    return s.equals(text);
}

bool waitForResponseOld(const char* text)
{
    const int BUFFER_SIZE = 64;
    char buffer[BUFFER_SIZE];
    int offset = 0;
    int startMillis = millis();

    while(true)
    {
        while(Serial1.available() > 0)
        {
            if(offset < BUFFER_SIZE-1)
            {
                int val = Serial1.read();
                char x = (char)val;
                if(x != '\n')  // end of command marker
                {
                    buffer[offset] = x;
                    offset++;
                }
                else
                {
                    buffer[offset] = 0;
                    String response(buffer);
                    if(response.equals(text))
                    {
                        return true;
                    }
                    else
                    {
                        String msg = "got back:"+response+ " WAS expecting:"+text;
                        publish(msg);
                        return false;
                    }                
                }
            }
            else
            {
                publish("ERROR: Serial buffer exceeded - resetting buffer index");
                offset = 0;
            }
        }
        if(millis() - startMillis >= 10000)
        {
            // we've waited 10 seconds, time to give up!!
            publish("ERROR: Exceeded response delay time limit (10s)");
            return false;
        }        
    }
}

void sendCommand(const char* command)
{
    publish(command); 
    Serial1.print(command);
    Serial1.print("\n"); // end of comm
    Serial1.flush();

    // wait for response from the client - should be an echo of the command we sent
    //return waitForResponse(command);
    lastCommand = command;
    waitingForResponse = true;
}

void startLightShow()
{
    if(!waitingForResponse)
    {
        sendCommand(START);       
    }
}

void stopLightShow()
{
    if(!waitingForResponse)
    {
        sendCommand(STOP);
    }
}

void updateBrightness()
{
    if(!waitingForResponse)
    {
        String cmd("B"+ String::format("%d", changeState.brightness));
        cmd.toCharArray(brightness, 8);
        sendCommand(brightness);
    }    
}

void requestStatus()
{
    if(!waitingForResponse && !hasStateChanged())
    {
        sendCommand(STATUS);
    }
}

void sendReady()
{
    if(((waitingForResponse && (lastCommand == READY)) || !waitingForResponse) && ((millis() - lastReadySend) > 5000))
    {
        lastReadySend = millis();
        sendCommand(READY);
    }
}

void publish(const char* msg)
{
    Serial.print(Time.timeStr());
    Serial.print(" ");
    Serial.println(msg);

    if(millis() - lastPublish >= 1000)
    {
        lastPublish = millis();
        Particle.publish("LIGHT_SHOW", msg, 60, PRIVATE);
    }
}

void handleStateChange()
{
    if(currentState.brightness != changeState.brightness)
    {
        updateBrightness();
    }
    else if(currentState.isRunning != changeState.isRunning)
    {
        if(changeState.isRunning)
        {
            startLightShow();
        }
        else
        {
            stopLightShow();
        }
    }
}

bool hasStateChanged()
{
    // check if currentstate != changeState
    return !currentState.isSame(changeState);
}

void processResponse()
{
    if(lastCommand == READY)
    {
        // if we get back READY, then all good
        waitingForResponse = false;
        if(strcmp(readBuf, READY) == 0)
        {
            publish("Last cmd was READY and got good response");
            lightShowReady = true;
            requestStatus();
        }
        else
        {
            publish("Last cmd was READY,but response was wrong");
            lightShowReady = false;
        }
    }
    else if(lastCommand == START)
    {
        waitingForResponse = false;
        if(strcmp(readBuf, START) == 0)
        {
            currentState.isRunning = true;
            publish("Start acknowledged");
        }
        else
        {
            publish("Start acked with BAD response - need to restart!");
        }
    }
    else if(lastCommand == STOP)
    {
        waitingForResponse = false;
        if(strcmp(readBuf, STOP) == 0)
        {
            currentState.isRunning = false;
            publish("Stop acknowledged");
        }
        else
        {
            publish("Stop acked with BAD response - need to resend");
        }
        
    }
    else if(lastCommand == brightness)
    {
        waitingForResponse = false;
        if(strcmp(readBuf, brightness) == 0)
        {
            currentState.brightness = changeState.brightness;
            publish("Brightness acknowledged");
        }
        else
        {
            publish("Brightness acked with BAD response -need to resend");
        }
    }
    else if(lastCommand == STATUS)
    {
        waitingForResponse = false;
        if(strcmp(readBuf, ON) == 0)
        {
            currentState.isRunning = true;
            publish("status received that light show is ON");
        }
        else if(strcmp(readBuf, OFF) == 0)
        {
            currentState.isRunning = false;
            publish("status received that light show is OFF");
        }
        else
        {
            publish("Got bad status back - will re-request");
            requestStatus();
        }
    }
    else
    {
        publish("Unknown response received for");
        char buf[32];
        strcpy(buf, lastCommand);
        delay(1000);
        publish(buf);
    }
}

void setup()
{
    Serial.begin(9600);
    Serial1.begin(9600);

    connect();
}

void loop()
{
    while(Serial1.available()) {
        if (readBufOffset < READ_BUF_SIZE) {
            char c = Serial1.read();
            if (c != '\n') {
                // Add character to buffer
                readBuf[readBufOffset++] = c;
                lastCharTime = millis();
            }
            else {
                // End of line character found, process line
                readBuf[readBufOffset] = 0;
                processResponse();
                readBufOffset = 0;
            }
        }
        else {
            Serial.println("readBuf overflow, emptying buffer");
            readBufOffset = 0;
        }
    }
    if (millis() - lastCharTime >= CHAR_TIMEOUT) {
        lastCharTime = millis();
    }

    if(lightShowReady == false)
    {
        sendReady();
    }
    else if(hasStateChanged())
    {
        handleStateChange();
    }

    if (client.isConnected())
    {
        client.loop();
    }
    else
    {
        connect();
    }
}

void connect()
{
    if(Particle.connected() == false)
    {
        Particle.connect();               //Connect to the Particle Cloud
        waitUntil(Particle.connected);    //Wait until connected to the Particle Cloud.
    }

    if(lastDSTCheckDay != Time.day())
    {
        handleDst();
    }

    Particle.syncTime();

    // connect to the server

    if(!client.isConnected())
    {
        publish("MQTT Connecting");
        client.connect("lightshow");
        publish("MQTT Connected");

        client.subscribe(deviceTopic);
    }
}

