/*
  gcc -Wall -std=c99 -o ttlyrics ttlyrics.c -lcurl -lxml2 -I/usr/include/libxml2/ && \
  ./ttlyrics beyond 摩登时代
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <curl/curl.h>
#include <libxml/parser.h>


#define URL_SEARCH_FMTSTR "http://ttlrcct2.qianqian.com/dll/lyricsvr.dll?sh?Artist=%s&Title=%s&Flags=0"
#define URL_DOWNLOAD_FMTSTR "http://ttlrcct2.qianqian.com/dll/lyricsvr.dll?dl?Id=%d&Code=%d"

struct song_info {
	int id;
	char artist[128];
	char  title[256];
};

char search_res[4096];

// Effect: encode the string "from" as unicode hexs,
//         and store the result to "to".
// Notes:
//  1. probably you wanna setlocale() before calling me.
//  2. I did an explicit endian-ness conversion (from little to big),
//     so this will not work on machines with big endian byte order,
//     which is rare for home computers anyway.
//  3. param "from": if it contains non-English chars, those chars will be
//     in UTF-8 format (UTF-8 != unicode). Each such char occupies 3 bytes.
//     for example, strlen("摩登时代") == 12.
//  4. therefore, "to" should be at least 4/3 times strlen() of "from".
void uni_encode (char *from, char *to) {
	int n = strlen(from);
	if (n == 0) {
		to[0] = '\0';
		return ;
	}

	wchar_t ws[n]; // minimal length n/3 (when solely made up of non-English chars),
	               // but let's just put n, 'cuz "from" could be a mix of English
	               // and non-English chars (thus strlen(from) not divisible by 3).
	mbstowcs(ws, from, 1+n); // count the NULL terminator!

	char unicode_hexs[4*n]; // similarly, minimal length of 4 * (n/3),
	                        // but put 4 * n to be safe.
	int l = wcslen(ws);

	for (int i = 0; i < l; i++) {
// "%4lx" triggers warning on Windows, and "%4x" on Linux. I don't like warnings.
#ifdef __linux__
		sprintf(unicode_hexs + 4*i, "%4lx", ws[i]);

#elif __CYGWIN__
		sprintf(unicode_hexs + 4*i, "%4x", ws[i]);

#elif __APPLE__
		// HELP!
#endif
	}

	l = strlen(unicode_hexs);
	for (int i = 0; i < l; i += 4) {
		to[i]   = unicode_hexs[i+2];
		to[i+1] = unicode_hexs[i+3];

		to[i+2] = (unicode_hexs[i]   == ' ') ? '0' : unicode_hexs[i];
		to[i+3] = (unicode_hexs[i+1] == ' ') ? '0' : unicode_hexs[i+1];
	}
	to[l] = '\0';
}

// Acknowledgement:
//   bulk of this function was in Java --- adapted from
//   http://www.iscripts.org/forum.php?mod=viewthread&tid=85
//   in fact, it is mostly C (the bit ops). Translation was no sweat. XD
// Notes:
//   1. this func expects "artist" and "title" to be char* (thus in utf-8 format).
int tt_code (char *artist, char *title, int lrcId) {
	char bytes[512];
	sprintf(bytes, "%s%s", artist, title);

	int i1 = 0, i2 = 0, i3 = 0;

	i1 = (lrcId & 0xFF00) >> 8;
	if ((lrcId & 0xFF0000) == 0) {
		i3 = 0xFF & ~i1;
	} else {
		i3 = 0xFF & ((lrcId & 0xFF0000) >> 16);
	}
	i3 = i3 | ((0xFF & lrcId) << 8);
	i3 = i3 << 8;
	i3 = i3 | (0xFF & i1);
	i3 = i3 << 8;
	if ((lrcId & 0xFF000000) == 0) {
		i3 = i3 | (0xFF & (~lrcId));
	} else {
		i3 = i3 | (0xFF & (lrcId >> 24));
	}
	int l = strlen(bytes);
	int u = l - 1;
	int c = 0;
	while (u >= 0) {
		c = bytes[u];
		if (c >= 0x80)
			c = c - 0x100;
		i1 = c + i2;
		i2 = i2 << (u % 2 + 4);
		i2 = i1 + i2;
		u -= 1;
	}
	u = 0;
	i1 = 0;
	int i4;
	while (u < l) {
		c = bytes[u];
		if (c >= 128)
			c = c - 256;
		i4 = c + i1;
		i1 = i1 << (u % 2 + 3);
		i1 = i1 + i4;
		u += 1;
	}
	int i5 = i2 ^ i3;
	i5 = i5 + (i1 | lrcId);
	i5 = i5 * (i1 | i3);
	i5 = i5 * (i2 ^ lrcId);
	return i5;
}

// CURLOPT_WRITEFUNCTION:
// Pass a pointer to a function that matches the following prototype:
//    size_t function( char *ptr, size_t size, size_t nmemb, void *userdata);

// This function gets called by libcurl as soon as there is data
// received that needs to be saved. The size of the data pointed to
// by "ptr" is size multiplied with "nmemb", it will not be zero
// terminated. Return the number of bytes actually taken care of. If
// that amount differs from the amount passed to your function,
// it'll signal an error to the library. This will abort the
// transfer and return CURLE_WRITE_ERROR.
size_t proc_xml (char *ptr, size_t size, size_t nmemb, void *userdata) {
	size_t l = size * nmemb;
	strncpy(search_res, ptr, l);
	return l;
}

void str_tolower (char *s) {
	int l = strlen(s);
	for (int i = 0; i < l; i++)
		s[i] = tolower((int) s[i]);
}

void str_rmchar (char *from, char *to, char c) {
	int l = strlen(from);
	int i1 = 0, i2 = 0;

	while (i1 < l) {
		if (from[i1] == c) {
			i1++;

		} else {
			to[i2] = from[i1];
			i1++;
			i2++;
		}
	}
	to[i2] = '\0';
}

int main (int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Not enough args. usage: %s <artist> <title>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	setlocale(LC_ALL, "en_US.utf8");


	/***** SEARCH *****/
	int l1 = strlen(argv[1]);
	char artist[4 * l1];
	char t1[l1];
	str_tolower(argv[1]);
	str_rmchar(argv[1], t1, ' ');
	str_rmchar(t1, argv[1], '\'');
	uni_encode(argv[1], artist);

	int l2 = strlen(argv[2]);
	char title[4 * l2];
	char t2[l2];
	str_tolower(argv[2]);
	str_rmchar(argv[2], t2, ' ');
	str_rmchar(t2, argv[2], '\'');
	uni_encode(argv[2], title);

	char url_s[512];
	sprintf(url_s, URL_SEARCH_FMTSTR, artist, title);

	curl_global_init(CURL_GLOBAL_ALL);

	CURL *curl0;
	curl0 = curl_easy_init();
	curl_easy_setopt(curl0, CURLOPT_URL, url_s);
	curl_easy_setopt(curl0, CURLOPT_WRITEFUNCTION, &proc_xml);

	curl_easy_perform(curl0);


	/***** PARSE SEARCH RESULTS *****/
	fprintf(stderr, "%s\n", search_res);

	xmlDocPtr res_xml = xmlParseMemory(search_res, strlen(search_res));
	if (res_xml == NULL ) {
		// document not parsed successfully
		fprintf(stderr,"Error: bad XML. Quiting.\n");
		exit(EXIT_FAILURE);
	}

	xmlNodePtr node0 = xmlDocGetRootElement(res_xml);
	if (node0 == NULL) {
		// empty XML document
		fprintf(stderr, "Error: empty web response. Quiting.\n");
		xmlFreeDoc(res_xml);
		exit(EXIT_FAILURE);
	}

	if (xmlStrcmp(node0->name, (const xmlChar *) "result")) {
		fprintf(stderr, "Warning: XML root node name \"%s\" != \"result\", continuing...\n", node0->name);
	}

	xmlAttr *a;
	struct song_info si[10]; // 10 results returned at most. I could be wrong.
	int n_res = 0;

	node0 = node0->xmlChildrenNode;
	while (node0 != NULL) {
		if (xmlStrcmp(node0->name, (const xmlChar *) "lrc")) {
			// fprintf(stderr, "Warning: XML child node name \"%s\" != \"lrc\", continuing...\n", node0->name);
			node0 = node0->next;
			continue;
		}

		a = node0->properties;
		while (a && a->name && a->children) {
			xmlChar *val = xmlNodeListGetString(a->doc, a->children, 1);

			if        (xmlStrcmp(a->name, (const xmlChar *) "id") == 0) {
				si[n_res].id = atoi((const char *) val);

			} else if (xmlStrcmp(a->name, (const xmlChar *) "artist") == 0) {
				strncpy(si[n_res].artist, (const char *) val, 128);

			} else if (xmlStrcmp(a->name, (const xmlChar *) "title") == 0) {
				strncpy(si[n_res].title,  (const char *) val, 256);
			}
			xmlFree(val);
			a = a->next;
		}

		node0 = node0->next;
		n_res++;
	}
	xmlFreeDoc(res_xml);
	xmlFreeNode(node0);

	int chosen_id = 0;
	if (n_res > 1) {
		fprintf(stderr, "Choose one (the index, 0 based): ");
		if (argc >= 4)
			chosen_id = atoi(argv[3]);
		else
			scanf("%d", &chosen_id);

		if (chosen_id >= n_res)
			chosen_id = n_res - 1;
		else if (chosen_id < 0)
			chosen_id = 0;

	} else if (n_res == 0) {

		fprintf(stderr, "Notice: No results were returned. Quiting.\n");
		exit(EXIT_FAILURE);
	}


	/***** DOWNLOAD *****/
	char url_d[512];
	int lrcId = si[chosen_id].id;
	sprintf(url_d, URL_DOWNLOAD_FMTSTR,
	        lrcId,
	        tt_code(si[chosen_id].artist, si[chosen_id].title, lrcId));

	curl_easy_setopt(curl0, CURLOPT_URL, url_d);
	curl_easy_setopt(curl0, CURLOPT_WRITEFUNCTION, NULL);

	curl_easy_perform(curl0);
	// curl_easy_setopt(curl0, CURLOPT_WRITEFUNCTION, saveLyrics);

	curl_easy_cleanup(curl0);
	curl_global_cleanup();
	return 0;
}
