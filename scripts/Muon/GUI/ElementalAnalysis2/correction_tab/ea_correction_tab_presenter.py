# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserver
from qtpy.QtWidgets import QFileDialog
from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist

MINIMUM_DETECTOR_DEFINED_ENERGY_FOR_EFFICIENCY = {"Detector 1": 0, "Detector 2": 60, "Detector 3": 25, "Detector 4": 20}


class EACorrectionTabPresenter:

    def __init__(self, view, model, context):
        self.view = view
        self.model = model
        self.context = context
        self.update_view_observer = GenericObserver(self.update_view)
        self.setup_buttons()

    def setup_buttons(self):
        self.view.select_effieciency_file_slot(self.handle_select_efficiency_data_file_button_clicked)
        self.view.select_absorption_coefficient_file_slot(self.handle_select_absorption_data_file_button_clicked)
        self.view.calculate_corrections_slot(self.handle_apply_correction_button_clicked)

    def update_view(self):
        group_names = self.context.group_context.group_names
        workspaces_to_add = {}
        for group in group_names:
            run, detector = [x.strip() for x in group.split(";")]
            if run not in workspaces_to_add:
                workspaces_to_add[run] = []

            workspaces_to_add[run].append(detector)
        self.view.add_workspace_to_view(workspaces_to_add)

    def handle_select_efficiency_data_file_button_clicked(self):
        filename = QFileDialog.getOpenFileName()
        if isinstance(filename, tuple):
            filename = filename[0]
        filename = str(filename)
        if filename:
            self.view.set_efficiency_data_file_label_text(filename)

    def handle_select_absorption_data_file_button_clicked(self):
        filename = QFileDialog.getOpenFileName()
        if isinstance(filename, tuple):
            filename = filename[0]
        filename = str(filename)
        if filename:
            self.view.set_absorption_coefficient_data_file_label_text(filename)

    def get_calibration_parameters(self):
        params = self.view.calibration_view.get_calibration_parameters()
        try:
            params["gradient"] = float(params["gradient"])
            params["shift"] = float(params["shift"])
        except ValueError:
            self.view.warning_popup("Gradient and energy shift must be a number")
            return
        return params

    def get_efficiency_parameters(self):
        params = self.view.efficiency_view.get_efficiency_parameters()
        if not params["use default efficiencies"]:
            if not params["detector filepath"]:
                self.view.warning_popup("Filepath for detector_efficiency data file must be given")
                return
        return params

    def get_absorption_parameters(self):
        params = self.view.absorption_view.get_absorption_parameters()
        if params["Geometry"] == "None":
            self.view.warning_popup("Geometry type not selected")
            return
        try:
            shape_parameters = params["shape_parameters"]
            for key in shape_parameters:
                if key == "Shape":
                    continue
                shape_parameters[key] = float(shape_parameters[key])
        except ValueError:
            self.view.warning_popup("Shape parameters must be a number")
            return
        if not params["Absorption_coefficient_filepath"]:
            self.view.warning_popup("Filepath for absorption coefficient data file must be given")
            return
        if not params["use_default_detector_settings"]:
            try:
                params["detector_distance"] = float(params["detector_distance"])
                params["detector_angle"] = float(params["detector_angle"])
            except ValueError:
                self.view.warning_popup("Detector settings must be a number")
                return

        if params["muon_profile_specifier"] == "Muon depth":
            try:
                params["muon_depth"] = float(params["muon_depth"])
                params["muon_range"] = float(params["muon_range"])
            except ValueError:
                self.view.warning_popup("Muon depth and range must be a number")
                return
        elif params["muon_profile_specifier"] == "Muon implantation workspace":
            if not check_if_workspace_exist(params["muon_implantation_workspace"]):
                self.view.warning_popup("Muon implantation workspace does not exist")
                return
        return params

    def get_initial_parameters(self):
        params = self.view.get_initial_parameters()
        if params["group_name"] == "; ":
            self.view.warning_popup("No workspace selected")
            return None

        try:
            params["energy_start"] = float(params["energy_start"])
            params["energy_end"] = float(params["energy_end"])
        except ValueError:
            self.view.warning_popup("Maximum and Minimum energy must be numbers")
            return None

        if params["energy_end"] < params["energy_start"]:
            self.view.warning_popup("Maximum energy must be less than Minimum energy")
            return None
        return params

    def handle_apply_correction_button_clicked(self):
        all_parameters = {}
        initial_params = self.get_initial_parameters()
        if initial_params is None:
            return
        all_parameters["initial"] = initial_params

        if self.view.calibration_view.apply_calibration():
            calibration_params = self.get_calibration_parameters()
            if calibration_params is None:
                return
            all_parameters["calibration"] = calibration_params

        if self.view.efficiency_view.apply_efficiency():
            efficiency_params = self.get_efficiency_parameters()
            if efficiency_params is None:
                return
            """
                Efficiency is not well defined at lower energies and users are warned if selected minimum energy is
                less than threshold energy but corrections are still applied
            """
            detector = self.context.group_context[initial_params["group_name"]].detector
            energy_start = initial_params["energy_start"]
            min_energy = MINIMUM_DETECTOR_DEFINED_ENERGY_FOR_EFFICIENCY[detector]
            if energy_start < min_energy:
                self.view.warning_popup(f"Efficiencies for {detector} below {min_energy} KeV is not defined well so "
                                        f"corrected data may be incorrect")

            all_parameters["efficiency"] = efficiency_params

        if self.view.absorption_view.apply_absorption():
            absorption_params = self.get_absorption_parameters()
            if absorption_params is None:
                return
            all_parameters["absorption"] = absorption_params

        # if no corrections are selected function returns without calling model
        if len(all_parameters.keys()) == 1:
            self.view.warning_popup("No corrections selected")
            return
        self.model.handle_calculate_corrections(all_parameters)
