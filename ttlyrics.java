/**
 * part of the code adapted from http://www.iscripts.org/forum.php?mod=viewthread&tid=85
 * origin author: 林俊海(ialvin.cn) 广东·普宁·里湖

 * compilation: javac ttlyrics.java
 * example use: java  ttlyrics beyond 秘密警察
 */

import java.io.IOException;
import java.io.StringReader;
import java.io.UnsupportedEncodingException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.net.*;
import java.io.*;
import javax.xml.parsers.*;

import org.xml.sax.*;
import org.xml.sax.helpers.*;


class myDefaultHandler extends DefaultHandler {
	public ArrayList<HashMap<String, String>> matches;

	public myDefaultHandler(ArrayList<HashMap<String, String>> m) {
		super();
		this.matches = m;
	}

	public void startElement(String uri, String localName, String qName, Attributes attributes) throws SAXException {
		if (uri.equals("") && "lrc" == qName) {
			HashMap<String, String> hm = new HashMap<String, String>();
			for (int i=0; i<attributes.getLength(); i++)
				hm.put(attributes.getLocalName(i), attributes.getValue(i));
			matches.add(hm);
		}
	}
};

class URLConnectionReader {
	URL url;

	public URLConnectionReader (String s) throws MalformedURLException { this.url = new URL(s); }
	public void setUrl (String s) throws MalformedURLException         { this.url = new URL(s); }

	public String read () throws IOException {
		URLConnection conn = url.openConnection();
		BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()));
		StringBuilder sb = new StringBuilder();

		char[] buf = new char[4096];
		int len;
		while ((len = reader.read(buf)) > -1)
			sb.append(buf, 0, len);

		reader.close();
		return sb.toString();
	}
}

class ttlyrics {
	public static String search_and_download (String title, String artist)
		throws NumberFormatException, IOException, SAXException, ParserConfigurationException {
		// lrccnc.ttplayer.com
		// ttlrcct.qianqian.com
		// ttlrcct2.qianqian.com

		// ------------	 Search	 ------------
		String searchUrl = "http://ttlrcct.qianqian.com/dll/lyricsvr.dll?sh?Artist={ar}&Title={ti}&Flags=0";
		String result = "";

		searchUrl = searchUrl.replace("{ar}", encode(artist))
			.replace("{ti}", encode(title));
		URLConnectionReader ucr = new URLConnectionReader(searchUrl);

		try {
			result = ucr.read();

		} catch (Exception e) {
			e.printStackTrace();
		}

		SAXParserFactory factory = SAXParserFactory.newInstance();
		SAXParser saxParser = factory.newSAXParser();
		ArrayList<HashMap<String, String>> matches = new ArrayList<HashMap<String,String>>();
		DefaultHandler handler = new myDefaultHandler(matches);

		InputStream xmlStream = new ByteArrayInputStream(result.getBytes("utf-8"));
		saxParser.parse(new InputSource(xmlStream), handler);


		// ------------ Download ------------
		String downloadURL = "http://ttlrcct2.qianqian.com/dll/lyricsvr.dll?dl?Id={id}&Code={code}";
		String songId = matches.get(0).get("id");
		artist = matches.get(0).get("artist");
		title = matches.get(0).get("title");

		downloadURL = downloadURL.replace("{id}", songId)
			.replace("{code}", verificationCode(artist, title, Integer.parseInt(songId, 10)));

		ucr.setUrl(downloadURL);
		return ucr.read();
	}

	// *le crack. thanks to the original authors/communities again.
	public static String verificationCode (String artist, String title, int lrcId)
		throws UnsupportedEncodingException {

		byte[] bytes = (artist + title).getBytes("UTF-8");
		int[] song = new int[bytes.length];
		for (int i = 0; i < bytes.length; i++)
			song[i] = bytes[i] & 0xff;
		int i1 = 0, i2 = 0, i3 = 0;
		i1 = (lrcId & 0xFF00) >> 8;
		if ((lrcId & 0xFF0000) == 0) {
			i3 = 0xFF & ~i1;
		} else {
			i3 = 0xFF & ((lrcId & 0x00FF0000) >> 16);
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
		int uBound = bytes.length - 1;
		while (uBound >= 0) {
			int c = song[uBound];
			if (c >= 0x80)
				c = c - 0x100;
			i1 = c + i2;
			i2 = i2 << (uBound % 2 + 4);
			i2 = i1 + i2;
			uBound -= 1;
		}
		uBound = 0;
		i1 = 0;
		while (uBound <= bytes.length - 1) {
			int c = song[uBound];
			if (c >= 128)
				c = c - 256;
			int i4 = c + i1;
			i1 = i1 << (uBound % 2 + 3);
			i1 = i1 + i4;
			uBound += 1;
		}
		int i5 = i2 ^ i3;
		i5 = i5 + (i1 | lrcId);
		i5 = i5 * (i1 | i3);
		i5 = i5 * (i2 ^ lrcId);
		return String.valueOf(i5);
	}

	// given a (possibly multi-lingual) string,
	// return a string of hex digits, representing the unicode encoding of 's', ordered from low to high according to the memory address.
	private static String encode(String s) {
		if (s == null)
			s = "";
		else
			s = s.replace(" ", "").replace("'", "").toLowerCase();

		byte[] bytes = null;
		try {
			bytes = s.getBytes("utf-16le");

		} catch (UnsupportedEncodingException e) {
			e.printStackTrace();
			bytes = s.getBytes();
		}
		return stringify(bytes);
	}


	final private static char[] digit = { '0', '1', '2', '3', '4', '5', '6',
										  '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	private static String stringify(byte[] bytes) {
		char[] s = new char[2];
		StringBuilder builder = new StringBuilder();
		for (byte b : bytes) {
			s[0] = digit[(b >>> 4) & 0X0F];
			s[1] = digit[b & 0X0F];
			builder.append(s);
		}
		return builder.toString();
	}


	public static void main(String[] args) throws Exception {
		String artist = args[0];
		String title = args[1];
		System.out.println(search_and_download(title, artist));
	}
}
