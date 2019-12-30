#include <Arduino.h>
char * TimeToString(unsigned long t)
{ 
 t=t/1000;
 static char str[8];
 long h = t / 3600;
 t = t % 3600;
 int m = t / 60;
 int s = t % 60;
 sprintf(str, "%01ld:%02d:%02d", h, m, s);
 return str;
}