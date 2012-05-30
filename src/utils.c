
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <regex.h>

#include "utils.h"

#define WHITESPACE_STR  " \f\n\r\t\v"

static int _current_debug_level = 0;
static int _quiet_flag = 0;

char* strcopy(const char* string)
{
  char* s = (char*)NULL;
  if (string)
  {
    s = (char*)malloc(strlen(string)+1);
    strcpy(s,string);
  }
  return(s);
}

/* from http://sources.redhat.com/autobook/autobook/autobook_74.html */
char* trim(const char* string)
{
  DEBUG(10,"trim","Original string: \"%s\"\n",string);
  char *result = 0;
  if (string)
  {
    const char *ptr = string;
    /* Skip leading whitespace */
    while (strchr(WHITESPACE_STR,*ptr)) ++ptr;
    /* Make a copy of the remainder.  */
    result = strdup(ptr);
    DEBUG(10,"trim","trimmed front: \"%s\"\n",result);
    char* p = result;
    /* Move to the last character of the copy */
    for (;strlen(p) > 0;++p) /* NOWORK */;
    /* Remove trailing whitespace */
    for (--p;strchr(WHITESPACE_STR,*p);--p) *p = '\0';
    DEBUG(10,"trim","trimmed: \"%s\"\n",result);
  }
  return result;
}

/* concatenate variable number of strings and append to the <str> and
   return a pointer to the complete string */
char *vstrcat(char *first,...)
{
  size_t len = 0;
  va_list argp;
  char *p;
  char* str = first; /* cause str might be changed by realloc */

  va_start(argp,first);
  while((p = va_arg(argp,char *)) != NULL) len += strlen(p);
  va_end(argp);
  if (len == 0) return(str);
  
  if (first) len += strlen(first);
  str = (char*)realloc(str,len+1);
  if(str == NULL) return((char*)NULL);
  if (first == NULL) str[0]='\0';
  va_start(argp,first);
  while((p = va_arg(argp,char *)) != NULL) strcat(str,p);
  va_end(argp);

  return(str);
}

/* checks <uri> and returns an absolute URI */
/* FIXME: should consolidate on the cellml.cpp/resolveAbsoluteURL() method
   and simply pass in the CWD as the base_uri to resolve about */
char* getAbsoluteURI(const char* uri)
{
  if (uri)
  {
    if (strstr(uri,"://") != NULL)
    {
      /*printf("URI (%s) already absolute.\n",uri);*/
      char* abs = (char*)malloc(strlen(uri)+1);
      strcpy(abs,uri);
      return(abs);
    }
    else if (uri[0]=='/')
    {
      /*printf("URI (%s) absolute path, making absolute URI: ",uri);*/
      char* abs = (char*)malloc(strlen(uri)+1+7);
      sprintf(abs,"file://%s",uri);
      /*printf("%s\n",abs);*/
      return(abs);
    }
    else
    {
      /* relative filename ? append absoulte path */
      /*printf("URI (%s) is relative path, making absolute URI: ",uri);*/
      int size = pathconf(".",_PC_PATH_MAX);
      char* cwd = (char*)malloc(size);
      if (!getcwd(cwd,size)) cwd[0] = '\0';
      char* abs = (char*)malloc(strlen(cwd)+strlen(uri)+1+8);
      sprintf(abs,"file://%s/%s",cwd,uri);
      free(cwd);
      /*printf("%s\n",abs);*/
      return(abs);
    }
  }
  return((char*)NULL);
}

void setQuiet()
{
  _quiet_flag = 1;
}

int quietSet()
{
  return(_quiet_flag);
}

void setDebugLevel()
{
  _current_debug_level++;
}

int debugLevel()
{
  return(_current_debug_level);
}

/* given a URI with a fragment identifier (http://fred.com/resource.html#ID)
   return the URI part of the string */
char* getURIFromURIWithFragmentID(const char* uri)
{
  char* u = (char*)NULL;
  if (uri)
  {
    const char* hash = strchr(uri,'#');
    if (hash)
    {
      int l = strlen(uri) - strlen(hash);
      u = (char*)malloc(l+1);
      strncpy(u,uri,l);
      u[l] = '\0';
    }
    else
    {
      /* assume no fragment and return copy of original string */
      u = strcopy(uri);
    }
  }
  return(u);
}

/* given a URI with a fragment identifier (http://fred.com/resource.html#ID)
   return the fragment identifier */
char* getFragmentIDFromURIWithFragmentID(const char* uri)
{
  char* id = (char*)NULL;
  if (uri)
  {
    const char* hash = strchr(uri,'#');
    if (hash)
    {
      hash++;
      id = (char*)malloc(strlen(hash)+1);
      strcpy(id,hash);
    }
  }
  return(id);
}

/* since HDF also uses '/'s for group hierarchies we need to get
   rid of them when using URIs as identifiers */
