#ifndef LATEX_H
#define LATEX_H

#include <memory>
#include <iostream>
#include <QFile>
#include <QString>
#include <QVector>
#include <QTextStream>
#include <utility>

class ILaTeXElement
{
public:
    class IReader
    {
    public:
        virtual QString readLine() = 0;

        virtual bool atEnd() const = 0;

        virtual ~IReader() = default;
    };

    virtual std::unique_ptr<IReader> getReader() const = 0;
};

class LaTeXParagraph final: public ILaTeXElement
{
public:
    QVector<QString> sentences;

    LaTeXParagraph() = default;

    LaTeXParagraph(std::initializer_list<QString> sentences)
        : sentences(sentences)
    {}

    std::unique_ptr<IReader> getReader() const override
    {
        return std::make_unique<Reader>(this);
    }

private:
    class Reader final: public IReader
    {
    public:
        explicit Reader(const LaTeXParagraph *source)
            : _source(source)
        {}

        QString readLine() override
        {
            if (atEnd()) {
                return {};
            }
            return _source->sentences[_position++];
        }

        inline bool atEnd() const override
        {
            return _position == _source->sentences.count();
        }

        ~Reader() override = default;

    private:
        const LaTeXParagraph *_source;
        int _position = 0;
    };
};

class LaTeXLongTable: public ILaTeXElement
{
public:
    struct Column
    {
        Column() = default;

        Column(QString name, const QChar &type)
            : name(std::move(name)), type(type)
        {}

        QString name;
        QChar type;
    };

    struct Row
    {
        Row() = default;

        Row(std::initializer_list<QString> values)
            : values(values)
        {}

        QList<QString> values;
    };

    LaTeXLongTable(QString label, QVector<Column> columns)
        : _label(std::move(label)), _columns(std::move(columns))
    {}

    QVector<Row> rows;

    std::unique_ptr<IReader> getReader() const override
    {
        return std::make_unique<Reader>(this);
    }

private:
    QString _label;
    QVector<Column> _columns;

    class Reader: public IReader
    {
    public:
        explicit Reader(const LaTeXLongTable *parent)
            : _parent(parent)
        {}

        QString readLine() override
        {
            if (atEnd()) {
                return {};
            }

            QString result;
            if (_position == 0) {
                result = getTableBegin();
            }
            else if (_position == 1) {
                result = getTableLabel();
            }
            else if (_position == 2) {
                result = getTableHeader();
            }
            else if (allRowsReady()) {
                result = TableEnd;
            }
            else {
                result = getRow(getCurrentRowIndex());
            }

            ++_position;
            return result;
        }

        bool atEnd() const override
        {
            return _position == _parent->rows.count() + 4;
        }

    private:
        const LaTeXLongTable *_parent;
        int _position = 0;

        const QString TableBegin = "\\begin{xltabular}[l]{\\textwidth}{%1}";
        const QString TableLabel = "\\multicolumn{%1}{l}{\\hspace{-\\tabcolsep}%2} \\\\ \\hline";
        const QString TableEnd = "\\end{xltabular}";

        const QString RowStart = "    ";
        const QString RowEnd = " \\\\ \\hline";

        const QString ColumnSeparator = " & ";
        const QChar ColumnTypeSeparator = '|';

        inline QString getTableBegin() const
        {
            return TableBegin.arg(getCols());
        }

        QString getCols() const
        {
            QVector<Column> columns = _parent->_columns;
            auto cols = QString();
            cols.reserve(2 * columns.count() + 1);
            cols.append(ColumnTypeSeparator);
            for (auto c = columns.cbegin(); c != columns.cend(); ++c) {
                cols.append(c->type);
                cols.append(ColumnTypeSeparator);
            }

            return cols;
        }

        inline QString getTableLabel() const
        {
            return TableLabel.arg(
                    QString::number(_parent->_columns.count()),
                    _parent->_label)
                .prepend(RowStart);
        }

        QString getTableHeader() const
        {
            QVector<Column> columns = _parent->_columns;
            QStringList header;
            header.reserve(columns.count());
            for (auto c = columns.cbegin(); c != columns.cend(); ++c) {
                header.append(c->name);
            }

            return header.join(ColumnSeparator).prepend(RowStart).append(RowEnd);
        }

        QString getRow(int rowIndex) const
        {
            if (_parent->rows.count() < rowIndex || rowIndex < 0) {
                return {};
            }

            Row row = _parent->rows.at(rowIndex);
            int columnsCount = _parent->_columns.count();
            if (row.values.count() != columnsCount) {
                throw std::exception();
            }

            QStringList rowValues(row.values);
            return rowValues.join(ColumnSeparator).prepend(RowStart).append(RowEnd);
        }

        inline int getCurrentRowIndex() const
        {
            return _position - 3;
        }

        inline bool allRowsReady() const
        {
            return _position == _parent->rows.count() + 3;
        }
    };
};

const QString DefaultPreamble = "\\documentclass[a4paper, 10pt]{article}\n"
                                "\n"
                                "\\usepackage[utf8]{inputenc}\n"
                                "\\usepackage[T1,T2A]{fontenc}\n"
                                "\\usepackage[russian, english]{babel}\n"
                                "\\usepackage[landscape]{geometry}\n"
                                "\\geometry{\n"
                                "    a4paper,\n"
                                "    total={210mm,297mm},\n"
                                "    left=20mm,\n"
                                "    right=20mm,\n"
                                "    top=20mm,\n"
                                "    bottom=20mm\n"
                                "}\n"
                                "\\usepackage{indentfirst}\n"
                                "\\setlength{\\parindent}{0pt}\n"
                                "\\usepackage{lastpage}\n"
                                "\\usepackage{array}\n"
                                "\\usepackage{xltabular}\n"
                                "\\setlength{\\tabcolsep}{2pt}\n"
                                "\\newcolumntype{T}{>{\\centering\\arraybackslash}p{16.5mm}}\n"
                                "\\newcolumntype{S}{>{\\centering\\arraybackslash}p{5mm}}\n"
                                "\\newcolumntype{I}{>{\\centering\\arraybackslash}p{7.5mm}}\n"
                                "\\newcolumntype{L}{>{\\centering\\arraybackslash}p{11mm}}\n"
                                "\\newcolumntype{C}{>{\\centering\\arraybackslash}X}";

class LaTeXDocument
{
public:
    explicit LaTeXDocument(const QVector<std::shared_ptr<ILaTeXElement>> &elements)
        : _preamble(DefaultPreamble), _elements(elements)
    {}

    LaTeXDocument(QString preamble, QVector<std::shared_ptr<ILaTeXElement>> elements)
        : _preamble(std::move(preamble)), _elements(std::move(elements))
    {}

    void render(QTextStream &out) const
    {
        out << _preamble << "\n";
        out << DocumentBegin << "\n";
        for (auto element = _elements.cbegin(); element != _elements.cend(); ++element) {
            auto elementReader = element->get()->getReader();
            while (!elementReader->atEnd()) {
                out << LineStart << elementReader->readLine() << "\n";
            }
        }
        out << DocumentEnd << "\n";
    }

private:
    QString _preamble;
    QVector<std::shared_ptr<ILaTeXElement>> _elements;

    const QString LineStart = "    ";
    const QString DocumentBegin = "\\begin{document}";
    const QString DocumentEnd = "\\end{document}";
};

#endif //LATEX_H
