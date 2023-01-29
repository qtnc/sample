# Sample: Simple and accessible media player
Sample is a media player, based on BASS library and using WXWidgets for its user interface. It is especially made to be accessible for screen reader users, relatively lightweight, and quite simple to use.
Sample is going to be the successor of [6player](https://github.com/qtnc/6player), although it doesn't yet cover all its features.

## Features
- Play music and audio files in many formats, most popular as well as less popular ones (MP3, OGG, AAC/MP4/M4A, Flac, Wave, MIDI, MOD/XM/S3M/IT, etc.). More than 40 formats thank to BASS extensibility mechanism. Also play the audio of some video files.
- Stream music from URL (HTTP, HTTPS, FTP), as well as stream from zip files without unzipping first
- Manage playlist and quickly find something with the quick filter
- Activate up to two microphones; speak or sing while your medias are played
- Microphones can also record lookback device, allowing you to comment while playing a game or showing how a software works as you do it
- Cast the music and your voice to a shoutcast 1.x, shoutcast 2, icecast or private HTTP server

## History
After a while without development, I finally decided that it was time for a complete rewrite. 6player was in C++03 and using Win32 API directly.
This led sometimes to strange bugs, without saying that C++03 is outdated, that C++17 is so much better, and that people are more and more switching away from windows.
Finally, starting over is a good excuse to experiment while trying to keep the spirit of the original project.
IN this particular case, it's a good occasion to learn WXWidgets, a cross-platform GUI library that produces accessible interfaces

## Dependencies
The following dependencies are needed to compile the application:

- wxWidgets 3.1.3+
- BASS audio library: bass, bass_fx, bassmix, bassmidi, bassenc_mp3/ogg/flac, + optional plugins
- `{fmt}` C++ library

## Download
Latest download can be found here:
http://vrac.quentinc.net/6player2.zip
