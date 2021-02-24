# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from ._common import getWorkspaces, exportFig

from mantid import logger
from mantid import plots

import matplotlib.pyplot as plt


def plotRQ(wsNames, filename=None):
    try:
        wss = getWorkspaces(wsNames)
    except Exception as ex:
        logger.error("Unable to plot R(Q), check your input : " + str(ex))
        return None, None

    fig, ax = plt.subplots()
    ax.set_title("R(Q)")
    ax.set_xscale("log")
    ax.set_yscale("log")
    for ws in wss:
        plots.axesfunctions.errorbar(ax, ws, specNum=1, label=ws.getName())
    ax.legend()

    exportFig(fig, filename)
    return fig, ax
