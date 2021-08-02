# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common import thread_model, message_box
from Muon.GUI.Common.ADSHandler.ADS_calls import retrieve_ws, remove_ws_if_present
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.simpleapi import ExtractSingleSpectrum, ConjoinWorkspaces, CropWorkspace, RenameWorkspace, ScaleX, \
    EvaluateFunction, Divide, CreateWorkspace, SetSample, SetSampleMaterial, XrayAbsorptionCorrection

EXTRACTED_SPECTRUM_SUFFIX = "EA_spectrum_"
CORRECTIONS_SUFFIX = "_EA_CORRECTED_DATA"
EFFICIENCY_WORKSPACE_NAME = "EA_Efficiency_workspace"
MUON_IMPLANTATION_WORKSPACE_NAME = "EA_Implantation_workspace"
XRAY_ABSORPTION_WORKSPACE = "EA_Absorption_coefficients"
DEFAULT_DETECTOR_EFFICIENCY_FUNCTION = {"Detector 1": "(93.2 + (59.2*ln(x))  + (-13.6*ln(x)*ln(x) ) + "
                                                      "(1.35*ln(x)*ln(x)*ln(x)) + (-0.049*ln(x)*ln(x)*ln(x)*ln(x)) )/x",
                                        "Detector 2": "(-5.01 + ( 1.80*ln(x))  + (-0.134*ln(x)*ln(x) )  )/x",
                                        "Detector 3": "(-1.79 + ( 0.805*ln(x))  + ( -0.0711*ln(x)*ln(x) )  )/x",
                                        "Detector 4": "(-1.67 + ( 0.801*ln(x))  + ( -0.0698*ln(x)*ln(x) )  )/x"}
DEFAULT_DETECTOR_ANGLE_SETTINGS = {"Detector 1": 135, "Detector 2": 315, "Detector 3": 45, "Detector 4": 225}
DEFAULT_DETECTOR_DISTANCE = 10
mm_to_cm = 0.1
cm_to_m = 0.01
SPHERE_XML_TEMPLATE = " \
                        <sphere id='sphere'> \
                        <centre x='0.0'  y='0.0' z='0.0' /> \
                        <radius val='{}' /> \
                        </sphere> \
                      "


