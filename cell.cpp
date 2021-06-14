#include "cell.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <string>

Cell::Cell(SheetInterface& sheet)
    : sheet_(sheet) {
    impl_ = std::move(std::make_unique<EmptyImpl>());
}

Cell::~Cell() {}

void Cell::Clear() {
    impl_ = std::move(std::make_unique<EmptyImpl>());
}

bool Cell::IsEmpty() {
    return impl_->GetText().empty();
}

void Cell::Set(std::string text) {
    if (text.empty()) {
        Clear();
    } else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        impl_ = std::move(std::make_unique<FormulaImpl>(sheet_, text.substr(1)));
    } else {
        impl_ = std::move(std::make_unique<TextImpl>(text));
    }
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

void Cell::ClearCache() {
    impl_->ClearCache();
}

const std::vector<Position> Cell::GetDependentCells() const {
    return dependent_cells_;
}

void Cell::AddDependentCell(Position pos) {
    dependent_cells_.push_back(pos);
}

TextImpl::TextImpl(std::string text)
    : text_(std::move(text)) {}

CellInterface::Value TextImpl::GetValue() {
    if (text_.front() == ESCAPE_SIGN) {
        return text_.substr(1);
    } else {
        return text_;
    }
}

std::string TextImpl::GetText() const {
    return text_;
}

FormulaImpl::FormulaImpl(const SheetInterface& sheet, std::string text)
    : sheet_(sheet) {
    try {
        parsed_obj_ptr_ = std::move(ParseFormula(text));
    } catch (...) {
        throw FormulaException("formula parsing error");
    }
}

CellInterface::Value FormulaImpl::CalculateFormula() const {
    FormulaInterface::Value calculated_value = parsed_obj_ptr_->Evaluate(sheet_);

    CellInterface::Value result;

    if (std::holds_alternative<double>(calculated_value)) {
        result = std::get<double>(calculated_value);
    }

    if (std::holds_alternative<FormulaError>(calculated_value)) {
        result = std::get<FormulaError>(calculated_value);
    }

    return result;
}

CellInterface::Value FormulaImpl::GetValue() {

    if (cached_value_ == std::nullopt) {
        cached_value_ = CalculateFormula();
    }

    return cached_value_.value();
}

void FormulaImpl::ClearCache() {
    cached_value_ = std::nullopt;
}

std::vector<Position> FormulaImpl::GetReferencedCells() const {
    return parsed_obj_ptr_->GetReferencedCells();
}

std::string FormulaImpl::GetText() const {
    return "="s + parsed_obj_ptr_->GetExpression();
}
