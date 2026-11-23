"""Build matching Chinese/English PDF manuals from the maintained Markdown sources.

Requires reportlab. Override SMALLTV_CJK_FONT and SMALLTV_CJK_BOLD_FONT on
non-Windows hosts with paths to suitable TrueType fonts.
"""
import html
import os
from pathlib import Path
import re

from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, Frame, Image, KeepTogether, PageBreak,
                               PageTemplate, Paragraph, Spacer, Table, TableStyle, XPreformatted)
from reportlab.platypus.tableofcontents import TableOfContents

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'docs/src/content/docs/manual'
OUTPUT = ROOT/'output/pdf'
ACCENT = colors.HexColor('#087e8b')
INK = colors.HexColor('#162a3b')
MUTED = colors.HexColor('#587080')


def register_fonts():
    regular = Path(os.environ.get('SMALLTV_CJK_FONT', 'C:/Windows/Fonts/msyh.ttc'))
    bold = Path(os.environ.get('SMALLTV_CJK_BOLD_FONT', 'C:/Windows/Fonts/msyhbd.ttc'))
    pdfmetrics.registerFont(TTFont('Manual', str(regular), subfontIndex=0))
    pdfmetrics.registerFont(TTFont('ManualBold', str(bold), subfontIndex=0))
    pdfmetrics.registerFontFamily('Manual', normal='Manual', bold='ManualBold', italic='Manual', boldItalic='ManualBold')


def inline(text):
    value = html.escape(text)
    value = re.sub(r'`([^`]+)`', r'<font color="#087e8b">\1</font>', value)
    value = re.sub(r'\*\*([^*]+)\*\*', r'<b>\1</b>', value)
    value = re.sub(r'\[([^\]]+)\]\(([^)]+)\)', r'\1', value)
    return value


class ManualDocument(BaseDocTemplate):
    def __init__(self, path, language, styles):
        self.language, self.manual_styles = language, styles
        super().__init__(str(path), pagesize=A4, leftMargin=44, rightMargin=44,
                         topMargin=52, bottomMargin=45, title='NMMiner SmallTV Pro User Manual - '+language,
                         author='NMMiner SmallTV Pro', pageCompression=1)
        frame = Frame(self.leftMargin, self.bottomMargin, self.width, self.height,
                      leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)
        self.addPageTemplates(PageTemplate(id='manual', frames=frame, onPage=self.decorate))

    def decorate(self, canvas, document):
        canvas.saveState()
        canvas.setFillColor(ACCENT)
        canvas.rect(0, A4[1]-9, A4[0], 9, fill=1, stroke=0)
        canvas.setFont('Manual', 8)
        canvas.setFillColor(MUTED)
        canvas.drawString(44, A4[1]-32, 'NMMiner SmallTV Pro / NM-TV-154')
        canvas.drawRightString(A4[0]-44, A4[1]-32, '使用手册' if self.language == 'zh' else 'User Manual')
        canvas.setStrokeColor(colors.HexColor('#dae4e9'))
        canvas.line(44, 33, A4[0]-44, 33)
        canvas.drawString(44, 20, '2026-11-23 | 3.13.1 | Rev. 3')
        canvas.drawRightString(A4[0]-44, 20, str(document.page))
        canvas.restoreState()

    def afterFlowable(self, flowable):
        if isinstance(flowable, Paragraph) and flowable.style.name == 'Chapter':
            title = flowable.getPlainText()
            chapter = re.match(r'\d+', title)
            if not chapter:
                return
            key = 'chapter-'+chapter.group()
            self.canv.bookmarkPage(key)
            self.canv.addOutlineEntry(title, key, level=0)
            self.notify('TOCEntry', (0, title, self.page, key))


def make_styles(language):
    common = dict(fontName='Manual', textColor=INK, wordWrap='CJK' if language == 'zh' else None)
    return {
        'body': ParagraphStyle('Body', fontSize=9.4, leading=15, spaceAfter=9, **common),
        'chapter': ParagraphStyle('Chapter', fontSize=19, leading=26, spaceAfter=17, fontName='ManualBold', textColor=ACCENT, keepWithNext=True),
        'sub': ParagraphStyle('Subheading', fontSize=11.5, leading=18, spaceBefore=10, spaceAfter=7, fontName='ManualBold', textColor=INK, keepWithNext=True),
        'cell': ParagraphStyle('Cell', fontSize=8.2, leading=12.8, spaceAfter=0, **common),
        'head': ParagraphStyle('TableHead', fontSize=8.2, leading=12.8, fontName='ManualBold', textColor=colors.white),
        'caption': ParagraphStyle('Caption', fontSize=8, leading=12, textColor=MUTED, fontName='Manual', spaceAfter=8),
        'code': ParagraphStyle('Code', fontSize=8.2, leading=13, fontName='Manual', textColor=INK,
                               backColor=colors.HexColor('#eff5f7'), borderPadding=10, spaceBefore=5, spaceAfter=12),
    }


