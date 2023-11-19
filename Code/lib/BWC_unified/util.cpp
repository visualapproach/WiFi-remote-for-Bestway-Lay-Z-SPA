#include "util.h"

float C2F(float c)
{
    return c*1.8+32;
}

float F2C(float f)
{
    return (f-32)/1.8;
}


int gpio2dp(int gpio)
{
    for(int i = 0; i < 9; i++)
    {
        if(gpio == D_pin[i]) return i;
    }
    return -1;
}

int dp2gpio(int dp)
{
    switch(dp)
    {
        case D0:
            return D0;
            break;
        case D1:
            return D1;
            break;
        case D2:
            return D2;
            break;
        case D3:
            return D3;
            break;
        case D4:
            return D4;
            break;
        case D5:
            return D5;
            break;
        case D6:
            return D6;
            break;
        case D7:
            return D7;
            break;
        case D8:
            return D8;
            break;
        default:
            return -1;
    }
}