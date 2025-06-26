# libscl: Spotify Client Library

#### A library for Spotify clients.

Designed as a redesigned replacement for spotify-qt-lib, focusing more on Qt
compatibility rather than standard C++. Very unfinished as long as the lib/
folder still exists.

## Why?

The initial goal of spotify-qt-lib was to provide a common library for any
alternative Spotify client, regardless of UI framework. Some experiments have
been made over the years with other UI frameworks like GTK and FLTK. However,
nothing really took of, as I didn't see any reason to maintain multiple
front-ends, all with the same, or very similar, use case. In the end, it just
added unnecessary complexity with having to translate standard C++ classes to Qt
equivalents, without any real advantage. This new library aims to fix most of
those issues, refocusing on Qt support, and if any other clients show up along
the way, they will just have to link with core Qt libraries.

## When?

During the various releases of spotify-qt v4, more and more components are
planned to be ported over from spotify-qt-lib over to libscl. Could be done
before v5, but could also never be completed, who knows.
