
#include <MQTT.h>

static const char* deviceTopic = "home1/noahsbedroom/device/lightshow";

int LIGHT_SHOW_PIN = D5;

int lastDSTCheckDay = 0;

void callback(char* topic, byte* payload, unsigned int length);
MQTT client("192.168.2.226", 1883, callback);

// function prototypes
void connect();


// recieve message
void callback(char* topic, byte* payload, unsigned int length)
{
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;

    String reboot("reboot");
    String on("on");
    String off("off");

    String ps(p);
    if(reboot.equalsIgnoreCase(ps))
    {
        Particle.publish("INFO", "Reboot request received", 60, PRIVATE);
        System.reset();
    }
    else if(on.equalsIgnoreCase(ps))
    {
        Particle.publish("INFO", "Received light show start", 60, PRIVATE);
        digitalWrite(LIGHT_SHOW_PIN, HIGH);
    }
    else if(off.equalsIgnoreCase(ps))
    {
        Particle.publish("INFO", "Received light show stop", 60, PRIVATE);
        digitalWrite(LIGHT_SHOW_PIN, LOW);
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

void handle_dst()
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

void setup()
{
    pinMode(LIGHT_SHOW_PIN, OUTPUT);
    digitalWrite(LIGHT_SHOW_PIN, LOW);

    connect();
}

void loop()
{
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
        handle_dst();
    }

    Particle.syncTime();

    // connect to the server

    if(!client.isConnected())
    {
        Particle.publish("MQTT", "Connecting", 60 , PRIVATE);
        client.connect("lightshow");
        Particle.publish("MQTT", "Connected", 60, PRIVATE);

        client.subscribe(deviceTopic);
    }
}
