# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from Muon.GUI.Common import message_box


class EAShiftCorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EAShiftCorrectionTabView, self).__init__(parent=parent)
        self.shift_label = QtWidgets.QLabel(" Shift (KeV) : ", parent=self)
        self.shift_lineedit = QtWidgets.QLineEdit(parent=self)
        self.apply_shift_button = QtWidgets.QPushButton(" Apply shift ", parent=self)
        self.horizontal_layout = QtWidgets.QHBoxLayout()
        self.horizontal_layout.addWidget(self.shift_label)
        self.horizontal_layout.addWidget(self.shift_lineedit)
        self.horizontal_layout.addWidget(self.apply_shift_button)
        self.setLayout(self.horizontal_layout)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def get_shift_parameters(self):
        try:
            shift = float(self.shift_lineedit.text)
        except ValueError:
            self.warning_popup("Shift should be a float")
            return
        return {"Shift": shift}

    def slot_for_apply_shift_button(self, slot):
        self.apply_shift_button.clicked.connect(slot)
