#include <sysinit.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "basic/basic.h"
#include "basic/config.h"

#include "filesystem/ff.h"

#include "funk/nrf24l01p.h"
#include "funk/rftransfer.h"

#include "lcd/render.h"
#include "lcd/print.h"

#include "usetable.h"

/*Global Communication Config*/
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

uint8_t linelength;

uint8_t recv_msg(unsigned char **msg);
void send_msg(unsigned char *msg);
int readTextFile(char * filename, char * data, int len);
void initQuestions(struct Question* const questions);
void showQuestion(const struct Question const * q, const uint8_t idx);
void printWrap(const char const * s);
uint8_t ceil(const uint8_t n, const uint8_t d);
void askQuestions(const struct Question const *q, uint8_t nQuestions,
                  unsigned char *answers);
void wrapPacket(const unsigned char const *answers, const uint8_t nAnswers, char* packet);
bool unWrapPacket(unsigned char *answers, const uint8_t nAnswers, char* nick, const char    const * packet);
uint8_t match(const unsigned char const * a1, const unsigned char const * a2, const    uint8_t n);
void printPacket(const char const * packet);
void showSplashScreen();

void ram(void)
{
    linelength = 14;

    showSplashScreen();

    lcdClear();
    uint8_t nQuestions = 3;
    struct Question q[nQuestions];
    unsigned char answers[nQuestions];
    char answerfile[] = "okr0ket.prf";
    if(readFile(answerfile, (char*)answers, nQuestions) == nQuestions) {
        printWrap("Loaded previous answers.");
    } else {
        lcdPrintln("Answers not read.");
        memset(answers, 0, nQuestions); // size in bytes
        initQuestions(q);
        askQuestions(q, nQuestions, answers);
        if(writeFile(answerfile, (char*)answers, nQuestions) == -1) {
            lcdPrintln("Error writing answers!");
        }
    }

    printWrap("Looking for matches ...");
    lcdRefresh();

    // Encode message
    char packet_send[32];
    wrapPacket(answers, nQuestions, packet_send);

    uint8_t key = BTN_NONE;
    while(key != BTN_ENTER) {
        // Send the message (of love!)
        send_msg((unsigned char*)packet_send);
        char* packet_recv;
        if(recv_msg((unsigned char**)&packet_recv)) {
            printPacket(packet_recv);
            // decode message
            char recv_nick[17];
            unsigned char recv_answers[nQuestions];
            if(unWrapPacket((unsigned char*)recv_answers, nQuestions,
                             recv_nick, packet_recv)) {
                // match against my profile
                uint8_t score = match(answers, recv_answers, nQuestions);

                // display!
                lcdPrint(recv_nick);
                lcdPrint(" ");
                lcdPrint(IntToStr(100*score/nQuestions, 3, 0));
                lcdPrintln("%");
            } else {
                //lcdPrintln("cannot read");
            }
        }
        key = getInput();
        lcdRefresh();

        delayms(100);
    }
}

void showSplashScreen()
{
    lcdClear();
    lcdPrintln("");
    lcdPrintln("");
    lcdPrintln("");
    lcdPrintln("  OK r0ket.");
    lcdPrintln("");
    printWrap("Press any key.");
    lcdRefresh();
    getInputWait();
    getInputWaitRelease();
}

void wrapPacket(const unsigned char const *answers, const uint8_t nAnswers, char* packet)
{
    // const uint8_t packetLength = 32;
    memset(packet, 0, 32);
    uint8_t offset = 0;
    packet[offset] = 'X';
    offset++;
    const uint8_t nickLength = 16;
    for (uint8_t i=0; i<nickLength; ++i) {
        packet[i+offset] = nickname[i];
    }
    offset += nickLength;
    for (uint8_t i=0; i<nAnswers; ++i) {
        packet[i+offset] = answers[i];
    }   
}

void printPacket(const char const * packet) 
{
    printWrap(packet);
    printWrap(packet+18);
}

bool unWrapPacket(unsigned char *answers, const uint8_t nAnswers, char* nick, const char const * packet)
{
    // const uint8_t packetLength = 32;
    uint8_t offset = 0;

    if (packet[offset] != 'X')
        return false;
    offset++;

    // read nick
    const uint8_t nickLength = 16;
    for (uint8_t i=0; i<nickLength; ++i) {
        nick[i] = packet[i+offset];
    }
    nick[nickLength] = 0;
    offset += nickLength;
    
    // read answers
    for (uint8_t i=0; i<nAnswers; ++i) {
        answers[i] = packet[i+offset];
    }
    return true;
}

void askQuestions(const struct Question const *q, uint8_t nQuestions,
                  unsigned char *answers)
{
    uint8_t key;
    for (uint8_t i = 0; i<nQuestions; ++i) {
        showQuestion(q,i);
        key = getInputWait();
        getInputWaitRelease();
        if(key == BTN_UP) {
            answers[i] = '1';
        } else if(key == BTN_DOWN) {
            answers[i] = '0';
        }
    }
}

// a1 and a2 are answer arrays
uint8_t match(const unsigned char const * a1, const unsigned char const * a2, const uint8_t n) 
{
    uint8_t score = 0;
    for (uint8_t i=0; i<n; ++i) {
        if (a1[i]==a2[i]) 
            score++;
    }
    return score;
}

// Copied from filesystem/util.c since the function is not exported
int readTextFile(char * filename, char * data, int len){
    UINT readbytes;

    readbytes=readFile(filename,data,len-1);
    if(len>=0)
        data[readbytes]=0;
    return readbytes;
};

uint8_t ceil(const uint8_t n, const uint8_t d) 
{
    uint8_t res = n/d;
    if (n%d==0) 
        return res;
    return res + 1;
}

void printWrap(const char const * s)
{
    uint8_t slen = strlen(s);
    for (uint8_t i=0; i < ceil(slen, linelength); ++i) {
        char substr[linelength+1];
        for (uint8_t j=0; j < linelength; ++j) {
            uint8_t sidx = i*linelength+j;
            if (sidx < slen)
                substr[j] = s[sidx];
            else
                substr[j] = ' ';
        }
        substr[linelength] = 0;
        lcdPrintln(substr);
    }
}

uint8_t recv_msg(unsigned char **msg)
{
    nrf_config_set(&config);
    int n = nrf_rcv_pkt_time(100,32,recvbuf);
    if(n != 32)
        return 0;
    *msg = recvbuf;
    return 1;
}

void send_msg(unsigned char *msg)
{
    memset(sendbuf, 0, 32);
    for(int i=0; i<32; ++i) {
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
    printWrap(q[idx].text);    
    lcdPrint("UP: ");
    printWrap(q[idx].up);    
    lcdPrint("DOWN: ");
    printWrap(q[idx].down);    
    lcdRefresh();
}

