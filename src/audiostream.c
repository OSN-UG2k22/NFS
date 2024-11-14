#include "common.h"

#define BUFFER_SIZE 1024

int stream_file(int client_socket, const char* filename) {
    char buffer[BUFFER_SIZE];
    int file_fd = open(filename, O_RDONLY);
    
    if (file_fd < 0) {
        perror("Failed to open file");
        return -1;
    }

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
        ssize_t bytes_sent = send(client_socket, buffer, bytes_read, 0);
        if (bytes_sent < 0) {
            perror("Failed to send data");
            close(file_fd);
            return -1;
        }
    }

    close(file_fd);
    return 0;
}
 
void stream_music(char* ip, uint16_t port)
{
    libvlc_instance_t * inst;
    libvlc_media_player_t *mp;
    libvlc_media_t *m;

    int running = 1;
    char media_url[256];
    snprintf(media_url, sizeof(media_url), "tcp://%s:%hu", ip, port);

    const char *args[] = {
    "--verbose=0",
    "--no-video",
    "--aout=alsa",
    "--network-caching=1000"  // Increase the network cache to 1000ms
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
    while (libvlc_media_player_is_playing(mp) && running)
    {
        sleep (1);
        int64_t milliseconds = libvlc_media_player_get_time(mp);
        int64_t seconds = milliseconds / 1000;
        int64_t minutes = seconds / 60;
        milliseconds -= seconds * 1000;
        seconds -= minutes * 60;
 
        // printf("Current time: %" PRId64 ":%" PRId64 ":%" PRId64 "\n",
        //        minutes, seconds, milliseconds);
        printf("Press 'q' to quit\n");
        char c = getchar();
        if (c == 'q')
        {
            running = 0;
            break;
        }
    }
 
    /* Stop playing */
    libvlc_media_player_stop (mp);
 
    /* Free the media_player */
    libvlc_media_player_release (mp);
 
    libvlc_release (inst);
}