
#ifndef _XML_UTILS_H_
#define _XML_UTILS_H_

char* getRDFXMLString(const char* inputURI);

/* Useful test to work out if we can handle a given file - returns non-zero
   if the document element in the given XML file at <uri> is in the namespace
   <ns> with the local name <name>. Returns zero otherwise. */
int xmlFileIs(const char* uri,const char* ns,const char* name);

/* Get the contents of the given XML file as a standard C string */
char* getXMLFileContentsAsString(const char* uri);

#endif /* _XML_UTILS_H_ */

