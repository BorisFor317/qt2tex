#ifndef LATEX_H
#define LATEX_H

#include <memory>
#include <iostream>
#include <QFile>
#include <QString>
#include <QVector>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <utility>

struct LaTeXSymbols
{
    static inline QString newLine()
    { return "\\\\"; }

    static inline QString totalPages()
    { return "\\pageref{LastPage}"; }

    LaTeXSymbols() = delete;
};

class ITeXElement
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

class LaTeXParagraph final: public ITeXElement
{
public:
    QVector<QString> sentences;

    LaTeXParagraph() = default;

    LaTeXParagraph(std::initializer_list<QString> sentences)
        : sentences(sentences)
    {}

    std::unique_ptr<IReader> getReader() const override
    {
        return std::unique_ptr<Reader>(new Reader(this));
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
            QString result;
            if (atEnd()) {
                result = "";
            }
            else {
                result = _source->sentences[_position];
            }

            ++_position;
            return result;
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

class LaTeXLongTable: public ITeXElement
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
        return std::unique_ptr<Reader>(new Reader(this));
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

const QString DefaultLaTeXPreamble = "\\documentclass[a4paper, 10pt]{article}\n"
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

class BaseDocument
{
public:
    void render(QTextStream &out) const
    {
        out << getPreamble() << "\n\n";
        out << DocumentBegin << "\n";
        for (auto element = _elements.cbegin(); element != _elements.cend(); ++element) {
            auto elementReader = element->get()->getReader();
            while (!elementReader->atEnd()) {
                out << LineStart << elementReader->readLine() << "\n";
            }
            out << "\n";
        }
        out << DocumentEnd << "\n";
    }

protected:
    explicit BaseDocument(const QVector<std::shared_ptr<ITeXElement>> &elements)
        : _elements(elements)
    {}

    virtual QString getPreamble() const = 0;

private:
    QVector<std::shared_ptr<ITeXElement>> _elements;

    const QString LineStart = "    ";
    const QString DocumentBegin = "\\begin{document}";
    const QString DocumentEnd = "\\end{document}";
};

class LaTeXDocument final: public BaseDocument
{
public:
    explicit LaTeXDocument(const QVector<std::shared_ptr<ITeXElement>> &elements)
        : BaseDocument(elements), _preamble(DefaultLaTeXPreamble)
    {}

    LaTeXDocument(QString preamble, const QVector<std::shared_ptr<ITeXElement>> &elements)
        : BaseDocument(elements), _preamble(std::move(preamble))
    {}

protected:
    QString getPreamble() const override
    {
        return _preamble;
    }

private:
    QString _preamble;
};

class LuaDocument final: public BaseDocument
{
public:
    struct ColumnType
    {
        enum Alignment
        {
            Left,
            Center,
            Right
        };

        QChar name;
        Alignment alignment;
        // size in mm
        uint8_t size;
        // if is true size is ignored and will be determened by latex
        bool autoFit;

        ColumnType(const QChar &name, Alignment alignment, uint8_t size, bool autoFit)
            : name(name), alignment(alignment), size(size), autoFit(autoFit)
        {}

        QString asCommand() const
        {
            if (autoFit) {
                return QString("\\newcolumntype{%1}{>{\\%2\\arraybackslash}X}").arg(
                    name,
                    getAlignmentCommand());
            }
            else {
                return QString("\\newcolumntype{%1}{>{\\%2\\arraybackslash}p{%3mm}}").arg(
                    name,
                    getAlignmentCommand(),
                    QString::number(size));
            }
        }
    private:
        QString getAlignmentCommand() const
        {
            if (alignment == Left) {
                return "raggedleft";
            }
            else if (alignment == Center) {
                return "centering";
            }
            else if (alignment == Right) {
                return "raggedright";
            }

            return "centering";
        }
    };

    struct Options
    {
        // possible options are 8pt, 9pt, 10pt, 11pt, 12pt, 14pt, 17pt, and 20pt
        uint8_t fontSize = 9;
        // margin size in mm
        uint8_t margin = 15;
        // column separation in mm
        uint8_t columnSep = 2;
        QString mainFont = "Liberation Serif";
        QString sansFont = "Liberation Sans";
        QString monoFont = "Liberation Mono";

        QVector<ColumnType> columnsTypes = {
            ColumnType{'T', ColumnType::Center, 15, false},
            ColumnType{'S', ColumnType::Center, 4, false},
            ColumnType{'I', ColumnType::Center, 7, false},
            ColumnType{'L', ColumnType::Center, 11, false},
            ColumnType{'C', ColumnType::Center, 0, true},
        };

        Options(uint8_t fontSize,
                uint8_t margin,
                uint8_t columnSep,
                QString mainFont,
                QString sansFont,
                QString monoFont,
                const QVector<ColumnType> &columnsTypes)
            : fontSize(fontSize),
              margin(margin),
              columnSep(columnSep),
              mainFont(std::move(mainFont)),
              sansFont(std::move(sansFont)),
              monoFont(std::move(monoFont)),
              columnsTypes(columnsTypes)
        {}

        Options() = default;
    };

    LuaDocument(std::initializer_list<std::shared_ptr<ITeXElement>> elements)
        : BaseDocument(elements)
    {}

    LuaDocument(const QVector<std::shared_ptr<ITeXElement>> &elements, Options options)
        : BaseDocument(elements), options(std::move(options))
    {}

    Options options;

protected:
    QString getPreamble() const override
    {
        QStringList preamble
            {
                QString("\\documentclass[russian,openany,a4paper,%1pt,landscape]{extarticle}").arg(options.fontSize),
                "\\usepackage[russian]{babel}",
                QString("\\usepackage[a4paper,margin=%1mm]{geometry}").arg(options.margin),
                "\\pagewidth=297mm",
                "\\pageheight=210mm",
                "\\setlength{\\parindent}{0pt}",
                "\\usepackage{lastpage}",
                "\\usepackage{array}",
                "\\usepackage{xltabular}",
                "\\usepackage{fontspec}",
                QString("\\setlength{\\tabcolsep}{%1pt}").arg(options.columnSep),
                QString("\\setmainfont{%1}").arg(options.mainFont),
                QString("\\setsansfont{%1}").arg(options.sansFont),
                QString("\\setmonofont{%1}").arg(options.monoFont)
            };

        for (auto const &columnType: options.columnsTypes) {
            preamble.append(columnType.asCommand());
        }

        return preamble.join('\n');
    }
};

bool render_pdf(const QFileInfo &outputFile, const LaTeXDocument &document, QObject *parent = nullptr)
{
    const QString command = "pdflatex";

    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        return false;
    }

    QString tmpTexFilePath = tmp.filePath("main.tex");
    QFile tmpTexFile(tmpTexFilePath, parent);
    if (!tmpTexFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    QTextStream tmpTexStream(&tmpTexFile);
    document.render(tmpTexStream);
    tmpTexFile.close();

    // launch pdflatex 2 times (1st for calculate total size of document)
    auto outDirParam = QString("-output-directory=%1").arg(tmp.path());

    QProcess firstPass(parent);
    firstPass.start(
        command,
        {
            "-draftmode",
            outDirParam,
            tmpTexFilePath
        });

    if (!firstPass.waitForFinished()) {
        return false;
    }

    QProcess secondPass(parent);
    secondPass.start(
        command,
        {
            outDirParam,
            tmpTexFilePath
        });

    if (!secondPass.waitForFinished()) {
        return false;
    }

    if (outputFile.exists()) {
        QFile(outputFile.filePath()).remove();
    }

    return QFile::copy(
        tmp.filePath("main.pdf"),
        outputFile.filePath());
}

bool render_pdf(const QString &outputFilePath, const LaTeXDocument &document, QObject *parent = nullptr)
{
    return render_pdf(
        QFileInfo(outputFilePath),
        document,
        parent);
}

class FileRenderer
{
public:
    virtual bool render(const QFileInfo &output, const BaseDocument &document) = 0;

    bool render(const QString &outputPath, const BaseDocument &document)
    {
        return render(QFileInfo(outputPath), document);
    }
};

class TeXFileRenderer final: public FileRenderer
{
public:
    explicit TeXFileRenderer(QObject *parent = nullptr)
        : _parent(parent)
    {}

    using FileRenderer::render;

    bool render(const QFileInfo &output, const BaseDocument &document) override
    {
        QFile outputFile(output.filePath(), _parent);
        if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return false;
        }
        QTextStream texFileStream(&outputFile);
        document.render(texFileStream);
        texFileStream.flush();
        outputFile.close();

        return true;
    }

private:
    QObject *_parent = nullptr;
};

class PdfFileRenderer: public FileRenderer
{
public:
    struct CommandDescription
    {
        QString name;
        QStringList args;

