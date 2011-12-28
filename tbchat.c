#include <sysinit.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "basic/basic.h"
#include "basic/config.h"


#include "funk/nrf24l01p.h"
#include "funk/rftransfer.h"

#include "lcd/render.h"
#include "lcd/print.h"
#include "lcd/backlight.h"

#include "usetable.h"

/*Global Communication Config*/
uint8_t mac[5] = {1,2,3,5,5};
struct NRF_CFG config =
{
   .channel= 81,
   .txmac= "\x1\x2\x3\x5\x5",
   .nrmacs=1,
   .mac0=  "\x1\x2\x3\x5\x5",
   .maclen ="\x20",
};

unsigned char recvbuf[32];
unsigned char sendbuf[32];

struct Question {
  char* text;
  char* up;
  char* down;
};

uint8_t recv_msg(unsigned char **msg);
void send_msg(unsigned char *msg);
void initQuestions(struct Question* const questions);
void showQuestion(const struct Question const * q, const uint8_t idx);
void printWrap(const char const * s, uint8_t len);
uint8_t ceil(const uint8_t n, const uint8_t d);

void ram(void)
{
    unsigned char *msg;
    uint8_t key;

    lcdClear();
    lcdPrintln("OK r0ket ready.");
    lcdPrintln("Press any button.");
    lcdRefresh();
    key = getInputWait();
    getInputWaitRelease();
    
    uint8_t nQuestions = 3;
    struct Question q[nQuestions];
    unsigned char answers[nQuestions];
    initQuestions(q);
    
    for (uint8_t i = 0; i<nQuestions; ++i) {
        showQuestion(q,i);
        key = getInputWait();
        getInputWaitRelease();
        if(key == BTN_UP) {
            answers[i] = 1;
        } else if(key == BTN_DOWN) {
            answers[i] = 0;
        } 
    }

    while(1) {
        if(recv_msg(&msg)) {
            lcdPrint("<-");
            lcdPrintln((char*)msg);
        }
        key = getInput();
        if(key == BTN_RIGHT) {
            send_msg((unsigned char*)"right!");
            lcdPrintln("->right");
        } else if(key == BTN_LEFT) {
            send_msg((unsigned char*)"left!");
            lcdPrintln("->left");
        } else if(key == BTN_UP) {
            send_msg((unsigned char*)"up!");
            lcdPrintln("->up");
        } else if(key == BTN_DOWN) {
            send_msg((unsigned char*)"down!");
            lcdPrintln("->down");
        }
        lcdRefresh();
    }
}

uint8_t ceil(const uint8_t n, const uint8_t d) 
{
    uint8_t res = n/d;
    if (n%d==0) 
        return res;
    return res + 1;
}

void printWrap(const char const * s, uint8_t len)
{
    uint8_t slen = strlen(s);
    for (uint8_t i=0; i < ceil(slen, len); ++i) {
        char substr[len+1];
        for (uint8_t j=0; j < len; ++j) {
            uint8_t sidx = i*len+j;
            if (sidx < slen)
                substr[j] = s[sidx];
            else
                substr[j] = ' ';
        }
        substr[len] = 0;
        lcdPrintln(substr);
    }
}


uint8_t recv_msg(unsigned char **msg)
{
    nrf_config_set(&config);
    int n = nrf_rcv_pkt_time(10,32,recvbuf);
    //getInputWaitTimeout(100);
    if(n != 32)
        return 0;
    *msg = recvbuf;
    return 1;
}

void send_msg(unsigned char *msg)
{
    memset(sendbuf, 0, 32);
    for(int i=0; i<32 && msg[i] != 0; ++i) {
        sendbuf[i] = msg[i];
    }
    nrf_config_set(&config);
    nrf_snd_pkt_crc(32, sendbuf);
}


void initQuestions(struct Question* const questions)  
{
    questions[0].text="Do you like men?";
    questions[0].up="Yes";
    questions[0].down="No";
    
    questions[1].text="Favourite Editor?";
    questions[1].up="vi";
    questions[1].down="emacs";

    questions[2].text="Schoedingers Cat?";
    questions[2].up="dead";
    questions[2].down="alive";

// Long Hair
// > 3 Freunde
// > Grammar Nazi
}

void showQuestion(const struct Question const * q, const uint8_t idx) 
{
    lcdClear();
    lcdPrintln("Q: ");
    uint8_t lineLength = 12;
    printWrap(q[idx].text, lineLength);    
    lcdPrint("UP: ");
    printWrap(q[idx].up, lineLength);    
    lcdPrint("DOWN: ");
    printWrap(q[idx].down, lineLength);    
    lcdRefresh();
}

/*
uint32_t max(const uint32_t a, const uint32_t b) 
{
    if (a>b) 
        return a; 
    return b;
}
     
uint32_t min(const uint32_t a, const uint32_t b) 
{
    if (a<b) 
        return a; 
    return b;
}

void brighter(void)
{
    uint32_t brightness = backlightGetBrightness();
    if (brightness < 100) {
        backlightSetBrightness(max(brightness + 20,100));
    }
}

void darker(void) 
{
    uint32_t brightness = backlightGetBrightness();
    if (brightness > 0) {
        backlightSetBrightness(min(brightness - 10,0));
    }
}
*/
