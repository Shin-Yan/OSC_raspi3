#include <mbox.h>
#include <mini_uart.h>
#include <utils.h>

/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[36];

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch,volatile unsigned int *mb)
{
    unsigned int r = (((unsigned int)((unsigned long)mb)&~0xF) | (ch&0xF));
    /* wait until we can write to the mailbox */
    do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_FULL);
    /* write the address of our message to the mailbox with channel identifier */
    *MBOX_WRITE = r;
    /* now wait for the response */
    while(1) {
        /* is there a response? */
        do{asm volatile("nop");}while(*MBOX_STATUS & MBOX_EMPTY);
        /* is it a response to our message? */
        if(r == *MBOX_READ)
            /* is it a valid successful response? */
            return mb[1]==MBOX_RESPONSE;
    }
    return 0;
}

void get_board_revision(unsigned int *revision){
    mbox[0] = 7*4;
    mbox[1] = REQUEST_CODE;
    mbox[2] = GET_BOARD_REVISION;
    mbox[3] = 4;
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;
    mbox[6] = END_TAG;

    // mbox call success
    if(mbox_call(MBOX_CH_PROP, mbox)!=0)
        *revision = mbox[5];
    else{
        uart_printf("Unable to get borad revision!");
        *revision = 0;
    }
}

void get_arm_memory(arm_info *arm_mem){
    mbox[0] = 8*4;
    mbox[1] = REQUEST_CODE;
    mbox[2] = GET_ARM_MEMORY;
    mbox[3] = 4*2;
    mbox[4] = TAG_REQUEST_CODE;
    mbox[5] = 0;
    mbox[6] = 0;
    mbox[7] = END_TAG;

    if(mbox_call(MBOX_CH_PROP, mbox)!=0){
        arm_mem->base_addr = mbox[5];
        arm_mem->size = mbox[6];
    }
    else
        uart_printf("Unable to get arm memory information!");
}