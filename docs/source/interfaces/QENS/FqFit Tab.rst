.. _FqFit-ref:

F(Q) Fit
--------

One of the models used to interpret diffusion is that of jump diffusion in which
it is assumed that an atom remains at a given site for a time :math:`\tau`; and
then moves rapidly, that is, in a time negligible compared to :math:`\tau`.

This interface can be used for a jump diffusion fit as well as fitting across
EISF. This is done by means of the
:ref:`QENSFitSequential <algm-QENSFitSequential>` algorithm.

The fit types available in F(Q)Fit are:

- :ref:`ChudleyElliot <func-ChudleyElliot>`.
- :ref:`HallRoss <func-Hall-Ross>`.
- :ref:`FickDiffusion <func-FickDiffusion>`.
- :ref:`TeixeiraWater <func-TeixeiraWater>`.
- :ref:`EISFDiffCylinder <func-EISFDiffCylinder>`.
- :ref:`EISFDiffSphere <func-EISFDiffSphere>`.
- :ref:`EISFDiffSphereAlkyl <func-EISFDiffSphereAlkyl>`.

.. figure::  ../../images/QENS/FQFitTab.png
   :height: 1000px

.. _fqfit-example-workflow:

F(Q) Fit Example Workflow
~~~~~~~~~~~~~~~~~~~~~~~~~
The F(Q) Fit tab operates on ``_result`` files which can be produced on the ConvFit tab.  The
sample file used in this workflow is produced on the Conv Fit tab as seen in the
:ref:`convfit-example-workflow`.

1. Click **Browse** and select the file ``irs26176_graphite002_conv_Delta1LFitF_s0_to_9_Result``.

2. Change the mini-plot data by choosing the type of **Fit Parameter** you want to display. For the
   purposes of this demonstration select **EISF**. The combobox immediately to the right can be used to
   choose which EISF you want to see in the mini-plot. In this example there is only one available.

3. Change the **Fit Parameter** back to **Width**.

4. Choose the **Fit Type** to be TeixeiraWater.

5. Click **Run** and wait for the interface to finish processing. This should generate a
   _Parameters table workspace and two group workspaces with end suffixes _Results and
   _Workspaces. The mini-plots should also update, with the upper plot displaying the
   calculated fit and the lower mini-plot displaying the difference between the input data and the
   fit.

6. In the **Output** section, you can choose which parameter you want to plot. In this case the plotting
   option is disabled as the output workspace ending in _Result only has one data point to plot.
**References**

1. Peters & Kneller, Journal of Chemical Physics, 139, 165102 (2013)
2. Yi et al, J Phys Chem B 116, 5028 (2012)