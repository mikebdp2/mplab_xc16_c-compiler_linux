//#define __dsPIC30F6014__
#include <p30F6014.h>
#include <stdio.h>

int bss[2100];
char _YDATA(32) dat[] = "abcdefghijklmnopqrstuvwxyz";

int main()
{ printf("a-z: "); printf("%s\n",&dat); }
