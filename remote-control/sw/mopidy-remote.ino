// This #include statement was automatically added by the Particle IDE.
#include "HttpClient.h"

// This #include statement was automatically added by the Particle IDE.
#include <ArduinoJson.h>

#include "Particle.h"

// system directives
STARTUP(WiFi.selectAntenna(ANT_INTERNAL));
SYSTEM_THREAD(ENABLED);

// Mopidy server IP sddress and port
#define MOPIDY_IP_ADDR 192,168,180,8
#define MOPIDY_PORT    6680

// prototypes
void tick_50Hz (void);
void DebounceButtons (void);

// pushbutton debounce state machine states
#define NUM_BUTTONS 6
int buttons[NUM_BUTTONS] = { A4, A5, A2, A3, A0, A1 };
uint8_t buttonStates[NUM_BUTTONS] = { 0, 0, 0, 0, 0, 0 };
uint8_t buttonTimers[NUM_BUTTONS] = { 0, 0, 0, 0, 0, 0 };
bool buttonDownEvents[NUM_BUTTONS] = { false, false, false, false, false, false };
bool buttonRepeatEnables[NUM_BUTTONS] = { false, false, false, false, true, true };

// LEDs
#define NUM_LEDS 6
int leds[NUM_LEDS] = { D4, D5, D2, D3, D0, D1 };
#define LED_OFF 0
#define LED_ON 1
bool ledStates[NUM_LEDS] = { false, false, false, false, false, false };

// globals
Timer timer (20, tick_50Hz, false);

int timer3s = 0;
int connected = false;
int wiFiReady = false;

void setup()
{
    int i;

    delay (1000);
    Serial.begin (9600);
    Serial.printf ("\n\r\n\rHello, world!\n\r");
    Serial.printlnf("System version: %s", System.version().c_str());

    // init buttons
    for (i = 0; i < NUM_BUTTONS; i++) {
        pinMode (buttons[i], INPUT_PULLUP);
    }
    
    // init LEDs
    for (i = 0; i < NUM_LEDS; i++) {
        pinMode (leds[i], OUTPUT);
        digitalWrite (leds[i], LED_OFF);
    }

    // cycle through the LEDs
    for (i = 0; i < NUM_LEDS+1; i++) {
        digitalWrite (leds[i], LED_ON);
        delay (100);
        digitalWrite (leds[i], LED_OFF);
    }

    // start periodic task to update time displayed on nixie tubes
    timer.start();
}

void loop()
{
    if (!connected && Particle.connected ()) {
        connected = true;
        Serial.printf ("Connected to the Particle Cloud.\n\r");
    } else if (connected && !Particle.connected ()) {
        connected = false;
        Serial.printf ("Disconnected from the Particle Cloud.\n\r");
    }
}

void tick_50Hz (void)
{
    int i;

    timer3s++;
    if (timer3s >= 150) {
        timer3s = 0;
        if (WiFi.ready ()) {
            Serial.printf (".");
        } else if (WiFi.connecting ()) {
            Serial.printf ("C");
        } else if (!WiFi.ready ()) {
            Serial.printf ("X");
        }
    }

    // check WiFi status and re-init udp routines if needed
    if (!wiFiReady && WiFi.ready ()) {
        wiFiReady = true;
        Serial.printf ("Connected to WiFi.\n\r");
    } else if (wiFiReady && !WiFi.ready ()) {
        wiFiReady = false;
        // udp.stop ();
        Serial.printf ("Lost WiFi connection.\n\r");
    }

    // debounce pushbuttons
    DebounceButtons ();

    // process button presses
    for (i = 0; i < NUM_BUTTONS; i++) {
        if (buttonDownEvents[i]) {
            buttonDownEvents[i] = false;
            Serial.printf ("button %d pressed\n\r", i);
            // ledStates[i] = !ledStates[i];
            if (wiFiReady) {
                switch (i) {
                    case 0: PreviousTrack (); break;
                    case 1: NextTrack (); break;
                    case 2: PlayPause (); break;
                    case 3: MuteUnmute (); break;
                    case 4: VolumeDown (); break;
                    case 5: VolumeUp (); break;
                }
            }
        }
    }

    // update leds
    for (i = 0; i < NUM_LEDS; i++) {
        digitalWrite (leds[i], ledStates[i] ? LED_ON : LED_OFF);
        if (wiFiReady) {
            // TODO
        }
    }
}

