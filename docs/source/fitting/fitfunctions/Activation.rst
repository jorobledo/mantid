.. _func-Activation:

==========
Activation
==========

.. index:: Activation

Description
-----------

The activation fitting function can be written as:

.. math:: y = A_R e^{-\frac{b}{x}}

where:
- A_R - Attempt Rate
- b - Barrier energy
- x - is in Kelvin


When fitting to meV instead it can be described as:

.. math:: y = A_R e^{-\frac{E\times b}{1000 k_B x}}

where:
- E - activation energy
- k_B - Boltzmann Constant


When using this function the Unit must be set to either K or meV otherwise the fitting function will not be applied.

Examples
--------

An example of when this might be used is for examining the ion dynamics in flouride-containing polyatomic anion cathodes using muon spectroscopy [1].



.. attributes::

.. properties::

References
----------
[1] Johnston, B. I., Baker, P. J. & Cussen, S. A. (2021). Ion dynamics in flouride-containing polyatomic anion cathodes by muon spectroscopy. Journal of Physics:Materials, Vol. 4 No. 4, `https://iopscience.iop.org/article/10.1088/2515-7639/ac22ba <https://iopscience.iop.org/article/10.1088/2515-7639/ac22ba>`.

.. categories::

.. sourcelink::