        CommandDescription(const QString &name, const QStringList &args)
            : name(name), args(args)
        {}

        CommandDescription() = default;
    };

    PdfFileRenderer(QObject *parent, int timeoutMSecs, const QVector<CommandDescription> &commands)
        : _parent(parent), _timeoutMSecs(timeoutMSecs), _commands(commands)
    {}

    PdfFileRenderer(std::initializer_list<CommandDescription> commands)
        : _parent(nullptr), _timeoutMSecs(50000), _commands(commands)
    {}

    using FileRenderer::render;

    bool render(const QFileInfo &output, const BaseDocument &document) override final
    {
        QTemporaryDir tmp;
        QString tmpTexFile;
        if (!writeTmpTexFile(tmp, document, tmpTexFile)) {
            return false;
        }
        for (const auto &command: _commands) {
            if (!launchCommandOverTexFile(tmp.path(), tmpTexFile, command.name, command.args)) {
                return false;
            }
        }
        if (!removeExistingOutputFile(output)) {
            return false;
        }

        return QFile::rename(tmp.filePath(TmpPdfFilename), output.filePath());
    }

private:
    QObject *_parent;
    int _timeoutMSecs;
    QVector<CommandDescription> _commands;

    const QString TmpTeXFilename = "main.tex";
    const QString TmpPdfFilename = "main.pdf";

