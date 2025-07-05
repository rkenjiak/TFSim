#ifndef INSTRUCTION_DEPENDENCY_HPP
#define INSTRUCTION_DEPENDENCY_HPP

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <cctype>

// Function to trim leading/trailing whitespace from a string
static inline 
std::string trim(const std::string& s) 
{
    size_t first = s.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) {
        return s;
    }
    size_t last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, (last - first + 1));
}

// Function to convert a string to uppercase
static inline 
std::string to_upper(std::string s) 
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return s;
}


// --- Main Dependency Identifier Class ---

/**
 * @class Dependency_Identifier
 * @brief Identifies data dependencies in a list of instructions.
 */
class Dependency_Identifier {
public:
    /**
     * @brief Constructor that takes the list of instructions to analyze.
     * @param instructions A vector of strings, where each string is a single assembly instruction.
     */
    Dependency_Identifier(const std::vector<std::string>& instructions) {
        _instructions = instructions;

        // --- UPDATED INSTRUCTION DEFINITIONS ---
        // Define the operand structure for known instructions.
        // Format: { "OPCODE", {destination_register_index, {source_register_indices...}} }
        // A destination index of 0 means the instruction does not write to a register.

        // Original Instructions
        operand_map["ADD"] = { 1, {2, 3} };
        operand_map["ADDI"] = { 1, {2} };
        operand_map["SUB"] = { 1, {2, 3} };
        operand_map["MUL"] = { 1, {2, 3} };
        operand_map["DIV"] = { 1, {2, 3} };
        operand_map["LD"] = { 1, {} };
        operand_map["SD"] = { 0, {1, 3} };

        // NEW: Double Precision Floating-Point Instructions
        operand_map["DADD"] = { 1, {2, 3} };
        operand_map["DADDI"] = { 1, {2} };
        operand_map["DSUB"] = { 1, {2, 3} };
        operand_map["DMUL"] = { 1, {2, 3} };
        operand_map["DDIV"] = { 1, {2, 3} };

        // NEW: Comparison Instructions
        operand_map["SLT"] = { 1, {2, 3} };
        operand_map["SGT"] = { 1, {2, 3} };

        // NEW: Branch Instructions (Read-only)
        operand_map["BEQ"] = { 0, {1, 2} }; // No destination register
        operand_map["BNE"] = { 0, {1, 2} }; // No destination register
        operand_map["BLTZ"] = { 0, {1, 2} }; // No destination register
        operand_map["BGTZ"] = { 0, {1, 2} }; // No destination register
        operand_map["BGEZ"] = { 0, {1, 2} }; // No destination register
        operand_map["BLEZ"] = { 0, {1, 2} }; // No destination register

        // Parse all instructions upon initialization
        for (const auto& inst : _instructions) {
            parsed_instructions.push_back(parse_instruction(inst));
        }
    }

    /**
     * @brief Finds and prints all RAW dependencies to std::cout.
     */
    void 
    find_RAW_dependencies() 
    {
        std::cout << "--- Identifying RAW Dependencies ---" << std::endl;
        if (parsed_instructions.empty()) {
            std::cout << "No instructions to analyze." << std::endl;
            return;
        }

        for (size_t i = 0; i < parsed_instructions.size(); ++i) {
            std::string dest_reg = get_destination_register(i);

            if (dest_reg.empty()) {
                continue;
            }

            for (size_t j = i + 1; j < parsed_instructions.size(); ++j) {
                std::vector<std::string> src_regs = get_source_registers(j);
                for (const auto& src_reg : src_regs) {
                    if (src_reg == dest_reg) {
                        std::cout << "RAW Dependency Found:" << std::endl;
                        std::cout << "\tInstruction " << i << ": (" << _instructions[i] << ") writes to register " << dest_reg << "." << std::endl;
                        std::cout << "\tInstruction " << j << ": (" << _instructions[j] << ") reads from register " << dest_reg << "." << std::endl;
                        std::cout << "------------------------------------" << std::endl;
                    }
                }
            }
        }
        std::cout << "--- Analysis Complete ---" << std::endl;
    }

