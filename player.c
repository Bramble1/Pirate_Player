
#include"player.h"

/*Under the assumption that the user will be entering time in seconds, so convert to miliseconds before
 * using any of the functions which work with miliseconds (returns the miliseconds)
 * @seconds - the seconds to convert to miliseconds*/
int64_t convert_to_ms(int64_t seconds)
{
	int64_t result = seconds * 1000;
	return result;
}
/*setup the vlc backend (returns -1 on failure and 1 on success) 
 * @filename - the audiobook to open*/
int init_vlc(char *filename)
{
//	m = mmap(NULL,sizeof m,PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS,-1,0);
	mp = mmap(NULL,sizeof mp,PROT_READ | PROT_WRITE , MAP_SHARED | MAP_ANONYMOUS,-1,0);

	/*load the engine*/
	inst = libvlc_new(0,NULL);
	if(inst==NULL)
	{
		perror("libvlc_new() returns NULL:");
		return -1;
	}

	/*set error messages to log file*/
	FILE *fd = fopen("LOG","a+");
	libvlc_log_set_file(inst,fd);	


	/*create the file to play*/
	m = libvlc_media_new_path(inst,filename);
	if(m==NULL)
	{
		perror("libvlc_media_new_path(): returns NULL:");
		return -1;
	}

	/*create a media play playing environment*/
	mp = libvlc_media_player_new_from_media(m);
	if(mp==NULL)
	{
		perror("libvlc_media_player_new_from_media() returns NULL:");
		return -1;
	}

	/*release_media*/
	libvlc_media_release(m);

	return 1;
}
/*get the length of audiobook to determine how long the program needs to run for (returns the total_length)
 * */
int64_t get_total_audio_length()
{
	int64_t total_length = libvlc_media_player_get_length(mp);
	return total_length;
}

/*get the current time position of the playing audiobook (returns the current_time)*/
int64_t get_current_time()
{
	int64_t current_time = libvlc_media_player_get_time(mp);
	return current_time;
}

/*set the current time of audiobook (returns -1 on failure and 1 on success)
 * @m_seconds - the miliseconds to set the audio to*/
void set_current_time(int64_t m_seconds)
{
	libvlc_media_player_set_time(mp,m_seconds);	
}
/*play the audiobook (returns -1 on failure and 1 on success)
 * @position_in_seconds - either 0 or the bookmark if an record exists , thus meaning a bookmark already exists*/
int play(int64_t position_in_m_seconds)
{
	int error;
	/*run the media player*/
	if(libvlc_media_player_play(mp)==-1)
	{
		perror("libvlc_media_player_player(mp): failed:");
		return -1;
	}
	/*set the current time or it will be taken from get_bookmark()*/
//	libvlc_media_player_set_time(mp,position_in_m_seconds);
	set_current_time(position_in_m_seconds);

	return 1;

}

/*pause_resume the audiobook
 * @pause_status - used to determine whether to pause or resume the audio*/
void pause_resume(int *pause_status)
{ //under the assumption by default pause_status = 1
	if(*pause_status==1) //pause
	{
		libvlc_media_player_set_pause(mp,*pause_status);
		*pause_status = 0;
		status_pause = 1;
	}
	else if(*pause_status==0) //resume
	{
		libvlc_media_player_set_pause(mp,*pause_status);
		*pause_status = 1;
		status_pause = 0;
	}

}
/*safely shutdown the vlc backend and eject the media (returns -1 on failure and 1 on success)*/
void quit()
{
	/*stop playing*/
	libvlc_media_player_stop(mp);
	

	/*free memory*/
	libvlc_media_player_release(mp);
	libvlc_release(inst);

}

/*rewind the audio by seconds (returns -1 on failure and 1 on success)
 * @seconds - the seconds to rewind the audio by*/
void rewind_audio(int64_t m_seconds)
{
	int64_t audio_position = get_current_time();
	audio_position -= m_seconds;
	usleep(10);
	set_current_time(audio_position);

}
/*forward the audio by seconds (returns -1 on failure and 1 on success)
 * @seconds - the seconds to forward the audio by*/
void forward_audio(int64_t m_seconds)
{
	int64_t total_audio = get_total_audio_length();
	int64_t audio_position = get_current_time();
	audio_position += m_seconds;

	if(audio_position>=total_audio)
	{
		//printf("audio_position = %d\n",audio_position);
		exit(EXIT_FAILURE);
	}
	else
	{
		usleep(10);
		set_current_time(audio_position);
	}
}