def build(language):
    source = SOURCE/('user-manual-'+language+'.md')
    content = source.read_text(encoding='utf-8').split('---', 2)[2].strip()
    lines = content.splitlines()
    title = lines.pop(0).lstrip('# ')
    styles = make_styles(language)
    title_style = ParagraphStyle('CoverTitle', fontName='ManualBold', fontSize=29, leading=41, textColor=INK, spaceAfter=18)
    subtitle = ParagraphStyle('CoverSub', fontName='Manual', fontSize=11, leading=19, textColor=MUTED, spaceAfter=20)
    toc_style = ParagraphStyle('TOC', fontName='Manual', fontSize=10, leading=22, textColor=INK)
    story = [Spacer(1, 60), Paragraph(inline(title), title_style),
             Paragraph('中文使用手册' if language == 'zh' else 'English user manual', subtitle),
             Paragraph('连接网络 · 选择内容 · 个性设置 · 更新维护' if language == 'zh' else
                       'Connect · Choose · Personalize · Maintain', subtitle)]
    preview = ROOT/'docs/product/weather-clocks-preview.png'
    cover_image = Image(str(preview))
    cover_image._restrictSize(490, 220)
    story.extend([Spacer(1, 20), cover_image, Spacer(1, 24)])
    intro = []
    while lines and not lines[0].startswith('## '):
        intro.append(lines.pop(0))
    for paragraph in '\n'.join(intro).split('\n\n'):
        if paragraph.strip():
            story.append(Paragraph(inline(paragraph.strip()), styles['body']))
    story.extend([PageBreak(), Paragraph('目录' if language == 'zh' else 'Contents', styles['chapter'])])
    toc = TableOfContents()
    toc.levelStyles = [toc_style]
    story.extend([toc, PageBreak()])
    index = 0
    first = True
    while index < len(lines):
        line = lines[index].strip()
        if not line:
            index += 1
            continue
        if line.startswith('## '):
            if not first:
                story.append(PageBreak())
            first = False
            story.append(Paragraph(inline(line[3:]), styles['chapter']))
        elif line.startswith('### '):
            story.append(Paragraph(inline(line[4:]), styles['sub']))
        elif line.startswith('```'):
            block = []
            index += 1
            while index < len(lines) and not lines[index].startswith('```'):
                block.append(html.escape(lines[index]))
                index += 1
            story.append(XPreformatted('\n'.join(block), styles['code']))
        elif line.startswith('|'):
            rows = []
            while index < len(lines) and lines[index].strip().startswith('|'):
                values = [value.strip() for value in lines[index].strip().strip('|').split('|')]
                if not all(re.fullmatch(r':?-+:?', value) for value in values):
                    rows.append(values)
                index += 1
            count = len(rows[0])
            widths = [507/count]*count
            if count == 2:
                widths = [165, 342]
            table = Table([[Paragraph(inline(cell), styles['head'] if row == 0 else styles['cell'])
                            for cell in cells] for row, cells in enumerate(rows)], colWidths=widths, repeatRows=1,
                          hAlign='LEFT')
            table.setStyle(TableStyle([
                ('BACKGROUND', (0,0), (-1,0), ACCENT), ('VALIGN', (0,0), (-1,-1), 'TOP'),
                ('ROWBACKGROUNDS', (0,1), (-1,-1), [colors.white, colors.HexColor('#f0f5f7')]),
                ('LEFTPADDING', (0,0), (-1,-1), 8), ('RIGHTPADDING', (0,0), (-1,-1), 8),
                ('TOPPADDING', (0,0), (-1,-1), 7), ('BOTTOMPADDING', (0,0), (-1,-1), 7),
                ('LINEBELOW', (0,-1), (-1,-1), 0.5, colors.HexColor('#dae4e9'))]))
            story.extend([table, Spacer(1, 12)])
            continue
        elif line.startswith('!['):
            match = re.fullmatch(r'!\[([^]]*)\]\(([^)]+)\)', line)
            figure = Image(str((source.parent/match[2]).resolve()))
            figure._restrictSize(507, 145 if 'pictures-preview' in match[2] else 230)
            story.append(KeepTogether([figure, Spacer(1, 6), Paragraph(inline(match[1]), styles['caption'])]))
        else:
            paragraph = [line]
            while index+1 < len(lines) and lines[index+1].strip() and not re.match(r'^(#|\||!\[|```|\d+\. )', lines[index+1]):
                index += 1
                paragraph.append(lines[index].strip())
            story.append(Paragraph(inline(' '.join(paragraph)), styles['body']))
        index += 1
    OUTPUT.mkdir(parents=True, exist_ok=True)
    output = OUTPUT/('SmallTV-User-Manual-'+language.upper()+'.pdf')
    ManualDocument(output, language, styles).multiBuild(story)
    print(output)


if __name__ == '__main__':
    register_fonts()
    for language in ('zh', 'en'):
        build(language)
