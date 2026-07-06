#pragma once
#include <string>
#include <vector>
#include "Piece.h"
#include <sstream>

class Figure {
public:
    std::string name;
    std::vector<Piece> pieces;

    Figure() {}
    Figure(const std::string& n) : name(n) {}

    std::string serialize() const {
        std::string result;
    
        for (const auto& p : pieces) {
            result += std::to_string(p.quantity) + " " + p.type + "\n";
        }
    
        return result;
    }

    static Figure deserialize(const std::string& name, const std::string& data) {
        Figure f(name);
    
        std::stringstream ss(data);
        std::string line;
    
        while (std::getline(ss, line)) {
            if (line.empty()) continue;
    
            std::stringstream ls(line);
    
            int quantity;
            ls >> quantity; 
    
            std::string type, word;
            while (ls >> word) {
                if (!type.empty()) type += " ";
                type += word;
            }
    
            if (!type.empty()) {
                f.pieces.emplace_back(type, quantity);
            }
        }
    
        return f;
    }
};