# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore

from Muon.GUI.Common import message_box

EFFICIENCY_SPECIFIERS = ["Use default", "From data file"]


class EAEfficiencyCorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EAEfficiencyCorrectionTabView, self).__init__(parent=parent)
        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.horizontal_layout1 = QtWidgets.QHBoxLayout()
        self.horizontal_layout2 = QtWidgets.QHBoxLayout()
        self.horizontal_layout3 = QtWidgets.QHBoxLayout()
        self.holder_widget = QtWidgets.QWidget(self)
        self.holder_widget_layout = QtWidgets.QVBoxLayout()
        self.setup_widget_layout()
        self.setup_initial_state()
        self.setup_active_elements()

    def setup_widget_layout(self):
        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)

        self.group = QtWidgets.QGroupBox("Efficiency corrections")
        self.group.setFlat(False)
        self.setStyleSheet("QGroupBox {border: 1px solid grey;border-radius: 10px;margin-top: 1ex; margin-right: 0ex}"
                           "QGroupBox:title {"
                           'subcontrol-origin: margin;'
                           "padding: 0 3px;"
                           'subcontrol-position: top center;'
                           'padding-top: 0px;'
                           'padding-bottom: 0px;'
                           "padding-right: 10px;"
                           ' color: grey; }')

        self.add_efficiency_checkbox = QtWidgets.QCheckBox(self)
        self.add_efficiency_label = QtWidgets.QLabel(" Apply efficiencies to corrections ", self)

        self.horizontal_layout1.addWidget(self.add_efficiency_checkbox, alignment=QtCore.Qt.AlignLeft)
        self.horizontal_layout1.addWidget(self.add_efficiency_label, alignment=QtCore.Qt.AlignLeft)
        self.horizontal_layout1.insertStretch(-1, 1)

        self.specify_efficiency_label = QtWidgets.QLabel(" Specify efficiency: ")
        self.specify_efficiency_label.setSizePolicy(size_policy)
        self.specify_efficiency_combobox = QtWidgets.QComboBox(self)

        self.horizontal_layout2.addWidget(self.specify_efficiency_label)
        self.horizontal_layout2.addWidget(self.specify_efficiency_combobox)

        self.efficiency_file_button = QtWidgets.QPushButton(" Select efficiency data file ", self)
        self.efficiency_file_label = QtWidgets.QLabel("", self)

        self.horizontal_layout3.addWidget(self.efficiency_file_button)
        self.horizontal_layout3.addWidget(self.efficiency_file_label)

        self.holder_widget_layout.addLayout(self.horizontal_layout2)
        self.holder_widget_layout.addLayout(self.horizontal_layout3)

        self.holder_widget.setLayout(self.holder_widget_layout)

        self.vertical_layout.addLayout(self.horizontal_layout1)
        self.vertical_layout.addWidget(self.holder_widget)
        self.group.setLayout(self.vertical_layout)
        self.widget_layout = QtWidgets.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)
        self.setLayout(self.widget_layout)

    def setup_active_elements(self):
        self.specify_efficiency_combobox.currentIndexChanged.connect(self.on_efficiency_specifier_changed)
        self.add_efficiency_checkbox.clicked.connect(self.on_efficiency_checkbox_changed)

    def on_efficiency_checkbox_changed(self):
        state = self.add_efficiency_checkbox.checkState()
        self.holder_widget.setVisible(state)
        if state:
            self.on_efficiency_specifier_changed()

    def on_efficiency_specifier_changed(self):
        specifier = self.specify_efficiency_combobox.currentText()
        if specifier == "Use default":
            self.efficiency_file_label.setVisible(False)
            self.efficiency_file_button.setVisible(False)
        else:
            self.efficiency_file_label.setVisible(True)
            self.efficiency_file_button.setVisible(True)

    def setup_initial_state(self):
        self.show_efficiency_file_widgets(False)
        self.add_efficiency_checkbox.setChecked(False)
        self.specify_efficiency_combobox.addItems(EFFICIENCY_SPECIFIERS)
        self.on_efficiency_checkbox_changed()
        self.on_efficiency_checkbox_changed()

    def show_efficiency_file_widgets(self, state):
        self.efficiency_file_label.setVisible(state)
        self.efficiency_file_button.setVisible(state)

    def on_default_efficiency_checkbox_changed(self):
        state = self.default_efficiency_checkbox.isChecked()
        self.show_efficiency_file_widgets(not state)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def get_efficiency_parameters(self):
        params = {}
        if self.specify_efficiency_combobox.currentText() == "Use default":
            use_default = True
        else:
            use_default = False
        params["use default efficiencies"] = use_default
        if not use_default:
            filepath = self.efficiency_file_label.text()
            params["detector filepath"] = filepath
        return params

    def select_detector_efficiency_file_slot(self, slot):
        self.efficiency_file_button.clicked.connect(slot)

    def set_efficiency_data_file_label_text(self, filename):
        self.efficiency_file_label.setText(filename)

    def apply_efficiency(self):
        return self.add_efficiency_checkbox.checkState()
