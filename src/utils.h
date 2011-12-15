/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is CellMLSimulator.
 *
 * The Initial Developer of the Original Code is
 * David Nickerson <nickerso@users.sourceforge.net>.
 * Portions created by the Initial Developer are Copyright (C) 2007-2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
#ifndef _UTILS_H_
#define _UTILS_H_

/* Different types of (data) files that we might want to handle */
enum FileType
{
  UNKNOWN = 0,
  CSV_WITH_HEADER=1,
  CSV_WITHOUT_HEADER=2,
  CSV_UNKNOWN_HEADER=3
};
/* return the file type based on the given mime type */
enum FileType getFileType(const char* mime_type);

/* allocate memory and copy given string */
char* strcopy(const char* string);

/* return a copy of string with leading and trainling whitespace removed */
char* trim(const char* string);

/* concatenate variable number of strings and append to the <str> and
   return a pointer to the complete string.
   calls to this must end with a NULL argument? */
char *vstrcat(char *str,...);
#define VSTRCAT( STRING , ... )                     \
  STRING = vstrcat(STRING,__VA_ARGS__,(char*)NULL)

/* return the given <uri> as a absolute URI */
char* getAbsoluteURI(const char* uri);

/* given a URI with a fragment identifier (http://fred.com/resource.html#ID)
   return the URI part of the string */
char* getURIFromURIWithFragmentID(const char* uri);

/* given a URI with a fragment identifier (http://fred.com/resource.html#ID)
   return the fragment identifier */
char* getFragmentIDFromURIWithFragmentID(const char* uri);

/* since HDF also uses '/'s for group hierarchies we need to get
   rid of them when using URIs as identifiers */
char* uriRemovePathSeparator(const char* uri);

/* grab the text following the last / in a given URI */
char* getFilePart(const char* uri);

/* abbreviate a string by truncating it and adding ...
   if <front> is non-zero truncate the front of the string instead of the end.
 */
char* abbreviateString(const char* string,int length,int front);
#define URI_ABBREV_LENGTH 30 /* a reasonable length to truncate URIs */

/* split the given <string> on the character <c> and return the number of
   strings found in <N>. Drop any trailing new line character */
char** splitString(const char* string,char c,int* N);
/* Same as split, but return an array of doubles */
double* splitRealNumbers(const char* string,char c,int* N);
/* Same as split, but return an array of ints */
int* splitIntNumbers(const char* string,char c,int* N);

/* regular expression matching */
int regex_match(const char *string,char *pattern);

void setQuiet();
int quietSet();
void setDebugLevel();
int debugLevel();

#define DEBUG( LEVEL , METHOD , ... )                                   \
  if (debugLevel() > LEVEL) fprintf(stdout,"debug("METHOD"): "__VA_ARGS__)

#define ERROR( METHOD , ... ) \
  fprintf(stderr,"ERROR("METHOD"): "__VA_ARGS__)

#define WARNING( METHOD , ... ) \
  fprintf(stderr,"WARNING("METHOD"): "__VA_ARGS__)

#define MESSAGE( ... )                          \
  if (!quietSet()) fprintf(stdout,__VA_ARGS__)

#define INVALID_ARGS( METHOD ) DEBUG(-1,METHOD,"Invalid argument(s)\n")

#endif /* _UTILS_H_ */
