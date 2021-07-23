# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from Muon.GUI.Common import message_box


class EAAbsorptionCorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EAAbsorptionCorrectionTabView, self).__init__(parent=parent)
        self.shape_table = QtWidgets.QTableWidget(parent=self)
        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.horizontal_layout1 = QtWidgets.QHBoxLayout()
        self.horizontal_layout2 = QtWidgets.QHBoxLayout()
        self.horizontal_layout3 = QtWidgets.QHBoxLayout()
        self.horizontal_layout4 = QtWidgets.QHBoxLayout()
        self.horizontal_layout5 = QtWidgets.QHBoxLayout()
        self.horizontal_layout6 = QtWidgets.QHBoxLayout()
        self.horizontal_layout7 = QtWidgets.QHBoxLayout()
        self.setup_widget_view()

    def setup_widget_view(self):
        self.shapes_label = QtWidgets.QLabel("Geometry type", parent=self)
        self.shapes_combobox = QtWidgets.QComboBox(parent=self)
        self.horizontal_layout1.addWidget(self.shapes_label)
        self.horizontal_layout1.addWidget(self.shapes_combobox)

        self.use_default_checkbox = QtWidgets.QCheckBox(parent=self)
        self.use_default_label = QtWidgets.QLabel(" Use default detector setting? ", parent=self)
        self.horizontal_layout2.addWidget(self.use_default_checkbox)
        self.horizontal_layout2.addWidget(self.use_default_label)

        self.detector_distance_label = QtWidgets.QLabel(" Distance (cm)", parent=self)
        self.detector_distance_lineedit = QtWidgets.QLineEdit(parent=self)
        self.detector_angle_label = QtWidgets.QLabel(" Angle in degrees ", parent=self)
        self.detector_angle_lineedit = QtWidgets.QLineEdit(parent=self)
        self.horizontal_layout3.addWidget(self.detector_distance_label)
        self.horizontal_layout3.addWidget(self.detector_distance_lineedit)
        self.horizontal_layout3.addWidget(self.detector_angle_label)
        self.horizontal_layout3.addWidget(self.detector_angle_lineedit)

        self.muon_profile_specifier_label = QtWidgets.QLabel(" Specify muon profle by ", parent=self)
        self.muon_profile_specifier_combobox = QtWidgets.QComboBox(parent=self)
        self.horizontal_layout4.addWidget(self.muon_profile_specifier_label)
        self.horizontal_layout4.addWidget(self.muon_profile_specifier_combobox)

        self.muon_depth_label = QtWidgets.QLabel(" Average muon depth (mm)", parent=self)
        self.muon_depth_lineedit = QtWidgets.QLineEdit(parent=self)
        self.muon_range_label = QtWidgets.QLabel(" Range of muons (mm)", parent=self)
        self.muon_range_lineedit = QtWidgets.QLineEdit(parent=self)
        self.horizontal_layout5.addWidget(self.muon_depth_label)
        self.horizontal_layout5.addWidget(self.muon_depth_lineedit)
        self.horizontal_layout5.addWidget(self.muon_range_label)
        self.horizontal_layout5.addWidget(self.muon_range_lineedit)

        self.muon_workspace_label = QtWidgets.QLabel(" Muon profile workspace ", parent=self)
        self.muon_workspace_lineedit = QtWidgets.QLineEdit(parent=self)
        self.horizontal_layout6.addWidget(self.muon_workspace_label)
        self.horizontal_layout6.addWidget(self.muon_workspace_lineedit)

        self.calculate_absorption_button = QtWidgets.QPushButton("Calculate absorption correction")
        self.horizontal_layout7.addWidget(self.calculate_absorption_button)

        self.vertical_layout.addLayout(self.horizontal_layout1)
        self.vertical_layout.addWidget(self.shape_table)
        self.vertical_layout.addLayout(self.horizontal_layout2)
        self.vertical_layout.addLayout(self.horizontal_layout3)
        self.vertical_layout.addLayout(self.horizontal_layout4)
        self.vertical_layout.addLayout(self.horizontal_layout5)
        self.vertical_layout.addLayout(self.horizontal_layout6)
        self.vertical_layout.addLayout(self.horizontal_layout7)
        self.setLayout(self.vertical_layout)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def create_table(self):
        pass
