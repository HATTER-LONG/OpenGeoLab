"""Convert Markdown text to themed HTML for QML RichText display."""
from __future__ import annotations

from html import escape
from html.parser import HTMLParser
from urllib.parse import urlsplit

import markdown as _md


def markdown_to_html(text: str, dark_mode: bool = True) -> str:
    """Convert Markdown to a themed HTML fragment."""
    markdown = _create_markdown_converter()
    html_body = markdown.convert(text)
    html_body = _sanitize_links(html_body)
    css = _build_css(dark_mode)
    return f"<style>{css}</style>{html_body}"


def _create_markdown_converter() -> _md.Markdown:
    """Create a Markdown converter with raw HTML disabled."""
    converter = _md.Markdown(extensions=["fenced_code", "tables"])
    converter.preprocessors.deregister("html_block")
    converter.inlinePatterns.deregister("html")
    converter.inlinePatterns.deregister("entity")
    return converter


def _build_css(dark_mode: bool) -> str:
    """Build a CSS block adapted to the current theme."""
    if dark_mode:
        text_color = "#e0e0e0"
        bg_code = "#2d2d2d"
        border_color = "#444444"
        link_color = "#58a6ff"
        table_alt_bg = "#2a2a2a"
    else:
        text_color = "#1a1a1a"
        bg_code = "#f5f5f5"
        border_color = "#d0d0d0"
        link_color = "#0969da"
        table_alt_bg = "#f0f0f0"

    return f"""
        body {{
            color: {text_color};
            font-family: -apple-system, 'Segoe UI', sans-serif;
            font-size: 13px;
            line-height: 1.5;
        }}
        pre {{
            background: {bg_code};
            border: 1px solid {border_color};
            border-radius: 6px;
            padding: 8px 12px;
            overflow-x: auto;
            font-family: 'Cascadia Code', 'Consolas', monospace;
            font-size: 12px;
        }}
        code {{
            background: {bg_code};
            border-radius: 3px;
            padding: 1px 4px;
            font-family: 'Cascadia Code', 'Consolas', monospace;
            font-size: 12px;
        }}
        pre code {{
            background: transparent;
            padding: 0;
        }}
        a {{
            color: {link_color};
            text-decoration: none;
        }}
        table {{
            border-collapse: collapse;
            width: 100%;
            margin: 8px 0;
        }}
        th, td {{
            border: 1px solid {border_color};
            padding: 4px 8px;
            text-align: left;
        }}
        tr:nth-child(even) {{
            background: {table_alt_bg};
        }}
    """


def _sanitize_links(html_body: str) -> str:
    """Remove unsafe href values while preserving Markdown-generated markup."""
    sanitizer = _HtmlLinkSanitizer()
    sanitizer.feed(html_body)
    sanitizer.close()
    return sanitizer.html


class _HtmlLinkSanitizer(HTMLParser):
    """HTML fragment sanitizer that strips unsafe anchor targets."""

    _ALLOWED_SCHEMES = {"", "http", "https", "mailto"}

    def __init__(self) -> None:
        super().__init__(convert_charrefs=False)
        self._parts: list[str] = []

    @property
    def html(self) -> str:
        """Return the rebuilt HTML fragment."""
        return "".join(self._parts)

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        self._parts.append(self._render_tag(tag, attrs, closed=False))

    def handle_startendtag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        self._parts.append(self._render_tag(tag, attrs, closed=True))

    def handle_endtag(self, tag: str) -> None:
        self._parts.append(f"</{tag}>")

    def handle_data(self, data: str) -> None:
        self._parts.append(escape(data, quote=False))

    def handle_entityref(self, name: str) -> None:
        self._parts.append(f"&{name};")

    def handle_charref(self, name: str) -> None:
        self._parts.append(f"&#{name};")

    def _render_tag(
        self,
        tag: str,
        attrs: list[tuple[str, str | None]],
        *,
        closed: bool,
    ) -> str:
        sanitized_attrs: list[tuple[str, str | None]] = []
        for name, value in attrs:
            if tag == "a" and name == "href" and value is not None:
                if not _is_safe_href(value):
                    continue
            sanitized_attrs.append((name, value))

        attr_text = "".join(
            f' {name}' if value is None else f' {name}="{escape(value, quote=True)}"'
            for name, value in sanitized_attrs
        )
        suffix = " />" if closed else ">"
        return f"<{tag}{attr_text}{suffix}"


def _is_safe_href(value: str) -> bool:
    """Allow only safe link schemes.

    Protocol-relative URLs (``//host/path``) are rejected because
    ``urlsplit`` returns an empty scheme with a non-empty netloc.
    """
    normalized = "".join(ch for ch in value.strip() if ch >= " ")
    parts = urlsplit(normalized)
    scheme = parts.scheme.lower()
    if scheme == "" and parts.netloc:
        return False
    return scheme in _HtmlLinkSanitizer._ALLOWED_SCHEMES
