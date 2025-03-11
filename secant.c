#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<sys/ioctl.h>
#include<string.h>

// struct termios orig_termios;

struct editor_config{
    int crsr_x, crsr_y; //x and y position of cursor
    struct termios orig_termios;
    int screenrows;
    int screencols;
} E;


// definitions
#define SECANT_VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)  // defines control key  + key k as CTRL(k)



void die(const char * s){

    write(STDOUT_FILENO, "\x1b[2J",4); //clear out screen -
    write(STDOUT_FILENO, "\x1b[2H",3); //                  on exit

    perror(s);
    exit(1);
}
void disable_raw_mode(){
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) ==-1) die("tcsetattr");
    
    // die("tscetattr");
}


void enable_raw_mode(){
    // struct termios raw;
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) ==-1) die("tcsetattr");
    atexit(disable_raw_mode);
    tcgetattr(STDIN_FILENO, &E.orig_termios);
    struct termios raw = E.orig_termios;
    
    // turning off all the flags

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP |  IXON);
    raw.c_cflag &= ~(CS8);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    
    // setting a time-out

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO,  TCSAFLUSH, &raw);
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) ==-1) die("tcsetattr");
    
}
// function to read keyboard inputs

char read_key(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
    }

    return c;
    
}

int get_cursor_position(int * rows, int * cols){
    char buff[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n]",4)!=4) return -1;
    
    while(1< sizeof(buff) -1){
        if(read(STDIN_FILENO, &buff[i], 1)!=1) break;
        if(buff[i]=='R') break;
        i++;
    }
    buff[i] = '\0';

    if(buff[0]!='\x1b' || buff[1]!='[') return -1;
    if(sscanf(&buff[2], "%d%d", rows, cols)!=2) return -1;


    return 0;
}

void editor_movecursor(char key){
    switch(key){
        case 'a':
            E.crsr_x--;
            break;
        case 'd':
            E.crsr_x++;
            break;
        case 'w':
            E.crsr_y--;
            break;
        case 's':
            E.crsr_y++;
            break;
    }
}



// function to process the input keystrokes

void process_keystrokes(){
    char c = read_key();

    switch(c){
        case CTRL_KEY('q'):
            exit(0);
            break;

        case 'a':
        case 'w':
        case 's':
        case 'd':
            editor_movecursor(c);
            break;

    }

}
//apart from the ioctl syscall, we use another method as in the secnd nested if to move  the cursor to the end of the window to get the window size

int get_window_size(int * rows, int * cols){
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws)== -1 || ws.ws_col==0) {
        if(write(STDOUT_FILENO, "\x1b[999C\x1b[999B",12)!=12) return -1;
        return get_cursor_position(rows, cols);
    }
    else{
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }

    
}
struct abuf{
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}
void ab_append(struct abuf *ab, const char *s, int len){
    char *new = realloc(ab->b, ab->len + len);

    if(new == NULL) return;

    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}  

void ab_free(struct abuf *ab){
    free(ab->b);
}

void draw_tildes(struct abuf *ab){
    int y;
    for(y = 0; y < E.screenrows ; y++){
        if( y == E.screenrows/3){
            char Welcome[80];
            int welcomelen = snprintf(Welcome, sizeof(Welcome), "secant texteditor --version %s", SECANT_VERSION);
            if(welcomelen>E.screencols) welcomelen=E.screencols;
            int padding = (E.screencols-welcomelen)/2;if(padding){
                ab_append(ab, "~",1);
                padding--;
            }
            while(padding--) ab_append(ab, " ", 1);
            ab_append(ab, Welcome, welcomelen);
        }else{
            ab_append(ab, "~", 1);
        }

        ab_append(ab, "\x1b[K", 3);

        if(y<E.screenrows - 1){
            ab_append(ab, "\r\n", 2);
        }
    }
}

// refreshes screen

void editor_refresh_screen(){
    struct abuf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[25?l", 6);

    // ab_append(&ab, "\x1b[2J", 4);
    ab_append(&ab, "\x1b[H", 3);

    draw_tildes(&ab);

    char buff[32];

    snprintf(buff, sizeof(buff), "\x1b[%d;%dH", E.crsr_x+1, E.crsr_y+1);
    ab_append(&ab, buff, strlen(buff));

    // ab_append(&ab, "\x1b[H", 3);
    ab_append(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    ab_free(&ab);

}

void init_editor(){

    E.crsr_x = 0;
    E.crsr_y = 0;
    if (get_window_size(&E.screenrows, &E.screencols) == -1) die("get_window_size");
}


int main(){
    enable_raw_mode();
    init_editor();
    
    while(1){
        process_keystrokes();
        editor_refresh_screen();
    }


    return 0;
}