    /**
     * @brief Finds and prints all WAR dependencies to std::cout.
     */
    void 
    find_WAR_dependencies() 
    {
        std::cout << "--- Identifying WAR Dependencies ---" << std::endl;
        if (parsed_instructions.empty()) {
            std::cout << "No instructions to analyze." << std::endl;
            return;
        }

        for (size_t i = 0; i < parsed_instructions.size(); ++i) {
            // Get the source registers of the first instruction
            std::vector<std::string> src_regs_i = get_source_registers(i);

            if (src_regs_i.empty()) {
                continue; // Instruction i doesn't read, so no WAR is possible
            }

            for (size_t j = i + 1; j < parsed_instructions.size(); ++j) {
                // Get the destination register of the second instruction
                std::string dest_reg_j = get_destination_register(j);
                if (dest_reg_j.empty()) {
                    continue;
                }

                // Check if any source from instruction i matches the destination of instruction j
                for (const auto& src_reg : src_regs_i) {
                    if (src_reg == dest_reg_j) {
                        std::cout << "WAR Dependency Found:" << std::endl;
                        std::cout << "\tInstruction " << i << ": (" << _instructions[i] << ") reads from register " << src_reg << "." << std::endl;
                        std::cout << "\tInstruction " << j << ": (" << _instructions[j] << ") writes to register " << dest_reg_j << "." << std::endl;
                        std::cout << "------------------------------------" << std::endl;
                    }
                }
            }
        }
        std::cout << "--- Analysis Complete ---" << std::endl;
    }

    /**
     * @brief Finds and prints all WAW dependencies to std::cout.
     */
    void 
    find_WAW_dependencies() 
    {
        std::cout << "--- Identifying WAW Dependencies ---" << std::endl;
        if (parsed_instructions.empty()) {
            std::cout << "No instructions to analyze." << std::endl;
            return;
        }

        for (size_t i = 0; i < parsed_instructions.size(); ++i) {
            // Get the destination register of the first instruction
            std::string dest_reg_i = get_destination_register(i);

            if (dest_reg_i.empty()) {
                continue; // Instruction i doesn't write, so no WAW is possible
            }

            for (size_t j = i + 1; j < parsed_instructions.size(); ++j) {
                // Get the destination register of the second instruction
                std::string dest_reg_j = get_destination_register(j);

                // If they write to the same register, it's a WAW dependency
                if (dest_reg_i == dest_reg_j) {
                    std::cout << "WAW Dependency Found:" << std::endl;
                    std::cout << "\tInstruction " << i << ": (" << _instructions[i] << ") writes to register " << dest_reg_i << "." << std::endl;
                    std::cout << "\tInstruction " << j << ": (" << _instructions[j] << ") also writes to register " << dest_reg_j << "." << std::endl;
                    std::cout << "------------------------------------" << std::endl;
                }
            }
        }
        std::cout << "--- Analysis Complete ---" << std::endl;
    }

private:
    /**
     * @brief Parses a single instruction string into its components.
     */
    std::vector<std::string> 
    parse_instruction(std::string instruction) 
    {
        std::replace(instruction.begin(), instruction.end(), ',', ' ');
        std::replace(instruction.begin(), instruction.end(), ';', ' ');
        std::replace(instruction.begin(), instruction.end(), '(', ' ');
        std::replace(instruction.begin(), instruction.end(), ')', ' ');
        std::vector<std::string> parts;
        std::stringstream ss(instruction);
        std::string part;
        while (ss >> part) {
            parts.push_back(trim(part));
        }
        return parts;
    }

    /**
     * @brief Retrieves the destination register for a given instruction.
     */
    std::string 
    get_destination_register(size_t index) 
    {
        if (index >= parsed_instructions.size() || parsed_instructions[index].empty()) {
            return "";
        }
        const auto& instruction_parts = parsed_instructions[index];
        std::string opcode = to_upper(instruction_parts[0]);

        auto it = operand_map.find(opcode);
        if (it != operand_map.end()) {
            int dest_index = it->second.first;
            // A destination index must be positive
            if (dest_index > 0 && dest_index < instruction_parts.size()) {
                return instruction_parts[dest_index];
            }
        }
        return "";
    }

    /**
     * @brief Retrieves all source registers for a given instruction.
     */
    std::vector<std::string> 
    get_source_registers(size_t index) 
    {
        std::vector<std::string> sources;
        if (index >= parsed_instructions.size() || parsed_instructions[index].empty()) {
            return sources;
        }

        const auto& instruction_parts = parsed_instructions[index];
        std::string opcode = to_upper(instruction_parts[0]);

        auto it = operand_map.find(opcode);
        if (it != operand_map.end()) {
            const auto& source_indices = it->second.second;
            for (int src_index : source_indices) {
                if (src_index > 0 && src_index < instruction_parts.size()) {
                    sources.push_back(instruction_parts[src_index]);
                }
            }
        }
        return sources;
    }

    // --- Member Variables ---
    std::vector<std::string> _instructions;
    std::vector<std::vector<std::string>> parsed_instructions;
    std::map<std::string, std::pair<int, std::vector<int>>> operand_map;
}; // End of Dependency_Identifier class


#endif // INSTRUCTION_DEPENDENCY_HPP

