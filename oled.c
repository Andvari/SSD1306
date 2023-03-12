#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "font8x8.h"

#define TAB_SIZE    2

#define BMP085_ADDR    0x77

#define OLED_ADDR 0x3C
#define OLED_ROWS 8
#define OLED_COLS 16

#define ON  1
#define OFF 0

#define NORMAL_MODE  0
#define INVERSE_MODE 1

#define H_MODE  0
#define V_MODE  1
#define P_MODE  2

typedef struct{
    char r;
    char c;
}CURSOR;

int iic;
char ram[8][128];
char address_mode;

CURSOR cursor;

char c[8];

void write_iic_6(char a, char b, char d, char e, char f, char g){
    c[0] = a; c[1] = b; c[2] = d; c[3] = e; c[4] = f; c[5] = g;
    write(iic, c, 6);
};


void write_iic_4(char a, char b, char d, char e){
    c[0] = a; c[1] = b; c[2] = d; c[3] = e;
    write(iic, c, 4);
};


void write_iic_2(char a, char b){
    c[0] = a; c[1] = b;
    write(iic, c, 2);
};


void write_iic_1(char a){
    write(iic, &a, 1);
};


unsigned short read_iic_2(){
    char bytes[2];
    read(iic, bytes, 2);

    return (bytes[0] << 8) | bytes[1];
};


unsigned char read_iic_1(){
    char byte;
    read(iic, &byte, 1);
    return byte;
};


void set_contrast(char percent){ write_iic_4(0x80, 0x81, 0x80, percent*255/100); }


void set_display(char c){ write_iic_2(0x80, 0xAE | c); }


void set_display_mode(char c){ write_iic_2(0x80, 0xA6 | c); }


void set_address_mode(char c){
    address_mode = c;
    write_iic_4(0x80, 0x20, 0x80, c);
}


void set_page(char c){ write_iic_2(0x80, 0xB0 | c); }


void clear(){
    char save_mode = address_mode;
    set_address_mode(V_MODE);
    write_iic_6(0x80, 0x21, 0x80, 0x00, 0x80, 0x7F);
    write_iic_6(0x80, 0x22, 0x80, 0x00, 0x80, 0x07);

    for(int r=0; r<8; ++r){
        for(int c=0; c<128; ++c){
	        write_iic_2(0x40, 0x00);
            ram[r][c] = 0;
	    }
    }
    address_mode = save_mode;
    set_address_mode(address_mode);
}


void set_cursor_position(char r, char c){
    cursor.r = r;
    cursor.c = c;
    set_page(7 - r);
    write_iic_2(0x80, (c*8)&0xF);
    write_iic_2(0x80, 0x10 | ((c*8)>>4));
}


void print_symbol(SYMBOL *s ){
    for(int i=0; i<FONT_W; ++i){
        write_iic_2(0x40,  s->c[i]);
        ram[cursor.r][cursor.c*8+i] = s->c[i];
    }
}


void print_inv_symbol(SYMBOL *s ){
    for(int i=0; i<FONT_W; ++i){
        write_iic_2(0x40,  ~s->c[i]);
        ram[cursor.r][cursor.c*8+i] = ~s->c[i];
    }
}


void print(char *text, char len, char mode){
    char r = cursor.r;
    char c = cursor.c;

    for(char i=0; i<len; ++i){
        switch(text[i]){
            case '\n':
                r = (r+1)%OLED_ROWS;
                break;
            case '\r':
                c = 0;
                break;
            case '\t':
                c += TAB_SIZE;
                if(c>OLED_COLS-1){
                    c -= OLED_COLS;
                    r = (r+1)%OLED_ROWS;
                }
                break;
            default:
                if(mode == NORMAL_MODE) { print_symbol(&font8x8[text[i]]); }
                else                    { print_inv_symbol(&font8x8[text[i]]); }
                c++;
                if(c==OLED_COLS){
                    c = 0;
                    r = (r+1)%OLED_ROWS;
                }
                break;
        }
        set_cursor_position(r, c);
    }
};


void scroll(){
    for(char r = 1; r<OLED_ROWS; ++r){
        set_cursor_position(r-1, 0);
        for(char c = 0; c<OLED_COLS*FONT_W; ++c){
            write_iic_2(0x40, ram[r][c]);
        }
    }
}


void select_dev(char dev_addr){ ioctl(iic, I2C_SLAVE, dev_addr); }


