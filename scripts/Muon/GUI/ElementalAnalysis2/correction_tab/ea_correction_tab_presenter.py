# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantidqt.utils.observer_pattern import GenericObserver
from qtpy.QtWidgets import QFileDialog


class EACorrectionTabPresenter:

    def __init__(self, view, model, context):
        self.view = view
        self.model = model
        self.context = context
        self.update_view_observer = GenericObserver(self.update_view)
        self.setup_buttons()

    def setup_buttons(self):
        self.view.select_effieciency_file_slot(self.handle_select_efficiency_data_file_button_clicked)
        self.view.select_absorption_coefficient_file_slot(self.handle_select_absorption_data_file_button_clicked)

    def update_view(self):
        group_names = self.context.group_context.group_names
        workspaces_to_add = {}
        for group in group_names:
            run, detector = [x.strip() for x in group.split(";")]
            if run not in workspaces_to_add:
                workspaces_to_add[run] = []

            workspaces_to_add[run].append(detector)
        self.view.add_workspace_to_view(workspaces_to_add)

    def handle_select_efficiency_data_file_button_clicked(self):
        filename = QFileDialog.getOpenFileName()
        if isinstance(filename, tuple):
            filename = filename[0]
        filename = str(filename)
        if filename:
            self.view.set_efficiency_data_file_label_text(filename)

    def handle_select_absorption_data_file_button_clicked(self):
        filename = QFileDialog.getOpenFileName()
        if isinstance(filename, tuple):
            filename = filename[0]
        filename = str(filename)
        if filename:
            self.view.set_absorption_coefficient_data_file_label_text(filename)
