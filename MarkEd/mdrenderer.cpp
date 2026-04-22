#include "mdrenderer.h"
#include <md4c.h>
#include <QByteArray>

struct State {
    QByteArray out;
    int imgNesting = 0;
};

static void escape(QByteArray& out, const char* str, unsigned len)
{
    for (unsigned i = 0; i < len; i++) {
        switch (str[i]) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;";  break;
            case '>': out += "&gt;";  break;
            case '"': out += "&quot;"; break;
            default:  out += str[i];
        }
    }
}

static void appendAttr(QByteArray& out, const MD_ATTRIBUTE& attr)
{
    escape(out, attr.text, attr.size);
}

static int onEnterBlock(MD_BLOCKTYPE type, void* detail, void* ud)
{
    auto& s = *static_cast<State*>(ud);
    switch (type) {
        case MD_BLOCK_H: {
            unsigned lv = static_cast<MD_BLOCK_H_DETAIL*>(detail)->level;
            s.out += "<h" + QByteArray::number(lv) + ">";
            break;
        }
        case MD_BLOCK_P:     s.out += "<p>"; break;
        case MD_BLOCK_QUOTE: s.out += "<blockquote>\n"; break;
        case MD_BLOCK_UL:    s.out += "<ul>\n"; break;
        case MD_BLOCK_OL: {
            auto* d = static_cast<MD_BLOCK_OL_DETAIL*>(detail);
            s.out += (d->start == 1)
                ? "<ol>\n"
                : "<ol start=\"" + QByteArray::number(d->start) + "\">\n";
            break;
        }
        case MD_BLOCK_LI: {
            auto* d = static_cast<MD_BLOCK_LI_DETAIL*>(detail);
            if (d->is_task) {
                bool checked = (d->task_mark == 'x' || d->task_mark == 'X');
                s.out += "<li><input type=\"checkbox\" disabled";
                if (checked) s.out += " checked";
                s.out += "> ";
            } else {
                s.out += "<li>";
            }
            break;
        }
        case MD_BLOCK_HR:   s.out += "<hr>\n"; break;
        case MD_BLOCK_CODE: {
            auto* d = static_cast<MD_BLOCK_CODE_DETAIL*>(detail);
            s.out += "<pre><code";
            if (d->lang.size > 0) {
                s.out += " class=\"language-";
                appendAttr(s.out, d->lang);
                s.out += "\"";
            }
            s.out += ">";
            break;
        }
        case MD_BLOCK_TABLE: s.out += "<table>\n"; break;
        case MD_BLOCK_THEAD: s.out += "<thead>\n"; break;
        case MD_BLOCK_TBODY: s.out += "<tbody>\n"; break;
        case MD_BLOCK_TR:    s.out += "<tr>\n"; break;
        case MD_BLOCK_TH: {
            auto* d = static_cast<MD_BLOCK_TD_DETAIL*>(detail);
            s.out += "<th";
            if      (d->align == MD_ALIGN_LEFT)   s.out += " align=\"left\"";
            else if (d->align == MD_ALIGN_CENTER) s.out += " align=\"center\"";
            else if (d->align == MD_ALIGN_RIGHT)  s.out += " align=\"right\"";
            s.out += ">";
            break;
        }
        case MD_BLOCK_TD: {
            auto* d = static_cast<MD_BLOCK_TD_DETAIL*>(detail);
            s.out += "<td";
            if      (d->align == MD_ALIGN_LEFT)   s.out += " align=\"left\"";
            else if (d->align == MD_ALIGN_CENTER) s.out += " align=\"center\"";
            else if (d->align == MD_ALIGN_RIGHT)  s.out += " align=\"right\"";
            s.out += ">";
            break;
        }
        default: break;
    }
    return 0;
}