void DebounceButtons (void)
{
    int i;

    for (i = 0; i < NUM_BUTTONS; i++) {
        switch (buttonStates[i]) {
            case 0:
                buttonStates[i] = (digitalRead (buttons[i]) == 0) ? 1 : 0;
                break;
            case 1:
                if (digitalRead (buttons[i]) == 0) {
                    buttonStates[i] = 2;
                    buttonDownEvents[i] = true;
                    buttonTimers[i] = 0;
                } else {
                    buttonStates[i] = 0;
                }
                break;
            case 2:
                buttonStates[i] = (digitalRead (buttons[i]) == 0) ? 2 : 3;
                buttonTimers[i]++;
                if (buttonTimers[i] == 50) {
                    buttonDownEvents[i] = buttonRepeatEnables[i];
                } else if (buttonTimers[i] == 60) {
                    buttonDownEvents[i] = buttonRepeatEnables[i];
                    buttonTimers[i] = 50;
                }
                break;
            case 3:
                buttonStates[i] = (digitalRead (buttons[i]) == 0) ? 2 : 0;
                break;
            default:
                buttonStates[i] = 0;
                break;
        }
    }
}

HttpClient http;

http_request_t request;
http_response_t response;
http_header_t headers[] = {
    { "Accept", "*/*" },
    { "Content-Type", "application/json" },
    { NULL, NULL }
};


void PreviousTrack (void)
{
    request.ip = IPAddress (MOPIDY_IP_ADDR);
    request.port = MOPIDY_PORT;
    request.path = "/mopidy/rpc";
    request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.playback.previous\"}";
    http.post (request, response, headers);
    // Serial.println("HTTP Response: ");
    // Serial.println(response.status);
    // Serial.println(response.body);
}

void NextTrack (void)
{
    request.ip = IPAddress (MOPIDY_IP_ADDR);
    request.port = MOPIDY_PORT;
    request.path = "/mopidy/rpc";
    request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.playback.next\"}";
    http.post (request, response, headers);
    // Serial.println("HTTP Response: ");
    // Serial.println(response.status);
    // Serial.println(response.body);
}

StaticJsonDocument<1024> doc;

void PlayPause (void)
{
    String result;
    
    request.ip = IPAddress (MOPIDY_IP_ADDR);
    request.port = MOPIDY_PORT;
    request.path = "/mopidy/rpc";
    request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.playback.get_state\"}";
    http.post (request, response, headers);
    // Serial.println ("HTTP Response: ");
    // Serial.println (response.status);
    // Serial.println (response.body);
    if (response.status == 200) {
        deserializeJson (doc, (const char *)response.body.c_str());
        result = doc["result"].as<char*>();
        // Serial.println (result);
        
        if (!strcmp (result.c_str (), "playing")) {
            request.ip = IPAddress (MOPIDY_IP_ADDR);
            request.port = MOPIDY_PORT;
            request.path = "/mopidy/rpc";
            request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.playback.pause\"}";
            http.post (request, response, headers);
            // Serial.println ("HTTP Response: ");
            // Serial.println (response.status);
            // Serial.println (response.body);
        } else if (!strcmp (result.c_str (), "paused")) {
            request.ip = IPAddress (MOPIDY_IP_ADDR);
            request.port = MOPIDY_PORT;
            request.path = "/mopidy/rpc";
            request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.playback.resume\"}";
            http.post (request, response, headers);
            // Serial.println ("HTTP Response: ");
            // Serial.println (response.status);
            // Serial.println (response.body);
       }
    }
}

