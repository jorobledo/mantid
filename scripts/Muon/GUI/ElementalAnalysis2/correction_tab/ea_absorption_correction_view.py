# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets

from Muon.GUI.Common import message_box

SHAPE_TYPE_COLUMNS = {"None": [], "Disk": ["Radius (cm)", "Thickness (cm)"], "Sphere": ["Radius (cm)"],
                      "Cuboid": ["Length (cm)", "Width (cm)", "Thickness (cm)"]}
MUON_PROFILE_SPECIFIERS = ["Muon depth", "Muon implantation workspace"]
DEFAULT_MUON_DEPTH = 0
DEFAULT_MUON_RANGE = 0
DEFAULT_DISTANCE = 10
DEFAULT_ANGLE = 45


class EAAbsorptionCorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EAAbsorptionCorrectionTabView, self).__init__(parent=parent)
        self.shape_table = QtWidgets.QTableWidget(parent=self)
        self.shape_table.setMaximumHeight(50)
        self.vertical_layout = QtWidgets.QVBoxLayout()
        self.horizontal_layout1 = QtWidgets.QHBoxLayout()
        self.horizontal_layout2 = QtWidgets.QHBoxLayout()
        self.horizontal_layout3 = QtWidgets.QHBoxLayout()
        self.horizontal_layout4 = QtWidgets.QHBoxLayout()
        self.horizontal_layout5 = QtWidgets.QHBoxLayout()
        self.horizontal_layout6 = QtWidgets.QHBoxLayout()
        self.horizontal_layout7 = QtWidgets.QHBoxLayout()
        self.horizontal_layout8 = QtWidgets.QHBoxLayout()
        self.setup_widget_view()
        self.setup_initial_state()
        self.setup_active_elements()

    def setup_widget_view(self):
        size_policy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Fixed, QtWidgets.QSizePolicy.Fixed)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)

        self.shapes_label = QtWidgets.QLabel(" Geometry type: ", self)
        self.shapes_combobox = QtWidgets.QComboBox(parent=self)
        self.shapes_label.setSizePolicy(size_policy)
        self.horizontal_layout1.addWidget(self.shapes_label)
        self.horizontal_layout1.addWidget(self.shapes_combobox)

        self.absorption_coefficient_file_button = QtWidgets.QPushButton(" Select absorption coefficient data file ",
                                                                        self)
        self.absorption_coefficient_file_label = QtWidgets.QLabel("", self)
        self.absorption_coefficient_file_button.setSizePolicy(size_policy)
        self.horizontal_layout2.addWidget(self.absorption_coefficient_file_button)
        self.horizontal_layout2.addWidget(self.absorption_coefficient_file_label)

        self.use_default_checkbox = QtWidgets.QCheckBox(self)
        self.use_default_label = QtWidgets.QLabel(" Use default detector setting? ", self)
        self.horizontal_layout3.addWidget(self.use_default_checkbox)
        self.horizontal_layout3.addWidget(self.use_default_label)

        self.detector_distance_label = QtWidgets.QLabel(" Distance (cm)", self)
        self.detector_distance_lineedit = QtWidgets.QLineEdit(self)
        self.detector_angle_label = QtWidgets.QLabel(" Angle in degrees ", self)
        self.detector_angle_lineedit = QtWidgets.QLineEdit(self)
        self.horizontal_layout4.addWidget(self.detector_distance_label)
        self.horizontal_layout4.addWidget(self.detector_distance_lineedit)
        self.horizontal_layout4.addWidget(self.detector_angle_label)
        self.horizontal_layout4.addWidget(self.detector_angle_lineedit)

        self.muon_profile_specifier_label = QtWidgets.QLabel(" Specify muon profle by: ", self)
        self.muon_profile_specifier_label.setSizePolicy(size_policy)
        self.muon_profile_specifier_combobox = QtWidgets.QComboBox(self)
        self.horizontal_layout5.addWidget(self.muon_profile_specifier_label)
        self.horizontal_layout5.addWidget(self.muon_profile_specifier_combobox)

        self.muon_depth_label = QtWidgets.QLabel(" Average muon depth (mm)", self)
        self.muon_depth_lineedit = QtWidgets.QLineEdit(self)
        self.muon_range_label = QtWidgets.QLabel(" Range of muons (mm)", self)
        self.muon_range_lineedit = QtWidgets.QLineEdit(self)
        self.horizontal_layout6.addWidget(self.muon_depth_label)
        self.horizontal_layout6.addWidget(self.muon_depth_lineedit)
        self.horizontal_layout6.addWidget(self.muon_range_label)
        self.horizontal_layout6.addWidget(self.muon_range_lineedit)

        self.muon_workspace_label = QtWidgets.QLabel(" Muon profile workspace ", self)
        self.muon_workspace_lineedit = QtWidgets.QLineEdit(self)
        self.horizontal_layout7.addWidget(self.muon_workspace_label)
        self.horizontal_layout7.addWidget(self.muon_workspace_lineedit)

        self.calculate_absorption_button = QtWidgets.QPushButton("Calculate absorption correction")
        self.horizontal_layout8.addWidget(self.calculate_absorption_button)

        self.vertical_layout.addLayout(self.horizontal_layout1)
        self.vertical_layout.addWidget(self.shape_table)
        self.vertical_layout.addLayout(self.horizontal_layout2)
        self.vertical_layout.addLayout(self.horizontal_layout3)
        self.vertical_layout.addLayout(self.horizontal_layout4)
        self.vertical_layout.addLayout(self.horizontal_layout5)
        self.vertical_layout.addLayout(self.horizontal_layout6)
        self.vertical_layout.addLayout(self.horizontal_layout7)
        self.setLayout(self.vertical_layout)

    def setup_active_elements(self):
        self.use_default_checkbox.clicked.connect(self.on_default_detector_checbox_changed)
        self.muon_profile_specifier_combobox.currentIndexChanged.connect(self.on_muon_profile_specifier_changed)
        self.shapes_combobox.currentIndexChanged.connect(self.on_shape_type_changed)

    def setup_initial_state(self):
        self.shapes_combobox.addItems(list(SHAPE_TYPE_COLUMNS))
        self.muon_profile_specifier_combobox.addItems(list(MUON_PROFILE_SPECIFIERS))
        self.use_default_checkbox.setChecked(True)
        self.show_detector_setting_widgets(False)
        self.on_shape_type_changed()
        self.on_muon_profile_specifier_changed()

        self.detector_distance_lineedit.setText(str(DEFAULT_DISTANCE))
        self.detector_angle_lineedit.setText(str(DEFAULT_ANGLE))
        self.muon_depth_lineedit.setText(str(DEFAULT_MUON_DEPTH))
        self.muon_range_lineedit.setText(str(DEFAULT_MUON_RANGE))

    def show_detector_setting_widgets(self, state):
        self.detector_distance_label.setVisible(state)
        self.detector_distance_lineedit.setVisible(state)
        self.detector_angle_label.setVisible(state)
        self.detector_angle_lineedit.setVisible(state)

    def on_muon_profile_specifier_changed(self):
        specifier = self.muon_profile_specifier_combobox.currentText()
        if specifier == "Muon depth":
            self.muon_depth_label.setVisible(True)
            self.muon_depth_lineedit.setVisible(True)
            self.muon_range_label.setVisible(True)
            self.muon_range_lineedit.setVisible(True)
            self.muon_workspace_label.setVisible(False)
            self.muon_workspace_lineedit.setVisible(False)
        elif specifier == "Muon implantation workspace":
            self.muon_workspace_label.setVisible(True)
            self.muon_workspace_lineedit.setVisible(True)
            self.muon_depth_label.setVisible(False)
            self.muon_depth_lineedit.setVisible(False)
            self.muon_range_label.setVisible(False)
            self.muon_range_lineedit.setVisible(False)

    def on_default_detector_checbox_changed(self):
        state = self.use_default_checkbox.isChecked()
        self.show_detector_setting_widgets(not state)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def create_table(self):
        pass

    def calculate_absorption_slot(self, slot):
        self.calculate_absorption_button.clicked.connect(slot)

    def select_absorption_coefficient_file_slot(self, slot):
        self.absorption_coefficient_file_button.clicked.connect(slot)

    def set_absorption_coefficient_data_file_label_text(self, filename):
        self.absorption_coefficient_file_label.setText(filename)

    def on_shape_type_changed(self):
        shape_type = self.shapes_combobox.currentText()
        self.shape_table.clear()
        self.shape_table.removeRow(0)
        for i in reversed(range(self.shape_table.columnCount())):
            self.shape_table.removeColumn(i)
        if shape_type == "None":
            return
        columns = SHAPE_TYPE_COLUMNS[shape_type]
        self.shape_table.setColumnCount(len(columns))
        self.shape_table.setHorizontalHeaderLabels(columns)
        header = self.shape_table.horizontalHeader()
        header.setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        header = self.shape_table.verticalHeader()
        header.setSectionResizeMode(QtWidgets.QHeaderView.Stretch)
        header.hide()
        self.shape_table.insertRow(0)