void main(){
    char str[80];
    cursor.r = 0;
    cursor.c = 0;

    iic = open("/dev/i2c-0", O_RDWR);

    ioctl(iic, I2C_SLAVE, OLED_ADDR);

    set_display(ON);
    write_iic_4(0x80, 0xA8, 0x80, 0x3F);
    write_iic_4(0x80, 0xD3, 0x80, 0x00);
    write_iic_2(0x80, 0x40);
    write_iic_2(0x80, 0xA1);
    write_iic_2(0x80, 0xC0);
    write_iic_4(0x80, 0xDA, 0x80, 0x12);

    set_contrast(100);

    write_iic_2(0x80, 0xA4);

    set_display_mode(NORMAL_MODE);

    write_iic_4(0x80, 0xD5, 0x80, 0x80);
    write_iic_4(0x80, 0x8D, 0x80, 0x14);
    set_display(ON);

    address_mode = P_MODE;
    set_address_mode(P_MODE);
/*
    write_iic_2(0x80, 0x00);
    write_iic_2(0x80, 0x10);
    write_iic_2(0x80, 0x40);
    write_iic_2(0x80, 0xA0);
    write_iic_4(0x80, 0xA8, 0x80, 0x3F);
    write_iic_4(0x80, 0xDA, 0x80, 0x12);
*/
    set_page(0);
    //write_iic_4(0x80, 0x00, 0x80, 0x10);
    //write_iic_6(0x80, 0x21, 0x80, 0x00, 0x80, 0x7F);
    write_iic_4(0x80, 0xD9, 0x80, 0x22);
    write_iic_4(0x80, 0xDB, 0x80, 0x20);

    set_contrast(10); sleep(1);
    set_contrast(100); sleep(1);
    set_contrast(10); sleep(1);
    set_contrast(100); sleep(1);

    clear();
    sleep(2);

    //sprintf(str, "0123456789ABCDEFGHIJKLMNOPQRTSUVWXYZabcdefghijklmnopqrtstuvwxyz!");
    set_cursor_position(0, 0);
    sprintf(str, "T:  ");
    print(str, strlen(str), NORMAL_MODE);

    set_cursor_position(1, 0);
    sprintf(str, "P:  ");
    print(str, strlen(str), NORMAL_MODE);
    //print(str, strlen(str), INVERSE_MODE);

    select_dev(BMP085_ADDR);

    write_iic_1(0xAA);
    short ac1 = (short)read_iic_2();
    short ac2 = (short)read_iic_2();
    short ac3 = (short)read_iic_2();
    unsigned short ac4 = read_iic_2();
    unsigned short ac5 = read_iic_2();
    unsigned short ac6 = read_iic_2();
    short b1 = (short)read_iic_2();
    short b2 = (short)read_iic_2();
    short mb = (short)read_iic_2();
    short mc = (short)read_iic_2();
    short md = (short)read_iic_2();
/*
    printf("ac1: %d\n", ac1);
    printf("ac2: %d\n", ac2);
    printf("ac3: %d\n", ac3);
    printf("ac4: %d\n", ac4);
    printf("ac5: %d\n", ac5);
    printf("ac6: %d\n", ac6);
    printf("b1: %d\n", b1);
    printf("b2: %d\n", b2);
    printf("mb: %d\n", mb);
    printf("mc: %d\n", mc);
    printf("md: %d\n", md);
*/
    volatile int a=0;
    while(1){
        select_dev(BMP085_ADDR);
        write_iic_2(0xF4, 0x2E);
        sleep(1);
        write_iic_1(0xF6);
        long ut = read_iic_2();
        long x1 = (ut - ac6)*ac5/(1<<15);
        long x2 = mc*(1<<11)/(x1+md);
        long b5 = x1 + x2;
        double t = (b5 + 8)/(1<<4)/10.;

        short oss = 0;
        write_iic_2(0xF4, 0x34 + (oss<<6));
        sleep(1);
        write_iic_1(0xF6);
        long up = read_iic_2();
        write_iic_1(0xF8);
        char xlsb = read_iic_1();
        up = ((up<<8) + xlsb) >> (8-oss);

        long b6 = b5 - 4000;
        x1 = b2*(b6*b6/(1<<12))/(1<<11);
        x2 = ac2*b6/(1<<11);
        long x3 = x1 + x2;
        long b3 = (((ac1*4 + x3) << oss) + 2)/4;
        x1 = ac3 * b6 /(1<<13);
        x2 = b1 * (b6 * b6 / (1<<12)) / (1<<16);
        x3 = ((x1 + x2) + 2)/(1<<2);
        unsigned long b4 = ac4 * (unsigned long)(x3+32768) / (1<<15);
        unsigned long b7 = ((unsigned long)up - b3) * (50000>>oss);
        long p;
        if(b7 < 0x80000000) { p = (b7*2)/b4; }
        else                { p = (b7/b4)*2; }

        x1 = (p/(1<<8))*(p/(1<<8));
        x1 = (x1*3038)/(1<<16);
        x2 = (-7357*p)/(1<<16);
        p = p + (x1+x2+3791)/(1<<4);

        select_dev(OLED_ADDR);

        set_cursor_position(0, 3);
        sprintf(str, "%+2.1fC", t);
        if(strlen(str) )
        print(str, strlen(str), NORMAL_MODE);

        set_cursor_position(1, 3);
        sprintf(str, "%2.2f mmHg", (double)p*0.0075);
        if(strlen(str) )
        print(str, strlen(str), NORMAL_MODE);
    }

    sleep(1);
    close(iic);

    printf("I2C stopped\n");
    return;
}
