// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------
// Includes
//----------------------
#include "MantidQtWidgets/Plugins/AlgorithmDialogs/LoadDAEDialog.h"
#include "MantidQtWidgets/Common/AlgorithmInputHistory.h"

#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QValidator>

namespace MantidQt::CustomDialogs {

DECLARE_DIALOG(LoadDAEDialog)

/// An object for constructing a shared_ptr that won't ever delete its pointee
class NoDeleting {
public:
  /// Does nothing
  void operator()(void * /*unused*/) {}
  /// Does nothing
  void operator()(const void * /*unused*/) {}
};

LoadDAEDialog::LoadDAEDialog(QWidget *parent)
    : MantidQt::API::AlgorithmDialog(parent), lineHost(nullptr), lineName(nullptr), minSpLineEdit(nullptr),
      maxSpLineEdit(nullptr), listSpLineEdit(nullptr), updateLineEdit(nullptr) {}

LoadDAEDialog::~LoadDAEDialog() {}

void LoadDAEDialog::initLayout() {
  auto *paramsLayout = new QGridLayout;
  auto *label = new QLabel(tr("DAE Name"));
  lineHost = new QLineEdit;
  label->setBuddy(lineHost);
  paramsLayout->addWidget(label, 0, 0);
  paramsLayout->addWidget(lineHost, 0, 1);
  tie(lineHost, "DAEname", paramsLayout);

  label = new QLabel(tr("Workspace Name"));
  lineName = new QLineEdit;
  label->setBuddy(lineName);
  paramsLayout->addWidget(label, 1, 0);
  paramsLayout->addWidget(lineName, 1, 1);
  tie(lineName, "OutputWorkspace", paramsLayout);

  auto *minSpLabel = new QLabel("Starting spectrum");
  minSpLineEdit = new QLineEdit;
  paramsLayout->addWidget(minSpLabel, 2, 0);
  paramsLayout->addWidget(minSpLineEdit, 2, 1);
  tie(minSpLineEdit, "SpectrumMin", paramsLayout);

  auto *maxSpLabel = new QLabel("Ending spectrum");
  maxSpLineEdit = new QLineEdit;
  paramsLayout->addWidget(maxSpLabel, 3, 0);
  paramsLayout->addWidget(maxSpLineEdit, 3, 1);
  tie(maxSpLineEdit, "SpectrumMax", paramsLayout);

  auto *listSpLabel = new QLabel("Spectrum List");
  listSpLineEdit = new QLineEdit;
  paramsLayout->addWidget(listSpLabel, 4, 0);
  paramsLayout->addWidget(listSpLineEdit, 4, 1);
  tie(listSpLineEdit, "SpectrumList", paramsLayout);

  auto *updateLayout = new QHBoxLayout;
  auto *updateLabel = new QLabel("Update every");
  updateLineEdit = new QLineEdit;
  QIntValidator *ival = new QIntValidator(1, 99999999, updateLineEdit);
  updateLineEdit->setValidator(ival);

  label = new QLabel(" seconds");
  paramsLayout->addWidget(updateLabel, 5, 0);
  updateLayout->addWidget(updateLineEdit);
  updateLayout->addWidget(label);
  paramsLayout->addLayout(updateLayout, 5, 1);
  tie(updateLineEdit, "UpdateRate", updateLayout);

  auto *mainLayout = new QVBoxLayout;
  mainLayout->addLayout(paramsLayout);
  mainLayout->addLayout(createDefaultButtonLayout("?", "Load", "Cancel"));

  setLayout(mainLayout);
  setWindowTitle(tr("Load Workspace from DAE"));
  setFixedHeight(sizeHint().height());
}
} // namespace MantidQt::CustomDialogs
