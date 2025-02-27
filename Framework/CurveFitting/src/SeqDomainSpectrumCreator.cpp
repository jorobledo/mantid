// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/SeqDomainSpectrumCreator.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/FunctionDomain1DSpectrumCreator.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/Matrix.h"

namespace Mantid::CurveFitting {

using namespace API;

/**
 * Constructor
 *
 * Manager can be a null-pointer if required in a context where no
 *PropertyManager is available. The same for the second argument,
 * it can be an empty string if the functionality is not required.
 *
 * @param manager :: Pointer to IPropertyManager instance.
 * @param workspacePropertyName :: Name of the output property for a created
 *workspace in case a PropertyManager is used.
 */
SeqDomainSpectrumCreator::SeqDomainSpectrumCreator(Kernel::IPropertyManager *manager,
                                                   const std::string &workspacePropertyName)
    : IDomainCreator(manager, std::vector<std::string>(1, workspacePropertyName), SeqDomainSpectrumCreator::Sequential),
      m_matrixWorkspace() {
  m_workspacePropertyName = m_workspacePropertyNames.front();
}

/**
 * Creates a sequential domain corresponding to the assigned MatrixWorkspace
 *
 * For each spectrum in the workspace, a FunctionDomain1DSpectrumCreator is
 *constructed and added to a SeqDomain. This way
 * each spectrum of the workspace is represented by its own, independent domain.
 *
 * @param domain :: Pointer to outgoing FunctionDomain instance.
 * @param values :: Pointer to outgoing FunctionValues object.
 * @param i0 :: Size offset for values object if it already contains data.
 */
void SeqDomainSpectrumCreator::createDomain(std::shared_ptr<FunctionDomain> &domain,
                                            std::shared_ptr<FunctionValues> &values, size_t i0) {
  setParametersFromPropertyManager();

  if (!m_matrixWorkspace) {
    throw std::invalid_argument("No matrix workspace assigned - can not create domain.");
  }

  SeqDomain *seqDomain = SeqDomain::create(m_domainType);

  size_t numberOfHistograms = m_matrixWorkspace->getNumberHistograms();
  for (size_t i = 0; i < numberOfHistograms; ++i) {
    if (histogramIsUsable(i)) {
      auto spectrumDomain = new FunctionDomain1DSpectrumCreator;
      spectrumDomain->setMatrixWorkspace(m_matrixWorkspace);
      spectrumDomain->setWorkspaceIndex(i);

      seqDomain->addCreator(IDomainCreator_sptr(spectrumDomain));
    }
  }

  domain.reset(seqDomain);

  if (!values) {
    values.reset(new FunctionValues(*domain));
  } else {
    values->expand(i0 + domain->size());
  }
}

/**
 * Creates an output workspace using the given function and domain
 *
 * This method creates a MatrixWorkspace with the same dimensions as the input
 *workspace that was assigned before or throws
 * std::invalid_argument if no valid workspace is present. The method also
 *checks that the provided domain is a SeqDomain.
 * The function needs to be able to handle a FunctionDomain1D-domain. If all
 *requirements are met, an output workspace
 * is created and populated with the calculated values.
 *
 * @param baseName :: Basename for output workspace.
 * @param function :: Function that can handle a FunctionDomain1D-domain.
 * @param domain :: Pointer to SeqDomain instance.
 * @param values :: Pointer to FunctionValues instance, currently not used.
 * @param outputWorkspacePropertyName :: Name of output workspace property, if
 *used.
 * @return MatrixWorkspace with calculated values.
 */

Workspace_sptr SeqDomainSpectrumCreator::createOutputWorkspace(const std::string &baseName, IFunction_sptr function,
                                                               std::shared_ptr<FunctionDomain> domain,
                                                               std::shared_ptr<FunctionValues> values,
                                                               const std::string &outputWorkspacePropertyName) {
  // don't need values, since the values need to be calculated spectrum by
  // spectrum (see loop below).
  UNUSED_ARG(values);

  std::shared_ptr<SeqDomain> seqDomain = std::dynamic_pointer_cast<SeqDomain>(domain);

  if (!seqDomain) {
    throw std::invalid_argument("CreateOutputWorkspace requires SeqDomain.");
  }

  if (!m_matrixWorkspace) {
    throw std::invalid_argument("No MatrixWorkspace assigned. Cannot construct "
                                "proper output workspace.");
  }

  MatrixWorkspace_sptr outputWs =
      std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create(m_matrixWorkspace));

