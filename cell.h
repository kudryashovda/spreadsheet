#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

using namespace std::string_literals;

class Sheet;

class Impl;
class EmptyImpl;

class Cell : public CellInterface {
public:
    explicit Cell(SheetInterface& sheet);
    ~Cell();

    Sheet& GetSheet();

    void Set(std::string text);
    void Clear();

    bool IsEmpty();

    Value GetValue() const override;
    void ClearCache();

    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    const std::vector<Position> GetDependentCells() const;
    void AddDependentCell(Position pos);

private:
    SheetInterface& sheet_;
    std::unique_ptr<Impl> impl_ = nullptr;
    std::vector<Position> dependent_cells_;
};

class Impl {
public:
    virtual ~Impl() {}
    virtual CellInterface::Value GetValue() = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
    virtual void ClearCache() = 0;
};

class EmptyImpl : public Impl {
public:
    EmptyImpl() {}

    std::string GetText() const override {
        return {};
    }

    CellInterface::Value GetValue() override {
        return 0.0;
    }

    std::vector<Position> GetReferencedCells() const override {
        return {};
    }

    void ClearCache() override {}
};

class TextImpl : public Impl {
public:
    TextImpl(std::string text);

    CellInterface::Value GetValue() override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override {
        return {};
    }

    void ClearCache() override {}

private:
    std::string text_;
};

class FormulaImpl : public Impl {
public:
    FormulaImpl(const SheetInterface& sheet, std::string text);

    CellInterface::Value CalculateFormula() const;

    CellInterface::Value GetValue() override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    void ClearCache() override;

private:
    const SheetInterface& sheet_;
    std::unique_ptr<FormulaInterface> parsed_obj_ptr_;
    std::optional<CellInterface::Value> cached_value_;
};

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& val);
