.. _ConvFit-ref:

Conv Fit
--------

ConvFit provides a simplified interface for controlling
various fitting functions (see the :ref:`Fit <algm-Fit>` algorithm for more
info). The functions are also available via the fit wizard.

Additionally, in the Settings below the fit function ConvFit features an additional options to output composite members
and to convolve members that, when using a composite function a spectra is produced for each of the fit functions used.

The fit types available in ConvFit are:

- One or two :ref:`Lorentzian <func-Lorentzian>`.
- :ref:`TeixeiraWater (SQE) <func-TeixeiraWaterSQE>`.
- :ref:`StretchedExpFT <func-StretchedExpFT>`.
- :ref:`DiffSphere <func-DiffSphere>`.
- :ref:`ElasticDiffSphere <func-ElasticDiffSphere>`.
- :ref:`InelasticDiffSphere <func-InelasticDiffSphere>`.
- :ref:`DiffRotDiscreteCircle <func-DiffRotDiscreteCircle>`.
- :ref:`ElasticDiffRotDiscreteCircle <func-ElasticDiffRotDiscreteCircle>`.
- :ref:`InelasticDiffRotDiscreteCircle <func-InelasticDiffRotDiscreteCircle>`.
- :ref:`IsoRotDiff <func-IsoRotDiff>`.
- :ref:`ElasticIsoRotDiff <func-ElasticIsoRotDiff>`.
- :ref:`InelasticIsoRotDiff <func-InelasticIsoRotDiff>`.

.. figure::  ../../images/QENS/ConvFitTab.png
   :height: 1000px

Conv Fit Options
~~~~~~~~~~~~~~~~

Sample
  Either a reduced file (*_red.nxs*) or workspace (*_red*) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*, *_sqw.dave*) or workspace (*_sqw*).

Resolution
  Either a resolution file (_res.nxs) or workspace (_res) or an :math:`S(Q,
  \omega)` file (*_sqw.nxs*, *_sqw.dave*) or workspace (*_sqw*).

.. _convfit-example-workflow:

ConvFit Example Workflow
~~~~~~~~~~~~~~~~~~~~~~~~
The Conv Fit tab allows ``_red`` and ``_sqw`` for its sample file, and allows ``_red``, ``_sqw`` and
``_res`` for the resolution file. The sample file used in this workflow can be produced using the run
number 26176 on the :doc:`Indirect Data Reduction <Indirect Data Reduction>` interface in the ISIS
Energy Transfer tab. The resolution file is created in the ISIS Calibration tab using the run number
26173. The instrument used to produce these files is IRIS, the analyser is graphite
and the reflection is 002.

1. Click **Browse** for the sample and select the file ``iris26176_graphite002_red``. Then click **Browse**
   for the resolution and select the file ``iris26173_graphite002_res``.

2. Choose the **Fit Type** to be One Lorentzian. Tick the **Delta Function** checkbox. Set the background
   to be a **Flat Background**.

3. Expand the variables called **f0-Lorentzian** and **f1-DeltaFunction**. To tie the delta functions Centre
   to the PeakCentre of the Lorentzian, right click on the Centre parameter and go to Tie->Custom Tie and then
   enter f0.PeakCentre.

4. Tick **Plot Guess** to get a prediction of what your fit will look like.

5. Click **Run** and wait for the interface to finish processing. This should generate a
   _Parameters table workspace and two group workspaces with end suffixes _Results and
   _Workspaces. The mini-plots should also update, with the upper plot displaying the
   calculated fit and the lower mini-plot displaying the difference between the input data and the
   fit.

6. Choose a default save directory and then click **Save Result** to save the _result workspaces
   found inside of the group workspace ending with _Results. The saved workspace will be used in
   the :ref:`fqfit-example-workflow`.

Theory
~~~~~~

For more on the theory of Conv Fit see the :ref:`ConvFitConcept` concept page.

ConvFit fitting model
~~~~~~~~~~~~~~~~~~~~~

The model used to perform fitting in ConvFit is described in the following tree, note that
everything under the Model section is optional and determined by the *Fit Type*
and *Use Delta Function* options in the interface.

- :ref:`CompositeFunction <func-CompositeFunction>`

  - :ref:`LinearBackground <func-LinearBackground>`

  - :ref:`Convolution <func-Convolution>`

    - Resolution

    - Model (:ref:`CompositeFunction <func-CompositeFunction>`)

      - DeltaFunction

      - :ref:`ProductFunction <func-ProductFunction>` (One Lorentzian)

        - :ref:`Lorentzian <func-Lorentzian>`

        - Temperature Correction

      - :ref:`ProductFunction <func-ProductFunction>` (Two Lorentzians)

        - :ref:`Lorentzian <func-Lorentzian>`

        - Temperature Correction

      - :ref:`ProductFunction <func-ProductFunction>` (InelasticDiffSphere)

        - :ref:`Inelastic Diff Sphere <func-DiffSphere>`

        - Temperature Correction

      - :ref:`ProductFunction <func-ProductFunction>` (InelasticDiffRotDiscreteCircle)

        - :ref:`Inelastic Diff Rot Discrete Circle <func-DiffRotDiscreteCircle>`

        - Temperature Correction

      - :ref:`ProductFunction <func-ProductFunction>` (ElasticDiffSphere)

        - :ref:`Elastic Diff Sphere <func-DiffSphere>`

        - Temperature Correction

      - :ref:`ProductFunction <func-ProductFunction>` (ElasticDiffRotDiscreteCircle)

        - :ref:`Elastic Diff Rot Discrete Circle <func-DiffRotDiscreteCircle>`

        - Temperature Correction

      - :ref:`ProductFunction <func-ProductFunction>` (StretchedExpFT)

        - :ref:`StretchedExpFT <func-StretchedExpFT>`

        - Temperature Correction

The Temperature Correction is a :ref:`UserFunction <func-UserFunction>` with the
formula :math:`((x * 11.606) / T) / (1 - exp(-((x * 11.606) / T)))` where
:math:`T` is the temperature in Kelvin.