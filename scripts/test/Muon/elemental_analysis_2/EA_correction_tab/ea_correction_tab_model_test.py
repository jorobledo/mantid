import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model import EACorrectionTabModel


class EACorrectionTabModelTest(unittest.TestCase):\

    def setUp(self):
        self.context = mock.Mock()
        self.model = EACorrectionTabModel(self.context)

    def test_calculate_corrections(self):
        params = {"initial": {"group_name": "mock_workspace"}, "calibration": "Calibration", "absorption": "Absorption",
                  "efficiency": "Efficiency"}
        spectrums = ["spectrum 1", "spectrum 2", "spectrum 3"]
        self.model.split_and_crop_workspace = mock.Mock(return_value=spectrums)
        self.model.join_workspaces = mock.Mock()
        self.model.handle_correction_successful = mock.Mock()
        self.model.apply_absorption_corrections = mock.Mock()
        self.model.apply_calibration_corrections = mock.Mock()
        self.model.apply_efficiency_corrections = mock.Mock()

        self.model.calculate_corrections(params)
        for spectrum in spectrums:
            self.model.apply_calibration_corrections.assert_called_with(spectrum, "Calibration")
            self.model.apply_efficiency_corrections.assert_called_with(spectrum, "Efficiency", "mock_workspace")
            self.model.apply_absorption_corrections.assert_called_with(spectrum, "Absorption", "mock_workspace")


if __name__ == '__main__':
    unittest.main()