/*Functions for bookmarking...*/
void write_bookmark(Record *book)
{
	Record *duplicate;
	int dup_exists=0;
	int fd;

	fd=open(BOOKMARKS,O_RDWR|O_CREAT|O_APPEND,S_IRWXU);
	if(fd==-1)
	{
		perror("open:");
		exit(EXIT_FAILURE);
	}

	struct stat st;
	stat(BOOKMARKS,&st);

	if(st.st_size==0)
		write(fd,book,sizeof(Record));
	else
	{
		/*check for duplication*/
		duplicate=(Record*)mmap(0,sizeof(Record),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

		for(int i=0;i<st.st_size;i++)
			if(strcmp(duplicate[i].name,book->name)==0)
			{
				printf("[!]Entry exists,overwriting.\n");
				duplicate[i].timestamp=book->timestamp;
				dup_exists=1;
				msync(duplicate,sizeof(Record),MS_SYNC);
				break;
			}
		if(dup_exists==0)
			write(fd,book,sizeof(Record));
	}
	close(fd);
}

int64_t read_bookmark(char *bookname)
{
	int fd;
	Record book;
	double timestamp=-1;
	
	fd=open(BOOKMARKS,O_RDWR);
	if(fd==-1)
	{
		perror("open:");
		exit(EXIT_FAILURE);
	}

	while(read(fd,&book,sizeof(book)))
	{
		if(strcmp(book.name,bookname)==0)
		{
			timestamp=book.timestamp;
			break;
		}
	}
	return timestamp;
}



/*time checking*/
void time_regex(char *buf)
{
	int byte_length = (int)strlen(buf);
	int length=0;

	if(byte_length!=8)
	{
		printf("[!]Insufficient number of bytes.\n");
		exit(EXIT_FAILURE);
	}
	else if(byte_length==8)
	{
		if((buf[0]-'0'<0 || buf[0]-'0'>9) || (buf[1]-'0'<0 || buf[1]-'0'>9))
		{
			printf("[!]hh must be numerical characters.\n");
			exit(EXIT_FAILURE);
		}
		if(buf[2]!=':')
		{
			printf("[!]Incorrect format.[+]\'hh:mm:ss\'\n");
			exit(EXIT_FAILURE);
		}
		if((buf[3]-'0'<0 || buf[3]-'0'>9) || (buf[4]-'0'<0 || buf[4]-'0'>9))
		{
			printf("[!]mm must be numerical characters.\n");
			exit(EXIT_FAILURE);
		}
		if(buf[5]!=':')
		{
			printf("[!]Incorrect format.[+]\'hh:mm:ss\'\n");
			exit(EXIT_FAILURE);
		}
		if((buf[6]-'0'<0 || buf[6]-'0'>9) || (buf[7]-'0'<0 || buf[7]-'0'>9))
		{
			printf("[!]ss must be numerical characters.\n");
			exit(EXIT_FAILURE);
		}
	}
}


void calculate_time(int flag,char *buffer,int *perc,int *t_total)
{
	int total_audio; //t_total;
	int f_hour,f_minute,f_second,whole;
	double remainder,number,total_seconds,total_minutes,total_hours;
	int percentage;
	//char buf1[100]="\0";
	//char buf2[100]="\0";
	
	sleep(1);

	/*flag==0 then total_audio_length if 1 then current_audio_time*/
	if(flag==0)
		*t_total = total_audio = get_total_audio_length();
	else if(flag==1)
		total_audio = get_current_time(); //total_audio = get_total_audio_length();

	/*convert to seconds*/
	total_seconds = total_audio * pow(10,-3);

	if(flag==1)
		percentage = total_seconds / *t_total * 100;


	total_minutes = total_seconds * pow(60,-1);
	whole = (int)total_minutes;
	remainder = total_minutes - whole;
	number = remainder * 60;
	f_second = (int)number;

	/*convert to hour and minutes*/
	total_hours = total_minutes * pow(60,-1);
	whole = (int)total_hours;
	remainder = total_hours - whole;
	number = remainder * 60;
	f_minute = (int)number;
	f_hour = whole;

	sprintf(buffer,"%d:%d:%d",f_hour,f_minute,f_second);
	
	if(flag==0){
		*t_total = total_seconds;
	}
	
}

void get_system_time(Time *systime)
{
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	systime->hour=timeinfo->tm_hour;
	systime->minute=timeinfo->tm_min;
	systime->second=timeinfo->tm_sec;

	//*sys_hour = timeinfo->tm_hour;
	//*sys_minute = timeinfo->tm_min;
}

/*calculate the time for the audiobook to finish and bookmark based on the duration to play*/
void calculate_end_time(char *duration,Time *endtime)
{

	Time systime;

	get_system_time(&systime);


	/*convert system time to seconds*/
	systime.total_seconds = (systime.hour*pow(60,2))+(systime.minute*60)+systime.second;

	/*operate on the duration="hh:mm:ss"*/
	endtime->hour=duration[0]-'0';endtime->hour*=10;endtime->hour+=duration[1]-'0';
	endtime->minute=duration[3]-'0';endtime->minute*=10;endtime->minute+=duration[4]-'0';
	endtime->second=duration[6]-'0';endtime->second*=10;endtime->second+=duration[7]-'0';

	endtime->total_seconds=(endtime->hour*pow(60,2))+(endtime->minute*60)+endtime->second;

	endtime->total_seconds += systime.total_seconds;

	/*now revert back to the hour,second and minute of the new endtime and overrite
	 * the endtime variables.*/
	double fhour,fminute,fsecond;

	fsecond=(int)endtime->total_seconds % 60;
	fhour=(endtime->total_seconds * pow(60,-1))*pow(60,-1);
	fminute=(int)(fhour*60) % 60;

	endtime->hour=fhour;
	endtime->minute=fminute;
	endtime->second=fsecond;

	
}


/*display the current progress of audiofile(Need to figure out how to pass arguments
 * but for now just create a global record object with name from args.*/
void *audio_status(void *args)
{
//	int status_pause = 0;

	Time systime;Time endtime;


	calculate_end_time(duration,&endtime);
//	printf("estimated endtime = %d:%d:%d\n\n",endtime.hour,endtime.minute,endtime.second);

//	exit(EXIT_SUCCESS);

	int total_audio,t_total;
	int f_hour,f_minute,f_second,whole;
	double remainder,number,total_seconds,total_minutes,total_hours;
	int percentage;
	char buf1[100]="\0";
	char buf2[100]="\0";

	sleep(1);

	/*workout total audio*/
	t_total = total_audio = get_total_audio_length();

	/*convert to seconds*/
	total_seconds = total_audio * pow(10,-3);
	total_minutes = total_seconds * pow(60,-1);
	whole = (int)total_minutes;
	remainder = total_minutes - whole;
	number = remainder * 60;
	f_second = (int)number;

	/*convert hour and minutes*/
	total_hours = total_minutes * pow(60,-1);
	whole = (int)total_hours;
	remainder = total_hours - whole;
	number = remainder * 60;
	f_minute = (int)number;
	f_hour = whole;

	sprintf(buf2,"%d:%d:%d",f_hour,f_minute,f_second);
	t_total = total_seconds;

	int once=0;

	while(1)
	{

		if(status_pause!=1)
		{
			once=0;
			/*get current timestamp*/
			total_audio = get_current_time();

			/*convert to seconds*/
			total_seconds = total_audio * pow(10,-3);
			percentage = total_seconds / t_total * 100;
			total_minutes = total_seconds * pow(60,-1);
			whole=(int)total_minutes;
			remainder = total_minutes - whole;
			number = remainder * 60;
			f_second = (int)number;
		
			/*convert hours and minutes*/
			total_hours = total_minutes * pow(60,-1);
			whole = (int)total_hours;
			remainder = total_hours - whole;
			number = remainder * 60;
			f_minute = (int)number;
			f_hour = whole;

			sprintf(buf1,"%d:%d:%d",f_hour,f_minute,f_second);

			printf("%s/%s (%d%)\n",buf1,buf2,percentage);
			sleep(1);
			system("clear");

			/*also call a function for checking if the duration target calculated
			 * time is equal to the current time of the system.*/
		}
		else
		{
			if(once==0)
			{
				printf("%s/%s (%d%)\n",buf1,buf2,percentage);
				once=1;
			}
		}

		
		/*check time,this could be it's own thread and to sleep,otherwise
		 * it's constantly checking, which isn't good in regards to
		 * system performance. But for now in this prototype,it will
		 * stay until the next iteration,it's not very accurate,may have to
		 * be it' own thread*/
		get_system_time(&systime);

		if(endtime.hour==systime.hour && endtime.minute==systime.minute && endtime.second==systime.second)
		{
			printf("[!]DURATION REACHED,EXITING!");
			printf("time to bookmark=%d\n",get_current_time());

			book.timestamp=get_current_time();

			write_bookmark(&book);

			exit(EXIT_SUCCESS);


			/*we call the bookmark function and then exit. We also need a function
			 * to check the bookmark sheet to see if a bookmark already exists
			 * and if so prompt the user if they wish to play the start or
			 * from the pre-existing bookmark, if a bookmark exists, we overrite it
			 * if not, then we add the entry to the textfile.*/
		}


	}

}
