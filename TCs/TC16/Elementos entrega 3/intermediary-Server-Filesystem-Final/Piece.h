#pragma once
#include <string>

class Piece {
public:
    std::string type;  
    int quantity;

    Piece() {}
    Piece(const std::string& t, int q) : type(t), quantity(q) {}
};