.. _MsdFit-ref:

MSD Fit
-------

Given either a saved NeXus file, or workspace generated using the :ref:`Elwin Tab <Elwin-ref>`, this tab fits
:math:`intensity` vs. :math:`Q` with one of three functions for each run specified to give the Mean Square
Displacement (MSD). It then plots the MSD as function of run number. This is done using the
:ref:`QENSFitSequential <algm-QENSFitSequential>` algorithm.

MSDFit searches for the log files named <runnumber>_sample.txt in your chosen
raw file directory (the name ‘sample’ is for OSIRIS). These log files will exist
if the correct temperature was loaded using SE-log-name in the Elwin tab. If they
exist the temperature is read and the MSD is plotted versus temperature; if they do
not exist the MSD is plotted versus run number (last 3 digits).

The fitted parameters for all runs are in _msd_Table and the <u2> in _msd. To
run the Sequential fit a workspace named <inst><first-run>_to_<last-run>_eq is
created, consisting of :math:`intensity` v. :math:`Q` for all runs. A contour or 3D plot of
this may be of interest.

A sequential fit is run by clicking the Run button at the bottom of the tab, a
single fit can be performed using the Fit Single Spectrum button underneath the
preview plot. A simultaneous fit may be performed in a very similar fashion by changeing the Fit Type to Simultaneous
and the clicking run.

The :ref:`Peters model <func-MsdPeters>` [1] reduces to a :ref:`Gaussian <func-MsdGauss>` at large
(towards infinity) beta. The :ref:`Yi Model <func-MsdYi>` [2] reduces to a :ref:`Gaussian <func-MsdGauss>` at sigma
equal to zero.

.. figure::  ../../images/QENS/MSDFitTab.png
   :height: 1000px

MSD Fit Example Workflow
~~~~~~~~~~~~~~~~~~~~~~~~
The MSD Fit tab operates on ``_eq`` files. The files used in this workflow are produced on the Elwin
tab as seen in the :ref:`elwin-example-workflow`.

1. Click **Browse** and select the file ``osi104371-104375_graphite002_red_elwin_eq``. Load this
   file and it will be automatically plotted in the upper mini-plot.

2. Change the **Plot Spectrum** spinbox seen underneath the mini-plots to change the spectrum displayed
   in the upper mini-plot.

3. Change the **EndX** variable to be around 0.8 in order to change the Q range over which the fit shall
   take place. Alternatively, drag the **EndX** blue line seen on the mini-plot using the cursor.

4. Choose the **Fit Type** to be Gaussian. The parameters for this function can be seen if you
   expand the row labelled **f0-MsdGauss**. Choose appropriate starting values for these parameters.
   As well as being able to change the value of the parameters, Two additional options are available.
   Clicking on the button with `...` will bring up more options to set constraints and ties on the parameters. The checkbox will toggle
   whether the parameter is local or global. You need to click on the parameter value to see these options.

5. Tick **Plot Guess** to get a prediction of what the fit will look like.

6. Click **Run** and wait for the interface to finish processing. This should generate a
   _Parameters table workspace and two group workspaces with end suffixes _Results and
   _Workspaces. The mini-plots should also update, with the upper plot displaying the
   calculated fit and the lower mini-plot displaying the difference between the input data and the
   fit.

7. Alternatively, you can click **Fit Single Spectrum** to perform a fit on just the currently displayed spectrum.
   Do not click this for the purposes of this demonstration.

8. In the **Output** section, select the **Msd** parameter and then click **Plot**. This plots the
   Msd parameter which can be found within the _Results group workspace.

References
~~~~~~~~~~

[1] Peters & Kneller, Journal of Chemical Physics, 139, 165102 (2013)
[2] Yi et al, J Phys Chem B 116, 5028 (2012)

.. _msdfit-example-workflow: