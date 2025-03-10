#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>

struct termios orig_termios;

// definitions

#define CTRL_KEY(k) ((k) & 0x1f)  // defines control key  + key k as CTRL(k)


void die(const char * s){

    write(STDOUT_FILENO, "\x1b[2J",4); 
    write(STDOUT_FILENO, "\x1b[2H",3);

    perror(s);
    exit(1);
}
void disable_raw_mode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) ==-1) die("tcsetattr");
    
    // die("tscetattr");
}


void enable_raw_mode(){
    // struct termios raw;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) ==-1) die("tcsetattr");
    atexit(disable_raw_mode);
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    
    // turning off all the flags

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP |  IXON);
    raw.c_cflag &= ~(CS8);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    
    // setting a time-out

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO,  TCSAFLUSH, &raw);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) ==-1) die("tcsetattr");
    
}

// char editorReadKey() {
//     int nread;
//     char c;
//     while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
//       if (nread == -1 && errno != EAGAIN) die("read");
//     }
//     return c;
//   }
// void editorProcessKeypress() {
//     char c = editorReadKey();
//     switch (c) {
//       case CTRL_KEY('q'):
//         exit(0);
//         break;
//     }
// }

// void editorDrawRows() {
//     int y;
//     for (y = 0; y < 24; y++) {
//       write(STDOUT_FILENO, "~\r\n", 3);
//     }
//   }
  



void editor_refresh_screen(){
    write(STDOUT_FILENO, "\x1b[2J",4); 
    write(STDOUT_FILENO, "\x1b[2H",3);
    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);

}
int main(){
    enable_raw_mode();
    
    while(1){
        editorProcessKeypress();
        editor_refresh_screen();
    }


    return 0;
}