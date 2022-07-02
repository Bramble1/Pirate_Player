#include<vlc/vlc.h>
#include<stdlib.h>
#include<string.h>
#include<sys/mman.h>
#include<unistd.h>
#include<math.h>
#include<pthread.h>
#include<time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/mman.h>
/*data structures for the following functions*/
libvlc_instance_t *inst;
libvlc_media_player_t *mp;
libvlc_media_t *m;

//mp = mmap(NULL,sizeof mp,PROT_READ | PROT_WRITE , MAP_SHARED | MAP_ANONYMOUS,-1,0);


/*temporary global variables, will be passed to functions when we rewrite
 * the design for version 2.*/
int status_pause;
char *duration;


typedef struct
{
	int hour;
	int minute;
	int second;
	int total_seconds;
}Time;


#define BOOKMARKS "bookmarks.txt"

typedef struct
{
	char name[200];
	int timestamp;
}Record;


/*temporary global for v1*/
Record book;

/*Functions*/
int64_t convert_to_ms(int64_t seconds);
int init_vlc(char *filename);
int64_t get_total_audio_length();
int64_t get_current_time();
void set_current_time(int64_t m_seconds);
int play(int64_t position_in_m_seconds);
void pause_resume(int *pause_status);
void quit();
void rewind_audio(int64_t m_seconds);
void forward_audio(int64_t m_seconds);

void write_bookmark(Record *book);
void play_until_then_bookmark();
int64_t read_bookmark(char *bookname);


/*calculate end time*/


/*time format check hh:mm*/
void time_regex(char *buf);

/*audio status will be called in a seperate thread and looped*/
void *audio_status(void *args);
