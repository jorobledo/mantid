// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/MultipleScatteringCorrection.h"
#include "MantidDataHandling/SetSample.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/UnitFactory.h"

using Mantid::Algorithms::MultipleScatteringCorrection;
using Mantid::API::AnalysisDataService;
using Mantid::Kernel::Logger;

namespace {
/// static logger
Logger g_log("MultipleScatteringCorrectionTest");
} // namespace

class MultipleScatteringCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MultipleScatteringCorrectionTest *createSuite() { return new MultipleScatteringCorrectionTest(); }
  static void destroySuite(MultipleScatteringCorrectionTest *suite) { delete suite; }

  void test_single() {
    // Create a workspace with vanadium data
    const std::string ws_name = "ws_vanadium";
    MakeSampleWorkspaceVanadium(ws_name);

    // to wavelength
    auto unitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    unitsAlg->initialize();
    unitsAlg->setPropertyValue("InputWorkspace", ws_name);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->setPropertyValue("OutputWorkspace", "ws_wavelength");
    unitsAlg->execute();

    // correct using multiple scattering correction
    // NOTE:
    // using smaller element size will dramatically increase the computing time, and it might lead to a
    // memory allocation error from std::vector
    MultipleScatteringCorrection msAlg;
    msAlg.initialize();
    // Wavelength target
    msAlg.setPropertyValue("InputWorkspace", "ws_wavelength");
    msAlg.setPropertyValue("Method", "SampleOnly");
    msAlg.setPropertyValue("OutputWorkspace", "rst_ms");
    // msAlg.setProperty("ElementSize", 0.4); // mm
    msAlg.execute();
    TS_ASSERT(msAlg.isExecuted());

    // Get the results from multiple scattering correction
    Mantid::API::MatrixWorkspace_sptr rst_ms =
        AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("rst_ms_sampleOnly");

    // Given the current condition, we can only verify with some static values calculate using the current version
    // of multiple scattering correction.
    // This is mostly to make sure other changes that impacting multiple scattering correction can be caught early,
    // and the reference values here are by no means physically correct
    TS_ASSERT_DELTA(rst_ms->readY(0)[0], 0.184945, 1e-3);
    TS_ASSERT_DELTA(rst_ms->readY(0)[1], 0.182756, 1e-3);
    TS_ASSERT_DELTA(rst_ms->readY(1)[0], 0.184469, 1e-3);
    TS_ASSERT_DELTA(rst_ms->readY(1)[1], 0.182175, 1e-3);
  }

  void test_sampleAndContainer() {
    // Create a workspace with vanadium data
    const std::string ws_name = "mstest";
    MakeSampleWorkspaceWithContainer(ws_name);

    // to wavelength
    auto unitsAlg = Mantid::API::AlgorithmManager::Instance().create("ConvertUnits");
    unitsAlg->initialize();
    unitsAlg->setPropertyValue("InputWorkspace", ws_name);
    unitsAlg->setProperty("Target", "Wavelength");
    unitsAlg->setPropertyValue("OutputWorkspace", ws_name);
    unitsAlg->execute();

    // correct using multiple scattering correction
    // NOTE:
    // using smaller element size will dramatically increase the computing time, and it might lead to a
    // memory allocation error from std::vector
    MultipleScatteringCorrection msAlg;
    msAlg.initialize();
    // Wavelength target
    msAlg.setPropertyValue("InputWorkspace", ws_name);
    msAlg.setPropertyValue("Method", "SampleOnly");
    msAlg.setPropertyValue("OutputWorkspace", "rst_ms");
    msAlg.setProperty("ElementSize", 0.5); // mm
    msAlg.execute();
    TS_ASSERT(msAlg.isExecuted());
    Mantid::API::MatrixWorkspace_sptr rst_ms_sampleOnly =
        AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("rst_ms_sampleOnly");

    //
    msAlg.initialize();
    msAlg.setPropertyValue("InputWorkspace", ws_name);
    msAlg.setPropertyValue("Method", "SampleAndContainer");
    msAlg.setProperty("ElementSize", 0.5); // mm
    msAlg.setPropertyValue("OutputWorkspace", "rst_ms");
    msAlg.execute();
    TS_ASSERT(msAlg.isExecuted());
    Mantid::API::MatrixWorkspace_sptr rst_ms_containerOnly =
        AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("rst_ms_containerOnly");
    Mantid::API::MatrixWorkspace_sptr rst_ms_sampleAndContainer =
        AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>("rst_ms_sampleAndContainer");

    /*
    NOTE: regression test results as there is no analytical solution as reference
    --------------------------------------------------------------------------
    DetID	SpectrumID	Method	           A1	       A2	      A2/A1	    Delta
    ==========================================================================
    0	        0	    SampleOnly	      8.41E-08	1.55E-09	1.84E-02	0.0923619
    0	        0	    ContainerOnly	    1.51E-07	1.16E-08	7.64E-02	0.223564
    0	        0	    Sample&Container	9.75E-06	1.36E-05	1.40E+00	0.111176
    1	        0	    SampleOnly	      8.13E-08	1.48E-09	1.82E-02	0.0911247
    1	        0	    ContainerOnly	    1.51E-07	1.15E-08	7.64E-02	0.223515
    1	        0	    Sample&Container	9.53E-06	1.32E-05	1.38E+00	0.109849
    0	        1	    SampleOnly	      7.82E-08	1.39E-09	1.78E-02	0.0891449
    0	        1	    ContainerOnly	    1.50E-07	1.15E-08	7.62E-02	0.222937
    0	        1	    Sample&Container	9.19E-06	1.24E-05	1.35E+00	0.107302
    1	        1	    SampleOnly	      7.55E-08	1.32E-09	1.75E-02	0.0876116
    1	        1	    ContainerOnly	    1.50E-07	1.14E-08	7.62E-02	0.222875
    1	        1	    Sample&Container	8.99E-06	1.19E-05	1.33E+00	0.105738
    --------------------------------------------------------------------------
    - Delta refers to the final multiple scattering correction factor where Im = I_total * Delta
    */
    TS_ASSERT_DELTA(rst_ms_sampleOnly->readY(0)[0], 0.0923619, 1e-3);
    TS_ASSERT_DELTA(rst_ms_containerOnly->readY(0)[0], 0.223564, 1e-3);
    TS_ASSERT_DELTA(rst_ms_sampleAndContainer->readY(0)[0], 0.111176, 1e-3);
  }

