
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE



#include<stdio.h>
#include<errno.h>
#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<ctype.h>
#include<sys/ioctl.h>
#include<string.h>
#include<sys/types.h>



 //stores rows of text to be  displayed
typedef struct erow{
    int size;
    char *chars;
}erow;

struct editor_config{
    int crsr_x, crsr_y; //x and y position of cursor
    struct termios orig_termios;
    int row_offset;
    int col_offset;
    int screenrows;
    int screencols;
    int numrows;
    erow * row;
} E;


// definitions
#define SECANT_VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)  // defines control key  + key k as CTRL(k)

enum editor_key{
    ARROW_LEFT =1000,
    ARROW_RIGHT,
    ARROW_UP ,
    ARROW_DOWN,
    DEL_KEY,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY
};

void die(const char * s){

    write(STDOUT_FILENO, "\x1b[2J",4); //clear out screen -
    write(STDOUT_FILENO, "\x1b[H",3); //                  on exit

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

int read_key(){
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1 && errno != EAGAIN) die("read");
    }
    if (c == '\x1b'){
        char seq[3];
    
        if(read(STDIN_FILENO, &seq[0], 1)!=1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';


        if(seq[0] == '['){
            if(seq[1]>='0' && seq[1]<='9'){
                if(read(STDIN_FILENO, &seq[2], 1)!=1) return '\x1b';

                if(seq[2]=='~'){
                    switch(seq[1]){
                        case '1' : return HOME_KEY;
                        case '3' : return DEL_KEY;
                        case '4' : return END_KEY;
                        case '5' : return PAGE_UP;
                        case '6' : return PAGE_DOWN;
                        case '7' : return HOME_KEY;
                        case '8' : return END_KEY;
                    }
                }
            }
            else{
                switch(seq[1]){
                    case 'A' : return ARROW_LEFT;
                    case 'B' : return ARROW_RIGHT;
                    case 'C' : return ARROW_DOWN;
                    case 'D' : return ARROW_UP;
                    case 'H' : return HOME_KEY;
                    case 'F' : return END_KEY;
                }
            }
        } else if(seq[0] == 'O'){
            switch(seq[1]){
                case 'H' : return HOME_KEY;
                case 'F' : return END_KEY;
            }
        }
        return '\x1b';
    }else {
        return c;
    }

    
}

