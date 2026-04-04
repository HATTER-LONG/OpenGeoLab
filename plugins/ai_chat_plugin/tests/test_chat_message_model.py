"""Tests for ChatMessageModel — QAbstractListModel for chat messages."""
from __future__ import annotations

import sys

import pytest

# PySide6 requires a QApplication to exist before using any Qt types.
_app = None


def _ensure_app():
    global _app
    if _app is None:
        from PySide6.QtWidgets import QApplication

        _app = QApplication.instance() or QApplication(sys.argv)
    return _app


@pytest.fixture(autouse=True)
def qt_app():
    """Ensure QApplication exists for all tests."""
    _ensure_app()


def _make_model():
    from ai_chat_plugin.chat_message_model import ChatMessageModel

    return ChatMessageModel()


class TestAppendMessage:
    def test_append_user_message(self):
        model = _make_model()
        row = model.appendMessage("user", "Hello!")
        assert row == 0
        assert model.rowCount() == 1

    def test_append_returns_incrementing_rows(self):
        model = _make_model()
        r0 = model.appendMessage("user", "msg1")
        r1 = model.appendMessage("assistant", "msg2")
        assert r0 == 0
        assert r1 == 1
        assert model.rowCount() == 2

    def test_data_roles(self):
        model = _make_model()
        model.appendMessage("user", "test content")
        idx = model.index(0, 0)

        role_names = model.roleNames()
        msg_type_role = None
        content_role = None
        for role_num, name in role_names.items():
            if name == b"msgType":
                msg_type_role = role_num
            elif name == b"content":
                content_role = role_num

        assert msg_type_role is not None
        assert content_role is not None
        assert model.data(idx, msg_type_role) == "user"
        assert model.data(idx, content_role) == "test content"

    def test_append_tool_message(self):
        model = _make_model()
        row = model.appendMessage(
            "tool",
            "",
            toolName="list_modules",
            toolCallId="tc_123",
            toolStatus="running",
        )
        assert row == 0
        assert model.rowCount() == 1

    def test_append_askuser_message(self):
        model = _make_model()
        row = model.appendMessage(
            "askUser",
            "Which option?",
            choices=["A", "B", "C"],
        )
        assert row == 0


class TestStreamingUpdate:
    def test_append_to_last_assistant(self):
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.appendToLastAssistant("Hello ")
        model.appendToLastAssistant("world")

        idx = model.index(0, 0)
        role_names = model.roleNames()
        content_role = [role for role, name in role_names.items() if name == b"content"][0]
        assert model.data(idx, content_role) == "Hello world"

    def test_finalize_assistant(self):
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.appendToLastAssistant("raw text")
        model.finalizeAssistant("msg_1", "<p>formatted</p>")

        idx = model.index(0, 0)
        role_names = model.roleNames()
        content_role = [role for role, name in role_names.items() if name == b"content"][0]
        is_html_role = [role for role, name in role_names.items() if name == b"isHtml"][0]
        assert model.data(idx, content_role) == "<p>formatted</p>"
        assert model.data(idx, is_html_role) is True

    def test_finalize_wrong_id_no_crash(self):
        """Finalizing a non-existent msgId should be a no-op."""
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.finalizeAssistant("msg_wrong", "<p>oops</p>")
        idx = model.index(0, 0)
        role_names = model.roleNames()
        content_role = [role for role, name in role_names.items() if name == b"content"][0]
        assert model.data(idx, content_role) == ""


class TestToolUpdate:
    def test_update_tool_status(self):
        model = _make_model()
        model.appendMessage(
            "tool",
            "",
            toolName="execute_action",
            toolCallId="tc_456",
            toolStatus="running",
        )
        model.updateToolStatus("tc_456", "success", '{"ok": true}')

        idx = model.index(0, 0)
        role_names = model.roleNames()
        status_role = [role for role, name in role_names.items() if name == b"toolStatus"][0]
        result_role = [role for role, name in role_names.items() if name == b"toolResult"][0]
        assert model.data(idx, status_role) == "success"
        assert model.data(idx, result_role) == '{"ok": true}'

    def test_update_unknown_tool_no_crash(self):
        model = _make_model()
        model.updateToolStatus("tc_unknown", "error", "not found")


class TestAskUserUpdate:
    def test_mark_answered(self):
        model = _make_model()
        model.appendMessage("askUser", "Pick one", choices=["A", "B"])
        model.markAskUserAnswered(0)

        idx = model.index(0, 0)
        role_names = model.roleNames()
        answered_role = [role for role, name in role_names.items() if name == b"answered"][0]
        assert model.data(idx, answered_role) is True


class TestClear:
    def test_clear_removes_all(self):
        model = _make_model()
        model.appendMessage("user", "a")
        model.appendMessage("assistant", "b")
        assert model.rowCount() == 2
        model.clear()
        assert model.rowCount() == 0


class TestReasoning:
    def test_set_reasoning_on_assistant(self):
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.setReasoning("msg_1", "I need to think about this carefully.")

        idx = model.index(0, 0)
        role_names = model.roleNames()
        reasoning_role = [
            role for role, name in role_names.items() if name == b"reasoning"
        ][0]
        assert model.data(idx, reasoning_role) == "I need to think about this carefully."

    def test_append_reasoning_accumulates(self):
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.appendReasoning("msg_1", "chunk1 ")
        model.appendReasoning("msg_1", "chunk2")

        idx = model.index(0, 0)
        role_names = model.roleNames()
        reasoning_role = [
            role for role, name in role_names.items() if name == b"reasoning"
        ][0]
        assert model.data(idx, reasoning_role) == "chunk1 chunk2"

    def test_set_reasoning_replaces_accumulated(self):
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.appendReasoning("msg_1", "partial ")
        model.setReasoning("msg_1", "final complete text")

        idx = model.index(0, 0)
        role_names = model.roleNames()
        reasoning_role = [
            role for role, name in role_names.items() if name == b"reasoning"
        ][0]
        assert model.data(idx, reasoning_role) == "final complete text"

    def test_set_reasoning_wrong_id_no_crash(self):
        """Setting reasoning for a non-existent msgId should be a no-op."""
        model = _make_model()
        model.appendMessage("assistant", "", msgId="msg_1")
        model.setReasoning("msg_wrong", "reasoning text")

        idx = model.index(0, 0)
        role_names = model.roleNames()
        reasoning_role = [
            role for role, name in role_names.items() if name == b"reasoning"
        ][0]
        assert model.data(idx, reasoning_role) == ""

    def test_default_reasoning_is_empty(self):
        model = _make_model()
        model.appendMessage("assistant", "hi")

        idx = model.index(0, 0)
        role_names = model.roleNames()
        reasoning_role = [
            role for role, name in role_names.items() if name == b"reasoning"
        ][0]
        assert model.data(idx, reasoning_role) == ""
