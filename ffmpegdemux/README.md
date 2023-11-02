# whale
This is a repository for media framework.

How to convert audio_dump.pcm file to wav file.
ffmpeg -f f32le -ar 48000 -ac 2 -i audio_dump.pcm audio_dump_2.wav