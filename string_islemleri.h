/******************************************
 * ////Gomulu Sistemlerde String Islem Yapmayi Saglayan Kutuphane////
 * Proje:      OSKAR
 * Derleme: Mustafa Özçelikörs
 *             
 ****/

#include <stdlib.h>
#define  STRING_BUF_SIZE     50   // maximum Grösse vom String

#define  USE_STR_FKT    1  // 0=nicht benutzen, 1=benutzen


#if USE_STR_FKT==1
  #include <string.h>
#endif

char STRING_BUF[STRING_BUF_SIZE];


#define  STRING_FLOAT_FAKTOR     1000   // 1000 = 3 Nachkommastellen
#define  STRING_FLOAT_FORMAT "%d.%03d"  // Formatierung

void UB_String_FloatToDezStr(float wert);
float UB_String_DezStringToFloat(unsigned char *ptr);
int16_t UB_String_DezStringToInt(char *ptr);
#if USE_STR_FKT==1
void UB_String_Mid(char *ptr, uint16_t start, uint16_t length);
void UB_String_Left(char *ptr, uint16_t length);
void UB_String_Right(char *ptr, uint16_t length);
#endif

void reverse(char s[]);
int itoa(int num, unsigned char* str, int len, int base);
char* ltoa( long value, char *string, int radix );


void UB_String_FloatToDezStr(float wert)
{
  int16_t vorkomma;
  uint16_t nachkomma;
  float rest;

  if((wert>32767) || (wert<-32767)) {
    sprintf(STRING_BUF,"%s","OVF");
    return;
  }

  vorkomma=(int16_t)(wert);
  if(wert>=0) {
    rest = wert-(float)(vorkomma);
  }
  else {
    rest = (float)(vorkomma)-wert;
  }
  nachkomma = (uint16_t)(rest*(float)(STRING_FLOAT_FAKTOR)+0.5);

  sprintf(STRING_BUF,STRING_FLOAT_FORMAT,vorkomma,nachkomma);
}


//--------------------------------------------------------------
// wandelt einn String in eine Float Zahl
// Bsp : String "123.457" wird zu Zahl 123.457
//--------------------------------------------------------------
float UB_String_DezStringToFloat(unsigned char *ptr)
{
  float ret_wert=0.0;

  ret_wert=atof((char const *)ptr);

  return(ret_wert);
}




//--------------------------------------------------------------
// wandelt einn String in eine Integer Zahl
// Bsp : String "-1234" wird zu Zahl -1234
//--------------------------------------------------------------
int16_t UB_String_DezStringToInt(char *ptr)
{
  int16_t ret_wert=0;

  ret_wert=atoi(ptr);

  return(ret_wert);
}

//--------------------------------------------------------------
// kopiert einen Teilstring
// Bsp : String "Hallo Leute",3,6 wird zu "lo Leu"
//--------------------------------------------------------------
#if USE_STR_FKT==1
void UB_String_Mid(char *ptr, uint16_t start, uint16_t length)
{
  uint16_t i,m;
  uint16_t cnt = 0;

  if(length==0) return;
  m=start+length;
  if(m>strlen(ptr)) m=strlen(ptr);

  for(i=start;i<m; i++) {
    STRING_BUF[cnt] = ptr[i];
    cnt++;
  }
  STRING_BUF[cnt]=0x00;
}
#endif


//--------------------------------------------------------------
// kopiert den linken Teil von einem String
// Bsp : String "Hallo Leute",3 wird zu "Hal"
//--------------------------------------------------------------
#if USE_STR_FKT==1
void UB_String_Left(char *ptr, uint16_t length)
{
  if(length==0) return;
  if(length>strlen(ptr)) length=strlen(ptr);

  strncpy(STRING_BUF,ptr,length);
  STRING_BUF[length]=0x00;
}
#endif


//--------------------------------------------------------------
// kopiert den rechten Teil von einem String
// Bsp : String "Hallo Leute",3 wird zu "ute"
//--------------------------------------------------------------
#if USE_STR_FKT==1
void UB_String_Right(char *ptr, uint16_t length)
{
  uint16_t i,m,start;
  uint16_t cnt = 0;

  if(length==0) return;
  m=strlen(ptr);
  if(length>m) length=m;
  start=m-length;

  for(i=start;i<m; i++) {
    STRING_BUF[cnt] = ptr[i];
    cnt++;
  }
  STRING_BUF[cnt]=0x00;
}
#endif


void reverse(char s[])
 {
     int i, j;
     char c;
 
     for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
         c = s[i];
         s[i] = s[j];
         s[j] = c;
     }
 }
int itoa(int num, unsigned char* str, int len, int base)
{
	int sum = num;
	int i = 0;
	int digit;
	if (len == 0)
		return -1;
	do
	{
		digit = sum % base;
		if (digit < 0xA)
			str[i++] = '0' + digit;
		else
			str[i++] = 'A' + digit - 0xA;
		sum /= base;
	}while (sum && (i < (len - 1)));
	if (i == (len - 1) && sum)
		return -1;
	str[i] = '\0';
	reverse(str);
	return 0;
}

char* ltoa( long value, char *string, int radix )
{
  char tmp[33];
  char *tp = tmp;
  long i;
  unsigned long v;
  int sign;
  char *sp;

  if ( string == NULL )
  {
    return 0 ;
  }

  if (radix > 36 || radix <= 1)
  {
    return 0 ;
  }

  sign = (radix == 10 && value < 0);
  if (sign)
  {
    v = -value;
  }
  else
  {
    v = (unsigned long)value;
  }

  while (v || tp == tmp)
  {
    i = v % radix;
    v = v / radix;
    if (i < 10)
      *tp++ = i+'0';
    else
      *tp++ = i + 'A' - 10;
  }

  sp = string;

  if (sign)
    *sp++ = '-';
  while (tp > tmp)
    *sp++ = *--tp;
  *sp = 0;

  return string;
}

char* concat(char *s1, char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);//+1 to copy the null-terminator
    return result;
}