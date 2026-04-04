"""Tests for markdown_converter module."""
from __future__ import annotations


def test_basic_markdown_to_html():
    """Bold and italic produce correct HTML tags."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html("**bold** and *italic*")
    assert "<strong>bold</strong>" in result
    assert "<em>italic</em>" in result


def test_fenced_code_block():
    """Fenced code blocks produce <pre><code> tags."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    md = "```python\nprint('hello')\n```"
    result = markdown_to_html(md)
    assert "<code" in result
    assert "print(" in result


def test_table_extension():
    """Tables produce <table> tags."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    md = "| A | B |\n|---|---|\n| 1 | 2 |"
    result = markdown_to_html(md)
    assert "<table>" in result or "<table" in result


def test_contains_style_block():
    """Output includes a <style> block for theming."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html("hello")
    assert "<style>" in result


def test_dark_mode_vs_light_mode():
    """Dark and light modes produce different CSS."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    dark = markdown_to_html("hello", dark_mode=True)
    light = markdown_to_html("hello", dark_mode=False)
    assert "<style>" in dark
    assert "<style>" in light
    assert dark != light


def test_raw_html_escaped():
    """Raw HTML in input is NOT rendered as real HTML (security)."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html('<script>alert("xss")</script>')
    assert "<script>" not in result
    assert "&lt;script&gt;alert(\"xss\")&lt;/script&gt;" in result


def test_javascript_links_sanitized():
    """Unsafe javascript: links are stripped from the generated HTML."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html("[click](javascript:alert(1))")
    assert "javascript:alert(1)" not in result
    assert "<a" in result


def test_protocol_relative_links_stripped():
    """Protocol-relative URLs (//host/path) are treated as unsafe."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html("[evil](//evil.example/path)")
    assert "//evil.example" not in result


def test_safe_relative_links_preserved():
    """Relative paths and https links are preserved."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html("[ok](https://example.com)")
    assert 'href="https://example.com"' in result


def test_empty_input():
    """Empty string produces a style block with empty body."""
    from ai_chat_plugin.markdown_converter import markdown_to_html

    result = markdown_to_html("")
    assert "<style>" in result
