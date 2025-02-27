# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
import functools
import unittest

from mantid.dataobjects import PeaksWorkspace
from mantid.kernel import V3D
from unittest.mock import Mock
from mantidqt.utils.testing.mocks.mock_mantid import MockWorkspace
from mantidqt.utils.testing.strict_mock import StrictMock
from mantidqt.widgets.workspacedisplay.table.model import TableWorkspaceColumnTypeMapping, TableWorkspaceDisplayModel


def with_mock_workspace(func):
    # type: (callable) -> callable
    @functools.wraps(func)
    def wrapper(self):
        ws = MockWorkspace()
        model = TableWorkspaceDisplayModel(ws)
        return func(self, model)

    return wrapper


class TableWorkspaceDisplayModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Allow the MockWorkspace to work within the model
        TableWorkspaceDisplayModel.ALLOWED_WORKSPACE_TYPES.append(MockWorkspace)

    def test_get_name(self):
        ws = MockWorkspace()
        expected_name = "TEST_WORKSPACE"
        ws.name = Mock(return_value=expected_name)
        model = TableWorkspaceDisplayModel(ws)

        self.assertEqual(expected_name, model.get_name())

    def test_raises_with_unsupported_workspace(self):
        self.assertRaises(ValueError, lambda: TableWorkspaceDisplayModel([]))
        self.assertRaises(ValueError, lambda: TableWorkspaceDisplayModel(1))
        self.assertRaises(ValueError, lambda: TableWorkspaceDisplayModel("test_string"))

    @with_mock_workspace
    def test_get_v3d_from_str(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        self.assertEqual(V3D(1, 2, 3), model._get_v3d_from_str("1,2,3"))
        self.assertEqual(V3D(4, 5, 6), model._get_v3d_from_str("[4,5,6]"))

    @with_mock_workspace
    def test_set_cell_data_non_v3d(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        test_data = 4444

        expected_col = 1111
        expected_row = 1

        model.set_cell_data(expected_row, expected_col, test_data, False)

        # check that the correct conversion function was retrieved
        # -> the one for the column for which the data is being set
        model.ws.setCell.assert_called_once_with(expected_row, expected_col, test_data, notify_replace=False)

    @with_mock_workspace
    def test_set_cell_data_v3d(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        test_data = "[1,2,3]"

        expected_col = 1111
        expected_row = 1

        model.set_cell_data(expected_row, expected_col, test_data, True)

        # check that the correct conversion function was retrieved
        # -> the one for the column for which the data is being set
        model.ws.setCell.assert_called_once_with(expected_row, expected_col, V3D(1, 2, 3), notify_replace=False)

    @with_mock_workspace
    def test_set_cell_data_hkl(self, model):
        """
        :type model: TableWorkspaceDisplayModel
        """
        mock_peaksWorkspace = Mock(spec=PeaksWorkspace)
        mock_peaksWorkspace.getColumnNames.return_value = ['h', 'k', 'l']
        mock_peak = Mock()
        mock_peaksWorkspace.getPeak.return_value = mock_peak
        model.ws = mock_peaksWorkspace

        row = 1
        HKL = [4, 3, 2]
        for col in range(0, len(HKL)):
            model.set_cell_data(row, col, HKL[col], False)

        # check the correct index was set
        mock_peak.setH.assert_called_once_with(HKL[0])
        mock_peak.setK.assert_called_once_with(HKL[1])
        mock_peak.setL.assert_called_once_with(HKL[2])
        # check setCell not called
        model.ws.setCell.assert_not_called()

    def test_no_raise_with_supported_workspace(self):
        from mantid.simpleapi import CreateEmptyTableWorkspace
        ws = MockWorkspace()
        expected_name = "TEST_WORKSPACE"
        ws.name = Mock(return_value=expected_name)

        # no need to assert anything - if the constructor raises the test will fail
        TableWorkspaceDisplayModel(ws)

        ws = CreateEmptyTableWorkspace()
        TableWorkspaceDisplayModel(ws)

    def test_initialise_marked_columns_one_of_each_type(self):
        num_of_repeated_columns = 10
        ws = MockWorkspace()
        # multiply by 3 to have 1 of each type
        ws.columnCount = StrictMock(return_value=num_of_repeated_columns * 3)
        # make 10 columns of each type
        mock_column_types = [TableWorkspaceColumnTypeMapping.X, TableWorkspaceColumnTypeMapping.Y,
                             TableWorkspaceColumnTypeMapping.YERR] * num_of_repeated_columns
        ws.getPlotType = lambda i: mock_column_types[i]
        ws.getLinkedYCol = lambda i: i - 1
        model = TableWorkspaceDisplayModel(ws)

        self.assertEqual(num_of_repeated_columns, len(model.marked_columns.as_x))
        self.assertEqual(num_of_repeated_columns, len(model.marked_columns.as_y))
        self.assertEqual(num_of_repeated_columns, len(model.marked_columns.as_y_err))

        for i, col in enumerate(range(2, 30, 3)):
            self.assertEqual(col - 1, model.marked_columns.as_y_err[i].related_y_column)

    def test_initialise_marked_columns_multiple_y_before_yerr(self):
        ws = MockWorkspace()
        # add 5 columns as that is how many the default mock WS has
        mock_column_types = [TableWorkspaceColumnTypeMapping.X, TableWorkspaceColumnTypeMapping.Y,
                             TableWorkspaceColumnTypeMapping.YERR, TableWorkspaceColumnTypeMapping.Y,
                             TableWorkspaceColumnTypeMapping.Y, TableWorkspaceColumnTypeMapping.YERR,
                             TableWorkspaceColumnTypeMapping.YERR, TableWorkspaceColumnTypeMapping.X]
        ws.columnCount = StrictMock(return_value=len(mock_column_types))

        ws.getPlotType = lambda i: mock_column_types[i]
        ws.getLinkedYCol = StrictMock(side_effect=[1, 3, 4])
        model = TableWorkspaceDisplayModel(ws)

        self.assertEqual(2, len(model.marked_columns.as_x))
        self.assertEqual(3, len(model.marked_columns.as_y))
        self.assertEqual(3, len(model.marked_columns.as_y_err))

        self.assertEqual(1, model.marked_columns.as_y_err[0].related_y_column)
        self.assertEqual(3, model.marked_columns.as_y_err[1].related_y_column)
        self.assertEqual(4, model.marked_columns.as_y_err[2].related_y_column)

    def test_is_editable_column(self):
        ws = MockWorkspace()
        model = TableWorkspaceDisplayModel(ws)
        ws.isColumnReadOnly = StrictMock(return_value=False)
        self.assertTrue(model.is_editable_column(0))
        ws.isColumnReadOnly = StrictMock(return_value=True)
        self.assertFalse(model.is_editable_column(0))


if __name__ == '__main__':
    unittest.main()
