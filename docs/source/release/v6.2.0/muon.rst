============
MuSR Changes
============

.. contents:: Table of Contents
   :local:

Frequency Domain Analysis
-------------------------

New Features
############

- The Frequency Domain Analysis interface now allows you to perform a sequential fit using the Sequential Fitting tab.
- The Sequential Fitting tab allows you to choose the type of dataset you want to fit.

Muon Analysis
-------------

New Features
############

- The Model Fitting tab allows you to perform fits across the sample logs and fit parameters stored in your results table.
- Users can now copy sequential fitting parameters to all other runs. To do this, click the checkbox to turn this feature
  on and change a parmeter. The change should be copied to the other cells in the same column. Turn the feature off to make
  changes to single rows.

Improvements
############

- When running the Dynamic Kubo Toyabe fit function you should now be able to see the BinWidth to 3 decimal places.

BugFixes
############
- A bug has been fixed in the BinWidth for the Dynamic Kobu Toyabe Fitting Function which caused a crash and did not provide
  any information about why the value was invalid. Will now revert to last viable BinWidth used and explain why.
- The autoscale option when `All` is selected will now show the largest and smallest y value from the all of the plots.

Muon Analysis and Frequency Domain Analysis
-------------------------------------------

New Features
############

- It is now possible to Exclude a range from a fit range when doing a fit on the Fitting tab.
- Added a 'Covariance Matrix' button to the Fitting tab that can be used to open and inspect the normalised covariance parameters of a fit.
- Can now plot the raw count data in the GUI.

Improvements
############

- It is now possible to do a vertical resize of the plot in Muon Analysis and Frequency Domain Analysis.
- The plotting has been updated for better stability.
- The plotting now has autoscale active by default.
- Added a table to store phasequads in the phase tab, phasequads also no longer automatically delete themselves
  when new data is loaded
- Frequency domain analysis can now use groups in :ref:`MuonMaxent <algm-MuonMaxent>` calculations.
- The labels on the tabs in the GUIs will now show in full

BugFixes
########

- In frequency domain analysis the phasetables calculated from :ref:`MuonMaxent <algm-MuonMaxent>` can be used for
  :ref:`PhaseQuad <algm-PhaseQuad>` calculations on the phase tab.

ALC
---

New Features
############

- Added an external plot button to the ALC interface which will plot in workbench the current tab's plot
- Added a period info button to the ALC interface which displays a table of period information from the loaded runs
  (this is equivalent to the periods button in the Muon Analysis and Frequency Domain Analysis Interfaces)

Elemental Analysis
------------------

Improvements
############
- Updated :ref:`LoadElementalAnalysisData <algm-LoadElementalAnalysisData>` algorithm to include Poisson errors for the counts data.

Algorithms
----------

Improvements
############
- Updated :ref:`LoadMuonLog <algm-LoadMuonLog>` to read units for most log values.
- It is now possible to exclude a fit range when executing the :ref:`CalculateMuonAsymmetry <algm-CalculateMuonAsymmetry>` algorithm.

BugFixes
############
- Fixed bug in :ref:`FitGaussianPeaks <algm-FitGaussianPeaks>` algorithm in which a peak at the end of range would cause an error due to not enough data point being available to fit parameters.

:ref:`Release 6.2.0 <v6.2.0>`
