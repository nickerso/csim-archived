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
