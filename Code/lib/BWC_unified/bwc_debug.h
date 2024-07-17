#ifndef BWC_DEBUG_H
#define BWC_DEBUG_H

#define BWC_DEBUG_OUTPUT_OFF 0
#define BWC_DEBUG_OUTPUT_SERIAL 1
#define BWC_DEBUG_OUTPUT_FILE 2

#if BWC_DEBUGGING == BWC_DEBUG_OUTPUT_OFF 
    #define BWC_LOG_P(s, ...) 
    #define BWC_LOG(s, ...) 
#elif BWC_DEBUGGING == BWC_DEBUG_OUTPUT_SERIAL
    #define BWC_LOG_P(pstr_string, ...) Serial.printf_P(pstr_string, __VA_ARGS__)
    #define BWC_LOG(s, ...) Serial.printf(s, __VA_ARGS__)
#elif BWC_DEBUGGING == BWC_DEBUG_OUTPUT_FILE
    #define BWC_LOG_P(pstr_string, ...) {char s[128]; sprintf_P(s, pstr_string, __VA_ARGS__); Serial.print(s); log2file(s); }
#else
    #define BWC_LOG_P(s, ...) 
    #define BWC_LOG(s, ...) 
#endif

#ifndef BWC_SPRINKLE_YIELDS
    #define BWC_YIELD
#else
    #define BWC_YIELD optimistic_yield(100)
#endif

void log2file(const char* s);

#endif //guard