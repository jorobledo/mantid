// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DensityOfStates.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

#include <QFileInfo>
#include <QString>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("DensityOfStates");
} // namespace

namespace MantidQt::CustomInterfaces {
DensityOfStates::DensityOfStates(QWidget *parent) : IndirectSimulationTab(parent) {
  m_uiForm.setupUi(parent);
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_uiForm.ipoPlotOptions, PlotWidget::Spectra));

  connect(m_uiForm.mwInputFile, SIGNAL(filesFound()), this, SLOT(handleFileChange()));

  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));

  m_uiForm.lwIons->setSelectionMode(QAbstractItemView::MultiSelection);
}

void DensityOfStates::setup() {}

/**
 * Validate the form to check the program can be run.
 *
 * @return Whether the form was valid
 */
bool DensityOfStates::validate() {
  UserInputValidator uiv;

  const auto filename = m_uiForm.mwInputFile->getFirstFilename();
  InputFormat format = filenameToFormat(filename);
  QString specType = m_uiForm.cbSpectrumType->currentText();
  auto items = m_uiForm.lwIons->selectedItems();

  if (specType == "DensityOfStates" && isPdosFile(format) && items.size() < 1)
    uiv.addErrorMessage("Must select at least one ion for DensityOfStates.");

  // Give error message when there are errors
  if (!uiv.isAllInputValid())
    emit showMessageBox(uiv.generateErrorMessage());

  return uiv.isAllInputValid();
}

/**
 * Configures and executes the DensityOfStates algorithm.
 */
void DensityOfStates::run() {
  setRunIsRunning(true);

  // Get the SimulatedDensityOfStates algorithm
  auto dosAlgo = AlgorithmManager::Instance().create("SimulatedDensityOfStates");

  const auto filename = m_uiForm.mwInputFile->getFirstFilename();
  const auto specType = m_uiForm.cbSpectrumType->currentText();
  const auto filePropName = formatToFilePropName(filenameToFormat(filename));

  // QFileInfo inputFileInfo(filename);

  m_outputWsName = QFileInfo(filename).baseName() + "_" + specType;

  // Set common properties
  dosAlgo->setProperty(filePropName, filename.toStdString());
  dosAlgo->setProperty("OutputWorkspace", m_outputWsName.toStdString());

  const auto peakShape = m_uiForm.cbPeakShape->currentText().toStdString();
  dosAlgo->setProperty("Function", peakShape);

  const auto peakWidth = m_uiForm.spPeakWidth->text().toStdString();
  dosAlgo->setProperty("PeakWidth", peakWidth);

  const auto binWidth = m_uiForm.spBinWidth->value();
  dosAlgo->setProperty("BinWidth", binWidth);

  const auto zeroThreshold = m_uiForm.spZeroThreshold->value();
  dosAlgo->setProperty("ZeroThreshold", zeroThreshold);

  const auto scale = m_uiForm.ckScale->isChecked();
  if (scale) {
    const auto scaleFactor = m_uiForm.spScale->value();
    dosAlgo->setProperty("Scale", scaleFactor);
  }

  // Set spectrum type specific properties
  if (specType == "DensityOfStates") {
    dosAlgo->setProperty("SpectrumType", "DOS");

    const auto crossSectionScale = m_uiForm.ckCrossSectionScale->isChecked();
    const auto crossSectionScaleType = m_uiForm.cbCrossSectionScale->currentText().toStdString();
    if (crossSectionScale)
      dosAlgo->setProperty("ScaleByCrossSection", crossSectionScaleType);

    const auto outputFormat = m_uiForm.cbOutputFormat->currentIndex();
    if (outputFormat == 1) {
      dosAlgo->setProperty("SumContributions", true);
    }
    if (outputFormat == 2) {
      dosAlgo->setProperty("CalculateIonIndices", true);
    }

    std::vector<std::string> selectedIons;
    auto items = m_uiForm.lwIons->selectedItems();
    for (auto &item : items)
      selectedIons.emplace_back(item->text().toStdString());
    dosAlgo->setProperty("Ions", selectedIons);
  } else if (specType == "IR") {
    dosAlgo->setProperty("SpectrumType", "IR_Active");
  } else if (specType == "Raman") {
    dosAlgo->setProperty("SpectrumType", "Raman_Active");

    const auto temperature = m_uiForm.spTemperature->value();
    dosAlgo->setProperty("Temperature", temperature);
  }

  m_batchAlgoRunner->addAlgorithm(dosAlgo);

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(dosAlgoComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handles completion of the SimulatedDensityOfStates algorithm.
 *
 * @param error If the algorithm failed
 */
void DensityOfStates::dosAlgoComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(dosAlgoComplete(bool)));

  setRunIsRunning(false);
  if (error)
    setSaveEnabled(false);
  else
    setOutputPlotOptionsWorkspaces({m_outputWsName.toStdString()});
}

