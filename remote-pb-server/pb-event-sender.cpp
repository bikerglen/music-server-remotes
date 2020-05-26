#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <iostream>
#include <curl/curl.h>

#define URL "http://192.168.180.8:6682"

void escape_quotes (std::string& data);
void curl_simple_post (std::string url, std::string& json);

int main (int argc, char *argv[])
{
	if (argc >= 2) {

		char *event = argv[1];

		if (!strcmp (event, "songstart")) {

			// printf ("EVENT:    SONGSTART\n");

			std::string input;
			std::string title = "";
			std::string artist = "";
			std::string album = "";

			while (!std::cin.eof ()) {
				size_t pos;

				// get a line of input
				std::getline (std::cin, input);
				// printf ("The input was '%s'\n", input.c_str ());
	
				// try to find a title
				pos = input.find ("title=");
				if (pos != std::string::npos) {
					title = input.substr (pos + 6, std::string::npos);
				}

				pos = input.find ("artist=");
				if (pos != std::string::npos) {
					artist = input.substr (pos + 7, std::string::npos);
				}

				pos = input.find ("album=");
				if (pos != std::string::npos) {
					album = input.substr (pos + 6, std::string::npos);
				}
			}

			escape_quotes (title);
			escape_quotes (artist);
			escape_quotes (album);

			// printf ("  TITLE:    '%s'\n", title.c_str());
			// printf ("  ARTIST:   '%s'\n", artist.c_str());
			// printf ("  ALBUM:    '%s'\n", album.c_str());

			std::string json = "{ event: \"songstart\", title: \"" + title + "\", " +
				"artist: \"" + artist + "\", " + "album: \"" + album + "\" }";

			// printf ("  JSON:     '%s'\n", json.c_str());

			curl_simple_post (URL, json);

		} else if (!strcmp (event, "songfinish")) {

			// printf ("EVENT:    SONGFINISH\n");

			std::string json = "{ event: \"songfinish\" }";

			// printf ("  JSON:     '%s'\n", json.c_str());

			curl_simple_post (URL, json);
		} else {
			printf ("EVENT:    '%s'\n", event);
		}
	}

	return 0;
}


void escape_quotes (std::string& data)
{
    std::string buffer;
    buffer.reserve (data.size());
    for (size_t pos = 0; pos != data.size(); pos++) {
        switch(data[pos]) {
            case '\"': buffer.append ("\\\"");        break;
            default:   buffer.append (&data[pos], 1); break;
        }
    }
    data.swap(buffer);
}


void curl_simple_post (std::string url, std::string& json)
{
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init ();
	if (curl) {
		curl_easy_setopt (curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt (curl, CURLOPT_POSTFIELDS, json.c_str());
 
		curl_easy_setopt (curl, CURLOPT_POSTFIELDSIZE, (long)strlen(json.c_str()));
 
		res = curl_easy_perform (curl);

		if (res != CURLE_OK) {
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
 
		curl_easy_cleanup(curl);
	}
}


/*
void encode(std::string& data) {
    std::string buffer;
    buffer.reserve(data.size());
    for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
            case '&':  buffer.append("&amp;");       break;
            case '\"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(&data[pos], 1); break;
        }
    }
    data.swap(buffer);
}
*/