void MuteUnmute (void)
{
    String result;
    
    request.ip = IPAddress (MOPIDY_IP_ADDR);
    request.port = MOPIDY_PORT;
    request.path = "/mopidy/rpc";
    request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.mixer.get_mute\"}";
    http.post (request, response, headers);
    // Serial.println ("HTTP Response: ");
    // Serial.println (response.status);
    // Serial.println (response.body);
    if (response.status == 200) {
        deserializeJson (doc, (const char *)response.body.c_str());
        int state = doc["result"].as<int>();
        Serial.printf ("state: %d\r\n", state);
        
        if (state == 0) {
            request.ip = IPAddress (MOPIDY_IP_ADDR);
            request.port = MOPIDY_PORT;
            request.path = "/mopidy/rpc";
            request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.mixer.set_mute\", \"params\": {\"mute\": true}}";
            http.post (request, response, headers);
            // Serial.println ("HTTP Response: ");
            // Serial.println (response.status);
            // Serial.println (response.body);
        } else if (state != 0) {
            request.ip = IPAddress (MOPIDY_IP_ADDR);
            request.port = MOPIDY_PORT;
            request.path = "/mopidy/rpc";
            request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.mixer.set_mute\", \"params\": {\"mute\": false}}";
            http.post (request, response, headers);
            // Serial.println ("HTTP Response: ");
            // Serial.println (response.status);
            // Serial.println (response.body);
       }
    }
}

void VolumeDown (void)
{
    String result;
    char volumeCommand[128];
    
    request.ip = IPAddress (MOPIDY_IP_ADDR);
    request.port = MOPIDY_PORT;
    request.path = "/mopidy/rpc";
    request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.mixer.get_volume\"}";
    http.post (request, response, headers);
    // Serial.println ("HTTP Response: ");
    // Serial.println (response.status);
    // Serial.println (response.body);
    if (response.status == 200) {
        deserializeJson (doc, (const char *)response.body.c_str());
        int volume = doc["result"].as<int>();
        Serial.printf ("volume: %d\r\n", volume);
        volume = volume - 10;
        if (volume < 0) {
            volume = 0;
        }
        request.ip = IPAddress (MOPIDY_IP_ADDR);
        request.port = MOPIDY_PORT;
        request.path = "/mopidy/rpc";
        sprintf (volumeCommand, "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.mixer.set_volume\", \"params\": {\"volume\": %d}}", volume);
        request.body = volumeCommand;
        http.post (request, response, headers);
        // Serial.println ("HTTP Response: ");
        // Serial.println (response.status);
        // Serial.println (response.body);
    }
}

void VolumeUp (void)
{
    String result;
    char volumeCommand[128];
    
    request.ip = IPAddress (MOPIDY_IP_ADDR);
    request.port = MOPIDY_PORT;
    request.path = "/mopidy/rpc";
    request.body = "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.mixer.get_volume\"}";
    http.post (request, response, headers);
    // Serial.println ("HTTP Response: ");
    // Serial.println (response.status);
    // Serial.println (response.body);
    if (response.status == 200) {
        deserializeJson (doc, (const char *)response.body.c_str());
        int volume = doc["result"].as<int>();
        Serial.printf ("volume: %d\r\n", volume);
        volume = volume + 10;
        if (volume > 100) {
            volume = 100;
        }
        request.ip = IPAddress (MOPIDY_IP_ADDR);
        request.port = MOPIDY_PORT;
        request.path = "/mopidy/rpc";
        sprintf (volumeCommand, "{\"jsonrpc\": \"2.0\", \"id\": 1, \"method\": \"core.mixer.set_volume\", \"params\": {\"volume\": %d}}", volume);
        request.body = volumeCommand;
        http.post (request, response, headers);
        // Serial.println ("HTTP Response: ");
        // Serial.println (response.status);
        // Serial.println (response.body);
    }
}