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


class EACorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EACorrectionTabView, self).__init__(parent=parent)
        self.calibration_view = EACalibrationCorrectionTabView(parent=self)
        self.efficiency_view = EAEfficiencyCorrectionTabView(parent=self)
        self.absorption_view = EAAbsorptionCorrectionTabView(parent=self)
        self.setup_widget_view()
        self.setup_active_elements()

    def setup_widget_view(self):
        self.calculate_button = QtWidgets.QPushButton("Apply corrections", self)
        self.group_combobox = QtWidgets.QComboBox(self)
        self.detector_combobox = QtWidgets.QComboBox(self)
        self.horizontal_layout1 = QtWidgets.QHBoxLayout()
        self.horizontal_layout1.addWidget(self.group_combobox)
        self.horizontal_layout1.addWidget(self.detector_combobox)
        self.horizontal_layout1.addWidget(self.calculate_button)

        self.energy_start_label = QtWidgets.QLabel("Minimum energy (KeV):")
        self.energy_start_line_edit = QtWidgets.QLineEdit(self)
        self.energy_end_label = QtWidgets.QLabel("Maximum energy (KeV):")
        self.energy_end_line_edit = QtWidgets.QLineEdit(self)
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

    def setup_active_elements(self):
        self.group_combobox.currentIndexChanged.connect(self.on_group_combobox_changed)

    def add_workspace_to_view(self, workspaces):
        self.group_combobox.clear()
        self.detector_combobox.clear()
        self.workspaces_in_view = workspaces
        self.group_combobox.addItems(sorted(list(self.workspaces_in_view)))
        self.on_group_combobox_changed()

    def on_group_combobox_changed(self):
        self.detector_combobox.clear()
        group_workspace = self.group_combobox.currentText()
        if not group_workspace:
            return
        detectors = self.workspaces_in_view[group_workspace]
        self.detector_combobox.addItems(detectors)

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
        run = self.group_combobox.currentText()
        detector = self.detector_combobox.currentText()
        group_name = "; ".join([run, detector])
        params["group_name"] = group_name
        params["energy_start"] = self.energy_start_line_edit.text()
        params["energy_end"] = self.energy_end_line_edit.text()
        return params
