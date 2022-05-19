#include <memory>
#include <iostream>
#include <QVector>
#include <QFile>
#include "latex.h"

int main(int argc, char *argv[])
{
    auto par = std::make_shared<LaTeXParagraph>();
    par->sentences.append({
                              "Hello world.",
                              "Let's go to bad.",
                              "Сложно, почему так сложно.",
                              QString("Total pages: %1").arg(LaTeXSymbols::totalPages())
                          });

    auto table = std::make_shared<LaTeXLongTable>(
        "Таблица №1337", QVector<LaTeXLongTable::Column>{
            LaTeXLongTable::Column{"Время", 'T'},
            LaTeXLongTable::Column{"№ машины", 'C'},
            LaTeXLongTable::Column{"Имя машины", 'C'},
        });

    table->rows.append(
        {
            LaTeXLongTable::Row{
                "2022-03-03 10:23:30", "10", "ППРУ"
            },
            LaTeXLongTable::Row{
                "2022-03-03 10:23:30", "10", "ППРУ"
            },
            LaTeXLongTable::Row{
                "2022-03-03 10:23:30", "10", "ППРУ"
            }
        });

    LaTeXDocument document({par, par, table, par, table});

    // write to stdout
    QTextStream stream(stdout);
    document.render(stream);
    stream.flush();

    // write to file
    QFile out_file("main.tex");
    if (!out_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return 1;
    }
    QTextStream out_file_stream(&out_file);
    document.render(out_file_stream);
    out_file_stream.flush();
    out_file.close();

    PdfLaTeXFileRenderer pdfRenderer;
    if (pdfRenderer.render("pdflatex_my.pdf", document)) {
        std::cout << "pdflatex OK" << std::endl;
    }
    else {
        std::cout << "pdflatex ERROR" << std::endl;
    }

    LuaLaTeXFileRenderer luaRenderer;
    if (pdfRenderer.render("lualatex_my.pdf", document)) {
        std::cout << "lualatex OK" << std::endl;
    }
    else {
        std::cout << "lualatex ERROR" << std::endl;
    }

    return 0;
}
