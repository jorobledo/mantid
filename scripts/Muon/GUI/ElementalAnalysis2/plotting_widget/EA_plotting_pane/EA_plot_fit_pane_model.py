# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.ADSHandler.workspace_naming import get_fit_function_name_from_workspace
from Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plotting_pane.EA_plot_data_pane_model import EAPlotDataPaneModel


class EAPlotFitPaneModel(EAPlotDataPaneModel):

    def __init__(self, context, name):
        super().__init__(context, name)

    @staticmethod
    def get_fit_workspace_and_indices(fit, with_diff=True):
        if fit is None:
            return [], []
        workspaces = []
        indices = []
        for workspace_name in fit.output_workspace_names():
            first_fit_index = 1  # calc
            second_fit_index = 2  # Diff
            workspaces.append(workspace_name)
            indices.append(first_fit_index)
            if with_diff:
                workspaces.append(workspace_name)
                indices.append(second_fit_index)

        return workspaces, indices

    @staticmethod
    def _get_fit_label(workspace_name, index):
        label = ''
        fit_function_name = get_fit_function_name_from_workspace(workspace_name)
        if fit_function_name:
            if index in [1]:
                workspace_type = 'Calc'
            elif index == 2:
                workspace_type = 'Diff'
            label = f";{fit_function_name};{workspace_type}"
        return label

    def _is_guess_workspace(self, workspace_name):
        return self.context.guess_workspace_prefix in workspace_name

    def _get_workspace_plot_axis(self, workspace_name: str, axes_workspace_map, index=None):
        if not self.context.plot_panes_context[self.name].settings._is_tiled:
            return 0
        workspace_name = workspace_name.replace(self.context.guess_workspace_prefix, "")
        run_as_string, detector = self.context.group_context.get_detector_and_run_from_workspace_name(workspace_name)
        if detector in axes_workspace_map:
            return axes_workspace_map[detector]

        if run_as_string in axes_workspace_map:
            return axes_workspace_map[run_as_string]

        return 0