static int onLeaveBlock(MD_BLOCKTYPE type, void* detail, void* ud)
{
    auto& s = *static_cast<State*>(ud);
    switch (type) {
        case MD_BLOCK_H: {
            unsigned lv = static_cast<MD_BLOCK_H_DETAIL*>(detail)->level;
            s.out += "</h" + QByteArray::number(lv) + ">\n";
            break;
        }
        case MD_BLOCK_P:     s.out += "</p>\n";          break;
        case MD_BLOCK_QUOTE: s.out += "</blockquote>\n"; break;
        case MD_BLOCK_UL:    s.out += "</ul>\n";         break;
        case MD_BLOCK_OL:    s.out += "</ol>\n";         break;
        case MD_BLOCK_LI:    s.out += "</li>\n";         break;
        case MD_BLOCK_CODE:  s.out += "</code></pre>\n"; break;
        case MD_BLOCK_TABLE: s.out += "</table>\n";      break;
        case MD_BLOCK_THEAD: s.out += "</thead>\n";      break;
        case MD_BLOCK_TBODY: s.out += "</tbody>\n";      break;
        case MD_BLOCK_TR:    s.out += "</tr>\n";         break;
        case MD_BLOCK_TH:    s.out += "</th>\n";         break;
        case MD_BLOCK_TD:    s.out += "</td>\n";         break;
        default: break;
    }
    return 0;
}

static int onEnterSpan(MD_SPANTYPE type, void* detail, void* ud)
{
    auto& s = *static_cast<State*>(ud);
    switch (type) {
        case MD_SPAN_EM:     s.out += "<em>";     break;
        case MD_SPAN_STRONG: s.out += "<strong>"; break;
        case MD_SPAN_DEL:    s.out += "<del>";    break;
        case MD_SPAN_CODE:   s.out += "<code>";   break;
        case MD_SPAN_U:      s.out += "<u>";      break;
        case MD_SPAN_A: {
            auto* d = static_cast<MD_SPAN_A_DETAIL*>(detail);
            s.out += "<a href=\"";
            appendAttr(s.out, d->href);
            s.out += "\"";
            if (d->title.size > 0) {
                s.out += " title=\"";
                appendAttr(s.out, d->title);
                s.out += "\"";
            }
            s.out += ">";
            break;
        }
        case MD_SPAN_IMG: {
            auto* d = static_cast<MD_SPAN_IMG_DETAIL*>(detail);
            s.out += "<img src=\"";
            appendAttr(s.out, d->src);
            s.out += "\" alt=\"";
            s.imgNesting++;
            break;
        }
        default: break;
    }
    return 0;
}

static int onLeaveSpan(MD_SPANTYPE type, void* detail, void* ud)
{
    auto& s = *static_cast<State*>(ud);
    switch (type) {
        case MD_SPAN_EM:     s.out += "</em>";     break;
        case MD_SPAN_STRONG: s.out += "</strong>"; break;
        case MD_SPAN_DEL:    s.out += "</del>";    break;
        case MD_SPAN_CODE:   s.out += "</code>";   break;
        case MD_SPAN_U:      s.out += "</u>";      break;
        case MD_SPAN_A:      s.out += "</a>";      break;
        case MD_SPAN_IMG: {
            auto* d = static_cast<MD_SPAN_IMG_DETAIL*>(detail);
            s.imgNesting--;
            s.out += "\"";
            if (d->title.size > 0) {
                s.out += " title=\"";
                appendAttr(s.out, d->title);
                s.out += "\"";
            }
            s.out += ">";
            break;
        }
        default: break;
    }
    return 0;
}

static int onText(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* ud)
{
    auto& s = *static_cast<State*>(ud);
    switch (type) {
        case MD_TEXT_NULLCHAR:
            s.out += "\xef\xbf\xbd";
            break;
        case MD_TEXT_BR:
            s.out += s.imgNesting > 0 ? " " : "<br>\n";
            break;
        case MD_TEXT_SOFTBR:
            s.out += s.imgNesting > 0 ? " " : "\n";
            break;
        case MD_TEXT_HTML:
        case MD_TEXT_ENTITY:
            s.out.append(text, size);
            break;
        default:
            escape(s.out, text, size);
            break;
    }
    return 0;
}

QString renderMarkdown(const QString& input)
{
    QByteArray src = input.toUtf8();
    State state;

    MD_PARSER parser = {
        0,
        MD_DIALECT_GITHUB,
        onEnterBlock,
        onLeaveBlock,
        onEnterSpan,
        onLeaveSpan,
        onText,
        nullptr,
        nullptr
    };

    md_parse(src.constData(), MD_SIZE(src.size()), &parser, &state);
    return QString::fromUtf8(state.out);
}
