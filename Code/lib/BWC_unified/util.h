#pragma once

#include <Arduino.h>


float C2F(float c);
float F2C(float f);

const int D_pin[9] = {D0, D1, D2, D3, D4, D5, D6, D7, D8};
int gpio2dp(int gpio);
int dp2gpio(int dp);