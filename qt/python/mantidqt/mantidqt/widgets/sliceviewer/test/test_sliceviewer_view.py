# Mantid Repository : https://github.com/mantidproject/mantid
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from functools import partial
import io
import sys
from threading import Thread
import unittest
from unittest.mock import patch

import matplotlib as mpl
from matplotlib.colors import Normalize
from numpy import hstack

mpl.use('Agg')
from mantidqt.widgets.colorbar.colorbar import MIN_LOG_VALUE  # noqa: E402
from mantidqt.widgets.sliceviewer.view import SCALENORM  # noqa: E402
from mantid.simpleapi import (  # noqa: E402
    CreateMDHistoWorkspace, CreateMDWorkspace, CreateSampleWorkspace, DeleteWorkspace, FakeMDEventData, ConvertToDistribution, Scale, SetUB,
    RenameWorkspace, ClearUB)
from mantid.api import AnalysisDataService  # noqa: E402
from mantidqt.utils.qt.testing import start_qapplication  # noqa: E402
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder  # noqa: E402
from mantidqt.widgets.sliceviewer.presenter import SliceViewer  # noqa: E402
from mantidqt.widgets.sliceviewer.toolbar import ToolItemText  # noqa: E402
from qtpy.QtWidgets import QApplication  # noqa: E402
from math import inf  # noqa: E402
from numpy.testing import assert_allclose  # noqa: E402


class MockConfig(object):
    def get(self, name):
        if name == SCALENORM:
            return "Log"

    def set(self, name):
        pass

    def has(self, name):
        if name == SCALENORM:
            return True


