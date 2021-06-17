.. _IqtFit-ref:

I(Q, t) Fit
-----------

I(Q, t) Fit provides a simplified interface for controlling various fitting
functions (see the :ref:`Fit <algm-Fit>` algorithm for more info). The functions
are also available via the fit wizard.

The fit types available for use in IqtFit are :ref:`Exponentials <func-ExpDecay>` and
:ref:`Stretched Exponential <func-StretchExp>`.

.. figure::  ../../images/QENS/IQtFitTab.png
   :height: 1000px

.. _iqtfit-example-workflow:

I(Q, t) Fit Example Workflow
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
The I(Q, t) Fit tab operates on ``_iqt`` files. The files used in this workflow are produced on the
I(Q, t) tab as seen in the :ref:`iqt-example-workflow`.

1. Click **Browse** and select the file ``irs26176_graphite002_iqt``.

2. Change the **EndX** variable to be around 0.2 in order to change the time range. Alternatively, drag
   the **EndX** blue line seen on the upper mini-plot using the cursor.

3. Choose the number of **Exponentials** to be 1. Select a **Flat Background**.

4. Change the **Fit Spectra** to go from 0 to 7. This will ensure that only the spectra within the input
   workspace with workspace indices between 0 and 7 are fitted.

5. Click **Run** and wait for the interface to finish processing. This should generate a
   _Parameters table workspace and two group workspaces with end suffixes _Results and
   _Workspaces. The mini-plots should also update, with the upper plot displaying the
   calculated fit and the lower mini-plot displaying the difference between the input data and the
   fit.

6. In the **Output** section, you can choose which parameter you want to plot.

7. Click **Fit Single Spectrum** to produce a fit result for the first spectrum.

8. In the **Output** section, click **Edit Result** and then select the _Result workspace containing
   multiple fits (1), and in the second combobox select the _Result workspace containing the single fit
   (2). Choose an output name and click **Replace Fit Result**. This will replace the corresponding fit result
   in (1) with the fit result found in (2). See the :ref:`IndirectReplaceFitResult <algm-IndirectReplaceFitResult>`
   algorithm for more details. Note that the output workspace is inserted into the group workspace in which
   (1) is found.