char* uriRemovePathSeparator(const char* uri)
{
  char* string = (char*)NULL;
  if (uri && (strlen(uri) > 0))
  {
    string = strcopy(uri);
    char* m = (char*)NULL;
    while((m = strchr(string,'/'))) *m = '_';
  }
  return(string);
}

/* grab the text following the last / in a given URI */
char* getFilePart(const char* uri)
{
  char* file = (char*)NULL;
  if (uri && (strlen(uri) > 0))
  {
    char* s = strrchr(uri,'/');
    if (++s) file = strcopy(s);
    else file = strcopy(uri);
  }
  else INVALID_ARGS("getFilePart");
  return(file);
}

/* abbreviate a string by truncating it and adding ...
   if <front> is non-zero truncate the front of the string instead of the end.
 */
char* abbreviateString(const char* string,int length,int front)
{
  char* s = (char*)NULL;
  if (string)
  {
    if (strlen(string) < length) s = strcopy(string);
    else
    {
      if (front != 0)
      {
        const char* str = string;
        str += strlen(string) - length;
        VSTRCAT(s,"...",str);
      }
      else
      {
        char* str = (char*)malloc(sizeof(char)*(length+1));
        strncpy(str,string,length);
        str[length] = '\0';
        VSTRCAT(s,str,"...");
        free(str);
      }
    }
  }
  else INVALID_ARGS("abbreviateString");
  return(s);
}

/* return the file type based on the given mime type */
enum FileType getFileType(const char* mime_type)
{
  enum FileType type = UNKNOWN;
  if (mime_type)
  {
    if (strncmp("text/csv",mime_type,strlen("text/csv")) == 0)
    {
      /* CSV files:
         - text/csv
         - text/csv; header=present
         - text/csv; header=absent */
      type = CSV_UNKNOWN_HEADER;
      char* header = strstr(mime_type,"header");
      if (header)
      {
        if (strstr(header,"present") != NULL) type = CSV_WITH_HEADER;
        else if (strstr(header,"absent") != NULL) type = CSV_WITHOUT_HEADER;
      }
    }
    else ERROR("getFileType","Unknow mime type: %s\n",mime_type);
  }
  else INVALID_ARGS("getFileType");
  return(type);
}

/* split the given <string> on the character <c> and return the number of
   strings found in <N>. Drop any trailing new line character */
char** splitString(const char* string,char c,int* N)
{
  char** s = (char**)NULL;
  char* found = (char*)NULL;
  const char* tmp = string;
  int n = 0;
  while ((found = strchr(tmp,c)) != NULL)
  {
    n++;
    s = (char**)realloc(s,sizeof(char*)*n);
    int len = strlen(tmp)-strlen(found);
    s[n-1] = (char*)malloc(len+1);
    strncpy(s[n-1],tmp,len);
    s[n-1][len] = '\0';
    tmp = found+1;
  }
  if(strlen(tmp) > 0)
  {
    n++;
    s = (char**)realloc(s,sizeof(char*)*n);
    int len = strlen(tmp);
    if (tmp[len-1] == '\n') len--;
    s[n-1] = (char*)malloc(len+1);
    strncpy(s[n-1],tmp,len);
    s[n-1][len] = '\0';
  }
  *N = n;
  return(s);
}

/* Same as split, but return an array of doubles */
double* splitRealNumbers(const char* string,char c,int* N)
{
  char** numbers = splitString(string,c,N);
  double* n = (double*)malloc(sizeof(double)*(*N));
  int i;
  double* tmp = n;
  for (i=0;i<(*N);i++)
  {
    if (sscanf(numbers[i],"%lf",tmp)) tmp++;
    else
    {
      ERROR("splitRealNumbers","Error reading data %d from string.\n",i);
      *tmp = 0.0;
      tmp++;
    }
    free(numbers[i]);
  }
  free(numbers);
  return(n);
}

/* Same as split, but return an array of ints */
int* splitIntNumbers(const char* string,char c,int* N)
{
  char** numbers = splitString(string,c,N);
  int* n = (int*)malloc(sizeof(int)*(*N));
  int i;
  int* tmp = n;
  for (i=0;i<(*N);i++)
  {
    if (sscanf(numbers[i],"%d",tmp)) tmp++;
    else
    {
      ERROR("splitIntNumbers","Error reading data %d from string.\n",i);
      *tmp = 0.0;
      tmp++;
    }
    free(numbers[i]);
  }
  free(numbers);
  return(n);
}

int regex_match(const char *string,char *pattern)
{
  int status;
  regex_t re;
  if (regcomp(&re,pattern,REG_EXTENDED|REG_NOSUB) != 0)
  {
    return(0);      /* Report error. */
  }
  status = regexec(&re,string,(size_t)0,NULL,0);
  regfree(&re);
  if (status != 0) {
    return(0);      /* Report error. */
  }
  return(1);
}

