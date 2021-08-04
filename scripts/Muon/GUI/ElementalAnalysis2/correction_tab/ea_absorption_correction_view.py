# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy import QtWidgets, QtCore

from Muon.GUI.Common import message_box

SHAPE_TYPE_COLUMNS = {"None": [], "Disk": ["Radius (cm)", "Thickness (cm)"], "Sphere": ["Radius (cm)"],
                      "Flat Plate": ["Height (cm)", "Width (cm)", "Thickness (cm)"]}
MUON_PROFILE_SPECIFIERS = ["Muon depth", "Muon implantation workspace"]
DEFAULT_MUON_DEPTH = 0
DEFAULT_MUON_RANGE = 0
DEFAULT_DISTANCE = 10
DEFAULT_ANGLE = 45


class EAAbsorptionCorrectionTabView(QtWidgets.QWidget):

    def __init__(self, parent=None):
        super(EAAbsorptionCorrectionTabView, self).__init__(parent=parent)
        self.shape_table = QtWidgets.QTableWidget(self)
        self.shape_table.setMaximumHeight(50)
        self.holder_widget = QtWidgets.QWidget(self)
        self.holder_widget_layout = QtWidgets.QVBoxLayout()
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

        self.group = QtWidgets.QGroupBox("Absorption corrections")
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
        self.add_absorption_checkbox = QtWidgets.QCheckBox(self)
        self.add_absorption_label = QtWidgets.QLabel(" Apply absorption to corrections ", self)
        self.horizontal_layout1.addWidget(self.add_absorption_checkbox, alignment=QtCore.Qt.AlignLeft)
        self.horizontal_layout1.addWidget(self.add_absorption_label, alignment=QtCore.Qt.AlignLeft)
        self.horizontal_layout1.insertStretch(-1, 1)

        self.shapes_label = QtWidgets.QLabel(" Geometry type: ", self)
        self.shapes_combobox = QtWidgets.QComboBox(self)
        self.shapes_label.setSizePolicy(size_policy)
        self.horizontal_layout2.addWidget(self.shapes_label)
        self.horizontal_layout2.addWidget(self.shapes_combobox)

        self.absorption_coefficient_file_button = QtWidgets.QPushButton(" Select absorption coefficient data file ",
                                                                        self)
        self.absorption_coefficient_file_label = QtWidgets.QLabel("", self)
        self.absorption_coefficient_file_button.setSizePolicy(size_policy)
        self.horizontal_layout3.addWidget(self.absorption_coefficient_file_button)
        self.horizontal_layout3.addWidget(self.absorption_coefficient_file_label)

        self.use_default_checkbox = QtWidgets.QCheckBox(self)
        self.use_default_label = QtWidgets.QLabel(" Use default detector setting? ", self)
        self.horizontal_layout4.addWidget(self.use_default_checkbox, alignment=QtCore.Qt.AlignLeft)
        self.horizontal_layout4.addWidget(self.use_default_label, alignment=QtCore.Qt.AlignLeft)
        self.horizontal_layout4.insertStretch(-1, 1)

        self.detector_distance_label = QtWidgets.QLabel(" Distance (cm)", self)
        self.detector_distance_lineedit = QtWidgets.QLineEdit(self)
        self.detector_angle_label = QtWidgets.QLabel(" Angle in degrees ", self)
        self.detector_angle_lineedit = QtWidgets.QLineEdit(self)
        self.horizontal_layout5.addWidget(self.detector_distance_label)
        self.horizontal_layout5.addWidget(self.detector_distance_lineedit)
        self.horizontal_layout5.addWidget(self.detector_angle_label)
        self.horizontal_layout5.addWidget(self.detector_angle_lineedit)

        self.muon_profile_specifier_label = QtWidgets.QLabel(" Specify muon profle by:", self)
        self.muon_profile_specifier_label.setSizePolicy(size_policy)
        self.muon_profile_specifier_combobox = QtWidgets.QComboBox(self)
        self.horizontal_layout6.addWidget(self.muon_profile_specifier_label)
        self.horizontal_layout6.addWidget(self.muon_profile_specifier_combobox)

        self.muon_depth_label = QtWidgets.QLabel(" Average muon depth (mm)", self)
        self.muon_depth_lineedit = QtWidgets.QLineEdit(self)
        self.muon_range_label = QtWidgets.QLabel(" Range of muons (mm)", self)
        self.muon_range_lineedit = QtWidgets.QLineEdit(self)
        self.horizontal_layout7.addWidget(self.muon_depth_label)
        self.horizontal_layout7.addWidget(self.muon_depth_lineedit)
        self.horizontal_layout7.addWidget(self.muon_range_label)
        self.horizontal_layout7.addWidget(self.muon_range_lineedit)

        self.muon_workspace_label = QtWidgets.QLabel(" Muon profile workspace ", self)
        self.muon_workspace_lineedit = QtWidgets.QLineEdit(self)
        self.horizontal_layout8.addWidget(self.muon_workspace_label)
        self.horizontal_layout8.addWidget(self.muon_workspace_lineedit)

        self.holder_widget_layout.addLayout(self.horizontal_layout2)
        self.holder_widget_layout.addWidget(self.shape_table)
        self.holder_widget_layout.addLayout(self.horizontal_layout3)
        self.holder_widget_layout.addLayout(self.horizontal_layout4)
        self.holder_widget_layout.addLayout(self.horizontal_layout5)
        self.holder_widget_layout.addLayout(self.horizontal_layout6)
        self.holder_widget_layout.addLayout(self.horizontal_layout7)
        self.holder_widget_layout.addLayout(self.horizontal_layout8)

        self.holder_widget.setLayout(self.holder_widget_layout)

        self.vertical_layout.addLayout(self.horizontal_layout1)
        self.vertical_layout.addWidget(self.holder_widget)
        self.group.setLayout(self.vertical_layout)
        self.widget_layout = QtWidgets.QVBoxLayout(self)
        self.widget_layout.addWidget(self.group)
        self.setLayout(self.widget_layout)

    def setup_active_elements(self):
        self.use_default_checkbox.clicked.connect(self.on_default_detector_checbox_changed)
        self.muon_profile_specifier_combobox.currentIndexChanged.connect(self.on_muon_profile_specifier_changed)
        self.shapes_combobox.currentIndexChanged.connect(self.on_shape_type_changed)
        self.add_absorption_checkbox.clicked.connect(self.on_absorption_checkbox_changed)

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

        self.on_absorption_checkbox_changed()

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

    def on_absorption_checkbox_changed(self):
        state = self.add_absorption_checkbox.checkState()
        self.holder_widget.setVisible(state)

    def warning_popup(self, message):
        message_box.warning(str(message), parent=self)

    def get_absorption_parameters(self):
        params = {}
        params.update(self.get_shape_parameters())
        params.update(self.get_detector_settings_parameters())
        params.update(self.get_muon_profile_parameters())

        params["Absorption_coefficient_filepath"] = self.absorption_coefficient_file_label.text()
        return params

    def get_detector_settings_parameters(self):
        params = {}
        use_detector_setting = self.use_default_checkbox.checkState()
        params["use_default_detector_settings"] = use_detector_setting
        if not use_detector_setting:
            params["detector_distance"] = self.detector_distance_lineedit.text()
            params["detector_angle"] = self.detector_angle_lineedit.text()
        return params

    def get_muon_profile_parameters(self):
        params = {}
        specifier = self.muon_profile_specifier_combobox.currentText()
        params["muon_profile_specifier"] = specifier
        if specifier == "Muon depth":
            params["muon_depth"] = self.muon_depth_lineedit.text()
            params["muon_range"] = self.muon_range_lineedit.text()
        elif specifier == "Muon implantation workspace":
            params["muon_implantation_workspace"] = self.muon_workspace_lineedit.text()
        return params

    def get_shape_parameters(self):
        params = {}
        shape_type = self.shapes_combobox.currentText()
        params["Geometry"] = shape_type
        shape_parameters = {}
        if shape_type == "Disk":
            shape_parameters["Shape"] = "Cylinder"
            shape_parameters["Radius"] = self.shape_table.cellWidget(0, 0).text()
            shape_parameters["Height"] = self.shape_table.cellWidget(0, 1).text()
        elif shape_type == "Flat Plate":
            shape_parameters["Shape"] = "FlatPlate"
            shape_parameters["Height"] = self.shape_table.cellWidget(0, 0).text()
            shape_parameters["Width"] = self.shape_table.cellWidget(0, 1).text()
            shape_parameters["Thick"] = self.shape_table.cellWidget(0, 2).text()
        elif shape_type == "Sphere":
            shape_parameters["Shape"] = "Sphere"
            shape_parameters["Radius"] = self.shape_table.cellWidget(0, 0).text()
        params["shape_parameters"] = shape_parameters
        return params

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
        for i in range(len(columns)):
            line_edit = QtWidgets.QLineEdit()
            self.shape_table.setCellWidget(0, i, line_edit)

    def apply_absorption(self):
        return self.add_absorption_checkbox.checkState()
