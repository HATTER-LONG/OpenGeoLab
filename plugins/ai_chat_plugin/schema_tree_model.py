"""QStandardItemModel representing the module → action hierarchy.

Populates from scene_tools cached schema. Custom data roles allow
retrieval of module/action names from model indexes.
"""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QStandardItem, QStandardItemModel


MODULE_NAME_ROLE = Qt.ItemDataRole.UserRole + 1
ACTION_NAME_ROLE = Qt.ItemDataRole.UserRole + 2


class SchemaTreeModel(QStandardItemModel):
    """Tree model: root items are modules, children are actions."""

    def __init__(self, parent=None) -> None:
        super().__init__(parent)
        self.setHorizontalHeaderLabels(["Command Schema"])

    def load_from_scene_tools(self) -> None:
        """Populate the tree from scene_tools cached schema.

        Clears existing items and rebuilds from current schema state.
        Safe to call multiple times (e.g., after invalidate_cache).
        """
        from ai_chat_plugin import scene_tools

        self.clear()
        self.setHorizontalHeaderLabels(["Command Schema"])

        modules = scene_tools.list_modules()

        if not modules:
            empty_item = QStandardItem("No modules available (standalone mode)")
            empty_item.setEnabled(False)
            empty_item.setSelectable(False)
            self.appendRow(empty_item)
            return

        for mod_info in modules:
            mod_name = mod_info["name"]
            mod_desc = mod_info.get("description", "")

            mod_item = QStandardItem(mod_name)
            mod_item.setData(mod_name, MODULE_NAME_ROLE)
            mod_item.setData(None, ACTION_NAME_ROLE)
            mod_item.setToolTip(mod_desc)
            mod_item.setEditable(False)

            # Fetch actions for this module.
            mod_detail = scene_tools.describe_module(mod_name)
            if mod_detail:
                for act_info in mod_detail.get("actions", []):
                    act_name = act_info["name"]
                    act_desc = act_info.get("description", "")

                    act_item = QStandardItem(act_name)
                    act_item.setData(mod_name, MODULE_NAME_ROLE)
                    act_item.setData(act_name, ACTION_NAME_ROLE)
                    act_item.setToolTip(act_desc)
                    act_item.setEditable(False)
                    mod_item.appendRow(act_item)

            self.appendRow(mod_item)

    @staticmethod
    def is_action_item(index) -> bool:
        """Return True if the index points to an action (not a module)."""
        return index.data(ACTION_NAME_ROLE) is not None

    @staticmethod
    def module_name(index) -> str | None:
        """Return the module name for the given index."""
        return index.data(MODULE_NAME_ROLE)

    @staticmethod
    def action_name(index) -> str | None:
        """Return the action name for the given index (None if module)."""
        return index.data(ACTION_NAME_ROLE)
