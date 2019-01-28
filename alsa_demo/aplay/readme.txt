./aplay -t raw -c 2 -f S16_LE -r 8000 8k.pcm

./arecord -D plughw:1,0 -d 100 -c 1 -f S16_LE -r 8000 -t raw foobar.raw

### set 1st soundcard PCM volume left volume=80%, right volume=60%
./amixer -c 0 set PCM 80%,60%
### set 1st soundcard element volume=80%, right volume=60%
./amixer cset numid=1 80%,60%
