#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::CorrectSheetSizeToNewPos(Position pos) {
    if (pos.row + 1 > sheet_size_.rows) {
        sheet_.resize(pos.row + 1);

        for (auto& it : sheet_) {
            it.resize(sheet_size_.cols + 1);
        }

        sheet_size_.rows = pos.row + 1;
    }

    if (pos.col + 1 > sheet_size_.cols) {
        for (auto& it : sheet_) {
            it.resize(pos.col + 1);
        }

        sheet_size_.cols = pos.col + 1;
    }
}

void Sheet::SetCell(Position pos, const std::string& text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("wrong position");
    }

    CorrectSheetSizeToNewPos(pos);

    Cell* cell_ptr = reinterpret_cast<Cell*>(this->GetCell(pos));

    if (cell_ptr != nullptr) {
        if (cell_ptr->GetText() == text) {
            // Do nothing if cell's content is the same
            return;
        }

        cell_ptr->Set(text);
    } else {
        auto new_cell = std::make_unique<Cell>(*this);
        new_cell->Set(text);
        sheet_[pos.row][pos.col] = std::move(new_cell);

        cell_ptr = reinterpret_cast<Cell*>(this->GetCell(pos));
    }

    // To handle REF cells for errores
    for (const auto& ref_cell_pos : cell_ptr->GetReferencedCells()) {
        if (!ref_cell_pos.IsValid()) {
            // "=A1+ZZZ3" we save this but do not check for cyclic link
            return;
        }

        // if REF pos is valid we have to create an empty cell for it
        if (this->GetCell(ref_cell_pos) == nullptr) {
            auto new_cell = std::make_unique<Cell>(*this);

            CorrectSheetSizeToNewPos(ref_cell_pos);

            sheet_[ref_cell_pos.row][ref_cell_pos.col] = std::move(new_cell);

            print_size_ = sheet_size_;

            return;
        }
    }

    // An exception will throw if cyclic link is found
    std::unordered_set<Cell*> closure; // to store checked cells
    CheckCycleOnReferencedCells(*this, cell_ptr, cell_ptr, closure);

    //add this cell as a dependent to nearest parent cells
    for (const auto& cell_pos : cell_ptr->GetReferencedCells()) {
        auto in_cell_ptr = reinterpret_cast<Cell*>(this->GetCell(cell_pos));

        if (in_cell_ptr != nullptr) {
            in_cell_ptr->AddDependentCell(pos); // this will help to invalidate cache later
        }
    }

    // If we change cell we have to invalidate all dependent cells
    CacheClearHelper(*this, cell_ptr);

    print_size_ = sheet_size_;
}

void Sheet::CheckCycleOnReferencedCells(Sheet& sheet, Cell* init_ptr, Cell* cell_ptr, std::unordered_set<Cell*>& closure) {
    if (cell_ptr == nullptr) {
        return;
    }

    auto cells = cell_ptr->GetReferencedCells();

    for (const auto& cell_pos : cells) {
        if (!cell_pos.IsValid()) {
            continue;
        }
        auto* in_cell_ptr = reinterpret_cast<Cell*>(sheet.GetCell(cell_pos));
        if (closure.find(in_cell_ptr) != closure.end()) {
            // this cell has been already checked, so return
            continue;
        }

        if (init_ptr == in_cell_ptr) {
            throw CircularDependencyException("cycle link found");
        }

        closure.insert(in_cell_ptr); // checked ptr, go next
        CheckCycleOnReferencedCells(sheet, init_ptr, in_cell_ptr, closure);
    }
}

void Sheet::CacheClearHelper(Sheet& sheet, Cell* cell_ptr) {
    for (const auto& cell_pos : cell_ptr->GetDependentCells()) {
        auto* in_cell_ptr = reinterpret_cast<Cell*>(sheet.GetCell(cell_pos));
        in_cell_ptr->ClearCache();
        CacheClearHelper(sheet, in_cell_ptr);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position");
    }

    if (IsPosOutOfSheet(pos)) {
        return nullptr;
    }

    auto& cell_link = sheet_.at(pos.row).at(pos.col);

    if (cell_link == nullptr || cell_link->IsEmpty()) {
        return nullptr;
    }

    return cell_link.get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("invalid position");
    }

    if (IsPosOutOfSheet(pos)) {
        return nullptr;
    }

    auto& cell_link = sheet_.at(pos.row).at(pos.col);

    if (cell_link == nullptr || cell_link->IsEmpty()) {
        return nullptr;
    }

    return cell_link.get();
}

void Sheet::ClearCell(Position pos) {

    auto* cell_ptr = reinterpret_cast<Cell*>(GetCell(pos));

    if (cell_ptr == nullptr) {
        return;
    }

    cell_ptr->Clear();

    UpdatePrintableArea();
}

void Sheet::UpdatePrintableArea() {
    int last_non_empty_row = Position::NONE.row;
    int last_non_empty_col = Position::NONE.col;

    for (int row = 0; row < print_size_.rows; ++row) {
        for (int col = 0; col < print_size_.cols; ++col) {
            auto* cell_ptr = reinterpret_cast<Cell*>(GetCell({ row, col }));
            if (cell_ptr != nullptr) {
                if (!cell_ptr->IsEmpty()) {
                    if (row > last_non_empty_row) {
                        last_non_empty_row = row;
                    }

                    if (col > last_non_empty_col) {
                        last_non_empty_col = col;
                    }
                }
            }
        }
    }

    print_size_ = { last_non_empty_row + 1, last_non_empty_col + 1 };
}

Size Sheet::GetPrintableSize() const {
    return print_size_;
}

void Sheet::PrintData(std::ostream& output, DataType data_type) const {
    auto printable_size = GetPrintableSize();

    for (int row = 0; row < printable_size.rows; ++row) {
        for (int col = 0; col < printable_size.cols; ++col) {
            const auto* cell_ptr = reinterpret_cast<const Cell*>(GetCell({ row, col }));
            if (cell_ptr != nullptr) {
                if (data_type == DataType::VALUES) {
                    output << cell_ptr->GetValue();
                }
                if (data_type == DataType::TEXT) {
                    output << cell_ptr->GetText();
                }
            }
            if (col + 1 != printable_size.cols) {
                output << "\t";
            }
        }

        output << "\n";
    }
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintData(output, DataType::VALUES);
}

void Sheet::PrintTexts(std::ostream& output) const {
    PrintData(output, DataType::TEXT);
}

bool Sheet::IsPosOutOfSheet(const Position& pos) const {
    return pos.col + 1 > sheet_size_.cols || pos.row + 1 > sheet_size_.rows;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
