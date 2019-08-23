#define FILENAME "mytext.txt"


typedef struct Message{
	char lineSent[1024]; //instead of char* lineSent;
	char sender_pid[7]; //pids can be set to any value up to 2^15 = 4194304 which means 7 digits. Servers might have a larger limit but we don't care for this at this momment.
        int endOfMessages; // starts with 0. 0: the end has not came yet 1:this is the end
}messageSent;

int countLines(void);
char* pickRandomLine(void);
char* str2md5(const char* str, int length);
