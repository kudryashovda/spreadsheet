#pragma once

#include "cell.h"
#include "common.h"

#include <functional>

enum class DataType {
    VALUES,
    TEXT
};

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, const std::string& text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    void CorrectSheetSizeToNewPos(Position pos);

    bool IsPosOutOfSheet(const Position& pos) const;
    void UpdatePrintableArea();
    void PrintData(std::ostream& output, DataType data_type) const;
    void CacheClearHelper(Sheet& sheet, Cell* cell_ptr);
    void CheckCycleOnReferencedCells(Sheet& sheet, Cell* init_ptr, Cell* cell_ptr, std::unordered_set<Cell*>& closure);

private:
    Size sheet_size_ = { 0, 0 };
    Size print_size_ = { 0, 0 };
    std::vector<std::vector<std::unique_ptr<Cell>>> sheet_;
};
