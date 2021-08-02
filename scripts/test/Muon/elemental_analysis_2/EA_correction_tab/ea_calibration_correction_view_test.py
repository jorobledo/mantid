import unittest
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_calibration_correction_view import EACalibrationCorrectionTabView
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder
from qtpy.QtWidgets import QApplication


@start_qapplication
class EACalibrationCorrectionTabViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = EACalibrationCorrectionTabView()
        self.view.show()
        self.view.setVisible(True)
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_apply_calibration(self):
        # When checked
        self.view.add_calibration_checkbox.setChecked(True)
        self.assertTrue(self.view.apply_calibration())

        # When unchecked
        self.view.add_calibration_checkbox.setChecked(False)
        self.assertFalse(self.view.apply_calibration())

    def test_calibration_checkbox_hides_view(self):
        # When checked
        self.view.add_calibration_checkbox.setChecked(True)
        self.assertTrue(self.view.holder_widget.isVisible())

        # When unchecked
        self.view.add_calibration_checkbox.setChecked(False)
        self.assertFalse(self.view.holder_widget.isVisible())

    def test_get_calibration_parameters(self):
        shift = "mock_shift"
        gradient = "mock_gradient"
        self.view.shift_lineedit.setText(shift)
        self.view.gradient_lineedit.setText(gradient)

        params = self.view.get_calibration_parameters()
        self.assertEqual(params, {"shift": shift, "gradient": gradient})


if __name__ == '__main__':
    unittest.main()