@start_qapplication
class SliceViewerViewTest(unittest.TestCase, QtWidgetFinder):
    @classmethod
    def setUpClass(cls):
        cls.histo_ws = CreateMDHistoWorkspace(Dimensionality=3,
                                              Extents='-3,3,-10,10,-1,1',
                                              SignalInput=range(100),
                                              ErrorInput=range(100),
                                              NumberOfBins='5,5,4',
                                              Names='Dim1,Dim2,Dim3',
                                              Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                              OutputWorkspace='ws_MD_2d')
        cls.histo_ws_positive = CreateMDHistoWorkspace(Dimensionality=3,
                                                       Extents='-3,3,-10,10,-1,1',
                                                       SignalInput=range(1, 101),
                                                       ErrorInput=range(100),
                                                       NumberOfBins='5,5,4',
                                                       Names='Dim1,Dim2,Dim3',
                                                       Units='MomentumTransfer,EnergyTransfer,Angstrom',
                                                       OutputWorkspace='ws_MD_2d_pos')
        cls.hkl_ws = CreateMDWorkspace(Dimensions=3,
                                       Extents='-10,10,-9,9,-8,8',
                                       Names='A,B,C',
                                       Units='r.l.u.,r.l.u.,r.l.u.',
                                       Frames='HKL,HKL,HKL',
                                       OutputWorkspace='hkl_ws')
        expt_info = CreateSampleWorkspace()
        cls.hkl_ws.addExperimentInfo(expt_info)
        SetUB('hkl_ws', 1, 1, 1, 90, 90, 90)

    def tearDown(self):
        for ii in QApplication.topLevelWidgets():
            ii.close()
        QApplication.sendPostedEvents()
        QApplication.sendPostedEvents()
        self.assert_no_toplevel_widgets()

    def test_deleted_on_close(self):
        pres = SliceViewer(self.histo_ws)
        self.assert_widget_created()
        pres.view.close()

        QApplication.sendPostedEvents()
        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()
        self.assertEqual(pres.ads_observer, None)

    def test_enable_nonorthogonal_view_disables_lineplots_if_enabled(self):
        pres = SliceViewer(self.hkl_ws)
        line_plots_action, region_sel_action, non_ortho_action = toolbar_actions(
            pres, (ToolItemText.LINEPLOTS, ToolItemText.REGIONSELECTION, ToolItemText.NONORTHOGONAL_AXES))
        line_plots_action.trigger()
        QApplication.sendPostedEvents()

        non_ortho_action.trigger()
        QApplication.sendPostedEvents()

        self.assertTrue(non_ortho_action.isChecked())
        self.assertFalse(line_plots_action.isChecked())
        self.assertFalse(line_plots_action.isEnabled())
        self.assertFalse(region_sel_action.isChecked())
        self.assertFalse(region_sel_action.isEnabled())

        pres.view.close()

    def test_non_orthog_view_disabled_when_Eaxis_viewed(self):
        ws_4D = CreateMDWorkspace(Dimensions=4, Extents=[-1, 1, -1, 1, -1, 1, -1, 1], Names="E,H,K,L",
                                  Frames='General Frame,HKL,HKL,HKL', Units='meV,r.l.u.,r.l.u.,r.l.u.')
        expt_info_4D = CreateSampleWorkspace()
        ws_4D.addExperimentInfo(expt_info_4D)
        SetUB(ws_4D, 1, 1, 2, 90, 90, 120)
        pres = SliceViewer(ws_4D)
        QApplication.sendPostedEvents()

        non_ortho_action = toolbar_actions(pres, [ToolItemText.NONORTHOGONAL_AXES])[0]
        self.assertFalse(non_ortho_action.isEnabled())

        pres.view.close()

    def test_disable_lineplots_disables_region_selector(self):
        pres = SliceViewer(self.hkl_ws)
        line_plots_action, region_sel_action = toolbar_actions(pres, (ToolItemText.LINEPLOTS, ToolItemText.REGIONSELECTION))
        line_plots_action.trigger()
        region_sel_action.trigger()
        QApplication.sendPostedEvents()

        line_plots_action.trigger()
        QApplication.sendPostedEvents()

        self.assertFalse(line_plots_action.isChecked())
        self.assertFalse(region_sel_action.isChecked())

        pres.view.close()

    def test_clim_edits_prevent_negative_values_if_lognorm(self):

        pres = SliceViewer(self.histo_ws)
        colorbar = pres.view.data_view.colorbar
        colorbar.autoscale.setChecked(False)
        colorbar.norm.setCurrentText("Log")
        prev_bottom_limit = colorbar.cmin.text()
        prev_top_limit = colorbar.cmax.text()

        colorbar.cmin.textChanged.emit(str(-10))
        colorbar.cmax.textChanged.emit(str(-5))

        self.assertEqual(colorbar.cmin.text(), prev_bottom_limit)
        self.assertEqual(colorbar.cmax.text(), prev_top_limit)

        pres.view.close()

    def test_norm_switches_if_workspace_contains_non_positive_data(self):
        conf = MockConfig()
        pres = SliceViewer(self.histo_ws, conf=conf)
        colorbar = pres.view.data_view.colorbar
        self.assertTrue(isinstance(colorbar.get_norm(), Normalize))
        pres.view.close()

    def test_changing_norm_updates_clim_validators(self):
        pres = SliceViewer(self.histo_ws_positive)
        colorbar = pres.view.data_view.colorbar
        colorbar.autoscale.setChecked(False)

        colorbar.norm.setCurrentText("Log")
        self.assertEqual(colorbar.cmin.validator().bottom(), MIN_LOG_VALUE)

        colorbar.norm.setCurrentText("Linear")
        self.assertEqual(colorbar.cmin.validator().bottom(), -inf)

        pres.view.close()

    def test_update_plot_data_updates_axes_limits_when_orthog_data_tranposed(self):
        pres = SliceViewer(self.hkl_ws)

        # not transpose
        pres.view.data_view.dimensions.transpose = False
        pres.update_plot_data()
        extent = pres.view.data_view.image.get_extent()
        assert_allclose(extent, [-10.0, 10.0, -9.0, 9.0])

        # transpose
        pres.view.data_view.dimensions.transpose = True
        pres.update_plot_data()
        extent = pres.view.data_view.image.get_extent()

        # Should be the same as before as transposition is handled in the model
        assert_allclose(extent, [-10.0, 10.0, -9.0, 9.0])
        assert_allclose(extent[0:2], list(pres.view.data_view.ax.get_xlim()))
        assert_allclose(extent[2:], list(pres.view.data_view.ax.get_ylim()))

        pres.view.close()

    def test_view_closes_on_shown_workspace_deleted(self):
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        DeleteWorkspace(ws)

        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()
        self.assertEqual(pres.ads_observer, None)

    def test_view_does_not_close_on_other_workspace_deleted(self):
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        other_ws = CreateSampleWorkspace()
        DeleteWorkspace(other_ws)

        QApplication.sendPostedEvents()

        # Strange behaviour between tests where views don't close properly means
        # we cannot use self.assert_widget_exists, as many instances of SliceViewerView
        # may be active at this time. As long as this view hasn't closed we are OK.
        self.assertTrue(pres.view in self.find_widgets_of_type(str(type(pres.view))))
        self.assertNotEqual(pres.ads_observer, None)

        pres.view.close()

    def test_view_closes_on_replace_when_model_properties_change(self):
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        ConvertToDistribution(ws)

        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()
        self.assertEqual(pres.ads_observer, None)

    def test_view_updates_on_replace_when_model_properties_dont_change_matrixws(self):
        def scale_ws(ws):
            ws = Scale(ws, 100, "Multiply")

        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        self._assertNoErrorInADSHandlerFromSeparateThread(partial(scale_ws, ws))

        QApplication.sendPostedEvents()

        self.assertTrue(pres.view in self.find_widgets_of_type(str(type(pres.view))))
        self.assertNotEqual(pres.ads_observer, None)

        pres.view.close()

    def test_view_updates_on_replace_when_model_properties_dont_change_mdeventws(self):
        def scale_ws(ws):
            ws = ws * 100

        ws = CreateMDWorkspace(Dimensions='3',
                               EventType='MDEvent',
                               Extents='-10,10,-5,5,-1,1',
                               Names='Q_lab_x,Q_lab_y,Q_lab_z',
                               Units='1\\A,1\\A,1\\A')
        FakeMDEventData(ws, UniformParams="1000000")
        pres = SliceViewer(ws)
        self._assertNoErrorInADSHandlerFromSeparateThread(partial(scale_ws, ws))

        QApplication.sendPostedEvents()

        self.assertTrue(pres.view in self.find_widgets_of_type(str(type(pres.view))))
        self.assertNotEqual(pres.ads_observer, None)

        pres.view.close()

    def test_view_title_correct_and_view_didnt_close_on_workspace_rename(self):
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        old_title = pres.model.get_title()

        renamed = RenameWorkspace(ws)  # noqa F841

        QApplication.sendPostedEvents()

        # View didn't close
        self.assertTrue(pres.view in self.find_widgets_of_type(str(type(pres.view))))
        self.assertNotEqual(pres.ads_observer, None)

        # Window title correct
        self.assertNotEqual(pres.model.get_title(), old_title)
        self.assertEqual(pres.view.windowTitle(), pres.model.get_title())

        pres.view.close()

    def test_view_title_did_not_change_other_workspace_rename(self):
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        title = pres.model.get_title()
        other_workspace = CreateSampleWorkspace()
        other_renamed = RenameWorkspace(other_workspace)  # noqa F841

        self.assertEqual(pres.model.get_title(), title)
        self.assertEqual(pres.view.windowTitle(), pres.model.get_title())

    def test_view_closes_on_ADS_cleared(self):
        ws = CreateSampleWorkspace()
        SliceViewer(ws)
        AnalysisDataService.clear()

        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()

    def test_view_closes_on_replace_when_changed_support_for_nonortho_transform(self):
        ws_non_ortho = CreateMDWorkspace(Dimensions='3', Extents='-6,6,-4,4,-0.5,0.5',
                                         Names='H,K,L', Units='r.l.u.,r.l.u.,r.l.u.',
                                         Frames='HKL,HKL,HKL',
                                         SplitInto='2', SplitThreshold='50')
        expt_info_nonortho = CreateSampleWorkspace()
        ws_non_ortho.addExperimentInfo(expt_info_nonortho)
        SetUB(ws_non_ortho, 1, 1, 2, 90, 90, 120)
        pres = SliceViewer(ws_non_ortho)
        ClearUB(ws_non_ortho)

        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()
        self.assertEqual(pres.ads_observer, None)

    def test_plot_matrix_xlimits_ignores_monitors(self):
        xmin = 5000
        xmax = 10000
        ws = CreateSampleWorkspace(NumBanks=1, NumMonitors=1, BankPixelWidth=1, XMin=xmin, XMax=xmax)
        ws.setX(0, 2 * ws.readX(0))  # change x limits of monitor spectrum
        pres = SliceViewer(ws)

        pres.view.data_view.plot_matrix(ws)

        self.assertEqual(pres.view.data_view.get_axes_limits()[0], (xmin, xmax))

    @patch("mantidqt.widgets.sliceviewer.dimensionwidget.Dimension.update_slider")
    def test_plot_matrix_xlimits_ignores_nans(self, mock_update_slider):
        # need to mock update slider as doesn't handle inf when initialising SliceViewer in this manner
        xmin = 5000
        xmax = 10000
        ws = CreateSampleWorkspace(NumBanks=2, BankPixelWidth=2, XMin=xmin, XMax=xmax)  # two non-monitor spectra
        ws.setX(0, hstack([2 * ws.readX(0)[0:-1], inf]))  # change x limits of spectrum and put inf in last element
        pres = SliceViewer(ws)

        pres.view.data_view.plot_matrix(ws)

        self.assertEqual(pres.view.data_view.get_axes_limits()[0], (xmin, xmax))

    def test_close_event(self):
        ws = CreateSampleWorkspace()
        pres = SliceViewer(ws)
        self.assert_widget_created()

        pres.view.close()
        pres = None
        QApplication.sendPostedEvents()

        self.assert_no_toplevel_widgets()

    def test_axes_limits_respect_nonorthog_transform(self):
        limits = (-10.0, 10.0, -9.0, 9.0)
        ws_nonrotho = CreateMDWorkspace(Dimensions=3,
                                        Extents=','.join([str(lim) for lim in limits]) + ',-8,8',
                                        Names='A,B,C',
                                        Units='r.l.u.,r.l.u.,r.l.u.',
                                        Frames='HKL,HKL,HKL')
        expt_info_nonortho = CreateSampleWorkspace()
        ws_nonrotho.addExperimentInfo(expt_info_nonortho)
        SetUB(ws_nonrotho, 1, 1, 2, 90, 90, 120)
        pres = SliceViewer(ws_nonrotho)

        # assert limits of orthog
        limits_orthog = pres.view.data_view.get_axes_limits()
        self.assertEqual(limits_orthog[0], limits[0:2])
        self.assertEqual(limits_orthog[1], limits[2:])

        # set nonorthog view and retrieve new limits
        pres.nonorthogonal_axes(True)
        limits_nonorthog = pres.view.data_view.get_axes_limits()
        self.assertAlmostEqual(limits_nonorthog[0][0], -19, delta=1e-5)
        self.assertAlmostEqual(limits_nonorthog[0][1], 19, delta=1e-5)
        self.assertEqual(limits_nonorthog[1], limits[2:])

        pres.view.close()

    # private methods
    def _assertNoErrorInADSHandlerFromSeparateThread(self, operation):
        """Check the error stream for any errors when calling operation.
        Raise an assertion error if errors are detected
        """
        try:
            # the ads handler catches all exceptions so that the handlers don't
            # bring down the sliceviewer. Check if anything is written to stderr
            stderr_capture = io.StringIO()
            stderr_orig = sys.stderr
            sys.stderr = stderr_capture
            op_thread = Thread(target=operation)
            op_thread.start()
            op_thread.join()
            self.assertTrue('Error occurred in handler' not in stderr_capture.getvalue(), msg=stderr_capture.getvalue())
        finally:
            sys.stderr = stderr_orig


# private helper functions
def toolbar_actions(presenter, text_labels):
    """
    Return a list of actions with the given text labels
    :param presenter: Presenter containing view
    :param text_labels: A list of text labels on toolbar buttons
    :return: A list of QAction objects matching the text labels given
    """
    toolbar_actions = presenter.view.data_view.mpl_toolbar.actions()
    return [action for action in toolbar_actions if action.text() in text_labels]


if __name__ == '__main__':
    unittest.main()
