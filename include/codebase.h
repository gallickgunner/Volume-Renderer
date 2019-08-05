// (c) by Stefan Roettger, licensed under GPL 2+

/*  This file is taken from VVV (V^3) Volume Renderer written by Roettger.
 *  The original files can be found at <https://sourceforge.net/p/volren/code>
 */

#ifndef CODEBASE_H
#define CODEBASE_H

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <float.h>
#include <math.h>

#include <iostream>

#if !defined WINOS && !defined MACOSX && !defined LINUX
#if defined _WIN32
#   define WINOS
#elif defined __APPLE__
#   define MACOSX
#else
#   define LINUX
#endif
#endif

#if defined(IRIX) || defined(LINUX) || defined(MACOSX)
#define UNIX
#endif

#include <time.h>
#ifdef UNIX
#include <sys/time.h>
#endif
#ifdef WINOS
#include <windows.h>
#include <winbase.h>
#endif

#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifndef NULL
#define NULL (0)
#endif

#define BOOLINT char

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

enum VR_ERROR
   {
   VR_ERROR_NONFATAL=0,
   VR_ERROR_FATAL=1,
   VR_ERROR_MEM=2,
   VR_ERROR_IO=3,
   VR_ERROR_CODE=4
   };

#define ERRORMSG() vrerrormsg(__FILE__,__LINE__,VR_ERROR_FATAL)
#define MEMERROR() vrerrormsg(__FILE__,__LINE__,VR_ERROR_MEM)
#define IOERROR() vrerrormsg(__FILE__,__LINE__,VR_ERROR_IO)
#define CODEERROR() vrerrormsg(__FILE__,__LINE__,VR_ERROR_CODE)
#define WARNMSG(msg) vrerrormsg(__FILE__,__LINE__,VR_ERROR_NONFATAL,msg)

inline void vrerrormsg(const char *file,int line,int fatal,const char *msg=NULL)
   {
      if (fatal==VR_ERROR_NONFATAL) fprintf(stderr,"warning");
      else if (fatal==VR_ERROR_MEM) fprintf(stderr,"insufficient memory");
      else if (fatal==VR_ERROR_IO) fprintf(stderr,"io error");
      else if (fatal==VR_ERROR_CODE) fprintf(stderr,"unimplemented code");
      else fprintf(stderr,"fatal error");
      fprintf(stderr," in <%s> at line %d!\n",file,line);
      if (msg!=NULL) fprintf(stderr,"description: %s\n",msg);
   }

#ifndef PI
#define PI (3.14159265358979323846264338327950288)
#endif
#ifndef RAD
#define RAD (PI/180.0)
#endif

#ifndef MAXFLOAT
#define MAXFLOAT (FLT_MAX)
#endif

#undef ffloor
#define ffloor(x) floor((double)(x))
#undef fceil
#define fceil(x) ceil((double)(x))
#define ftrc(x) (int)ffloor(x)

inline double FABS(const double x) {return((x<0.0)?-x:x);}
#ifndef MACOSX
#define fabs(x) FABS(x)
#endif

#ifndef min
inline int min(const int a,const int b) {return((a<b)?a:b);}
#endif
inline double FMIN(const double a,const double b) {return((a<b)?a:b);}
#define fmin(a,b) FMIN(a,b)

#ifndef max
inline int max(const int a,const int b) {return((a>b)?a:b);}
#endif
inline double FMAX(const double a,const double b) {return((a>b)?a:b);}
#define fmax(a,b) FMAX(a,b)

inline int sqr(const int x) {return(x*x);}
inline double fsqr(const double x) {return(x*x);}

#undef fsqrt
#define fsqrt(x) sqrt((double)(x))

#undef fsin
#define fsin(x) sin((double)(x))
#undef fcos
#define fcos(x) cos((double)(x))
#undef ftan
#define ftan(x) tan((double)(x))

#undef fasin
#define fasin(x) asin((double)(x))
#undef facos
#define facos(x) acos((double)(x))
#undef fatan
#define fatan(x) atan((double)(x))

#undef fexp
#define fexp(x) exp((double)(x))
#undef flog
#define flog(x) log((double)(x))
#undef fpow
#define fpow(x,y) pow((double)(x),(double)(y))

#ifdef UNIX
#define GETRANDOM() drand48()
#endif
#ifdef WINOS
#define GETRANDOM() ((double)rand()/RAND_MAX)
#endif

