#include "cell.h"
#include "common.h"
#include "test_runner_p.h"

inline std::ostream& operator<<(std::ostream& output, Position pos) {
    return output << "(" << pos.row << ", " << pos.col << ")";
}

inline Position operator"" _pos(const char* str, std::size_t) {
    return Position::FromString(str);
}

inline std::ostream& operator<<(std::ostream& output, Size size) {
    return output << "(" << size.rows << ", " << size.cols << ")";
}

inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit([&](const auto& x) { output << x; }, value);

    return output;
}

namespace {

    void TestEmpty() {
        auto sheet = CreateSheet();
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 0, 0 }));
    }

    void TestInvalidPosition() {
        auto sheet = CreateSheet();
        try {
            sheet->SetCell(Position{ -1, 0 }, "");
        } catch (const InvalidPositionException&) {
        }

        try {
            sheet->GetCell(Position{ 0, -2 });
        } catch (const InvalidPositionException&) {
        }

        try {
            sheet->ClearCell(Position{ Position::MAX_ROWS, 0 });
        } catch (const InvalidPositionException&) {
        }
    }

    void TestSetCellPlainText() {
        auto sheet = CreateSheet();

        auto checkCell = [&](Position pos, std::string text) {
            sheet->SetCell(pos, text);

            CellInterface* cell = sheet->GetCell(pos);
            ASSERT(cell != nullptr);
            ASSERT_EQUAL(cell->GetText(), text);
            ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), text);
        };

        checkCell("A1"_pos, "Hello");
        checkCell("A1"_pos, "World");
        checkCell("B2"_pos, "Purr");
        checkCell("A3"_pos, "Meow");

        const SheetInterface& constSheet = *sheet;
        ASSERT_EQUAL(constSheet.GetCell("B2"_pos)->GetText(), "Purr");

        sheet->SetCell("A3"_pos, "'=escaped");
        CellInterface* cell = sheet->GetCell("A3"_pos);
        ASSERT_EQUAL(cell->GetText(), "'=escaped");
        ASSERT_EQUAL(std::get<std::string>(cell->GetValue()), "=escaped");
    }

    void TestClearCell() {
        auto sheet = CreateSheet();

        sheet->SetCell("C2"_pos, "Me gusta");
        sheet->ClearCell("C2"_pos);
        ASSERT(sheet->GetCell("C2"_pos) == nullptr);

        sheet->ClearCell("A1"_pos);
        sheet->ClearCell("J10"_pos);
    }

    void TestPrint() {
        auto sheet = CreateSheet();

        sheet->SetCell("A2"_pos, "meow");
        sheet->SetCell("B2"_pos, "=1+2");
        sheet->SetCell("A1"_pos, "=1/0");

        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 2, 2 }));

        std::ostringstream texts;
        sheet->PrintTexts(texts);
        ASSERT_EQUAL(texts.str(), "=1/0\t\nmeow\t=1+2\n");

        std::ostringstream values;
        sheet->PrintValues(values);
        ASSERT_EQUAL(values.str(), "#DIV/0!\t\nmeow\t3\n");

        sheet->ClearCell("B2"_pos);
        ASSERT_EQUAL(sheet->GetPrintableSize(), (Size{ 2, 1 }));
    }

    void TestSheetCalcCache() {
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=3");
            auto* cell_A1_ptr = sheet->GetCell("A1"_pos);
            ASSERT_EQUAL(std::get<double>(cell_A1_ptr->GetValue()), 3);

            sheet->SetCell("A2"_pos, "=A1");
            auto* cell_A2_ptr = sheet->GetCell("A2"_pos);
            ASSERT_EQUAL(std::get<double>(cell_A2_ptr->GetValue()), 3);

            sheet->SetCell("A1"_pos, "=4");
            cell_A1_ptr = sheet->GetCell("A1"_pos);
            ASSERT_EQUAL(std::get<double>(cell_A1_ptr->GetValue()), 4);

            cell_A2_ptr = sheet->GetCell("A2"_pos);
            ASSERT_EQUAL(std::get<double>(cell_A2_ptr->GetValue()), 4);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=3");
            sheet->SetCell("A2"_pos, "=5");

            sheet->SetCell("A3"_pos, "=A1+A2");
            auto* cell_A3_ptr = sheet->GetCell("A3"_pos);
            ASSERT_EQUAL(std::get<double>(cell_A3_ptr->GetValue()), 8);
        }

        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "=3");
            sheet->SetCell("A2"_pos, "=5");
            sheet->SetCell("A3"_pos, "=7");
            sheet->SetCell("A4"_pos, "=9");

            sheet->SetCell("A5"_pos, "=A1+A2+A3+A4");

            auto* cell_A5_ptr = sheet->GetCell("A5"_pos);
            ASSERT_EQUAL(std::get<double>(cell_A5_ptr->GetValue()), 24);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "3");
            sheet->SetCell("A2"_pos, "5");
            sheet->SetCell("A3"_pos, "7");
            sheet->SetCell("A4"_pos, "9");

            sheet->SetCell("A5"_pos, "=A1+A2+A3+A4");

            auto* cell_A5_ptr = sheet->GetCell("A5"_pos);
            ASSERT_EQUAL(std::get<double>(cell_A5_ptr->GetValue()), 24);
        }
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "2");
            sheet->SetCell("A2"_pos, "=A1+1");
            sheet->SetCell("A3"_pos, "=A2+2");
            sheet->SetCell("A4"_pos, "=A3+3");

            sheet->SetCell("A5"_pos, "=A1+A2+A3+A4");

            auto* cell_A5_ptr = sheet->GetCell("A5"_pos);
            ASSERT_EQUAL(std::get<double>(cell_A5_ptr->GetValue()), 18);

            sheet->SetCell("A5"_pos, "=A2+A1+A4+A3");
            cell_A5_ptr = sheet->GetCell("A5"_pos);
            ASSERT_EQUAL(std::get<double>(cell_A5_ptr->GetValue()), 18);
        }
    }

    void TestSheetCyclicRef() {
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "1");
            sheet->SetCell("A2"_pos, "2");

            sheet->SetCell("A1"_pos, "=A2");
            try {
                sheet->SetCell("A2"_pos, "=A1");
            } catch (...) {
                std::cout << "exeption cyclic link found\n";
            }
        }
    }

    void TestSheetCyclicDep() {
        {
            auto sheet = CreateSheet();
            sheet->SetCell("A1"_pos, "1");
            sheet->SetCell("A2"_pos, "=A1");
            sheet->SetCell("A3"_pos, "3");
            sheet->SetCell("A4"_pos, "=A2");
            sheet->SetCell("A5"_pos, "=A1");

            try {
                sheet->SetCell("A1"_pos, "=A4");
            } catch (...) {
                std::cout << "exeption cyclic link found\n";
            }
        }
    }
    void TestSheetCyclicSelf() {
        {
            auto sheet = CreateSheet();
            try {
                sheet->SetCell("A2"_pos, "=A2");
            } catch (CircularDependencyException& err) {
                std::cout << "exept self cyclic link found\n";
            }
        }
    }

    void TestFormulaErrors() {
        {
            auto sheet = CreateSheet();

            sheet->SetCell("A1"_pos, "1");
            sheet->SetCell("A2"_pos, "0");
            sheet->SetCell("A3"_pos, "=A1/A2");

            auto cell_A3_ptr = sheet->GetCell("A3"_pos);
            auto val = cell_A3_ptr->GetValue();

            ASSERT_EQUAL(std::get<FormulaError>(val).ToString(), "#DIV0!");
        }
        {
            auto sheet = CreateSheet();

            sheet->SetCell("A1"_pos, "10");
            sheet->SetCell("A2"_pos, "5");
            sheet->SetCell("A3"_pos, "=A1/A2");
            sheet->SetCell("A4"_pos, "=A3+1");

            auto cell_A3_ptr = sheet->GetCell("A3"_pos);
            auto val = cell_A3_ptr->GetValue();
            ASSERT_EQUAL(std::get<double>(val), 2);

            sheet->SetCell("A2"_pos, "0");
            cell_A3_ptr = sheet->GetCell("A3"_pos);
            val = cell_A3_ptr->GetValue();

            ASSERT_EQUAL(std::get<FormulaError>(val).ToString(), "#DIV0!");

            auto cell_A4_ptr = sheet->GetCell("A4"_pos);
            val = cell_A4_ptr->GetValue();

            ASSERT_EQUAL(std::get<FormulaError>(val).ToString(), "#DIV0!");
        }
        {
            auto sheet = CreateSheet();

            sheet->SetCell("A1"_pos, "45");
            sheet->SetCell("A2"_pos, "text");
            sheet->SetCell("A3"_pos, "=A1/A2");
            sheet->SetCell("A4"_pos, "=A3+1");

            auto cell_A3_ptr = sheet->GetCell("A3"_pos);
            auto val = cell_A3_ptr->GetValue();

            ASSERT_EQUAL(std::get<FormulaError>(val).ToString(), "#VALUE!");

            auto cell_A4_ptr = sheet->GetCell("A4"_pos);
            val = cell_A4_ptr->GetValue();

            ASSERT_EQUAL(std::get<FormulaError>(val).ToString(), "#VALUE!");
        }

        {
            auto sheet = CreateSheet();

            sheet->SetCell("A1"_pos, "45");
            sheet->SetCell("A2"_pos, "=ZZZ2");
            sheet->SetCell("A3"_pos, "=A2");

            auto cell_A2_ptr = sheet->GetCell("A2"_pos);
            auto val = cell_A2_ptr->GetValue();

            ASSERT_EQUAL(std::get<FormulaError>(val).ToString(), "#REF!");

            auto cell_A3_ptr = sheet->GetCell("A3"_pos);
            val = cell_A3_ptr->GetValue();

            ASSERT_EQUAL(std::get<FormulaError>(val).ToString(), "#REF!");
        }
    }
    void TestEmptyFieldZero() {
        auto sheet = CreateSheet();

        sheet->SetCell("A1"_pos, "45");
        sheet->SetCell("A2"_pos, "=A1+A3");

        auto cell_A2_ptr = sheet->GetCell("A2"_pos);
        auto val = cell_A2_ptr->GetValue();

        ASSERT_EQUAL(std::get<double>(val), 45);
    }

} // namespace

int main() {
    TestRunner tr;
    RUN_TEST(tr, TestEmpty);
    RUN_TEST(tr, TestInvalidPosition);
    RUN_TEST(tr, TestSetCellPlainText);
    RUN_TEST(tr, TestClearCell);
    RUN_TEST(tr, TestPrint);

    RUN_TEST(tr, TestSheetCalcCache);
    RUN_TEST(tr, TestSheetCyclicRef);
    RUN_TEST(tr, TestSheetCyclicDep);
    RUN_TEST(tr, TestSheetCyclicSelf);

    RUN_TEST(tr, TestFormulaErrors);
    RUN_TEST(tr, TestEmptyFieldZero);

    return 0;
}
