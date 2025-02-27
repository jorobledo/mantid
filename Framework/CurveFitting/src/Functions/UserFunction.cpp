// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/UserFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MuParserUtils.h"
#include "MantidGeometry/muParser_Silent.h"
#include <boost/tokenizer.hpp>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

// Register the class into the function factory
DECLARE_FUNCTION(UserFunction)

using Mantid::API::MuParserUtils::extraOneVarFunctions;
using namespace Kernel;
using namespace API;

/// Constructor
UserFunction::UserFunction() : m_parser(new mu::Parser()), m_x(0.), m_x_set(false) { extraOneVarFunctions(*m_parser); }

/// Destructor
UserFunction::~UserFunction() { delete m_parser; }

/** Static callback function used by MuParser to initialize variables implicitly
@param varName :: The name of a new variable
@param pufun :: Pointer to the function
*/
double *UserFunction::AddVariable(const char *varName, void *pufun) {
  UserFunction &fun = *reinterpret_cast<UserFunction *>(pufun);

  if (std::string(varName) != "x") {
    try {
      fun.declareParameter(varName, 0.0);
    } catch (...) {
    }
  } else {
    fun.m_x_set = true;
    fun.m_x = 0.;
  }

  return &fun.m_x;
}

/**  Declare fit parameters using muParser's implicit variable initialization.
 * @param attName :: Attribute name, must be "Formula"
 * @param value :: The attribute value. For "Formula" it must be a mu::Parser
 * expression string
 */
void UserFunction::setAttribute(const std::string &attName, const Attribute &value) {
  IFunction::setAttribute(attName, value);

  if (attName != "Formula") {
    return;
  }

  m_x_set = false;
  clearAllParameters();

  try {
    mu::Parser tmp_parser;
    extraOneVarFunctions(tmp_parser);
    tmp_parser.SetVarFactory(AddVariable, this);

    m_formula = value.asString();
    tmp_parser.SetExpr(m_formula);

    // Call Eval() to implicitly initialize the variables
    tmp_parser.Eval();

  } catch (...) {
    // Formula may be edited by a GUI component
    return;
  }

  if (!m_x_set) {
    // Formula may be edited by a GUI component
    return;
  }

  m_parser->ClearVar();
  m_parser->DefineVar("x", &m_x);
  for (size_t i = 0; i < nParams(); i++) {
    m_parser->DefineVar(parameterName(i), getParameterAddress(i));
  }

  m_parser->SetExpr(m_formula);
}

/** Calculate the fitting function.
 *  @param out :: A pointer to the output fitting function buffer. The buffer
 * must be large enough to receive nData double values.
 *        The fitting procedure will try to minimise Sum(out[i]^2)
 *  @param xValues :: The array of nData x-values.
 *  @param nData :: The size of the fitted data.
 */
void UserFunction::function1D(double *out, const double *xValues, const size_t nData) const {
  for (size_t i = 0; i < nData; i++) {
    m_x = xValues[i];
    try {
      out[i] = m_parser->Eval();
    } catch (mu::Parser::exception_type &e) {
      throw std::invalid_argument("Error evaluating function: " + e.GetMsg());
    }
  }
}

/**
 * @param domain :: the space on which the function acts
 * @param jacobian :: the set of partial derivatives of the function with
 * respect to the fitting parameters
 */
void UserFunction::functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

} // namespace Mantid::CurveFitting::Functions
