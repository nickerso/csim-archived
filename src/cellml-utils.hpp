
#ifndef _CELLML_UTILS_HPP_
#define _CELLML_UTILS_HPP_

/**
 * Load the model from the given URL, make sure the xml:base is set appropriately
 * and return the model as a string
 */
std::string modelUrlToString(const std::string& url);

#endif /* _CELLML_UTILS_HPP_ */
