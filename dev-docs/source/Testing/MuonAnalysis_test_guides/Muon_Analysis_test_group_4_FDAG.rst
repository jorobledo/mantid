.. _Muon_Analysis_TestGuide_4_FDAG-ref:

Muon Unscripted Testing: Group 4 (Frequency Domain Analysis)
=============================================================

.. contents:: Table of Contents
    :local:

Introduction
^^^^^^^^^^^^

These are unscripted tests for the :program:`Frequency Domain Analysis` interface.
The master testing guide is located at :ref:`Muon_Analysis_TestGuide-ref`.

Tests
^^^^^

Setup
-----
- Open `Frequency Domain Analysis`
- Load `MUSR62260`
- Tick the **Autoscale y** option on the plotting window

Test 1: Basic FFT
-----------------
- Go to the **Transform** tab
- Set the workspace to "MUSR00062260; Group; bkwd; Asym; FD"
- Click the calculate FFT button and a plot will appear
- The plot window will show a broad peak
- In the **Fitting** tab it will contain 3 workspace ending in `Re` (real), `Im` (imaginary) and `mod` (modulus)
- Untick the Imaginary Data and the row beneath should disappear
- Click the Calculate FFT button

Test 2: Advanced FFT
--------------------
- The "Apodization Function" determines the amount of smoothing of the data
- Set the "Apodization Function" to `None` and press calculate
- The plot will show a large peak at 0 and then lots of noise
- Set the "Apodization Function" to `Gaussian` and press calculate
- There will be a clear peak near to 100 Gauss
- The "padding" adds zeros to the end of the time domain data set, to improve the sampling of the FFT
- Set the xrange for the plot to be from `50` to `150`
- Set the "padding" to zero and press calculate
- The plots should be a nice peak, but it will have lots of straight lines
- Set the "padding" to `50` and press calculate
- The plot will now be nice and smooth


Test 3: PhaseQuad
-----------------
- Go to the phase tab
- Click "calculate phase table"
- Click "calculate phasequad"
- When asked for a name enter `pq`
- Go to the transform tab
- Tick the **imaginary Workspace** option
- You select the real and imaginary parts of 'pq' to be the **Workspace** and **Imaginary Workspace** respectively
- Click calculate


Test 4: MaxEnt
--------------
- Change the drop-down menu at the top of the interface to "MaxEnt"
- The interface should look different
- Click the Calculate MaxEnt button
- The calculate button will be disabled and cancel enabled
- Click the cancel button
- Click Calculate MaxEnt
- The plot should update to a mostly flat line with a peak
- Make sure everything in the top table is ticked
- Click calculate MaxEnt
- In the plotting window change the plot to ``Maxent Dual Plot``
- You will now see 5 plots (1 frequency and 4 time domain)
- In the ADS expand the `MUSR62260 MaxEnt FD` group
- It will contain several workspaces
- The phase convergence will show a plot that tends to a single y value as x gets larger (just check a spectrum or two)
- Deadtimes and phase table will be table of spectrum number then some numbers
- The reconstructed spectra will look like the original data
