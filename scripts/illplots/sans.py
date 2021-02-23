# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from mantid import mtd, config, logger
from mantid import plots

import matplotlib.pyplot as plt

import os


def _getWorkspaces(names):
    if isinstance(names, str):
        return [mtd[names]]
    if isinstance(names, list):
        return [mtd[name] for name in names]
    else:
        raise ValueError('"' + str(names) + '"' + " is not a workspace name or "
                         "a list of workspace names.")


def plotIQ(wsNames, filename=None):
    try:
        wss = _getWorkspaces(wsNames)
    except Exception as ex:
        logger.error("Unable to plot I(Q), check your input : " + str(ex))
        return

    fig, ax = plt.subplots()
    ax.set_title("I (Q)")
    ax.set_xscale("log")
    ax.set_yscale("log")
    for ws in wss:
        plots.axesfunctions.errorbar(ax, ws, specNum=1, label=ws.getName())
    ax.legend()

    if filename is None:
        fig.show()
    else:
        exportPath = config.getString("defaultsave.directory")
        fullPath = os.path.join(exportPath, filename)
        fig.savefig(fullPath)


def plotIQxQy(wsNames, filename=None):
    try:
        wss = _getWorkspaces(wsNames)
    except Exception as ex:
        logger.error("Unable to plot I(Qx, Qy), check your input : " + str(ex))
        return

    fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
    ax.set_title("I (Qx, Qy)")
    for ws in wss:
        ax.pcolormesh(ws)

    if filename is None:
        fig.show()
    else:
        exportPath = config.getString("defaultsave.directory")
        fullPath = os.path.join(exportPath, filename)
        fig.savefig(fullPath)
