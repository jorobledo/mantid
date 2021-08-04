import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model import EACorrectionTabModel, \
    DEFAULT_DETECTOR_EFFICIENCY_FUNCTION, EFFICIENCY_WORKSPACE_NAME, XRAY_ABSORPTION_WORKSPACE, \
    MUON_IMPLANTATION_WORKSPACE_NAME, EXTRACTED_SPECTRUM_SUFFIX, SPHERE_XML_TEMPLATE, cm_to_m
import numpy as np


class EACorrectionTabModelTest(unittest.TestCase):

    def setUp(self):
        self.context = mock.Mock()
        self.model = EACorrectionTabModel(self.context)

    def test_calculate_corrections(self):
        group_name = "mock_workspace"
        output_name = "mock_workspace_EA_CORRECTED_DATA"
        params = {"initial": {"group_name": group_name}, "calibration": "Calibration", "absorption": "Absorption",
                  "efficiency": "Efficiency"}
        spectrums = ["spectrum 1", "spectrum 2", "spectrum 3"]
        self.model.split_and_crop_workspace = mock.Mock(return_value=spectrums)
        self.model.join_workspaces = mock.Mock()
        self.model.handle_correction_successful = mock.Mock()
        self.model.apply_absorption_corrections = mock.Mock()
        self.model.apply_calibration_corrections = mock.Mock()
        self.model.apply_efficiency_corrections = mock.Mock()

        self.model.calculate_corrections(params)

        self.assertEqual(self.model.apply_calibration_corrections.call_count, len(spectrums))
        self.assertEqual(self.model.apply_efficiency_corrections.call_count, len(spectrums))
        self.assertEqual(self.model.apply_absorption_corrections.call_count, len(spectrums))
        for spectrum in spectrums:
            self.model.apply_calibration_corrections.assert_any_call(spectrum, "Calibration")
            self.model.apply_efficiency_corrections.assert_any_call(spectrum, "Efficiency", "mock_workspace")
            self.model.apply_absorption_corrections.assert_any_call(spectrum, "Absorption", "mock_workspace")
        self.model.join_workspaces.assert_called_once_with(spectrums, output_name)
        self.model.handle_correction_successful.assert_called_once_with(group_name, output_name)

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.ScaleX")
    def test_apply_calibration_corrections(self, mock_scale):
        gradient = 3
        shift = 50
        params = {"gradient": gradient, "shift": shift}
        workspace_name = "mock_workspace"

        self.model.apply_calibration_corrections(workspace_name, params)

        mock_scale.assert_any_call(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Factor=gradient,
                                   Operation="Multiply")
        mock_scale.assert_any_call(InputWorkspace=workspace_name, OutputWorkspace=workspace_name, Factor=shift,
                                   Operation="Add")
        # ScaleX algorithm is called twice first to apply gradient and then to apply shift
        self.assertEqual(mock_scale.call_count, 2)

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.remove_ws_if_present")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.EvaluateFunction")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.ExtractSingleSpectrum")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.Divide")
    def test_apply_efficiency_corrections_with_default(self, mock_divide, mock_extract, mock_evaluate,
                                                       mock_retrieve_ws):
        mock_group = mock.Mock()
        mock_group.detector = "Detector 2"
        self.context.group_context.__getitem__ = lambda x, y: mock_group
        params = {"use default efficiencies": True}
        workspace_name = "mock_workspace"
        self.model.load_efficency_datafile = mock.Mock()
        function = DEFAULT_DETECTOR_EFFICIENCY_FUNCTION["Detector 2"]

        self.model.apply_efficiency_corrections(workspace_name, params, "mock_workspace")

        mock_evaluate.assert_called_once_with(Function="name=UserFunction,Formula=" + function,
                                              InputWorkspace=workspace_name,
                                              OutputWorkspace=EFFICIENCY_WORKSPACE_NAME)

        mock_extract.assert_called_once_with(InputWorkspace=EFFICIENCY_WORKSPACE_NAME,
                                             OutputWorkspace=EFFICIENCY_WORKSPACE_NAME, WorkspaceIndex=1)

        mock_divide.assert_called_once_with(LHSWorkspace=workspace_name, RHSWorkspace=EFFICIENCY_WORKSPACE_NAME,
                                            OutputWorkspace=workspace_name)

        mock_retrieve_ws.assert_called_once_with(EFFICIENCY_WORKSPACE_NAME)
        self.model.load_efficency_datafile.assert_not_called()

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.remove_ws_if_present")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.Divide")
    def test_apply_efficiency_corrections_with_filepath(self, mock_divide, mock_retrieve_ws):
        mock_group = mock.Mock()
        mock_group.detector = "Detector 2"
        self.context.group_context.__getitem__ = lambda x, y: mock_group
        filepath = "mock_path"
        params = {"use default efficiencies": False, "detector filepath": filepath}
        workspace_name = "mock_workspace"
        self.model.load_efficency_datafile = mock.Mock()

        self.model.apply_efficiency_corrections(workspace_name, params, "mock_workspace")

        mock_divide.assert_called_once_with(LHSWorkspace=workspace_name, RHSWorkspace=EFFICIENCY_WORKSPACE_NAME,
                                            OutputWorkspace=workspace_name)

        mock_retrieve_ws.assert_called_once_with(EFFICIENCY_WORKSPACE_NAME)
        self.model.load_efficency_datafile.assert_called_once_with(workspace_name, filepath)

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.XrayAbsorptionCorrection")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.Divide")
    def test_apply_absorption_corrections_using_defaults(self, mock_divide, mock_xray_absorption):
        workspace_name = "mock_workspace"
        group_name = "mock_group"
        muon_depth = 0.6
        muon_range = 0.15
        default_detector_angle = 135
        default_detector_distance = 10
        mock_group = mock.Mock()
        mock_group.detector = "Detector 1"
        self.context.group_context.__getitem__ = lambda x, y: mock_group
        params = {'Geometry': 'Disk', 'shape_parameters': "params",
                  'use_default_detector_settings': True, 'muon_profile_specifier': 'Muon depth',
                  'muon_depth': muon_depth, 'muon_range': muon_range,
                  'Absorption_coefficient_filepath': 'mock_path'}
        self.model.set_sample = mock.Mock()
        self.model.create_muon_implantation_workspace = mock.Mock()

        self.model.apply_absorption_corrections(workspace_name, params, group_name)

        mock_xray_absorption.assert_called_once_with(InputWorkspace=workspace_name,
                                                     MuonImplantationProfile=MUON_IMPLANTATION_WORKSPACE_NAME,
                                                     OutputWorkspace=XRAY_ABSORPTION_WORKSPACE,
                                                     DetectorAngle=default_detector_angle,
                                                     DetectorDistance=default_detector_distance)
        mock_divide.assert_called_once_with(LHSWorkspace=workspace_name, RHSWorkspace=XRAY_ABSORPTION_WORKSPACE,
                                            OutputWorkspace=workspace_name)
        # muon_depth is given in mm and needs to be changed to cm so is divided by 10
        self.model.create_muon_implantation_workspace.assert_called_once_with(muon_depth / 10, muon_range / 10)

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.XrayAbsorptionCorrection")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.Divide")
    def test_apply_absorption_corrections_using_implantation_workspace(self, mock_divide, mock_xray_absorption):
        self.model.set_sample = mock.Mock()
        detector_distance = 20
        detector_angle = 45
        workspace_name = "mock_workspace"
        group_name = "mock_group"
        params = {'Geometry': 'Disk', 'shape_parameters': "params",
                  'use_default_detector_settings': False, 'detector_distance': detector_distance,
                  'detector_angle': detector_angle, 'muon_profile_specifier': 'Muon implantation workspace',
                  'muon_implantation_workspace': 'muon_profile',
                  'Absorption_coefficient_filepath': 'mock_path'}

        self.model.apply_absorption_corrections(workspace_name, params, group_name)

        self.model.set_sample.assert_called_once_with(workspace_name, params)
        mock_xray_absorption.assert_called_once_with(InputWorkspace=workspace_name,
                                                     MuonImplantationProfile="muon_profile",
                                                     OutputWorkspace=XRAY_ABSORPTION_WORKSPACE,
                                                     DetectorAngle=detector_angle,
                                                     DetectorDistance=detector_distance)
        mock_divide.assert_called_once_with(LHSWorkspace=workspace_name, RHSWorkspace=XRAY_ABSORPTION_WORKSPACE,
                                            OutputWorkspace=workspace_name)

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.ConjoinWorkspaces")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.RenameWorkspace")
    def test_join_workspaces(self, mock_rename, mock_conjoin):
        workspace_list = ["spectrum_1", "spectrum_2", "spectrum_3"]
        output_name = "mock_name"
        self.model.join_workspaces(workspace_list, output_name)

        self.assertEqual(mock_conjoin.call_count, 2)
        mock_conjoin.assert_any_call(InputWorkspace1=workspace_list[0], InputWorkspace2=workspace_list[1])
        mock_conjoin.assert_any_call(InputWorkspace1=workspace_list[0], InputWorkspace2=workspace_list[2])
        mock_rename.assert_called_once_with(InputWorkspace=workspace_list[0], OutputWorkspace=output_name)

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.ExtractSingleSpectrum")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.CropWorkspace")
    def test_split_and_crop_workspace(self, mock_crop, mock_extract):
        group_name = "mock_group"
        mock_get_item = mock.Mock()
        mock_group = mock.Mock()
        mock_group.get_counts_workspace_for_run.return_value = group_name
        mock_get_item.return_value = mock_group
        self.context.group_context.__getitem__ = mock_get_item

        params = {"group_name": group_name, "energy_start": 50, "energy_end": 1000}
        xmin = params["energy_start"]
        xmax = params["energy_end"]
        workspace_name = self.model.split_and_crop_workspace(params)

        correct_workspace_names = []
        # Algorithms are called 3 times as there are 3 spectrums in workspace
        number_of_spectrums = 3
        for i in range(number_of_spectrums):
            output_name = EXTRACTED_SPECTRUM_SUFFIX + str(i + 1)
            correct_workspace_names.append(output_name)
            mock_extract.assert_any_call(InputWorkspace=group_name, OutputWorkspace=output_name,
                                         WorkspaceIndex=i)
            mock_crop.assert_any_call(InputWorkspace=output_name, OutputWorkspace=output_name, XMin=xmin, XMax=xmax)
        self.assertEqual(mock_crop.call_count, number_of_spectrums)
        self.assertEqual(mock_extract.call_count, number_of_spectrums)
        self.assertEqual(workspace_name, correct_workspace_names)
        mock_group.get_counts_workspace_for_run.assert_called_once()
        mock_get_item.assert_called_once_with(group_name)

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.CreateWorkspace")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.np.loadtxt")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.retrieve_ws")
    def test_load_efficency_datafile(self, mock_retrieve, mock_loadtxt, mock_createworksace):
        mock_name = "mock_name"
        mock_filepath = "mock_filepath"
        loaded_data = np.array([range(0, 9, 2), [2, 4, 6, 8, 10]])
        loaded_data = loaded_data.transpose()
        mock_workspace = mock.Mock()
        xdata = list(range(10))
        ydata = [2, 3, 4, 5, 6, 7, 8, 9, 10, 10]
        mock_workspace.readX = lambda i: xdata
        mock_retrieve.return_value = mock_workspace
        mock_loadtxt.return_value = loaded_data
        self.model.load_efficency_datafile(mock_name, mock_filepath)
        mock_retrieve.assert_called_once_with(mock_name)
        mock_loadtxt.assert_called_once_with(mock_filepath)
        argument_dict = mock_createworksace.call_args_list[0][1]
        self.assertEqual(argument_dict["OutputWorkspace"], EFFICIENCY_WORKSPACE_NAME)
        self.assertEqual(argument_dict["OutputWorkspace"], EFFICIENCY_WORKSPACE_NAME)
        self.assertEqual(list(argument_dict["DataX"]), xdata)
        self.assertEqual(list(argument_dict["DataY"]), ydata)

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.CreateWorkspace")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.np.linspace")
    def test_create_muon_implantation_workspace(self, mock_linspace, mock_createworkspace):
        ydata = [0, 2, 4, 6, 8, 10]
        self.model.gaussian = mock.Mock(return_value=ydata)
        depth = 1
        muon_range = 0.1
        start = depth - 3 * muon_range
        stop = depth + 3 * muon_range
        mock_linspace.return_value = ydata
        self.model.create_muon_implantation_workspace(depth, muon_range)

        self.model.gaussian.assert_called_once_with(ydata, depth, muon_range)
        mock_linspace.assert_called_once_with(start, stop, 100)
        mock_createworkspace.assert_called_once_with(OutputWorkspace=MUON_IMPLANTATION_WORKSPACE_NAME,
                                                     DataX=ydata, DataY=ydata)

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.SetSample")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.SetSampleMaterial")
    def test_set_sample(self, mock_set_material, mock_set_sample):
        workspace_name = "mock_workspace"
        shape_params = {"Shape": "Cylinder", "mock": "mock_params"}
        params = {"shape_parameters": shape_params, "Absorption_coefficient_filepath": "mock_filepath"}
        self.model.set_sample(workspace_name, params)

        mock_set_sample.assert_called_once_with(InputWorkspace=workspace_name, Geometry=shape_params)
        mock_set_material.assert_called_once_with(InputWorkspace=workspace_name, ChemicalFormula="Au",
                                                  XRayAttenuationProfile=params["Absorption_coefficient_filepath"])

    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.SetSample")
    @mock.patch("Muon.GUI.ElementalAnalysis2.correction_tab.ea_correction_tab_model.SetSampleMaterial")
    def test_set_sample_with_sphere(self, mock_set_material, mock_set_sample):
        workspace_name = "mock_workspace"
        radius = 100
        shape_params = {"Shape": "Sphere", "Radius": radius}
        params = {"shape_parameters": shape_params, "Absorption_coefficient_filepath": "mock_filepath"}
        shape_xml = SPHERE_XML_TEMPLATE.format(radius * cm_to_m)
        self.model.set_sample(workspace_name, params)

        mock_set_sample.assert_called_once_with(InputWorkspace=workspace_name,
                                                Geometry={'Shape': 'CSG', 'Value': shape_xml})
        mock_set_material.assert_called_once_with(InputWorkspace=workspace_name, ChemicalFormula="Au",
                                                  XRayAttenuationProfile=params["Absorption_coefficient_filepath"])


if __name__ == '__main__':
    unittest.main()
