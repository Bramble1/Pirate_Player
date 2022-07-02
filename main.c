#include "player.h"
#include<termios.h>

int main(int argc,char **argv)
{
	if(argc==1)
	{
		printf("./play <Audio_file> <duration = hh:mm:ss>\n");
		return 0;
	}

	time_regex(argv[2]);
	duration = argv[2];
	strcpy(book.name,argv[1]);

	int c;
	static struct termios oldt , newt;
	tcgetattr(STDIN_FILENO,&oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON|ECHO);
	tcsetattr(STDIN_FILENO,TCSANOW,&newt);


	/*setup*/
	if(init_vlc(argv[1])==-1)
	{
		exit(EXIT_FAILURE);
	}

	int start=1;
	int pause=1;
	char input[2]="\0";
//	status_pause = &pause;


	int position=read_bookmark(book.name);
	if(position!=-1)
	{
		printf("Continue where you left off (y/n)?: ");
		fgets(input,2,stdin);
		if(strcmp(input,"y")==0)
		{
			play(convert_to_ms(0));
			set_current_time(position);
		}
		else
			goto start;
	}
	else
start:		play(convert_to_ms(0));

	/*create thread for audiostatus*/
	pthread_t thread_id2;
	pthread_create(&thread_id2,NULL,audio_status,NULL);
	pthread_detach(thread_id2);


	while(1)
	{
		if(c=getchar()=='p')
		{
			pause_resume(&pause);
				
		}
		if(c=getchar()=='f')
		{
			pause_resume(&pause);
			forward_audio(convert_to_ms(10));
			pause_resume(&pause);
		}
		if(c=getchar()=='r')
		{
			rewind_audio(convert_to_ms(10));
		}
		if(c=getchar()=='q')
		{
			pthread_join(thread_id2,NULL);
			pthread_cancel(thread_id2);
			quit();
			break;

		}
		/*test version*/
		
	}
	
	newt=oldt;
	tcsetattr(STDIN_FILENO,TCSANOW,&newt);
	
	return 0;
}
