# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.thread_model_wrapper import ThreadModelWrapper
from Muon.GUI.Common import thread_model, message_box
from mantidqt.utils.observer_pattern import GenericObservable
from mantid.simpleapi import ExtractSingleSpectrum, ConjoinWorkspaces, CropWorkspace, RenameWorkspace, ScaleX, \
    EvaluateFunction, LoadAscii, Divide

EXTRACTED_SPECTRUM_SUFFIX = "_EA_spectrum_"
CORRECTIONS_SUFFIX = "_EA_CORRECTED_DATA"
DEFAULT_DETECTOR_EFFICIENCY_FUNCTION = {"Detector 1": "(-49.5 + ( 26.5*ln(x))  + (-4.55*ln(x)*ln(x) ) +"
                                                      " (0.259*ln(x)*ln(x)*ln(x))  )/x",
                                        "Detector 2": "(-5.01 + ( 1.80*ln(x))  + (-0.134*ln(x)*ln(x) )  )/x",
                                        "Detector 3": "(-1.79 + ( 0.805*ln(x))  + ( -0.0711*ln(x)*ln(x) )  )/x",
                                        "Detector 4": "(-1.67 + ( 0.801*ln(x))  + ( -0.0698*ln(x)*ln(x) )  )/x"}


class EACorrectionTabModel:

    def __init__(self, context):
        self.context = context
        self.group_context = context.group_context
        self.calculation_started_notifier = GenericObservable()
        self.calculation_finished_notifier = GenericObservable()

    def calculate_corrections(self, parameters):
        print(parameters)
        group_name = parameters["initial"]["group_name"]
        workspace_names = self.split_and_crop_workspace(parameters["initial"])
        for name in workspace_names:
            if "calibration" in parameters:
                self.apply_calibration_corrections(name, parameters["calibration"])
            if "efficiency" in parameters:
                self.apply_efficiency_corrections(name, parameters["efficiency"], group_name)
            if "absorption" in parameters:
                self.apply_absorption_corrections(name, parameters["absorption"])
        self.join_workspaces(workspace_names, group_name + CORRECTIONS_SUFFIX)

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
            output_name = workspace_name + EXTRACTED_SPECTRUM_SUFFIX + str(i + 1)
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
                             OutputWorkspace="Efficiency")
            ExtractSingleSpectrum(InputWorkspace="Efficiency", OutputWorkspace="Efficiency", WorkspaceIndex=1)
        else:
            # this will not work for now as loaded workspace and original workspaces will have different bins
            filepath = params["detector filepath"]
            LoadAscii(Filename=filepath, OutputWorkspace="Efficiency")

        Divide(LHSWorkspace=workspace_name, RHSWorkspace="Efficiency", OutputWorkspace=workspace_name)

    def apply_absorption_corrections(self, workspace_name, params):
        pass
