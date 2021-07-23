# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_view import EACorrectionTabView
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model import EACorrectionTabModel
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_presenter import EACorrectionTabPresenter


class EACorrectionTabWidget:

    def __init__(self, context):
        self.correction_tab_view = EACorrectionTabView()
        self.correction_tab_model = EACorrectionTabModel(context)
        self.correction_tab_presenter = EACorrectionTabPresenter(self.correction_tab_view, self.correction_tab_model,
                                                                 context)
