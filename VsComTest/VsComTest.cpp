#include "AtCommand.h"
#include <stdio.h>
#include <stdbool.h>
#include <thread>

using std::thread;

int main()
{
    char str[1024];
    thread acThr(acThread);
    for (int i = 0; i < 100000; i++)
    {
        scanf_s("%s", str, sizeof(str));
        //STRCPY(str, sizeof(str), "  AT+START  =  120 ,  WDDW 23OI    ,-2101213213, -3.1415926   ");
        AcMessage msg;
        int count = (int)strlen(str) + 1;
        msg.cmd = (char*)MALLOC(count);
        STRCPY(msg.cmd, count, str);
        msg.id = AT_COMMAND;
        acPostMessage(&msg);
    }
    acThr.join();
    return 0;
}











