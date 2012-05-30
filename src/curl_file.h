
#ifndef _CURL_FILE_H_
#define _CURL_FILE_H_

/* From the cURL example program to provide ability to open files using
 * URI's rather than assuming local file paths.
 */

typedef struct fcurl_data URL_FILE;

/* exported functions */
URL_FILE *url_fopen(const char *url,const char *operation);
int url_fclose(URL_FILE *file);
int url_feof(URL_FILE *file);
size_t url_fread(void *ptr, size_t size, size_t nmemb, URL_FILE *file);
char * url_fgets(char *ptr, int size, URL_FILE *file);
void url_rewind(URL_FILE *file);

#endif /* _CURL_FILE_H_ */
