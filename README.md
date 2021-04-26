# Nihongo
Japanese Translation Helper

I wrote this program back in 2010 as a way of helping me to learn Japanese grammar.  
Japanese text has no spaces between words, so one of the first things you need to
do is break the text into words.  This program takes a rather simplistic approach to 
this: it scans the text and attempts to find the longest match in a dictionary to the
characters it is inspecting at the moment.  It makes use of the publicly available
"EDICT" Japanese dictionary file.  After reading the raw dictionary, it expands the
dictionary by conjugating all verbs and adjectives.

As a sanity check, a publicly available Japanese parser called JUMAN is also incorporated
as an option.

I would have gone further with the program, but realized that I really needed to learn
to read Japanese for myself, and so began working on various translation projects.  But I
continue to use this applet to do the grunt work of dictionary lookups.

To build this project, you will need Microsoft Visual Studio 2019 (Community Edition or 
better), and you will need to install the Wix Toolset, version 3.14 or later, along with 
the appropriate Visual Studio extension.

The two principle projects here are "NIHONGO" and "HONYAKU_NO_HOJO".  The former is a command-line
version of the helper which is now mainly used to create the dictionary file.  The latter
is an MFC dialog applet used to interactively perform dictionary lookups.  Note that the 
Analyzer and Parser2 projects were never completed.

System requirements for running: Windows X64, 16Gb RAM or more.

--Rick Papo
