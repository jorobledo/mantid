# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import mantid.simpleapi as mantid
from mantid.kernel import logger

import isis_powder.routines.common as common
from isis_powder.routines.run_details import create_run_details_object, get_cal_mapping_dict


def attenuate_workspace(attenuation_file_path, ws_to_correct):
    original_units = ws_to_correct.getAxis(0).getUnit().unitID()
    wc_attenuated = mantid.PearlMCAbsorption(attenuation_file_path)
    wc_attenuated = mantid.ConvertToHistogram(InputWorkspace=wc_attenuated, OutputWorkspace=wc_attenuated)
    ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                        Target=wc_attenuated.getAxis(0).getUnit().unitID())
    wc_attenuated = mantid.RebinToWorkspace(WorkspaceToRebin=wc_attenuated, WorkspaceToMatch=ws_to_correct,
                                            OutputWorkspace=wc_attenuated)
    pearl_attenuated_ws = mantid.Divide(LHSWorkspace=ws_to_correct, RHSWorkspace=wc_attenuated)
    common.remove_intermediate_workspace(workspaces=wc_attenuated)
    pearl_attenuated_ws = mantid.ConvertUnits(InputWorkspace=pearl_attenuated_ws, OutputWorkspace=pearl_attenuated_ws,
                                              Target=original_units)
    return pearl_attenuated_ws


def apply_vanadium_absorb_corrections(van_ws, run_details, absorb_ws=None):

    def generate_det_id_list(ws):
        det_id_list = []
        for i in range(0, ws.getNumberHistograms()):
            try:
                det_ids = ws.getSpectrum(i).getDetectorIDs()

            except RuntimeError:
                pass
            else:
                for det_id in det_ids:
                    det_id_list.append(det_id)
        return det_id_list

    if absorb_ws is None:
        absorb_ws = mantid.Load(Filename=run_details.vanadium_absorption_path)

    van_original_units = van_ws.getAxis(0).getUnit().unitID()
    absorb_units = absorb_ws.getAxis(0).getUnit().unitID()
    if van_original_units != absorb_units:
        van_ws = mantid.ConvertUnits(InputWorkspace=van_ws, Target=absorb_units, OutputWorkspace=van_ws)

    # PEARL sometimes do special runs with different detector cards so extract common spectra before doing
    # RebinToWorkspace to ensure histogram by histogram by rebin
    abs_det_id_list = generate_det_id_list(absorb_ws)
    van_det_id_list = generate_det_id_list(van_ws)

    common_det_ids = [det_id for det_id in abs_det_id_list if det_id in van_det_id_list]
    if not common_det_ids:
        raise RuntimeError("No common detectors in Vanadium and sample workspaces")

    MSG_STEM = "Vanadium workspace and absorption workspaces have different spectra. "
    if common_det_ids != van_det_id_list:
        logger.warning(MSG_STEM + "Removing unmatched spectra from the Vanadium workspace")
        van_ws = mantid.ExtractSpectra(InputWorkspace=van_ws, DetectorList=common_det_ids)
    if common_det_ids != abs_det_id_list:
        logger.warning(MSG_STEM + "Removing unmatched spectra from the absorption workspace")
        absorb_ws = mantid.ExtractSpectra(InputWorkspace=absorb_ws, DetectorList=common_det_ids)

    absorb_ws = mantid.RebinToWorkspace(WorkspaceToRebin=absorb_ws, WorkspaceToMatch=van_ws, OutputWorkspace=absorb_ws)
    van_ws = mantid.Divide(LHSWorkspace=van_ws, RHSWorkspace=absorb_ws, OutputWorkspace=van_ws, AllowDifferentNumberSpectra=True)

    if van_original_units != absorb_units:
        van_ws = mantid.ConvertUnits(InputWorkspace=van_ws, Target=van_original_units, OutputWorkspace=van_ws)

    common.remove_intermediate_workspace(absorb_ws)
    return van_ws


def generate_vanadium_absorb_corrections(van_ws, output_filename):
    shape_ws = mantid.CloneWorkspace(InputWorkspace=van_ws)
    shape_ws = mantid.ConvertUnits(InputWorkspace=shape_ws, OutputWorkspace=shape_ws, Target="Wavelength")
    mantid.CreateSampleShape(InputWorkspace=shape_ws, ShapeXML='<sphere id="sphere_1"> <centre x="0" y="0" z= "0" />\
                                                      <radius val="0.005" /> </sphere>')

    absorb_ws = \
        mantid.AbsorptionCorrection(InputWorkspace=shape_ws, AttenuationXSection="5.08",
                                    ScatteringXSection="5.1", SampleNumberDensity="0.072",
                                    NumberOfWavelengthPoints="25", ElementSize="0.05")
    mantid.SaveNexus(Filename=output_filename,
                     InputWorkspace=absorb_ws, Append=False)
    common.remove_intermediate_workspace(shape_ws)
    return absorb_ws