inline double GETTIME()
   {
#ifdef UNIX
   struct timeval t;
   gettimeofday(&t,NULL);
   return(t.tv_sec+t.tv_usec/1.0E6);
#endif
#ifdef WINOS
   static int cpus=0;
   if (cpus==0)
      {
      SYSTEM_INFO SystemInfo;
      GetSystemInfo(&SystemInfo);
      cpus=SystemInfo.dwNumberOfProcessors;
      }
   if (cpus==1)
      {
      LARGE_INTEGER freq,count;
      if (QueryPerformanceFrequency(&freq)==0) ERRORMSG();
      QueryPerformanceCounter(&count);
      return((double)count.QuadPart/freq.QuadPart);
      }
   return((double)clock()/CLOCKS_PER_SEC);
#endif
   }

inline double gettime()
   {
   static double time;
   static BOOLINT settime=FALSE;

   if (!settime)
      {
      time=GETTIME();
      settime=TRUE;
      }

   return(GETTIME()-time);
   }

inline void waitfor(double secs)
   {
   if (secs<=0.0) return;
#ifdef UNIX
   struct timespec dt,rt;
   dt.tv_sec=ftrc(secs);
   dt.tv_nsec=ftrc(1.0E9*(secs-ftrc(secs)));
   while (nanosleep(&dt,&rt)!=0) dt=rt;
#else
   double time=gettime()+secs;
   while (gettime()<time);
#endif
   }

inline double getclockticks()
   {
   static double clockticks;
   static BOOLINT setclockticks=FALSE;

   if (!setclockticks)
      {
      double time=gettime();
      while (time==gettime());
      clockticks=1.0/(gettime()-time);
      setclockticks=TRUE;
      }

   return(clockticks);
   }

#ifdef WINOS

#ifndef _strdup
inline char* strdup(const char *str)
   {
   char *strcpy;

   if (str==NULL) return(NULL);
   if ((strcpy=(char *)malloc(strlen(str)+1))==NULL) MEMERROR();

   memcpy(strcpy,str,strlen(str)+1);
   return(strcpy);
   }
#else
#define strdup _strdup
#endif // _strdup

#ifndef strcasecmp
inline int strcasecmp(const char *str1, const char* str2)
   {
   int c1, c2;
   do
    {
        c1 = (unsigned char) *str1++;
        c2 = (unsigned char) *str2++;
        c1 = tolower(c1);
        c2 = tolower(c2);
    }while(c1 != '\0' && c1 == c2);
    return (c1-c2);
   }
#endif // strcasecmp

#define snprintf _snprintf

#ifndef __MINGW32__
inline int strcasecmp(const char *str1,const char *str2)
   {
   const char *ptr1,*ptr2;
   for (ptr1=str1,ptr2=str2; tolower(*ptr1)==tolower(*ptr2) && *ptr1!='\0' && *ptr2!='\0'; ptr1++,ptr2++);
   return(*ptr1-*ptr2);
   }
#endif

inline char *strcasestr(const char *str1,const char *str2)
   {
   unsigned int i,j;

   unsigned int len1,len2;

   len1=strlen(str1);
   len2=strlen(str2);

   for (i=0; i+len2<=len1; i++)
      {
      for (j=0; j<len2; j++)
         if (tolower(str2[j])!=tolower(str1[i+j])) break;

      if (j==len2) return((char *)&str1[i]);
      }

   return(NULL);
   }

#endif

inline char *strdup2(const char *str1,const char *str2)
   {
   char *str;

   if (str1==NULL && str2==NULL) return(NULL);

   if (str1==NULL) return(strdup(str2));
   if (str2==NULL) return(strdup(str1));

   if ((str=(char *)malloc(strlen(str1)+strlen(str2)+1))==NULL) MEMERROR();

   memcpy(str,str1,strlen(str1));
   memcpy(str+strlen(str1),str2,strlen(str2)+1);

   return(str);
   }

#ifdef _WIN32
#include <stdio.h>
#include <direct.h>
inline int removefile(const char *file) {return(remove(file));}
inline int removedir(const char *dir) {return(_rmdir(dir));}
#else
#include <stdio.h>
#include <unistd.h>
inline int removefile(const char *file) {return(remove(file));}
inline int removedir(const char *dir) {return(rmdir(dir));}
#endif

#endif
