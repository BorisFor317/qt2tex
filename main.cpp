#include <memory>
#include <iostream>
#include <QString>
#include <QVector>
#include <utility>

class ILaTeXElement {
public:
    class IReader {
    public:
        virtual QString readLine() = 0;
        virtual bool atEnd() const = 0;
        virtual ~IReader() = default;
    };

    virtual std::unique_ptr<IReader> getReader() const = 0;
};

class Paragraph final : public ILaTeXElement {
public:
    explicit Paragraph(const QVector <QString> &sentences) : _sentences(sentences) {}

    std::unique_ptr<IReader> getReader() const override {
        return std::make_unique<Reader>(this);
    }

private:
    class Reader final : public IReader {
    public:
        explicit Reader(const Paragraph *source) : _source(source) {}

        QString readLine() override {
            if (atEnd())
            {
                return {};
            }
            QString newLine = _source->_sentences[_position] + QString('\n');
            ++_position;
            return newLine;
        }

        inline bool atEnd() const override {
            return _position == _source->_sentences.count();
        }

        ~Reader() override = default;

    private:
        std::size_t _position = 0;
        const Paragraph* _source;
    };

    QVector<QString> _sentences;
};


//class LaTeXDocument {
//public:
//    void render() {
//        _out << _preamble << "\n";
//        _out << documentBegin() << "\n";
//        for (auto element = _elements.constBegin(); element != _elements.constEnd(); ++element) {
//            auto elementReader = element->getReader();
//            while (!elementReader->atEnd())
//            {
//                _out << _line_start << element->readLine() << "\n";
//            }
//        }
//        _out << documentEnd() << "\n";
//    }
//
//private:
//    QVector<ILaTeXElement> _elements;
//};

int main(int argc, char *argv[]) {
    Paragraph par({"Hello world.", "We are all good friends.", "Let's go to bad"});
    auto reader = par.getReader();
    while (!reader->atEnd()) {
        std::cout << reader->readLine().toStdString();
    }
    reader.release();

    return 0;
}