class EACorrectionTabModel:

    def __init__(self, context):
        self.context = context
        self.group_context = context.group_context
        self.calculation_started_notifier = GenericObservable()
        self.calculation_finished_notifier = GenericObservable()

    @staticmethod
    def gaussian(xvals, centre, sigma, height=1):
        exp_val = (xvals - centre) / (np.sqrt(2) * sigma)
        return height * np.exp(-exp_val * exp_val)

    def calculate_corrections(self, parameters):
        group_name = parameters["initial"]["group_name"]
        workspace_names = self.split_and_crop_workspace(parameters["initial"])
        for name in workspace_names:
            if "calibration" in parameters:
                self.apply_calibration_corrections(name, parameters["calibration"])
            if "efficiency" in parameters:
                self.apply_efficiency_corrections(name, parameters["efficiency"], group_name)
            if "absorption" in parameters:
                self.apply_absorption_corrections(name, parameters["absorption"], group_name)
        output_name = group_name + CORRECTIONS_SUFFIX
        self.join_workspaces(workspace_names, output_name)
        self.handle_correction_successful(group_name, output_name)

    def handle_calculate_corrections(self, parameters):
        self.corrections_model = ThreadModelWrapper(lambda: self.calculate_corrections(parameters))
        self.corrections_thread = thread_model.ThreadModel(self.corrections_model)
        self.corrections_thread.threadWrapperSetUp(self.handle_calculation_started,
                                                   self.calculation_success,
                                                   self.handle_calculation_error)
        self.corrections_thread.start()

    def handle_calculation_started(self):
        self.calculation_started_notifier.notify_subscribers()

    def calculation_success(self):
        self.calculation_finished_notifier.notify_subscribers()

    def handle_calculation_error(self, error):
        message_box.warning("ERROR: " + str(error), None)
        self.calculation_finished_notifier.notify_subscribers()
        for i in range(3):
            remove_ws_if_present(EXTRACTED_SPECTRUM_SUFFIX + str(i+1))
        remove_ws_if_present(MUON_IMPLANTATION_WORKSPACE_NAME)
        remove_ws_if_present(XRAY_ABSORPTION_WORKSPACE)
        remove_ws_if_present(EFFICIENCY_WORKSPACE_NAME)

    def split_and_crop_workspace(self, params):
        """ switch to getting from EAGroup
        group_name = params["group_name"]
        workspace_name = self.group_context[group_name].get_counts_workspace()
        """
        workspace_name = params["group_name"]
        xmin = params["energy_start"]
        xmax = params["energy_end"]
        workspace_names = []
        for i in range(3):
            output_name = EXTRACTED_SPECTRUM_SUFFIX + str(i + 1)
            ExtractSingleSpectrum(InputWorkspace=workspace_name, OutputWorkspace=output_name, WorkspaceIndex=i)
            CropWorkspace(InputWorkspace=output_name, OutputWorkspace=output_name, XMin=xmin, XMax=xmax)
            workspace_names.append(output_name)
        return workspace_names

    def join_workspaces(self, workspace_names, output_name):
        spectrum_1 = workspace_names[0]
        spectrum_2 = workspace_names[1]
        spectrum_3 = workspace_names[2]
        ConjoinWorkspaces(InputWorkspace1=spectrum_1, InputWorkspace2=spectrum_2)
        ConjoinWorkspaces(InputWorkspace1=spectrum_1, InputWorkspace2=spectrum_3)
        RenameWorkspace(InputWorkspace=spectrum_1, OutputWorkspace=output_name)

    def apply_calibration_corrections(self, workspace_name, params):
        gradient = params["gradient"]
        shift = params["shift"]
        ScaleX(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Factor=gradient, Operation="Multiply")
        ScaleX(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Factor=shift, Operation="Add")

    def apply_efficiency_corrections(self, workspace_name, params, group_name):

        if params["use default efficiencies"]:
            group = self.group_context[group_name]
            function = DEFAULT_DETECTOR_EFFICIENCY_FUNCTION[group.detector]
            EvaluateFunction(Function="name=UserFunction,Formula=" + function, InputWorkspace=workspace_name,
                             OutputWorkspace=EFFICIENCY_WORKSPACE_NAME)
            ExtractSingleSpectrum(InputWorkspace=EFFICIENCY_WORKSPACE_NAME, OutputWorkspace=EFFICIENCY_WORKSPACE_NAME,
                                  WorkspaceIndex=1)
        else:
            filepath = params["detector filepath"]
            self.load_efficency_datafile(workspace_name, filepath)

        Divide(LHSWorkspace=workspace_name, RHSWorkspace=EFFICIENCY_WORKSPACE_NAME, OutputWorkspace=workspace_name)
        remove_ws_if_present(EFFICIENCY_WORKSPACE_NAME)

    def apply_absorption_corrections(self, workspace_name, params, group_name):
        self.set_sample(workspace_name, params)
        implantation_workspace = ""
        if params["muon_profile_specifier"] == "Muon depth":
            implantation_workspace = MUON_IMPLANTATION_WORKSPACE_NAME
            depth_in_cm = params["muon_depth"] * mm_to_cm
            range_in_cm = params["muon_range"] * mm_to_cm
            self.create_muon_implantation_workspace(depth_in_cm, range_in_cm)
        elif params["muon_profile_specifier"] == "Muon implantation workspace":
            implantation_workspace = params["muon_implantation_workspace"]

        if params["use_default_detector_settings"]:
            group = self.group_context[group_name]
            detector_distance = DEFAULT_DETECTOR_DISTANCE
            detector_angle = DEFAULT_DETECTOR_ANGLE_SETTINGS[group.detector]
        else:
            detector_distance = params["detector_distance"]
            detector_angle = params["detector_angle"]
        XrayAbsorptionCorrection(InputWorkspace=workspace_name, MuonImplantationProfile=implantation_workspace,
                                 OutputWorkspace=XRAY_ABSORPTION_WORKSPACE, DetectorAngle=detector_angle,
                                 DetectorDistance=detector_distance)

        Divide(LHSWorkspace=workspace_name, RHSWorkspace=XRAY_ABSORPTION_WORKSPACE, OutputWorkspace=workspace_name)
        remove_ws_if_present(MUON_IMPLANTATION_WORKSPACE_NAME)
        remove_ws_if_present(XRAY_ABSORPTION_WORKSPACE)

    def load_efficency_datafile(self, workspace_name, filepath):
        # Efficiency data has to be interpolated to get desired y_values
        workspace = retrieve_ws(workspace_name)
        x_data = workspace.readX(0)
        data = np.loadtxt(filepath)

        y_data = np.interp(x_data, data[:, 0], data[:, 1])

        CreateWorkspace(OutputWorkspace=EFFICIENCY_WORKSPACE_NAME, DataX=x_data, DataY=y_data)

    def set_sample(self, workspace_name, params):
        """
            method sets sample shape and material properties.
            see: https://docs.mantidproject.org/nightly/algorithms/SetSample-v1.html#algm-setsample
        """
        shape_parameters = params["shape_parameters"]
        if shape_parameters["Shape"] == "Sphere":
            # Sphere can only be defined by using the CSG shape type and shape_xml, shape_xml takes values in metres
            shape_xml = SPHERE_XML_TEMPLATE.format(shape_parameters["Radius"] * cm_to_m)
            SetSample(InputWorkspace=workspace_name, Geometry={'Shape': 'CSG', 'Value': shape_xml})
        else:
            # shape parameters in dict form are given in cm
            SetSample(InputWorkspace=workspace_name, Geometry=shape_parameters)
        # A chemical formula has to be given or SetSampleMaterial algorithm  will fail and has no bearing on calculation
        SetSampleMaterial(InputWorkspace=workspace_name, ChemicalFormula="Au",
                          XRayAttenuationProfile=params["Absorption_coefficient_filepath"])

    def create_muon_implantation_workspace(self, depth, muon_range):
        """
            muon implantation wokspace is assumed to be a guassian with an mean that is the depth and sigma
             that is the range. gaussian is evaluated from mean - 3*sigma to mean + 3*sigma
        """
        start = depth - (3 * muon_range)
        stop = depth + (3 * muon_range)
        if start < 0:
            start = 0
        x_data = np.linspace(start, stop, 100)
        y_data = self.gaussian(x_data, depth, muon_range)
        # XrayAbsorptionCorrection algorithm will be unable to find solution if first x_data is zero.
        if start == 0:
            x_data = x_data[1:]
            y_data = y_data[1:]
        CreateWorkspace(OutputWorkspace=MUON_IMPLANTATION_WORKSPACE_NAME, DataX=x_data, DataY=y_data)

    def handle_correction_successful(self, group_name, output_name):
        group = self.group_context[group_name]
        group_workspace = retrieve_ws(group.run_number)
        group_workspace.addWorkspace(retrieve_ws(output_name))
        # group.update_corrected_workspace(output_name)
