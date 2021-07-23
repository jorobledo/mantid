# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from Muon.GUI.Common import message_box
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_absorption_correction_view import EAAbsorptionCorrectionTabView
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_efficiency_correction_view import EAEfficiencyCorrectionTabView
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_shift_correction_view import EAShiftCorrectionTabView


class EACorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EACorrectionTabView, self).__init__(parent=parent)
        self.shift_view = EAShiftCorrectionTabView(parent=self)
        self.efficiency_view = EAEfficiencyCorrectionTabView(parent=self)
        self.absorption_view = EAAbsorptionCorrectionTabView(parent=self)
        self.setup_widget_view()

    def setup_widget_view(self):
        self.calculate_button = QtWidgets.QPushButton("Calculate all corrections", self)
        self.group_combobox = QtWidgets.QComboBox(self)
        self.detector_combobox = QtWidgets.QComboBox(self)
        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.addWidget(self.group_combobox)
        self.horizontal_layout.addWidget(self.detector_combobox)
        self.horizontal_layout.addWidget(self.calculate_button)

        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.vertical_layout.addLayout(self.horizontal_layout)
        self.vertical_layout.addWidget(self.shift_view)
        self.vertical_layout.addWidget(self.efficiency_view)
        self.vertical_layout.addWidget(self.absorption_view)

        self.setLayout(self.vertical_layout)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)
