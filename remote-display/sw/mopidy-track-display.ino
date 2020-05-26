#include <ArduinoJson.h>
#include "Websockets.h"
#include "Display.h"

STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));
SYSTEM_THREAD(ENABLED);

void tick_50Hz (void);

int connected = false;
Timer timer (20, tick_50Hz, false);

WebSocketClient client;
WebSocketClient client2;

int initState = 0;

void setup ()
{
    int i;
    
    // say hello world
    delay (500);
    Serial.begin (9600);
    Serial.printf ("\n\r\n\rHello, world!\n\r");
    
    // initialize SPI
    SPI.setClockSpeed (1, MHZ);
    SPI.setBitOrder (MSBFIRST);
    SPI.setDataMode (SPI_MODE0);
    SPI.begin (A2);
    digitalWrite (A2, HIGH);

    // init display boards
    disp4xPDC54_Init ();
    
    // display something
    // disp4xPDC54_Display ((const unsigned char *)"* HELLO WORLD *");

    scroll_Message ("CONNECTING");
    
    // start periodic task to update time displayed on nixie tubes
    timer.start();
}

void loop ()
{
    bool result;
    
    if (!connected) {
        if (Particle.connected ()) {
            connected = true;
            scroll_Message ("CONNECTED");
            
            // mopidy client
            client.onMessage (onMessage);
            client.connect ("192.168.180.8", 6680, "", "/mopidy/ws");
            
            // pianobar proxy client
            client2.onMessage (onMessage2);
            client2.connect ("192.168.180.8", 6683, "", "/");
        }
    }
    if (connected) {
        client.monitor ();
        client2.monitor ();
        
        if (initState == 0) {
            result = client.send ("{\"jsonrpc\": \"2.0\", \"id\": 1001, \"method\": \"core.playback.get_current_track\"}");
            if (result) {
                initState = 1;
            }
        } else if (initState == 1) {
            result = client.send ("{\"jsonrpc\": \"2.0\", \"id\": 1002, \"method\": \"core.playback.get_state\"}");
            if (result) {
                initState = 2;
            }
        } else if (initState == 2) {
            result = client.send ("{\"jsonrpc\": \"2.0\", \"id\": 1003, \"method\": \"core.mixer.get_mute\"}");
            if (result) {
                initState = 3;
            }
        } else if (initState == 3) {
            result = client.send ("{\"jsonrpc\": \"2.0\", \"id\": 1004, \"method\": \"core.mixer.get_volume\"}");
            if (result) {
                initState = 4;
            }
        }
    }
}

#define PLAYING 0
#define PAUSED 1
#define STOPPED 2

bool pianoBarMode = false;

int mute = 0;
int volume_timer = 0;
int volume = 0;
int playback = 0;
int pauseTimer = 0;
int scroll_timer = 0;
int scroll_position = 0;
String scroll_string = "";

int pbState = STOPPED;
int pbTimer = 0;

StaticJsonDocument<2048> doc;

void onMessage (WebSocketClient client, char* message)
{
    pianoBarMode = false;
    
    Serial.print ("MPD Received: ");
    Serial.println (message);
    
    deserializeJson (doc, (const char *)message);
    String event = doc["event"].as<char*>();
    int id = doc["id"].as<int>();
    
    Serial.print("Event: ");
    Serial.println (event);
    Serial.print("ID: ");
    Serial.println (id);
    
    if (!strcmp (event, "mute_changed")) {
        mute = doc["mute"].as<int>();
        if (!mute) {
            volume_timer = 50;
        }
        Serial.printf ("Mute: %d\r\n", mute);
    } else if (!strcmp (event, "volume_changed")) {
        volume = doc["volume"].as<int>();
        volume_timer = 50;
        Serial.printf ("Volume: %d\r\n", volume);
    } else if (!strcmp (event, "track_playback_started")) {
        playback = PLAYING;
        String track = doc["tl_track"]["track"]["name"].as<char*>();
        Serial.println (track);
        String artist = doc["tl_track"]["track"]["artists"][0]["name"].as<char*>();
        Serial.println (artist);
        String album = doc["tl_track"]["track"]["album"]["name"].as<char*>();
        Serial.println (album);
        scroll_Message (track + " - " + artist + " - " + album);
    } else if (!strcmp (event, "track_playback_paused")) {
        playback = PAUSED;
        pauseTimer = 5 * 60 * 50;
    } else if (!strcmp (event, "track_playback_resumed")) {
        playback = PLAYING;
    } else if (!strcmp (event, "track_playback_ended")) {
        playback = STOPPED;
        scroll_Message ("                ");
    } else if (!strcmp (event, "playback_state_changed")) {
        String new_state = doc["new_state"].as<char*>();
        if (!strcmp (new_state, "stopped")) {
            playback = STOPPED;
            scroll_Message ("                ");
        }
    } else if (id != 0) {
        if (id == 1001) {
            String result = doc["result"].as<char*>();
            if (!strcmp (result, "null")) {
                scroll_Message ("                ");
            } else {
                String track = doc["result"]["name"].as<char*>();
                Serial.println (track);
                String artist = doc["result"]["artists"][0]["name"].as<char*>();
                Serial.println (artist);
                String album = doc["result"]["album"]["name"].as<char*>();
                Serial.println (album);
                scroll_Message (track + " - " + artist + " - " + album);
            }
        } else if (id == 1002) {
            String result = doc["result"].as<char*>();
            if (!strcmp (result, "playing")) {
                playback = PLAYING;
                Serial.println ("Setting initial playback state to playing.");
            } else if (!strcmp (result, "paused")) {
                playback = PAUSED;
                Serial.println ("Setting initial playback state to paused.");
            } else if (!strcmp (result, "stopped")) {
                playback = STOPPED;
                Serial.println ("Setting initial playback state to stopped.");
                scroll_Message ("                ");
            }
        } else if (id == 1003) {
            mute = doc["result"].as<int>();
            Serial.printf ("Setting initial mute state to %d\n\r", mute);
        } else if (id == 1004) {
            volume = doc["result"].as<int>();
            Serial.printf ("Setting initial volume state to %d\n\r", volume);
        }
    }
}