def _pearl_get_tt_grouping_file_name(inst_settings):
    tt_grouping_key = str(inst_settings.tt_mode).lower() + '_grouping'
    try:
        grouping_file_name = getattr(inst_settings, tt_grouping_key)
    except AttributeError:
        raise ValueError("The tt_mode: " + str(inst_settings.tt_mode).lower() + " is unknown")
    return grouping_file_name


def _get_run_numbers_for_key(current_mode_run_numbers, key):
    err_message = "this must be under the relevant Rietveld or PDF mode."
    return common.cal_map_dictionary_key_helper(current_mode_run_numbers, key=key,
                                                append_to_error_message=err_message)


def get_run_details(run_number_string, inst_settings, is_vanadium_run):
    all_run_numbers = get_cal_mapping_dict(run_number_string, inst_settings.cal_mapping_path)
    empty_runs = _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="empty_run_numbers")
    vanadium_runs = _get_run_numbers_for_key(current_mode_run_numbers=all_run_numbers, key="vanadium_run_numbers")

    grouping_file_name = _pearl_get_tt_grouping_file_name(inst_settings)

    spline_identifier = [inst_settings.tt_mode]
    if inst_settings.long_mode:
        spline_identifier.append("long")

    return create_run_details_object(run_number_string=run_number_string, inst_settings=inst_settings,
                                     is_vanadium_run=is_vanadium_run, splined_name_list=spline_identifier,
                                     grouping_file_name=grouping_file_name, empty_run_number=empty_runs,
                                     vanadium_string=vanadium_runs, van_abs_file_name=inst_settings.van_absorb_file)


def normalise_ws_current(ws_to_correct, monitor_ws, spline_coeff, lambda_values, integration_range, ex_regions):
    processed_monitor_ws = mantid.ConvertUnits(InputWorkspace=monitor_ws, Target="Wavelength")
    processed_monitor_ws = mantid.CropWorkspace(InputWorkspace=processed_monitor_ws,
                                                XMin=lambda_values[0], XMax=lambda_values[-1])

    for reg in range(0, 4):
        processed_monitor_ws = mantid.MaskBins(InputWorkspace=processed_monitor_ws, XMin=ex_regions[0, reg],
                                               XMax=ex_regions[1, reg])

    splined_monitor_ws = mantid.SplineBackground(InputWorkspace=processed_monitor_ws,
                                                 WorkspaceIndex=0, NCoeff=spline_coeff)

    normalised_ws = mantid.ConvertUnits(InputWorkspace=ws_to_correct, Target="Wavelength",
                                        OutputWorkspace=ws_to_correct)
    normalised_ws = mantid.NormaliseToMonitor(InputWorkspace=normalised_ws, MonitorWorkspace=splined_monitor_ws,
                                              IntegrationRangeMin=integration_range[0],
                                              IntegrationRangeMax=integration_range[-1],
                                              OutputWorkspace=normalised_ws)

    normalised_ws = mantid.ConvertUnits(InputWorkspace=normalised_ws, Target="TOF", OutputWorkspace=normalised_ws)

    common.remove_intermediate_workspace(processed_monitor_ws)
    common.remove_intermediate_workspace(splined_monitor_ws)

    return normalised_ws


def strip_bragg_peaks(ws_list_to_correct):
    # TODO move hardcoded spline values into adv. config
    if len(ws_list_to_correct) == 14:
        # Strip peaks on banks 1-12 (or 0-11 using 0 based index)
        for index, ws in enumerate(ws_list_to_correct[:12]):
            ws_list_to_correct[index] = mantid.StripPeaks(InputWorkspace=ws, OutputWorkspace=ws, FWHM=15, Tolerance=8)

        # Banks 13 / 14 have broad peaks which are missed so compensate for that and run twice as peaks are very broad
        for _ in range(2):
            ws_list_to_correct[12] = mantid.StripPeaks(InputWorkspace=ws_list_to_correct[12],
                                                       OutputWorkspace=ws_list_to_correct[12], FWHM=100, Tolerance=10)

            ws_list_to_correct[13] = mantid.StripPeaks(InputWorkspace=ws_list_to_correct[13],
                                                       OutputWorkspace=ws_list_to_correct[13], FWHM=60, Tolerance=10)
    else:
        for index, ws in enumerate(ws_list_to_correct):
            ws_list_to_correct[index] = mantid.StripPeaks(InputWorkspace=ws, OutputWorkspace=ws, FWHM=15, Tolerance=8)

    return ws_list_to_correct
