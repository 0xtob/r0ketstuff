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

uint8_t recv_msg(unsigned char **msg);

void ram(void)
{
    unsigned char *msg;
    uint8_t key;

    lcdClear();
    lcdPrintln("tbchat ready.");
    lcdPrintln("press any direction to send a message");
    lcdRefresh();

    while(1) {
        if(recv_msg(&msg)) {
            lcdPrint("<-");
            lcdPrintln((char*)msg);
        }
        key = getInput();
        if(key == BTN_RIGHT) {
            send_msg("right!");
            lcdPrintln("->right");
        } else if(key == BTN_LEFT) {
            send_msg("left!");
            lcdPrintln("->left");
        } else if(key == BTN_UP) {
            send_msg("up!");
            lcdPrintln("->up");
        } else if(key == BTN_DOWN) {
            send_msg("down!");
            lcdPrintln("->down");
        }
        lcdRefresh();
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

