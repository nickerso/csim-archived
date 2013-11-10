#ifndef XMLDOC_HPP
#define XMLDOC_HPP

class XmlDoc
{
public:public:
    XmlDoc();
    ~XmlDoc();
    int parseString(const std::string &data);
    int parseDocument(const char *url);
    std::string serialise(int format = 1);

    std::string getTextContent(const char* xpathExpr);
    int getDoubleContent(const char* xpathExpr, double* value);
    void* getCSimOutputVariables();
    std::string getVariableId(const char* xpathExpr, const std::map<std::string, std::string> &namespaces);

private:
    void* mXmlDocPtr;
};

#endif // XMLDOC_HPP
