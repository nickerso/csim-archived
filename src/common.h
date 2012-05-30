
#ifndef _COMMON_H_
#define _COMMON_H_

#define OK  1   /* must test true */
#define ERR 0   /* must test false */

#define REAL_FORMAT "%0.8le"
#define REAL_FORMAT_W L"%0.8le"

#define CELLML_1_1_NS        "http://www.cellml.org/cellml/1.1#"
#define CELLML_1_0_NS        "http://www.cellml.org/cellml/1.0#"
#define CMETA_NS             "http://www.cellml.org/metadata/1.0#"
#define CS_NS                "http://www.cellml.org/metadata/simulation/1.0#"
#define CG_NS                "http://www.cellml.org/metadata/graphs/1.0#"
#define BQS_NS               "http://www.cellml.org/bqs/1.0#"
#define MATHML_NS            "http://www.w3.org/1998/Math/MathML"
#define RDF_NS               "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define RDF_DATATYPE_DOUBLE  "http://www.w3.org/2001/XMLSchema#double"
#define RDF_DATATYPE_BOOLEAN "http://www.w3.org/2001/XMLSchema#boolean"
#define RDF_DATATYPE_INTEGER "http://www.w3.org/2001/XMLSchema#integer"
#define BQMODEL_NS           "http://biomodels.net/model-qualifiers/"
#define DC_NS                "http://purl.org/dc/elements/1.1/"
#define DCTERMS_NS           "http://purl.org/dc/terms/"
#define VCARD_NS             "http://www.w3.org/2001/vcard-rdf/3.0#"
#define XML_NS               "http://www.w3.org/XML/1998/namespace"

#define ANALYSIS_NS          "http://cellml.sourceforge.net/analysis/1.0#"
#define CSIM_NS              "http://cellml.sourceforge.net/CellMLSimulator/#"

#define ZERO_TOL 1.0e-10

#endif /* _COMMON_H_ */
