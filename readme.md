# Spreadsheet

It's a diploma project written on C++ during Yandex Praktikum course

## Description

A back-end realization of spreadsheet. Supports basic arithmetic operations and references (links) to other cells

Example:
```C++
sheet->SetCell("A1"_pos, "2");
sheet->SetCell("A2"_pos, "=A1+1");
sheet->SetCell("A3"_pos, "=A2+2");
sheet->SetCell("A4"_pos, "=A3+3");
sheet->SetCell("A5"_pos, "=A1+A2+A3+A4");

auto* cell_A5_ptr = sheet->GetCell("A5"_pos);
ASSERT_EQUAL(std::get<double>(cell_A5_ptr->GetValue()), 18);
```

Realized exceptions support:

* #DIV0! - if formula contains division by zero
* #VALUE! - if an operand contains a text value instead of number
* #REF!  - if a referenced cell position is out of sheet possible size

## Note
Link cyclic dependency check was realized. In case of such situation:
```cpp
sheet->SetCell("A1"_pos, "=A2");
sheet->SetCell("A2"_pos, "=A1");
```
a CircularDependencyException will be thrown. 

Realized a mechanism of fast items announcing in case of cell value was changed.

## Used language features
OOP, polymorphism, templates, lyambda functions, std algorithms, abstract syntax tree (AST), patterns.

## Build

[ANTLR](https://www.antlr.org/) generator is used. Grammar rules are located in Formula.g4 file.

To generate executive files run command:
```
antlr -Dlanguage=Cpp Formula.g4
```

To use CMAKE CMakeLists.txt and FindANTLR.cmake files are provided.

