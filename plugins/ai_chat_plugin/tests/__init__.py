"""Test package setup for ai_chat_plugin."""
from __future__ import annotations

import sys
from pathlib import Path


PLUGIN_ROOT = Path(__file__).resolve().parents[2]
plugin_root_str = str(PLUGIN_ROOT)
if plugin_root_str not in sys.path:
    sys.path.insert(0, plugin_root_str)