  // Assign y-values, taking into account masked detectors
  for (size_t i = 0; i < seqDomain->getNDomains(); ++i) {
    FunctionDomain_sptr localDomain;
    FunctionValues_sptr localValues;

    seqDomain->getDomainAndValues(i, localDomain, localValues);
    function->function(*localDomain, *localValues);

    std::shared_ptr<FunctionDomain1DSpectrum> spectrumDomain =
        std::dynamic_pointer_cast<FunctionDomain1DSpectrum>(localDomain);

    if (spectrumDomain) {
      size_t wsIndex = spectrumDomain->getWorkspaceIndex();

      auto &yValues = outputWs->mutableY(wsIndex);
      for (size_t j = 0; j < yValues.size(); ++j) {
        yValues[j] = localValues->getCalculated(j);
      }
    }
  }

  // Assign x-values on all histograms
  for (size_t i = 0; i < m_matrixWorkspace->getNumberHistograms(); ++i) {
    outputWs->setSharedX(i, m_matrixWorkspace->sharedX(i));
  }

  if (m_manager && !outputWorkspacePropertyName.empty()) {
    declareProperty(new WorkspaceProperty<MatrixWorkspace>(outputWorkspacePropertyName, "", Kernel::Direction::Output),
                    "Result workspace");

    m_manager->setPropertyValue(outputWorkspacePropertyName, baseName + "Workspace");
    m_manager->setProperty(outputWorkspacePropertyName, outputWs);
  }

  // If the input is a not an EventWorkspace and is a distrubution, then convert
  // the output also to a distribution
  if (!std::dynamic_pointer_cast<Mantid::API::IEventWorkspace>(m_matrixWorkspace)) {
    if (m_matrixWorkspace->isDistribution()) {
      outputWs->setDistribution(true);
    }
  }

  return outputWs;
}

/**
 * Returns the domain size. Throws if no MatrixWorkspace has been set.
 *
 * @return Total domain size.
 */
size_t SeqDomainSpectrumCreator::getDomainSize() const {
  if (!m_matrixWorkspace) {
    throw std::invalid_argument("No matrix workspace assigned.");
  }

  size_t nHist = m_matrixWorkspace->getNumberHistograms();
  size_t totalSize = 0;

  for (size_t i = 0; i < nHist; ++i) {
    totalSize += m_matrixWorkspace->y(i).size();
  }

  return totalSize;
}

/// Tries to extract a workspace from the assigned property manager.
void SeqDomainSpectrumCreator::setParametersFromPropertyManager() {
  if (m_manager) {
    Workspace_sptr workspace = m_manager->getProperty(m_workspacePropertyName);

    setMatrixWorkspace(std::dynamic_pointer_cast<MatrixWorkspace>(workspace));
  }
}

/// Sets the MatrixWorkspace the created domain is based on.
void SeqDomainSpectrumCreator::setMatrixWorkspace(const MatrixWorkspace_sptr &matrixWorkspace) {
  if (!matrixWorkspace) {
    throw std::invalid_argument("InputWorkspace must be a valid MatrixWorkspace.");
  }

  m_matrixWorkspace = matrixWorkspace;
}

/// Determines whether a spectrum is masked, in case there is no instrument this
/// always returns true.
bool SeqDomainSpectrumCreator::histogramIsUsable(size_t i) const {
  if (!m_matrixWorkspace) {
    throw std::invalid_argument("No matrix workspace assigned.");
  }

  const auto &spectrumInfo = m_matrixWorkspace->spectrumInfo();

  if (!spectrumInfo.hasDetectors(i)) {
    return true;
  }
  return !spectrumInfo.isMasked(i);
}

} // namespace Mantid::CurveFitting
