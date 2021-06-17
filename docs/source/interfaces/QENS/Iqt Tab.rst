.. _Iqt-ref:

I(Q, t)
-------

Given sample and resolution inputs, carries out a fit as per the theory detailed
in the :ref:`TransformToIqt <algm-TransformToIqt>` algorithm.

.. figure::  ../../images/QENS/IQtTab.png
   :height: 1000px

I(Q, t) Options
~~~~~~~~~~~~~~~

Sample
  Either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

Resolution
  Either a resolution file (_res.nxs) or workspace (_res) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*) or workspace (*_sqw*).

ELow, EHigh
  The rebinning range.

SampleBinning
  The number of neighbouring bins are summed.

Symmetric Energy Range
  Untick to allow an asymmetric energy range.

Spectrum
  Changes the spectrum displayed in the preview plot.

Plot Current Preview
  Plots the currently selected preview plot in a separate external window

Calculate Errors
  The calculation of errors using a Monte Carlo implementation can be skipped by unchecking
  this option.

Number Of Iterations
  The number of iterations to perform in the Monte Carlo routine for error calculation
  in I(Q,t).

Run
  Runs the processing configured on the current tab.

Plot Spectra
  If enabled, it will plot the selected workspace indices in the selected output workspace.

Plot Tiled
  Generates a tiled plot containing the selected workspace indices. This option is accessed via the down
  arrow on the **Plot Spectra** button.

Save Result
  Saves the result workspace in the default save directory.

.. _iqt-example-workflow:

I(Q, t) Example Workflow
~~~~~~~~~~~~~~~~~~~~~~~~
The I(Q, t) tab allows ``_red`` and ``_sqw`` for it's sample file, and allows ``_red``, ``_sqw`` and
``_res`` for the resolution file. The sample file used in this workflow can be produced using the run
number 26176 on the :doc:`Indirect Data Reduction <Indirect Data Reduction>` interface in the ISIS
Energy Transfer tab. The resolution file is created in the ISIS Calibration tab using the run number
26173. The instrument used to produce these files is IRIS, the analyser is graphite
and the reflection is 002.

1. Click **Browse** for the sample and select the file ``iris26176_graphite002_red``. Then click **Browse**
   for the resolution and select the file ``iris26173_graphite002_res``.

2. Change the **SampleBinning** variable to be 5. Changing this will calculate values for the **EWidth**,
   **SampleBins** and **ResolutionBins** variables automatically by using the
   :ref:`TransformToIqt <algm-TransformToIqt>` algorithm where the **BinReductionFactor** is given by the
   **SampleBinning** value. The **SampleBinning** value must be low enough for the **ResolutionBins** to be
   at least 5. A description of this option can be found in the :ref:`a-note-on-binning` section.

3. Untick **Calculate Errors** if you do not want to calculate the errors for the output workspace which
   ends with the suffix _iqt.

4. Click **Run** and wait for the interface to finish processing. This should generate a workspace ending
   with a suffix _iqt.

5. In the **Output** section, select some workspace indices (e.g.0-2,4,6) for a tiled plot and then click
   the down arrow on the **Plot Spectra** button before clicking **Plot Tiled**.

6. Choose a default save directory and then click **Save Result** to save the _iqt workspace.
   This workspace will be used in the :ref:`iqtfit-example-workflow`.

.. _a-note-on-binning:

A note on Binning
~~~~~~~~~~~~~~~~~

The bin width is determined from the energy range and the sample binning factor. The number of bins is automatically
calculated based on the **SampleBinning** specified. The width is determined from the width of the range divided
by the number of bins.

The following binning parameters cannot be modified by the user and are instead automatically calculated through
the :ref:`TransformToIqt <algm-TransformToIqt>` algorithm once a valid resolution file has been loaded. The calculated
binning parameters are displayed alongside the binning options:

EWidth
  The calculated bin width.

SampleBins
  The number of bins in the sample after rebinning.

ResolutionBins
  The number of bins in the resolution after rebinning. Typically this should be at
  least 5 and a warning will be shown if it is less.

:ref:`Inelastic Data Analysis <interface-inelastic-data-analysis>`

.. categories:: Interfaces Indirect