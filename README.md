# orgmusic
A command-line music organizer, organizing into a directory tree based on file metadata.

## Run
orgmusic ~/Music ~/music

orgmusic --genre ~/Music

## Issues
My first attempt used libmediainfo0v5:amd64 25.04+dfsg-1 in Debian 13 (trixie) but it kept
crashing. So I pulled the upstream source from 
https://github.com/MediaArea/MediaInfoLib.git
and confirmed that the issue is fixed there.
