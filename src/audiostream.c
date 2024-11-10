#include "common.h"
 
void stream_music(char* ip, int port)
{
    libvlc_instance_t * inst;
    libvlc_media_player_t *mp;
    libvlc_media_t *m;

    int running = 1;
    const char* ip = ip;
    int port = port;
    char media_url[256];
    snprintf(media_url, sizeof(media_url), "tcp://%s:%d", ip, port);

    const char *args[] = {
        "--verbose=2",
        "--no-video",  // Disable video output for audio files
        "--aout=pulse"  // Use PulseAudio output (change if needed)
    };
 
    /* Load the VLC engine */
    inst = libvlc_new(sizeof(args) / sizeof(*args), args);
 
    /* Create a new item */
    // m = libvlc_media_new_location("http://mycool.movie.com/test.mov");
    m = libvlc_media_new_location(inst, media_url);
    // m = libvlc_media_new_path(inst,"ff-16b-2c-44100hz.mp4");
 
    /* Create a media player playing environement */
    mp = libvlc_media_player_new_from_media (m);
 
    /* No need to keep the media now */
    libvlc_media_release (m);
 
    /* play the media_player */
    libvlc_media_player_play (mp);
    
    sleep(1);
    while (libvlc_media_player_is_playing(mp))
    {
        sleep (1);
        int64_t milliseconds = libvlc_media_player_get_time(mp);
        int64_t seconds = milliseconds / 1000;
        int64_t minutes = seconds / 60;
        milliseconds -= seconds * 1000;
        seconds -= minutes * 60;
 
        printf("Current time: %" PRId64 ":%" PRId64 ":%" PRId64 "\n",
               minutes, seconds, milliseconds);
    }
 
    /* Stop playing */
    libvlc_media_player_stop (mp);
 
    /* Free the media_player */
    libvlc_media_player_release (mp);
 
    libvlc_release (inst);
 
    return 0;
}