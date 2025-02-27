// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "Common/DllConfig.h"
#include "IPreviewModel.h"
#include "IPreviewPresenter.h"
#include "IPreviewView.h"
#include "InstViewModel.h"
#include "PreviewJobManager.h"
#include "PreviewModel.h"
#include "PreviewPresenter.h"
#include <memory>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

class PreviewPresenterFactory {
public:
  PreviewPresenterFactory() = default;

  std::unique_ptr<IPreviewPresenter> make(IPreviewView *view, IJobRunner *jobRunner,
                                          std::unique_ptr<IReflAlgorithmFactory> algFactory) {
    auto jobManager = std::make_unique<PreviewJobManager>(jobRunner, std::move(algFactory));
    auto dependencies = PreviewPresenter::Dependencies{view, std::make_unique<PreviewModel>(), std::move(jobManager),
                                                       std::make_unique<InstViewModel>()};
    return std::make_unique<PreviewPresenter>(std::move(dependencies));
  }
};
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
