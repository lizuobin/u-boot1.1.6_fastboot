

#ifndef __MEM_H__
#define __MEM_H__

void * memset(void * s,int c,size_t count);

int memcmp(const void * cs,const void * ct,size_t count);

int memcmp(const void * cs,const void * ct,size_t count);

int strcmp(const char * cs,const char * ct);

int strncmp(const char * cs,const char * ct,size_t count);

size_t strlen(const char * s);

char * strcpy(char * dest,const char *src);

char * strncpy(char * dest,const char *src,size_t count);

char * strcat(char * dest, const char * src);

#endif
