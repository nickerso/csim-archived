#ifndef _UTILS_HXX_
#define _UTILS_HXX_

#define RETURN_INTO_STRING(lhs, rhs)  \
  char* tmp_##lhs = rhs;              \
  std::string lhs;                    \
  if (tmp_##lhs)                      \
  {                                   \
    lhs = std::string(tmp_##lhs);     \
    free(tmp_##lhs);                  \
  }

#define GET_SET_WSTRING( GET , SET ) \
  {                                  \
    wchar_t* str = GET;              \
    if (str)                         \
    {                                \
      SET = std::wstring(str);       \
      free(str);                     \
    }                                \
  }

#define GET_SET_URI( GET , SET )          \
  {                                       \
    iface::cellml_api::URI* uri = GET;    \
    if (uri)                              \
    {                                     \
      SET = uri->asText();                \
      uri->release_ref();                 \
    }                                     \
  }

#define CELLML_TO_VARIABLE_INTERFACE( cellml, variable)                 \
  if (cellml == iface::cellml_api::INTERFACE_IN) variable(Variable::IN); \
  else if (cellml == iface::cellml_api::INTERFACE_OUT)                  \
    variable(Variable::OUT);                                              \
  else if (cellml == iface::cellml_api::INTERFACE_NONE)                 \
    variable(Variable::NONE);                                             \
  else ERROR("CELLML_TO_VARIABLE_INTERFACE","Invalid variable interface\n")

#endif /* _UTILS_HXX_ */