private:
  /**
   * @brief generate a workspace and register in ADS with given name
   *
   * @param name
   */
  void MakeSampleWorkspace(std::string const &name) {
    // Create a fake workspace with TOF data
    auto sampleAlg = Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
    sampleAlg->initialize();
    sampleAlg->setProperty("Function", "Powder Diffraction");
    sampleAlg->setProperty("NumBanks", 2);
    sampleAlg->setProperty("BankPixelWidth", 1);
    sampleAlg->setProperty("XUnit", "TOF");
    sampleAlg->setProperty("XMin", 1000.0);
    sampleAlg->setProperty("XMax", 1500.0);
    sampleAlg->setPropertyValue("OutputWorkspace", name);
    sampleAlg->execute();

    // edit the instrument geometry
    auto editAlg = Mantid::API::AlgorithmManager::Instance().create("EditInstrumentGeometry");
    editAlg->initialize();
    editAlg->setPropertyValue("Workspace", name);
    editAlg->setProperty("PrimaryFlightPath", 5.0);
    editAlg->setProperty("SpectrumIDs", "1,2");
    editAlg->setProperty("L2", "2.0,2.0");
    editAlg->setProperty("Polar", "10.0,90.0");
    editAlg->setProperty("Azimuthal", "0.0,45.0");
    editAlg->setProperty("DetectorIDs", "1,2");
    editAlg->setProperty("InstrumentName", "Instrument");
    editAlg->execute();
  }

  /**
   * @brief make a sample workspace with V
   *
   * @param name
   */
  void MakeSampleWorkspaceVanadium(std::string const &name) {
    // make the workspace with given name
    MakeSampleWorkspace(name);

    // vanadium
    const std::string chemical_formula = "V";
    const double number_density = 0.07261;
    const double center_bottom_base_x = 0.0;
    const double center_bottom_base_y = -0.0284;
    const double center_bottom_base_z = 0.0;
    const double height = 2.95;  // cm
    const double radius = 0.568; // cm

    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    using FloatProperty = Mantid::Kernel::PropertyWithValue<double>;
    using FloatArrayProperty = Mantid::Kernel::ArrayProperty<double>;
    // material
    auto material = std::make_shared<Mantid::Kernel::PropertyManager>();
    material->declareProperty(std::make_unique<StringProperty>("ChemicalFormula", chemical_formula), "");
    material->declareProperty(std::make_unique<FloatProperty>("SampleNumberDensity", number_density), "");
    // geometry
    auto geometry = std::make_shared<Mantid::Kernel::PropertyManager>();
    geometry->declareProperty(std::make_unique<StringProperty>("Shape", "Cylinder"), "");
    geometry->declareProperty(std::make_unique<FloatProperty>("Height", height), "");
    geometry->declareProperty(std::make_unique<FloatProperty>("Radius", radius), "");
    std::vector<double> center{center_bottom_base_x, center_bottom_base_y, center_bottom_base_z};
    geometry->declareProperty(std::make_unique<FloatArrayProperty>("Center", std::move(center)), "");
    std::vector<double> cylinderAxis{0, 1, 0};
    geometry->declareProperty(std::make_unique<FloatArrayProperty>("Axis", cylinderAxis), "");
    // set sample
    Mantid::DataHandling::SetSample setsample;
    setsample.initialize();
    setsample.setPropertyValue("InputWorkspace", name);
    setsample.setProperty("Material", material);
    setsample.setProperty("Geometry", geometry);
    setsample.execute();
  }

  void MakeSampleWorkspaceWithContainer(std::string const &name) {
    // make the workspace with given name
    MakeSampleWorkspace(name);

    auto setSampleAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SetSample");
    setSampleAlg->setRethrows(true);
    setSampleAlg->initialize();
    setSampleAlg->setPropertyValue("InputWorkspace", name);
    setSampleAlg->setPropertyValue("Material",
                                   R"({"ChemicalFormula": "La-(B11)5.94-(B10)0.06", "SampleNumberDensity": 0.1})");
    setSampleAlg->setPropertyValue("Geometry",
                                   R"({"Shape": "Cylinder", "Height": 1.0, "Radius": 0.2, "Center": [0., 0., 0.]})");
    setSampleAlg->setPropertyValue("ContainerMaterial", R"({"ChemicalFormula":"V", "SampleNumberDensity": 0.0721})");
    setSampleAlg->setPropertyValue(
        "ContainerGeometry",
        R"({"Shape": "HollowCylinder", "Height": 1.0, "InnerRadius": 0.2, "OuterRadius": 0.3, "Center": [0., 0., 0.]})");
    TS_ASSERT_THROWS_NOTHING(setSampleAlg->execute());
  }
};