int get_cursor_position(int * rows, int * cols){
    char buff[32];
    unsigned int i = 0;

    if(write(STDOUT_FILENO, "\x1b[6n",4)!=4) return -1;
    
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

void editor_movecursor(int key){

    erow *row = (E.crsr_y>=E.numrows) ? NULL : &E.row[E.crsr_y];
    switch(key){
        case ARROW_UP:
            if(E.crsr_x!=0){
                E.crsr_x--;
            }
            break;
        case ARROW_DOWN:
            if(row && E.crsr_x < row ->size){
                E.crsr_x++;
            }
            break;
        case ARROW_LEFT:
            if(E.crsr_y!=0){
                E.crsr_y--;
            }
            break;
        case ARROW_RIGHT:
        if(E.crsr_y<E.numrows){
            E.crsr_y++;
        }
        break;
    }
     row = (E.crsr_y >= E.numrows) ? NULL : &E.row[E.crsr_y];
     int rowlen = row ? row->size : 0;
     if(E.crsr_x > rowlen){
        E.crsr_x = rowlen;
     }
}



// function to process the input keystrokes

void process_keystrokes(){
    __int16_t c = read_key();

    switch(c){
        case CTRL_KEY('q'):
            exit(0);
            break;
        
        case HOME_KEY:
            E.crsr_x = 0;
            break;
        case END_KEY:
            E.crsr_x = E.screencols-1;
            break;

        case PAGE_UP:
        case PAGE_DOWN:
        {
            int times = E.screenrows;
            while(times--)
            {
                editor_movecursor(c== PAGE_UP? ARROW_UP:ARROW_DOWN);
            }
        }
        break;

        case ARROW_UP:
        case ARROW_RIGHT:
        case ARROW_LEFT:
        case ARROW_DOWN:
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
 

void editor_append_row(char *s, size_t len){
    E.row = realloc(E.row, sizeof(erow) *(E.numrows + 1));


    int at = E.numrows;
    E.row[at].size = len;
    E.row[at].chars = malloc(len+1);
    memcpy(E.row[at].chars, s, len);
    E.row->chars[len] = '\0';
    E.numrows ++;
}
void editor_open(char *filename){

    FILE *fp = fopen(filename,"r");
    if(!fp) die("fopen");
    char *line = NULL;
    size_t linecap = 0;
    ssize_t line_len;

    while((line_len = getline(&line, &linecap, fp))!= -1){
        while(line_len > 0 && (line[line_len - 1] == '\n' || line[line_len-1]== '\r')) line_len--;
        editor_append_row(line, line_len);
    }

    free(line);
    free(fp);




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

void editor_scroll(){
    if(E.crsr_y < E.row_offset){
        E.row_offset = E.crsr_y;
    }
    if(E.crsr_y >= E.row_offset + E.screenrows){
        E.row_offset = E.crsr_y - E.screenrows + 1;
    }
    if(E.crsr_x < E.col_offset){
        E.col_offset = E.crsr_x;
    }
    if(E.crsr_x>= E.col_offset + E.screencols){
        E.col_offset = E.crsr_x - E.screencols + 1;
    }
}

void draw_tildes(struct abuf *ab){
    int y;
    for(y = 0; y < E.screenrows ; y++){
        int effective_row = y + E.row_offset;
        if(effective_row>= E.numrows){
            if( y == E.screenrows/3 && E.numrows == 0){
                char Welcome[80];
                int welcomelen = snprintf(Welcome, sizeof(Welcome), "secant texteditor --version %s", SECANT_VERSION);
                if(welcomelen>E.screencols) welcomelen=E.screencols;
                int padding = (E.screencols-welcomelen)/2;
                if(padding){
                    ab_append(ab, "~",1);
                    padding--;
                }
                while(padding--) ab_append(ab, " ", 1);
                ab_append(ab, Welcome, welcomelen);
            }else{
                ab_append(ab, "~", 1);
            }
        }else{
            int  len = E.row[effective_row].size - E.col_offset;
            if(len<0) len =0;
            if(len > E.screencols) len = E.screencols;
            ab_append(ab, &E.row[effective_row].chars[E.col_offset], len);
        }

        ab_append(ab, "\x1b[K", 3);

        if(y<E.screenrows - 1){
            ab_append(ab, "\r\n", 2);
        }
    }
}

// refreshes screen

void editor_refresh_screen(){
    editor_scroll();
    struct abuf ab = ABUF_INIT;

    ab_append(&ab, "\x1b[25?l", 6);

    // ab_append(&ab, "\x1b[2J", 4);
    ab_append(&ab, "\x1b[H", 3);

    draw_tildes(&ab);

    char buff[32];

    snprintf(buff, sizeof(buff), "\x1b[%d;%dH", (E.crsr_y-E.row_offset)+1, E.crsr_x+1);
    ab_append(&ab, buff, strlen(buff));

    // ab_append(&ab, "\x1b[H", 3);
    ab_append(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    ab_free(&ab);

}

void init_editor(){
    E.row = NULL;
    E.crsr_x = 0;
    E.crsr_y = 0;
    E.numrows = 0;
    E.row_offset = 0;
    E.col_offset = 0;
    if (get_window_size(&E.screenrows, &E.screencols) == -1) die("get_window_size");
}


int main(int argc, char*argv[]){
    enable_raw_mode();
    init_editor();

    if(argc >=2){
        editor_open(argv[1]);
    }
    
    while(1){
        process_keystrokes();
        editor_refresh_screen();
    }


    return 0;
}