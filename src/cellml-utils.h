
#ifndef _CELLML_UTILS_H_
#define _CELLML_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif

  char* wstring2string(const wchar_t* str);
  wchar_t* string2wstring(const char* str);
  char* getCellMLMetadataAsRDFXMLString(const char* uri);
  int updateModelInitialValues(const char* inputURI,const char* outputFile,
    char** variables,double* values,int N);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* _CELLML_UTILS_H_ */
