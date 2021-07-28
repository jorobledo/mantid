import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_presenter import EACorrectionTabPresenter


class EACorrectionTabPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.Mock()
        self.model = mock.Mock()
        self.context = mock.Mock()
        self.presenter = EACorrectionTabPresenter(self.view, self.model, self.context)

    def test_get_calibration_parameters_with_invalid_parameters(self):
        self.view.calibration_view.get_calibration_parameters = mock.Mock()
        self.view.warning_popup = mock.Mock()
        self.view.calibration_view.get_calibration_parameters.return_value = {"gradient": "mock", "shift": "1"}
        params = self.presenter.get_calibration_parameters()
        self.view.warning_popup.assert_called_once_with("Gradient and energy shift must be a number")
        self.assertEqual(params, None)

        self.view.calibration_view.get_calibration_parameters = mock.Mock()
        self.view.warning_popup = mock.Mock()
        self.view.calibration_view.get_calibration_parameters.return_value = {"gradient": "4", "shift": "test"}
        params = self.presenter.get_calibration_parameters()
        self.view.warning_popup.assert_called_once_with("Gradient and energy shift must be a number")
        self.assertEqual(params, None)

        self.view.calibration_view.get_calibration_parameters = mock.Mock()
        self.view.warning_popup = mock.Mock()
        self.view.calibration_view.get_calibration_parameters.return_value = {"gradient": "mock", "shift": "test"}
        params = self.presenter.get_calibration_parameters()
        self.view.warning_popup.assert_called_once_with("Gradient and energy shift must be a number")
        self.assertEqual(params, None)

    def test_get_calibration_parameters_with_valid_parameters(self):
        correct_params = {"gradient": 3, "shift": 1}
        self.view.calibration_view.get_calibration_parameters = mock.Mock()
        self.view.warning_popup = mock.Mock()
        self.view.calibration_view.get_calibration_parameters.return_value = {"gradient": "3", "shift": "1"}
        params = self.presenter.get_calibration_parameters()
        self.view.warning_popup.assert_not_called()
        self.assertEqual(correct_params, params)

    def test_get_efficiency_parameters_with_invalid_parameters(self):
        params = {"use default efficiencies": False, "detector filepath": ""}
        self.view.warning_popup = mock.Mock()
        self.view.efficiency_view.get_efficiency_parameters.return_value = params
        params = self.presenter.get_efficiency_parameters()
        self.view.warning_popup.assert_called_once_with("Filepath for detector_efficiency data file must be given")
        self.assertEqual(params, None)

    def test_get_efficiency_parameters_with_valid_parameters(self):
        params = {"use default efficiencies": False, "detector filepath": "mock_filepath"}
        self.view.warning_popup = mock.Mock()
        self.view.efficiency_view.get_efficiency_parameters.return_value = params
        params = self.presenter.get_efficiency_parameters()
        self.view.warning_popup.assert_not_called()
        self.assertEqual(params, params)

    def test_get_absorption_parameters_with_invalid_parameters(self):
        pass

    def test_get_absorption_parameters_with_valid_parameters(self):
        pass

    def test_handle_apply_correction_button_clicked(self):
        initial_parameters = {"group_name": "Mock_workspace", "energy_start": 30, "energy_end": 100}
        self.presenter.get_calibration_parameters = mock.Mock(return_value="Calibration")
        self.presenter.get_efficiency_parameters = mock.Mock(return_value="Efficiency")
        self.presenter.get_absorption_parameters = mock.Mock(return_value="Absorption")
        self.view.calibration_view.apply_calibration.return_value = True
        self.view.efficiency_view.apply_efficiency.return_value = True
        self.view.absorption_view.apply_absorption.return_value = True
        self.presenter.get_initial_parameters = mock.Mock(return_value=initial_parameters)
        mock_group = mock.Mock()
        mock_group.detector = "Detector 4"
        self.context.group_context.__getitem__ = lambda x, y: mock_group

        self.presenter.handle_apply_correction_button_clicked()

        self.view.warning_popup.assert_not_called()
        self.presenter.get_initial_parameters.assert_called_once()
        self.presenter.get_calibration_parameters.assert_called_once()
        self.presenter.get_efficiency_parameters.assert_called_once()
        self.presenter.get_absorption_parameters.assert_called_once()
        self.model.handle_calculate_corrections.assert_called_once_with({"initial": initial_parameters,
                                                                         "calibration": "Calibration",
                                                                         "efficiency": "Efficiency",
                                                                         "absorption": "Absorption"})

    def test_handle_apply_correction_button_clicked_with_invalid_start_energy(self):
        initial_parameters = {"group_name": "Mock_workspace", "energy_start": 30, "energy_end": 100}
        self.presenter.get_calibration_parameters = mock.Mock(return_value="Calibration")
        self.presenter.get_efficiency_parameters = mock.Mock(return_value="Efficiency")
        self.presenter.get_absorption_parameters = mock.Mock(return_value="Absorption")
        self.view.calibration_view.apply_calibration.return_value = False
        self.view.efficiency_view.apply_efficiency.return_value = True
        self.view.absorption_view.apply_absorption.return_value = True
        self.presenter.get_initial_parameters = mock.Mock(return_value=initial_parameters)
        mock_group = mock.Mock()
        mock_group.detector = "Detector 2"
        self.context.group_context.__getitem__ = lambda x, y: mock_group

        self.presenter.handle_apply_correction_button_clicked()

        self.view.warning_popup.assert_called_once_with("Efficiencies for Detector 2 below 60 KeV is not defined well"
                                                        " so corrected data may be incorrect")
        self.presenter.get_initial_parameters.assert_called_once()
        self.presenter.get_calibration_parameters.assert_not_called()
        self.presenter.get_efficiency_parameters.assert_called_once()
        self.presenter.get_absorption_parameters.assert_called_once()
        self.model.handle_calculate_corrections.assert_called_once_with({"initial": initial_parameters,
                                                                         "efficiency": "Efficiency",
                                                                         "absorption": "Absorption"})

    def test_handle_apply_correction_button_clicked_when_no_correction_selected(self):
        self.presenter.get_calibration_parameters = mock.Mock()
        self.presenter.get_efficiency_parameters = mock.Mock()
        self.presenter.get_absorption_parameters = mock.Mock()

        self.view.calibration_view.apply_calibration.return_value = False
        self.view.efficiency_view.apply_efficiency.return_value = False
        self.view.absorption_view.apply_absorption.return_value = False
        self.presenter.get_initial_parameters = mock.Mock(return_value="mock_params")

        self.presenter.handle_apply_correction_button_clicked()

        self.view.warning_popup.assert_called_once_with("No corrections selected")
        self.model.handle_calculate_corrections.assert_not_called()
        self.presenter.get_initial_parameters.assert_called_once()
        self.presenter.get_calibration_parameters.assert_not_called()
        self.presenter.get_efficiency_parameters.assert_not_called()
        self.presenter.get_absorption_parameters.assert_not_called()

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_presenter.QFileDialog")
    def test_handle_select_efficiency_data_file_button_clicked(self, mock_file_dialog):
        mock_file_dialog.getOpenFileName.return_value = ("mock_file", "test")
        self.presenter.handle_select_efficiency_data_file_button_clicked()
        mock_file_dialog.getOpenFileName.assert_called_once()
        self.view.set_efficiency_data_file_label_text.assert_called_once_with("mock_file")

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_presenter.QFileDialog")
    def test_handle_select_efficiency_data_file_button_clicked_with_invalid_filepath(self, mock_file_dialog):
        mock_file_dialog.getOpenFileName.return_value = ""
        self.presenter.handle_select_efficiency_data_file_button_clicked()
        mock_file_dialog.getOpenFileName.assert_called_once()
        self.view.set_efficiency_data_file_label_text.assert_not_called()

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_presenter.QFileDialog")
    def test_handle_select_absorption_data_file_button_clicked(self, mock_file_dialog):
        mock_file_dialog.getOpenFileName.return_value = ("mock_file", "test")
        self.presenter.handle_select_absorption_data_file_button_clicked()
        mock_file_dialog.getOpenFileName.assert_called_once()
        self.view.set_absorption_coefficient_data_file_label_text.assert_called_once_with("mock_file")

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_presenter.QFileDialog")
    def test_handle_select_absorption_data_file_button_clicked_with_invalid_filepath(self, mock_file_dialog):
        mock_file_dialog.getOpenFileName.return_value = ""
        self.presenter.handle_select_absorption_data_file_button_clicked()
        mock_file_dialog.getOpenFileName.assert_called_once()
        self.view.set_absorption_coefficient_data_file_label_text.assert_not_called()

    def test_update_view(self):
        self.context.group_context.group_names = ["mock; detector 1", "mock; detector 2", "test; detector 3",
                                                  "test; detector 4"]
        self.presenter.update_view()

        self.view.add_workspace_to_view.assert_called_once_with({"mock": ["detector 1", "detector 2"],
                                                                 "test": ["detector 3", "detector 4"]})


if __name__ == '__main__':
    unittest.main()
