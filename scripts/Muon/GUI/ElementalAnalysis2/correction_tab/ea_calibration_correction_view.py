# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore
from Muon.GUI.Common import message_box

DEFAULT_SHIFT = 0
DEFAULT_GRADIENT = 1


class EACalibrationCorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EACalibrationCorrectionTabView, self).__init__(parent=parent)
        self.holder_widget = QtWidgets.QWidget(self)

        self.setup_widget_layout()
        self.add_calibration_checkbox.clicked.connect(self.on_calibration_checkbox_changed)
        self.on_calibration_checkbox_changed()

    def setup_widget_layout(self):
        self.horizontal_layout1 = QtWidgets.QHBoxLayout()
        self.horizontal_layout2 = QtWidgets.QHBoxLayout()
        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.group = QtWidgets.QGroupBox("Calibration corrections")
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
        self.add_calibration_checkbox = QtWidgets.QCheckBox(self)
        self.add_calibration_label = QtWidgets.QLabel(" Apply calibration change to corrections ", self)
        self.horizontal_layout1.addWidget(self.add_calibration_checkbox, alignment=QtCore.Qt.AlignLeft)
        self.horizontal_layout1.addWidget(self.add_calibration_label, alignment=QtCore.Qt.AlignLeft)
        self.horizontal_layout1.insertStretch(-1, 1)

        self.gradient_label = QtWidgets.QLabel(" Gradient:", self)
        self.gradient_lineedit = QtWidgets.QLineEdit(self)
        self.gradient_lineedit.setText(str(DEFAULT_GRADIENT))
        self.shift_label = QtWidgets.QLabel(" Energy shift (KeV) :", self)
        self.shift_lineedit = QtWidgets.QLineEdit(self)
        self.shift_lineedit.setText(str(DEFAULT_SHIFT))

        self.horizontal_layout2.addWidget(self.gradient_label)
        self.horizontal_layout2.addWidget(self.gradient_lineedit)
        self.horizontal_layout2.addWidget(self.shift_label)
        self.horizontal_layout2.addWidget(self.shift_lineedit)
        self.holder_widget.setLayout(self.horizontal_layout2)

        self.vertical_layout.addLayout(self.horizontal_layout1)
        self.vertical_layout.addWidget(self.holder_widget)
        self.group.setLayout(self.vertical_layout)
        self.widget_layout = QtWidgets.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)
        self.setLayout(self.widget_layout)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def on_calibration_checkbox_changed(self):
        state = self.add_calibration_checkbox.checkState()
        self.holder_widget.setVisible(state)

    def get_calibration_parameters(self):
        params = {}
        shift = self.shift_lineedit.text()
        gradient = self.gradient_lineedit.text()
        params["shift"] = shift
        params["gradient"] = gradient
        return params

    def apply_calibration(self):
        return self.add_calibration_checkbox.checkState()
