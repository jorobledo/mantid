# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plotting_pane.EA_plot_data_pane_model import SPECTRA_INDICES, \
    INVERSE_SPECTRA_INDICES
from Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plotting_pane.EA_plot_data_pane_presenter import \
    EAPlotDataPanePresenter
from mantidqt.utils.observer_pattern import GenericObserver, GenericObserverWithArgPassing


class EAPlotFitPanePresenter(EAPlotDataPanePresenter):

    def __init__(self, view, model, context, figure_presenter):
        super().__init__(view, model, context, figure_presenter)
        self._view.show_plot_diff()
        self._current_fit_info = None
        self._view.enable_tile_plotting_options()
        self._view.on_plot_diff_checkbox_changed(self.plot_diff_changed)
        self.view.hide_plot_type()

        self.plot_selected_fit_observer = GenericObserverWithArgPassing(self.handle_plot_selected_fits)
        self.remove_plot_guess_observer = GenericObserver(self.handle_remove_plot_guess)
        self.update_plot_guess_observer = GenericObserver(self.handle_update_plot_guess)
        self.fit_spectrum_changed_observer = GenericObserverWithArgPassing(self.handle_fit_spectrum_changed)

    def plot_diff_changed(self):
        self.handle_plot_selected_fits(self._current_fit_info)

    def handle_plot_selected_fits(self, fit_information_list, autoscale=False):
        """Plots a list of selected fit workspaces (obtained from fit and seq fit tabs).
        :param fit_information_list: List of named tuples each entry of the form (fit, input_workspaces)
        """
        workspace_list = []
        indices = []
        all_fit_workspaces = []
        raw = self._view.is_raw_plot()
        with_diff = self._view.is_plot_diff()
        plot_type = self._view.get_plot_type()
        if fit_information_list:
            self._current_fit_info = fit_information_list
            for fit_information in fit_information_list:
                fit = fit_information.fit
                fit_workspaces, fit_indices = self._model.get_fit_workspace_and_indices(fit, with_diff)
                all_fit_workspaces.extend(fit_workspaces)
                workspace_list += self.match_raw_selection(fit_information.input_workspaces, raw) + fit_workspaces
                indices += [SPECTRA_INDICES[plot_type]] * len(fit_information.input_workspaces) + fit_indices
        self._figure_presenter.remove_workspace_names_from_plot(all_fit_workspaces)
        self._figure_presenter.plot_workspaces(workspace_list, indices, hold_on=False, autoscale=autoscale)

    def match_raw_selection(self, workspace_names, plot_raw):
        workspace_list = []
        for workspace in workspace_names:
            group_name = self.context.get_group_name(workspace)
            group = self.context.group_context[group_name]
            if plot_raw:
                workspace_list.append(group.get_counts_workspace_for_run())
            else:
                workspace_list.append(group.get_rebined_or_unbinned_version_of_workspace_if_it_exists())
        return workspace_list

    def handle_data_type_changed(self):
        """
        Handles the data type being changed in the view by plotting the workspaces corresponding to the new data type
        """
        self.handle_data_updated(autoscale=True, hold_on=False)
        # the data change probably means its the wrong scale
        self._figure_presenter.force_autoscale()

    def handle_use_raw_workspaces_changed(self):
        if self.check_if_can_use_rebin():
            self.handle_plot_selected_fits(self._current_fit_info)

    def handle_data_updated(self, autoscale=True, hold_on=False):
        self.handle_plot_selected_fits(self._current_fit_info, autoscale)

    def handle_remove_plot_guess(self):
        if self.context.fitting_context.guess_workspace_name is not None:
            self._figure_presenter.remove_workspace_names_from_plot([self.context.fitting_context.guess_workspace_name])

    def handle_update_plot_guess(self):
        if self.context.fitting_context.guess_workspace_name is not None and self.context.fitting_context.plot_guess:
            self._figure_presenter.plot_guess_workspace(self.context.fitting_context.guess_workspace_name)

    def handle_fit_spectrum_changed(self, spectrum_index):
        spectrum_name = INVERSE_SPECTRA_INDICES[spectrum_index]
        self.view.set_plot_type(spectrum_name)
        self.handle_data_type_changed()