    bool writeTmpTexFile(const QTemporaryDir &tmp, const BaseDocument &document, QString &outputTexFile)
    {
        QString tmpTexFile = tmp.filePath(TmpTeXFilename);
        TeXFileRenderer texFileRenderer(_parent);
        outputTexFile = tmpTexFile;
        return texFileRenderer.render(tmpTexFile, document);
    }

    bool launchCommandOverTexFile(const QString &dir,
                                  const QString &texFile,
                                  const QString &commandName,
                                  const QStringList &commandArgs)
    {
        auto launchArguments = commandArgs;
        launchArguments.append(outputDirOption(dir));
        launchArguments.append(texFile);

        QProcess pdflatex(_parent);
        pdflatex.setProcessChannelMode(QProcess::MergedChannels);
        pdflatex.setProgram(commandName);
        pdflatex.setArguments(launchArguments);
        pdflatex.start();

        if (!pdflatex.waitForFinished(_timeoutMSecs)) {
            return false;
        }

        return pdflatex.exitCode() == 0;
    }

    static bool removeExistingOutputFile(const QFileInfo &outputFileInfo)
    {
        if (outputFileInfo.exists()) {
            return QFile(outputFileInfo.filePath()).remove();
        }

        return true;
    }

    static inline QString outputDirOption(const QString &dir)
    {
        return QString("-output-directory=%1").arg(dir);
    }
};

class PdfLaTeXFileRenderer final: public PdfFileRenderer
{
public:
    PdfLaTeXFileRenderer(QObject *parent, int timeoutMSecs)
        : PdfFileRenderer(
        parent,
        timeoutMSecs,
        {
            {"pdflatex", {"-halt-on-error", "-draftmode"}},
            {"pdflatex", {"-halt-on-error"}}
        })
    {}

    PdfLaTeXFileRenderer()
        : PdfFileRenderer(
        {
            {"pdflatex", {"-halt-on-error", "-draftmode"}},
            {"pdflatex", {"-halt-on-error"}}
        })
    {}
};

class LuaLaTeXFileRenderer final: public PdfFileRenderer
{
public:
    LuaLaTeXFileRenderer(QObject *parent, int timeoutMSecs)
        : PdfFileRenderer(
        parent,
        timeoutMSecs,
        {
            {"lualatex", {"--halt-on-error", "--draftmode"}},
            {"lualatex", {"--halt-on-error"}}
        })
    {}

    LuaLaTeXFileRenderer()
        : PdfFileRenderer(
        {
            {"lualatex", {"--halt-on-error", "--draftmode"}},
            {"lualatex", {"--halt-on-error"}}
        })
    {}
};

#endif //LATEX_H
