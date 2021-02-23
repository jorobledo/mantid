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
import math


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

    n = len(wss)
    if n == 1:
        fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
        ax.pcolormesh(wss[0])
        ax.set_title(wss[0].getName())
    else:
        nr = int(math.ceil(math.sqrt(n)))
        nc = int(n / nr)
        if n%nr != 0:
            nc += 1
        fig, axs = plt.subplots(nr, nc, subplot_kw={'projection':'mantid'})
        axs = axs.flatten()
        for i in range(len(axs)):
            if i < n:
                axs[i].pcolormesh(wss[i])
                axs[i].set_title(wss[i].getName())
            else:
                axs[i].set_visible(False)

    if filename is None:
        fig.show()
    else:
        exportPath = config.getString("defaultsave.directory")
        fullPath = os.path.join(exportPath, filename)
        fig.savefig(fullPath)
