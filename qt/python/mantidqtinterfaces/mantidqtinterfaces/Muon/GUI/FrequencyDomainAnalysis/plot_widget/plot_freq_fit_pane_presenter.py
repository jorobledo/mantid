# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.fit_pane.plot_fit_pane_presenter import PlotFitPanePresenter


class PlotFreqFitPanePresenter(PlotFitPanePresenter):

    def __init__(self, view, model, context, fitting_context, figure_presenter):
        super().__init__(view, model, context, fitting_context, figure_presenter)
        self._data_type = ["Frequency"]
        self._sort_by = [""]
        self.update_view()
        self._view.hide_plot_raw()
        self._view.hide_tiled_by()

    def handle_rebin_options_changed(self):
        # there is no way to rebin the data -> do nothing
        return