void onMessage2 (WebSocketClient client, char* message)
{
    pianoBarMode = true;
    
    Serial.print ("PB Received: ");
    Serial.println (message);
    
    deserializeJson (doc, (const char *)message);
    String event = doc["event"].as<char*>();

    Serial.print("Event: ");
    Serial.println (event);

    if (!strcmp (event, "songstart")) {
        pbState = PLAYING;
        pbTimer = 10 * 60 * 50; // go to stopped state and blank display after ten minutes
        String track = doc["title"].as<char*>();
        Serial.println (track);
        String artist = doc["artist"].as<char*>();
        Serial.println (artist);
        String album = doc["album"].as<char*>();
        Serial.println (album);
        scroll_Message (track + " - " + artist + " - " + album);
    } else if (!strcmp (event, "songfinish")) {
        pbState = STOPPED;
        scroll_Message ("                ");
    }
}

void tick_50Hz (void)
{
    scroll_Tick ();
}

void scroll_Message (String s)
{
    scroll_string = s;
    scroll_timer = 0;
    scroll_position = s.length ();
    // Serial.printf ("%d %d %d %d '%s' %d %d", playback, volume_timer, mute, pauseTimer, scroll_string.c_str(), scroll_timer, scroll_position);
}

void scroll_Tick (void)
{
    if (pianoBarMode) {
        if (pbState == STOPPED) {
            scroll_timer = 9;
            disp4xPDC54_Display ((const unsigned char *)"                ");
        } else {
            scroll_timer++;
            if (scroll_timer >= 10) {
                scroll_timer = 0;
                scroll_position++;
                if (scroll_position >= scroll_string.length () + 16) {
                    scroll_position = 0;
                }
                scroll_DisplaySubString ();
            }
            pbTimer--;
            if (pbTimer == 0) {
                pbState = STOPPED;
            }
        }
    } else {
        if (volume_timer != 0) {
            volume_timer--;
            scroll_timer = 9;
            char s[17];
            sprintf (s, " - VOLUME %3d - ", volume);
            disp4xPDC54_Display ((const unsigned char *)s);
        } else if (mute) {
            scroll_timer = 9;
            disp4xPDC54_Display ((const unsigned char *)"  --- MUTE ---  ");
        } else if (playback == PAUSED) {
            scroll_timer = 9;
            if (pauseTimer == 0) {
                // blank display if paused for more than 5 minutes
                disp4xPDC54_Display ((const unsigned char *)"                ");
            } else {
                disp4xPDC54_Display ((const unsigned char *)"  -- PAUSED --  ");
                pauseTimer--;
            }
        } else if (playback == STOPPED) {
            scroll_timer = 9;
            disp4xPDC54_Display ((const unsigned char *)"                ");
        } else {
            scroll_timer++;
            if (scroll_timer >= 10) {
                scroll_timer = 0;
                scroll_position++;
                if (scroll_position >= scroll_string.length () + 16) {
                    scroll_position = 0;
                }
                scroll_DisplaySubString ();
            }
        }
    }
}

