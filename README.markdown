Mingus is a frontend for GNU Emacs to the Music Player daemon. The
interface closely, though not strictly, resembles that of ncmpc, which
I had been using before (the main drawback of ncmpc for me was its
apparent lack of support for unicode). It has some advantages over
other clients, mainly in the field of playlist editing. This is
achieved through marking regions as you used to in, say, Dired.
Mingus also provides a point-of-insertion for inserting new
songs. This can be handy to drop a song after the currently playing
one (enqueue), but is more flexible. Mingus is distributed under the
GPL.

w3m
===
Another plus is its integration with [emacs-w3m](http://emacs-w3m.namazu.org/),
in the sense that,
when bumping into a sexy podcast or radio-stream while browsing in
the browser of browsers, you have the ability to add any stream at
the point (and play it directly, if so desired).

Dired
=====
Jump from Mingus to song at point in Dired, and, vice versa, add songs
from Dired to the playlist.

Autocompletion on queries
=========================
Mingus has kind of autocompletion on queries. The browser can show
either the song results or their parent directories. This lets you go
easy on the tagging, and make good use of a simple file-system
structured song collection. Mingus can be configured to use ido-mode
completion.

Bookmarking
===========
Mingus with version >= 0.32 (Fleurette Africaine) has bookmarks to
remember filename and position. This can be very useful with
audiobooks or working with language courses.

Global bookmarking shortcuts `C-x r b` and `C-x r m` are rewritten in
Mingus' global map to provide functionality alike, and `mingus-bookmark-delete`
(`C-x r d`) is added. The function `mingus-bookmark-set` (`C-x r m`) records
the currently playing file and its current elapsed time. Position of the cursor
in the buffer is ignored. `mingus-bookmark-jump` jumps to a bookmark. It
inserts the file into the current playlist when it is not yet there.

Mingus stays home
=================
If mingus stays home, i.e. the daemon mpd is run from the same
computer as the client, mingus-stays-home.el provides an experimental
cd-burning tool (mingus-burns). When you have
[taggit.el](https://github.com/pft/elisp-assorted/blob/master/taggit.el)
and the command-line [taggit program](https://github.com/ft/taggit)
mingus can call that with a selection of songs to perform (batch) tag
editing.

Prerequisites
=============
`libmpdee.el` by R. Ramkumar. Currently mingus runs on GNU Emacs22 and
GNU Emacs 23. If you also want to use mingus-stays-home.el, cdrecord (or
any other command-line cd-burning tool taking files as arguments) might
be needed.

ToDo
====
mingus.el
---------
- Add text-properties (fringe-bitmap?) for point-of-insertion
- Add a search buffer In this search buffer, you would be able to
   filter songs by their attributes. It would have a tabular layout,
   and you could safe-sort the songs. Maybe as-you-type, maybe not.
