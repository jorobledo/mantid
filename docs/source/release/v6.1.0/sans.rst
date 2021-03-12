============
SANS Changes
============

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Algorithms and instruments
--------------------------

New
###

- Sectors drawn in the instrument viewer can now be used to define wedges in :ref:`Q1DWeighted <algm-Q1DWeighted>`


Improvements
############

- :ref:`SANSILLReduction <algm-SANSILLReduction>` has a new property `SolventInputWorkspace`, to provide
  reduced solvent data to be subtracted from the sample data.
- :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` has a new property `SolventFiles`, to communicate
  with :ref:`SANSILLReduction <algm-SANSILLReduction>` the file names of the reduced solvent data.
- :ref:`SANSILLAutoProcess <algm-SANSILLAutoProcess>` has new property:
  `StitchReferenceIndex` to denote the index of ws that should be a reference
  for scaling during stitching
- With SANSILLAutoProcess, the detector distance, the collimation position and the wavelength are appended
  to the names of the output workspaces (values are taken from the sample logs).
- :ref:`SANSILLReduction <algm-SANSILLReduction>` adds sample log information to reduced data about facility,
  sample transmission numor, and all SampleRuns numors, with relevant algebra.
  
Bugfixes
########

- Fix a bug that made it impossible to process flux in SANSILLAutoprocess.

:ref:`Release 6.1.0 <v6.1.0>`
>>>>>>> master
