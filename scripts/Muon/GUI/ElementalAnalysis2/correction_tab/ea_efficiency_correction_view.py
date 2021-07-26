# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from Muon.GUI.Common import message_box


class EAEfficiencyCorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EAEfficiencyCorrectionTabView, self).__init__(parent=parent)
        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.horizontal_layout1 = QtWidgets.QHBoxLayout()
        self.horizontal_layout2 = QtWidgets.QHBoxLayout()
        self.horizontal_layout3 = QtWidgets.QHBoxLayout()
        self.setup_widget_layout()
        self.setup_initial_state()
        self.setup_active_elements()

    def setup_widget_layout(self):
        self.add_efficiency_checkbox = QtWidgets.QCheckBox(parent=self)
        self.add_efficiency_label = QtWidgets.QLabel(" Add efficiencies to corrections ", parent=self)
        self.default_efficiency_checkbox = QtWidgets.QCheckBox(parent=self)
        self.default_efficiency_label = QtWidgets.QLabel(" Use default detector efficiency ", parent=self)

        self.horizontal_layout1.addWidget(self.add_efficiency_checkbox)
        self.horizontal_layout1.addWidget(self.add_efficiency_label)
        self.horizontal_layout1.addWidget(self.default_efficiency_checkbox)
        self.horizontal_layout1.addWidget(self.default_efficiency_label)

        self.efficiency_file_button = QtWidgets.QPushButton(" Select efficiency data file ", parent=self)
        self.efficiency_file_label = QtWidgets.QLabel("", parent=self)

        self.horizontal_layout2.addWidget(self.efficiency_file_button)
        self.horizontal_layout2.addWidget(self.efficiency_file_label)

        self.calculate_efficiency_button = QtWidgets.QPushButton(" Calculate efficiency corrections ", parent=self)

        self.horizontal_layout3.addWidget(self.calculate_efficiency_button)

        self.vertical_layout.addLayout(self.horizontal_layout1)
        self.vertical_layout.addLayout(self.horizontal_layout2)
        self.vertical_layout.addLayout(self.horizontal_layout3)
        self.setLayout(self.vertical_layout)

    def setup_active_elements(self):
        self.default_efficiency_checkbox.clicked.connect(self.on_default_efficiency_checkbox_changed)

    def setup_initial_state(self):
        self.show_efficiency_file_widgets(False)
        self.add_efficiency_checkbox.setChecked(False)
        self.default_efficiency_checkbox.setChecked(True)

    def show_efficiency_file_widgets(self, state):
        self.efficiency_file_label.setVisible(state)
        self.efficiency_file_button.setVisible(state)

    def on_default_efficiency_checkbox_changed(self):
        state = self.default_efficiency_checkbox.isChecked()
        self.show_efficiency_file_widgets(not state)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def get_efficiency_parameters(self):
        params = {"Add_efficiencies": self.add_efficiency_checkbox.checkState()}
        use_default = self.default_efficiency_checkbox.checkState()
        if use_default:
            params["use default efficiencies"] = use_default
        else:
            filepath = self.default_efficiency_label.text()
            if filepath:
                params["use default efficiencies"] = use_default
                params["detector filepath"] = filepath
            else:
                self.warning_popup("Filepath must be given")
                return

        return params

    def select_detector_efficiency_file_slot(self, slot):
        self.efficiency_file_button.clicked.connect(slot)

    def calculate_efficiency_slot(self, slot):
        self.calculate_efficiency_button.clicked.connect(slot)

    def set_efficiency_data_file_label_text(self, filename):
        self.efficiency_file_label.setText(filename)
