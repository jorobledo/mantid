// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataObjects/CoordTransformDistanceParser.h"
#include "MantidAPI/SingleValueParameterParser.h"
#include "MantidAPI/VectorParameterParser.h"
#include "MantidDataObjects/CoordTransformDistance.h"

namespace Mantid::DataObjects {
/// Constructor
CoordTransformDistanceParser::CoordTransformDistanceParser() {}

//-----------------------------------------------------------------------------------------------
/*
Create the transform object.
@param coordTransElement : xml coordinate transform element
@return a fully constructed coordinate transform object.
*/
Mantid::API::CoordTransform *
CoordTransformDistanceParser::createTransform(Poco::XML::Element *coordTransElement) const {
  // Typdef the parameter parsers required.
  using InDimParameterParser = Mantid::API::SingleValueParameterParser<Mantid::API::InDimParameter>;
  using OutDimParameterParser = Mantid::API::SingleValueParameterParser<Mantid::API::OutDimParameter>;
  using CoordCenterParser = Mantid::API::VectorParameterParser<CoordCenterVectorParam>;
  using DimsUsedParser = Mantid::API::VectorParameterParser<DimensionsUsedVectorParam>;

  using namespace Poco::XML;
  if ("CoordTransform" != coordTransElement->localName()) {
    std::string message = "This is not a coordinate transform element: " + coordTransElement->localName();
    throw std::invalid_argument(message);
  }
  if ("CoordTransformDistance" != coordTransElement->getChildElement("Type")->innerText()) {
    // Delegate
    if (!m_successor) {
      throw std::runtime_error("CoordTransformDistanceParser has no successor parser.");
    }
    return m_successor->createTransform(coordTransElement);
  }

  Element *paramListElement = coordTransElement->getChildElement("ParameterList");
  Poco::AutoPtr<Poco::XML::NodeList> parameters = paramListElement->getElementsByTagName("Parameter");

  // Parse the in dimension parameter.
  InDimParameterParser inDimParamParser;
  auto *parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(0));
  std::shared_ptr<Mantid::API::InDimParameter> inDimParameter(inDimParamParser.createWithoutDelegation(parameter));

  // Parse the out dimension parameter.
  OutDimParameterParser outDimParamParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(1));
  std::shared_ptr<Mantid::API::OutDimParameter> outDimParameter(outDimParamParser.createWithoutDelegation(parameter));
  UNUSED_ARG(outDimParameter); // not actually used as an input.

  // Parse the coordinate centre parameter.
  CoordCenterParser coordCenterParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(2));
  std::shared_ptr<Mantid::DataObjects::CoordCenterVectorParam> coordCenterParam(
      coordCenterParser.createWithoutDelegation(parameter));

  // Parse the dimensions used parameter.
  DimsUsedParser dimsUsedParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(3));
  std::shared_ptr<Mantid::DataObjects::DimensionsUsedVectorParam> dimsUsedVecParm(
      dimsUsedParser.createWithoutDelegation(parameter));

  ////Generate the coordinate transform and return
  auto transform = new CoordTransformDistance(inDimParameter->getValue(), coordCenterParam->getPointerToStart(),
                                              dimsUsedVecParm->getPointerToStart());

  return transform;
}
} // namespace Mantid::DataObjects
