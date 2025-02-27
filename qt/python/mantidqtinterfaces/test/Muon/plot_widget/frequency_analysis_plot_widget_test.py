# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget import DATA, MODEL, FIT, RAW
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.frequency_analysis_plot_widget import FrequencyAnalysisPlotWidget,\
        MAXENT
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.base_pane.base_pane_view import BasePaneView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.raw_pane.raw_pane_view import RawPaneView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.main_plot_widget_view import MainPlotWidgetView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.main_plot_widget_presenter import MainPlotWidgetPresenter
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.plotting_canvas.plotting_canvas_widget import PlottingCanvasWidget

from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context

from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.dual_plot_maxent_pane.\
    dual_plot_maxent_pane_view import DualPlotMaxentPaneView
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.quick_edit.quick_edit_widget import DualQuickEditWidget


class FrequencyAnalysisPlotWidgetTest(unittest.TestCase):

    def make_canvas_mock(self, key):
        self.canvas[key] =  mock.MagicMock(autospec=PlottingCanvasWidget)
        return self.canvas[key]

    def canvas_mocks(self, parent, context, plot_model, figure_options=None):
        if plot_model.name == "Plot Data":
            return self.make_canvas_mock(DATA)
        elif plot_model.name == "Frequency Data":
            return self.make_canvas_mock(FIT)
        elif plot_model.name == "Raw Data":
            return self.make_canvas_mock(RAW)
        elif plot_model.name == "Model Data":
            return self.make_canvas_mock(MODEL)
        elif plot_model.name == "Maxent Dual plot":
            return self.make_canvas_mock(MAXENT)
        return self.make_canvas_mock("not found")

    # set up get methods for mock view
    @property
    def mock_view(self):
        return self.mock_main_view.return_value

    # since some functions are only called as part of the inherited class, we need to patch them there
    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.PlottingCanvasWidget')
    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.BasePaneView')
    @mock.patch('mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.frequency_analysis_plot_widget.MainPlotWidgetView')
    @mock.patch('mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.frequency_analysis_plot_widget.PlottingCanvasWidget')
    @mock.patch('mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.frequency_analysis_plot_widget.BasePaneView')
    @mock.patch('mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.frequency_analysis_plot_widget.DualQuickEditWidget')
    @mock.patch('mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.frequency_analysis_plot_widget.DualPlotMaxentPaneView')
    def setUp(self,mock_dual_view, mock_quick_edit, mock_base_view, mock_plot_canvas, mock_main_view,
              mock_muon_base_view, mock_muon_canvas):
        self.mock_dual_view = mock_dual_view
        self.mock_quick_edit = mock_quick_edit
        self.mock_main_view = mock_main_view
        self.mock_base_view = mock_base_view
        self.mock_plot_canvas = mock_plot_canvas
        self.mock_muon_base_view = mock_muon_base_view
        self.mock_muon_canvas = mock_muon_canvas

        self.canvas ={}
        self._num_count=0
        self._letter_count=0

        self.mock_dual_view.return_value = mock.MagicMock(autospec=DualPlotMaxentPaneView)
        self.mock_quick_edit.return_value = mock.MagicMock(autospec=DualQuickEditWidget)
        self.mock_main_view.return_value = mock.MagicMock(autospec=MainPlotWidgetView)
        self.mock_base_view.return_value = mock.MagicMock(autospec=BasePaneView)
        self.mock_plot_canvas.side_effect = self.canvas_mocks
        self.mock_muon_base_view.return_value = mock.MagicMock(autospec=BasePaneView)
        self.mock_muon_canvas.side_effect = self.canvas_mocks

        self.context = setup_context(False)
        self.widget = FrequencyAnalysisPlotWidget(self.context)

    def test_setup(self):
        # want to check that by default only data, fit and maxent panes

        # check we have correct number of canvases, views etc.
        self.assertEqual(self.mock_plot_canvas.call_count, 2)
        self.assertEqual(self.mock_muon_canvas.call_count, 1)
        self.assertEqual(self.mock_muon_base_view.call_count,1)
        self.assertEqual(self.mock_base_view.call_count,1)
        self.assertEqual(self.mock_dual_view.call_count,1)

        # check panes have been added, its just a list
        self.assertEqual(len(self.widget._panes),3)

        # check the plot modes have been added
        self.assertEqual(list(self.widget.modes.keys()), [DATA, FIT, MAXENT])
        self.assertEqual(self.widget.modes[DATA].name, "Plot Data")
        # frequency is fit in this widget
        self.assertEqual(self.widget.modes[FIT].name, "Frequency Data")
        self.assertEqual(self.widget.modes[MAXENT].name, "Maxent Dual plot")

    def number_list(self):
        self._num_count+=1
        outputs = ["not used", "data 1", "fit 2", "maxent 3", "4"]
        return outputs[self._num_count]

    def letter_list(self):
        self._letter_count+=1
        outputs = ["not used", "data a", "fit b", "maxent c", "d"]
        return outputs[self._letter_count]

    def test_insert_plot_panes(self):
        type(self.mock_view).get_plot_mode= mock.PropertyMock(return_value="Plot Data")

        for key in self.widget.modes.keys():
            type(self.widget.modes[key]).workspace_replaced_in_ads_observer = mock.PropertyMock(return_value = self.number_list())
            type(self.widget.modes[key]).workspace_deleted_from_ads_observer = mock.PropertyMock(return_value = self.letter_list())

        self.context.update_plots_notifier.add_subscriber = mock.Mock()
        self.context.deleted_plots_notifier.add_subscriber = mock.Mock()

        self.widget.insert_plot_panes()

        self.context.update_plots_notifier.add_subscriber.assert_any_call("data 1")
        self.context.update_plots_notifier.add_subscriber.assert_any_call("fit 2")
        self.context.update_plots_notifier.add_subscriber.assert_any_call("maxent 3")
        self.assertEqual(self.context.update_plots_notifier.add_subscriber.call_count,3)

        self.context.deleted_plots_notifier.add_subscriber.assert_any_call("data a")
        self.context.deleted_plots_notifier.add_subscriber.assert_any_call("fit b")
        self.context.deleted_plots_notifier.add_subscriber.assert_any_call("maxent c")
        self.assertEqual(self.context.deleted_plots_notifier.add_subscriber.call_count,3)

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetView')
    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.PlottingCanvasWidget')
    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.BasePaneView')
    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.RawPaneView')
    def add_all_panes(self, mock_raw_view, mock_base_view, mock_plot_canvas, mock_main_view):
        self.mock_main_view = mock_main_view
        self.mock_base_view = mock_base_view
        self.mock_raw_view = mock_raw_view
        self.mock_plot_canvas = mock_plot_canvas

        self.mock_main_view.return_value = mock.MagicMock(autospec=MainPlotWidgetView)
        self.mock_base_view.return_value = mock.MagicMock(autospec=BasePaneView)
        self.mock_raw_view.return_value = mock.MagicMock(autospec=RawPaneView)
        self.mock_plot_canvas.side_effect = self.canvas_mocks

        # can exclude the defaults (data and fit)
        self.widget.create_model_fit_pane()
        self.widget.create_raw_pane()
        # mock the update methods in new panes
        self.widget.modes[RAW].handle_data_updated = mock.Mock()

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_leave_data(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)

        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Plot Data"
        type(mock_presenter.return_value).get_plot_mode = "Raw Data"

        self.widget.handle_plot_mode_changed_by_user()
        self.canvas[DATA].set_quick_edit_info.assert_not_called()
        self.canvas[FIT].set_quick_edit_info.assert_not_called()
        mock_presenter.return_value.hide.assert_called_once_with("Plot Data")

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_leave_fit(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)

        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Fit Data"
        type(mock_presenter.return_value).get_plot_mode = mock.PropertyMock(return_value="Raw Data")

        self.widget.handle_plot_mode_changed_by_user()
        mock_presenter.return_value.hide.assert_called_once_with("Fit Data")
        self.canvas[DATA].set_quick_edit_info.assert_not_called()
        self.canvas[FIT].set_quick_edit_info.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_leave_raw(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)

        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Raw Data"
        type(mock_presenter.return_value).get_plot_mode = mock.PropertyMock(return_value="Fit Data")

        self.widget.handle_plot_mode_changed_by_user()
        # check it does not force any plots to update
        self.canvas[DATA].set_quick_edit_info.assert_not_called()
        self.canvas[FIT].set_quick_edit_info.assert_not_called()
        mock_presenter.return_value.hide.assert_called_once_with("Raw Data")

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_leave_model(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)

        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Model Data"
        type(mock_presenter.return_value).get_plot_mode = mock.PropertyMock(return_value="Fit Data")

        self.widget.handle_plot_mode_changed_by_user()
        # check it does not force any plots to update
        self.canvas[DATA].set_quick_edit_info.assert_not_called()
        self.canvas[FIT].set_quick_edit_info.assert_not_called()
        mock_presenter.return_value.hide.assert_called_once_with("Model Data")

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_leave_maxent(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)

        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Maxent Dual plot"
        type(mock_presenter.return_value).get_plot_mode = mock.PropertyMock(return_value="Fit Data")

        self.widget.handle_plot_mode_changed_by_user()
        # check it does not force any plots to update
        self.canvas[DATA].set_quick_edit_info.assert_not_called()
        self.canvas[FIT].set_quick_edit_info.assert_not_called()
        mock_presenter.return_value.hide.assert_called_once_with("Maxent Dual plot")

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_to_data(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)

        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Model Data"
        type(mock_presenter.return_value).get_plot_mode = mock.PropertyMock(return_value="Plot Data")

        self.widget.handle_plot_mode_changed_by_user()
        mock_presenter.return_value.show.assert_called_once_with("Plot Data")
        self.widget.modes[RAW].handle_data_updated.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_to_fit(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)

        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Model Data"
        type(mock_presenter.return_value).get_plot_mode = mock.PropertyMock(return_value="Fit Data")

        self.widget.handle_plot_mode_changed_by_user()
        mock_presenter.return_value.show.assert_called_once_with("Fit Data")
        self.widget.modes[RAW].handle_data_updated.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_to_raw(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)

        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Model Data"
        type(mock_presenter.return_value).get_plot_mode = mock.PropertyMock(return_value="Raw Data")

        self.widget.handle_plot_mode_changed_by_user()
        mock_presenter.return_value.show.assert_called_once_with("Raw Data")
        self.widget.modes[RAW].handle_data_updated.assert_called_once_with()

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_to_model(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)
        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Raw Data"
        type(mock_presenter.return_value).get_plot_mode = mock.PropertyMock(return_value="Model Data")

        self.widget.handle_plot_mode_changed_by_user()
        mock_presenter.return_value.show.assert_called_once_with("Model Data")
        self.widget.modes[RAW].handle_data_updated.assert_not_called()

    @mock.patch('mantidqtinterfaces.Muon.GUI.MuonAnalysis.plot_widget.muon_analysis_plot_widget.MainPlotWidgetPresenter')
    def test_plot_mode_changed_by_user_to_maxent(self, mock_presenter):
        mock_presenter.return_value = mock.MagicMock(autospec=MainPlotWidgetPresenter)
        self.add_all_panes()
        self.widget.insert_plot_panes()
        self.widget._current_plot_mode = "Raw Data"
        type(mock_presenter.return_value).get_plot_mode = mock.PropertyMock(return_value="Maxent Dual plot")

        self.widget.handle_plot_mode_changed_by_user()
        mock_presenter.return_value.show.assert_called_once_with("Maxent Dual plot")
        self.widget.modes[RAW].handle_data_updated.assert_not_called()


if __name__ == '__main__':
    unittest.main()