/**
 * Handles a new file being selected by the browser.
 */
void DensityOfStates::handleFileChange() {
  QString filename = m_uiForm.mwInputFile->getFirstFilename();
  InputFormat fileFormat = filenameToFormat(filename);
  bool pdosAvailable = isPdosFile(fileFormat);

  if (pdosAvailable) {
    // Load the ion table to populate the list of ions
    IAlgorithm_sptr ionTableAlgo = AlgorithmManager::Instance().create("SimulatedDensityOfStates");
    ionTableAlgo->initialize();
    ionTableAlgo->setProperty(formatToFilePropName(fileFormat), filename.toStdString());
    ionTableAlgo->setProperty("SpectrumType", "IonTable");
    ionTableAlgo->setProperty("OutputWorkspace", "__dos_ions");

    m_batchAlgoRunner->addAlgorithm(ionTableAlgo);

    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(ionLoadComplete(bool)));
    m_batchAlgoRunner->executeBatchAsync();
  } else {
    m_uiForm.lwIons->clear();
    m_uiForm.ckCrossSectionScale->setChecked(false);
  }

  // Enable partial DOS related optons when they can be used
  m_uiForm.lwIons->setEnabled(pdosAvailable);
  m_uiForm.pbSelectAllIons->setEnabled(pdosAvailable);
  m_uiForm.pbDeselectAllIons->setEnabled(pdosAvailable);
  m_uiForm.ckCrossSectionScale->setEnabled(pdosAvailable);
}

/**
 * Handles the algorithm loading the list of ions in a file
 * being completed.
 *
 * @param error If the algorithm failed
 */
void DensityOfStates::ionLoadComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(ionLoadComplete(bool)));

  if (error) {
    g_log.error("Could not get a list of ions from input file");
  } else {

    // Get the list of ions from algorithm
    auto ionTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("__dos_ions");
    Column_sptr ionColumn = ionTable->getColumn("Species");
    size_t numIons = ionColumn->size();

    // Remove old ions
    m_uiForm.lwIons->clear();

    // Add ions to list
    QStringList ionSpecies;
    for (size_t ion = 0; ion < numIons; ion++) {
      const QString species = QString::fromStdString(ionColumn->cell<std::string>(ion));
      if (!ionSpecies.contains(species))
        ionSpecies << species;
    }
    m_uiForm.lwIons->addItems(ionSpecies);

    // Select all ions by default
    m_uiForm.lwIons->selectAll();
  }
}

/**
 * Set the data selectors to use the default save directory
 * when browsing for input files.
 *
 * @param settings :: The settings to loading into the interface
 */
void DensityOfStates::loadSettings(const QSettings &settings) { m_uiForm.mwInputFile->readSettings(settings.group()); }

void DensityOfStates::runClicked() {
  clearOutputPlotOptionsWorkspaces();
  runTab();
}

/**
 * Handle saving of workspace
 */
void DensityOfStates::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_outputWsName.toStdString(), false))
    addSaveWorkspaceToQueue(m_outputWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

void DensityOfStates::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void DensityOfStates::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setSaveEnabled(enabled);
}

void DensityOfStates::setRunEnabled(bool enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void DensityOfStates::setSaveEnabled(bool enabled) { m_uiForm.pbSave->setEnabled(enabled); }

/**
 * Handle file formats
 */

enum class DensityOfStates::InputFormat : int { Unsupported = 0, Phonon, Castep, ForceConstants };

DensityOfStates::InputFormat DensityOfStates::filenameToFormat(QString filename) {
  QFileInfo inputFileInfo(filename);
  const auto suffix = inputFileInfo.suffix().toStdString();

  InputFormat format;

  if (suffix == "phonon") {
    format = InputFormat::Phonon;
  } else if (suffix == "castep") {
    format = InputFormat::Castep;
  } else if (suffix == "castep_bin") {
    format = InputFormat::ForceConstants;
  } else if (suffix == "yaml") {
    format = InputFormat::ForceConstants;
  } else {
    format = InputFormat::Unsupported;
  }

  return format;
}

std::string DensityOfStates::formatToFilePropName(InputFormat format) {
  std::string filePropName;

  switch (format) {
  case InputFormat::Phonon:
    filePropName = "PHONONFile";
    break;
  case InputFormat::Castep:
    filePropName = "CASTEPFile";
    break;
  case InputFormat::ForceConstants:
    filePropName = "ForceConstantsFile";
    break;
  default:
    g_log.error("Could not determine appropriate input field for this file type. ");
  }

  return filePropName;
}

bool DensityOfStates::isPdosFile(InputFormat dosFileFormat) {
  return (dosFileFormat == InputFormat::Phonon) || (dosFileFormat == InputFormat::ForceConstants);
}

} // namespace MantidQt::CustomInterfaces
