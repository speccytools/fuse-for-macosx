#include <config.h>

#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include "fuse.h"
#include "display.h"
#include "ui.h"
#include "uidisplay.h"
#include "font.h"
#include "keyboard.h"
#include "snapshot.h"

int widget_keymode;

static void printchar(int x, int y, int col, int ch);
static void printstring(int x, int y, int col, char *s);
static void rect(int x, int y, int w, int h, int col);

static void printchar(int x, int y, int col, int ch) {
    
    int mx, my;

    if((ch<32)||(ch>127)) ch=33;
    ch-=32;
    
    for(my=0; my<8; my++) {
        int b;
        b=font[ch*8+my];
        for(mx=0; mx<8; mx++) {
            if(b&128) uidisplay_putpixel(mx+DISPLAY_BORDER_WIDTH+8*x, my+DISPLAY_BORDER_HEIGHT+8*y, col);
            b<<=1;
        }
    }
}

static void printstring(int x, int y, int col, char *s) {
    int i;
    i=0;
    if(s) {
        while((x<32) && s[i]) {
            printchar(x,y,col,s[i]);
            i++;
            x++;
        }
    }
}

static void rect(int x, int y, int w, int h, int col) {
    int mx, my;
    for(my=0;my<h;my++)for(mx=0;mx<w;mx++)
        uidisplay_putpixel(mx+DISPLAY_BORDER_WIDTH+x, my+DISPLAY_BORDER_HEIGHT+y, col);
}

/* ------------------------------------------------------------------ */

static int numfiles;
static char filenames[8192][64];

static int select_file(const struct dirent *dirent){
    if(dirent->d_name && strcmp(dirent->d_name,".")) {
        strncpy(filenames[numfiles], dirent->d_name, 63);
        numfiles++;
    }
    return 0;
}

static void scan(char *dir) {
    struct dirent **d;
    numfiles=0;
    scandir(dir,&d,select_file,NULL);
    qsort(filenames[0], numfiles, 64, strcmp);
}

static int i,k;

void widget_selectfile(void) {
    char d[512];
    int j;
    char s[14];
    
    getcwd(d,510);
    scan(d);
    i=0;
    k=0;
    
    rect(6,6,244,180,9);
    rect(8,8,240,176,8);
    for(j=0;j<36;j++) if(i+j<numfiles){
        strncpy(s,filenames[i+j],13);
        s[13]=0;
        printstring(2+(j&1)*15, 3+j/2, 2, s);
    }
    strncpy(s,filenames[k],13);
    s[13]=0;
    printstring(2+(k&1)*15, 3+k/2, 1, s);
    uidisplay_lines(DISPLAY_BORDER_HEIGHT, DISPLAY_BORDER_HEIGHT+192);
    strncpy(s,filenames[k],13);
    s[13]=0;
    printstring(2+(k&1)*15, 3+k/2, 2, s);
}

void widget_handlekeys(int key) {
    int nk;
    char fn[1024];
    nk=k;
    switch(key) {
        case KEYBOARD_1:
            widget_keymode=0;
            display_refresh_all();
            break;
        case KEYBOARD_8:
            if(k<numfiles-1)nk++;
            break;
        case KEYBOARD_6:
            if(k<numfiles-2)nk+=2;
            break;
        case KEYBOARD_5:
            if(k>0)nk--;
            break;
        case KEYBOARD_7:
            if(k>1)nk-=2;
            break;
        case KEYBOARD_Enter:
            getcwd(fn,500);
            strcat(fn,"/");
            strncat(fn,filenames[k],500);
            if(chdir(fn)==-1) {
                if(errno==ENOTDIR) {
                    snapshot_read(fn);
                    widget_keymode=0;
                    display_refresh_all();
                }
            } else {
                widget_selectfile();
                nk=0;
            }
            break;
    }
    if(nk!=k) {
        char s[14];
        int r;
        
        r=0;
        if(nk<i) {
            i=nk&~1;
            r=1;
        }
        if(nk>i+35) {
            i=nk&~1;
            i-=34;
            r=1;
        }
        if(r) {
            int j;
            rect(8,8,240,176,8);
            for(j=0;j<36;j++) if(i+j<numfiles){
                strncpy(s,filenames[i+j],13);
                s[13]=0;
                printstring(2+(j&1)*15, 3+j/2, 2, s);
            }
        }
        
        strncpy(s,filenames[nk],13);
        s[13]=0;
        printstring(2+((nk-i)&1)*15, 3+(nk-i)/2, 1, s);
        uidisplay_lines(DISPLAY_BORDER_HEIGHT, DISPLAY_BORDER_HEIGHT+192);
        strncpy(s,filenames[nk],13);
        s[13]=0;
        printstring(2+((nk-i)&1)*15, 3+(nk-i)/2, 2, s);
        k=nk;
    }
}
