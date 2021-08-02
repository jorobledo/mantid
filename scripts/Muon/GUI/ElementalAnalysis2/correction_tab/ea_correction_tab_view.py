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
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_calibration_correction_view import EACalibrationCorrectionTabView
from Muon.GUI.Common.data_selectors.cyclic_data_selector_view import CyclicDataSelectorView

DEFAULT_MINIMUM_ENERGY = 100
DEFAULT_MAXIMUM_ENERGY = 1000


class EACorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EACorrectionTabView, self).__init__(parent=parent)
        self.calibration_view = EACalibrationCorrectionTabView(parent=self)
        self.efficiency_view = EAEfficiencyCorrectionTabView(parent=self)
        self.absorption_view = EAAbsorptionCorrectionTabView(parent=self)
        self.data_selector = CyclicDataSelectorView(parent=self)
        self.setup_widget_view()

    def setup_widget_view(self):
        self.calculate_button = QtWidgets.QPushButton("Apply corrections", self)
        self.horizontal_layout1 = QtWidgets.QHBoxLayout()
        self.horizontal_layout1.addWidget(self.data_selector)
        self.horizontal_layout1.addWidget(self.calculate_button)

        self.energy_start_label = QtWidgets.QLabel("Minimum energy (KeV):")
        self.energy_start_line_edit = QtWidgets.QLineEdit(self)
        self.energy_start_line_edit.setText(str(DEFAULT_MINIMUM_ENERGY))
        self.energy_end_label = QtWidgets.QLabel("Maximum energy (KeV):")
        self.energy_end_line_edit = QtWidgets.QLineEdit(self)
        self.energy_end_line_edit.setText(str(DEFAULT_MAXIMUM_ENERGY))
        self.horizontal_layout2 = QtWidgets.QHBoxLayout()
        self.horizontal_layout2.addWidget(self.energy_start_label)
        self.horizontal_layout2.addWidget(self.energy_start_line_edit)
        self.horizontal_layout2.addWidget(self.energy_end_label)
        self.horizontal_layout2.addWidget(self.energy_end_line_edit)

        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.vertical_layout.addLayout(self.horizontal_layout1)
        self.vertical_layout.addLayout(self.horizontal_layout2)
        self.vertical_layout.addWidget(self.calibration_view)
        self.vertical_layout.addWidget(self.efficiency_view)
        self.vertical_layout.addWidget(self.absorption_view)

        self.setLayout(self.vertical_layout)

    def add_workspace_to_view(self, workspaces):
        self.data_selector.update_dataset_names_combo_box(workspaces)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def calculate_corrections_slot(self, slot):
        self.calculate_button.clicked.connect(slot)

    def select_effieciency_file_slot(self, slot):
        self.efficiency_view.select_detector_efficiency_file_slot(slot)

    def set_efficiency_data_file_label_text(self, filename):
        self.efficiency_view.set_efficiency_data_file_label_text(filename)

    def select_absorption_coefficient_file_slot(self, slot):
        self.absorption_view.absorption_coefficient_file_button.clicked.connect(slot)

    def set_absorption_coefficient_data_file_label_text(self, filename):
        self.absorption_view.absorption_coefficient_file_label.setText(filename)

    def get_initial_parameters(self):
        params = {}
        group_name = self.data_selector.current_dataset_name
        params["group_name"] = group_name
        params["energy_start"] = self.energy_start_line_edit.text()
        params["energy_end"] = self.energy_end_line_edit.text()
        return params
