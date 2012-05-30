
#ifndef _CELLML_HPP_
#define _CELLML_HPP_


/* methods shared for other C++ code to use? */

/* a method to generate an absolute URL based on a model and a possibly
   relative initial URL */
std::wstring
resolveAbsoluteURL(iface::cellml_api::Model* aModel,std::wstring aURL);

#endif /* _CELLML_HPP_ */
