#pragma once

#include <zlib.h>
#include <iostream>
#include <cmath>
#include <cassert>

static const int buffer_size = 1048576;

class StreamBuffer {
   gzFile        in;
   unsigned char buf[buffer_size];
   int           pos;
   int           size;

   void assureLookahead() {
       if (pos >= size) {
           pos  = 0;
           size = gzread(in, buf, sizeof(buf)); } }

public:
   explicit StreamBuffer(gzFile i) : in(i), pos(0), size(0) { assureLookahead(); }

   int  operator *  () const { return (pos >= size) ? EOF : buf[pos]; }
   void operator ++ ()       { pos++; assureLookahead(); }
   int  position    () const { return pos; }
};


//-------------------------------------------------------------------------------------------------
// End-of-file detection functions for StreamBuffer and char*:


static inline bool isEof(StreamBuffer& in) { return *in == EOF;  }
static inline bool isEof(const char*   in) { return *in == '\0'; }

//-------------------------------------------------------------------------------------------------
// Generic parse functions parametrized over the input-stream type.



template<class B>
static void skipWhitespace(B& in) {
    while ((*in >= 9 && *in <= 13) || *in == 32)
        ++in;
}

template<class B>
static void skipLine(B& in) {
    for (;;){
        if (isEof(in)) return;
        if (*in == '\n') { ++in; return; }
        ++in;
    }
}

template<class B>
static double parseDouble(B& in) { // only in the form X.XXXXXe-XX
    bool    neg= false;
    double accu = 0.0;
    double currentExponent = 1;
    int exponent;

    skipWhitespace(in);
    if(*in == EOF) return 0;
    if      (*in == '-') neg = true, ++in;
    else if (*in == '+') ++in;
    if (*in < '1' || *in > '9') printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
    accu = (double)(*in - '0');
    ++in;
    if (*in != '.') printf("PARSE ERROR! Unexpected char: %c\n", *in),exit(3);
    ++in; // skip dot
    currentExponent = 0.1;
    while (*in >= '0' && *in <= '9')
        accu = accu + currentExponent * ((double)(*in - '0')),
                currentExponent /= 10,
                ++in;
    if (*in != 'e') printf("PARSE ERROR! Unexpected char: %c\n", *in),exit(3);
    ++in; // skip dot
    exponent = parseInt(in); // read exponent
    accu *= std::pow(10,exponent);
    return neg ? -accu:accu;
}


template<class B>
static int parseInt(B& in) {
    int     val = 0;
    bool    neg = false;
    skipWhitespace(in);

    if      (*in == '-') neg = true, ++in;
    else if (*in == '+') ++in;
    if (*in < '0' || *in > '9')
        fprintf(stderr, "PARSE ERROR!!! Unexpected char in parseInt: %c\n", *in), exit(3);

    // Convert string number to int
    while (*in >= '0' && *in <= '9')
        val = val*10 + (*in - '0'),
                ++in;
    return neg ? -val : val;
}


template<class B>
static int64_t parseInt64(B& in) {

    int64_t     val = 0;
    bool    neg = false;
    skipWhitespace(in);
    if      (*in == '-') neg = true, ++in;
    else if (*in == '+') ++in;
    if (*in < '0' || *in > '9') {
        fprintf(stderr, "PARSE ERROR! Unexpected char in parseInt64: %c\n", *in);
        assert(false);
        exit(3);
    }
    while (*in >= '0' && *in <= '9')
        val = val*10 + (*in - '0'),
                ++in;
    return neg ? -val : val;
}


template<class B>
static unsigned long long int parseWeight(B& in) {

    unsigned long long int     val = 0;
    skipWhitespace(in);
    if (*in == 'h') {
        ++in;
        if(*in != ' ') {
            fprintf(stderr, "o PARSE ERROR! Unexpected char in parseWeight: %c\n", *in);
            exit(3);
        }
        //++in;
        return (unsigned long long int)-1;
    }
    if      (*in == '-') fprintf(stderr, "o PARSE ERROR! Unexpected negative weight\n"), exit(3);
    else if (*in == '+') ++in;
    if (*in < '0' || *in > '9') fprintf(stderr, "o PARSE ERROR! Unexpected char in parseWeight: %c\n", *in), exit(3);
    while (*in >= '0' && *in <= '9')
        val = val*10 + (*in - '0'),
                ++in;
    return val;
}

// String matching: in case of a match the input iterator will be advanced the corresponding
// number of characters.
template<class B>
static bool match(B& in, const char* str) {
    int i;
    for (i = 0; str[i] != '\0'; i++)
        if (in[i] != str[i])
            return false;

    in += i;

    return true;
}

// String matching: consumes characters eagerly, but does not require random access iterator.
template<class B>
static bool eagerMatch(B& in, const char* str) {
    for (; *str != '\0'; ++str, ++in)
        if (*str != *in)
            return false;
    return true;
}


//=================================================================================================