void scroll_DisplaySubString (void)
{
    char c, m[17];
    int len = scroll_string.length ();
    
    for (int i = 0; i < 16; i++) {
        c = (scroll_position + i) % (len + 16);
        if (c < len) {
            m[i] = scroll_string.charAt (c);
        } else {
            m[i] = ' ';
        }
    }
    
    m[16] = 0;
    // Serial.println (m);
    
    disp4xPDC54_Display ((const unsigned char *)m);
}


//---------------------------------------------------------------------------------------------
// write the first max6954 in the chain
//

void max6954_write (const unsigned char addr, const unsigned char data)
{
    // set cs_n low
    digitalWrite (A2, LOW);

    // shift nop
    SPI.transfer (0x00);

    // shift nop
    SPI.transfer (0x00);

    // shift write bit + address bits
    SPI.transfer (addr);

    // shift data bits
    SPI.transfer (data);

    // set cs_n high
    digitalWrite (A2, HIGH);
}


//---------------------------------------------------------------------------------------------
// write the second max6954 in the chain
//

void max6954_write2 (const unsigned char addr, const unsigned char data)
{
    // set cs_n low
    digitalWrite (A2, LOW);

    // shift write bit + address bits
    SPI.transfer (addr);

    // shift data bits
    SPI.transfer (data);

    // shift nop
    SPI.transfer (0x00);

    // shift nop
    SPI.transfer (0x00);

    // set cs_n high
    digitalWrite (A2, HIGH);
}


//---------------------------------------------------------------------------------------------
// initialize 2 x 4 x PDC54 dual alphanumeric digit display board
//

void disp4xPDC54_Init (void)
{
	unsigned char i;

	// clear display
	for (i = 0; i < 16; i++) {
		max6954_write (0x60 + i, 0x00);
		max6954_write2 (0x60 + i, 0x00);
	}

    max6954_write (0x01, 0xff);  // decode mode -- decode all digits
    max6954_write (0x02, 0x0f);  // global intensity -- maximum
    max6954_write (0x03, 0x07);  // scan limit -- display 8 14 segment digits
    max6954_write (0x0c, 0xff);  // digit type -- all 14 segment displays
    max6954_write (0x04, 0x01);  // configuration -- enable w/ global intensity
    max6954_write (0x05, 0x00);  // gpio data
    max6954_write (0x06, 0x00);  // port configuration 
    max6954_write (0x07, 0x00);  // display test -- off

    max6954_write (0x10, 0x00);  // intensity 10 -- unused
    max6954_write (0x11, 0x00);  // intensity 32 -- unused
    max6954_write (0x12, 0x00);  // intensity 54 -- unused
    max6954_write (0x13, 0x00);  // intensity 76 -- unused
    max6954_write (0x14, 0x00);  // intensity 10a -- unused
    max6954_write (0x15, 0x00);  // intensity 32a -- unused
    max6954_write (0x16, 0x00);  // intensity 54a -- unused
    max6954_write (0x17, 0x00);  // intensity 66a -- unused

    max6954_write2 (0x01, 0xff);  // decode mode -- decode all digits
    max6954_write2 (0x02, 0x0f);  // global intensity -- maximum
    max6954_write2 (0x03, 0x07);  // scan limit -- display 8 14 segment digits
    max6954_write2 (0x0c, 0xff);  // digit type -- all 14 segment displays
    max6954_write2 (0x04, 0x01);  // configuration -- enable w/ global intensity
    max6954_write2 (0x05, 0x00);  // gpio data
    max6954_write2 (0x06, 0x00);  // port configuration 
    max6954_write2 (0x07, 0x00);  // display test -- off

    max6954_write2 (0x10, 0x00);  // intensity 10 -- unused
    max6954_write2 (0x11, 0x00);  // intensity 32 -- unused
    max6954_write2 (0x12, 0x00);  // intensity 54 -- unused
    max6954_write2 (0x13, 0x00);  // intensity 76 -- unused
    max6954_write2 (0x14, 0x00);  // intensity 10a -- unused
    max6954_write2 (0x15, 0x00);  // intensity 32a -- unused
    max6954_write2 (0x16, 0x00);  // intensity 54a -- unused
    max6954_write2 (0x17, 0x00);  // intensity 66a -- unused
}


//---------------------------------------------------------------------------------------------
// display up to 8 charaters on one display or 16 characters on two displays
//

void disp4xPDC54_Display (const unsigned char *s)
{
	unsigned char i;

	for (i = 0; i < 16; i++) {
		if (*s) {
			if (i < 8) {
				max6954_write (0x60+i, *s++);
			} else {
				max6954_write2 (0x60+i-8, *s++);
			}
		} else {
			break;
		}
	}

	for (; i < 16; i++) {
		if (i < 8) {
			max6954_write (0x60+i, 0x20);
		} else {
			max6954_write2 (0x60+i-8, 0x20);
		}
	}
}