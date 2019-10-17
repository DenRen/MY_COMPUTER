//
// Created by tester on 10.10.2019.
//

// Костыль ))
#define ___END '\0'

/*
 * ax, bx, cx, dx
*/

#ifndef MY_COMPUTER_COMPILATOR_H
#define MY_COMPUTER_COMPILATOR_H

#define MAXLENCOMM 100
#define MAXLENREGS 5

typedef double number_t;
typedef __uint8_t comand_t;
typedef __uint8_t reg_t;

enum COMMANDS {
    cmd_PUSH,
    cmd_POP,
    cmd_ADD,
    cmd_SUB,
    cmd_MUL,
    cmd_EXIT,
    cmd_SQRT,
    cmd_SIN,
    cmd_COS,
    cmd_LOGN
};

enum REGISTERS {
    AX, BX, CX, DX
};

const char commands[][MAXLENCOMM] = {
        "push",
        "pop",
        "add",
        "sub",
        "mul",
        "exit",
        "sqrt",
        "sin",
        "cos",
        "logn"
};

const char registers[][MAXLENREGS] = {
        "ax", "bx", "cx", "dx"
};

const unsigned size_commands = sizeof (commands) / MAXLENCOMM;
const unsigned size_registers = sizeof (registers) / MAXLENREGS;

const char symb_comment = ';';
const char symb_separator = ' '; // Не должен быть

// Разрешённые символы. '[]' - для обращение к RAM,
// '+' - для удобной работы с массивами в, к примеру,
// [ax + 5] обратится к пятому элементу массива в RAM
const char necess_symb[] = {
        '[', ']', '+', ___END
};

const char RAM_symb[] = {
        '[', ']', ___END
};

const char critically_forbidden_characters[] = {
        '-', '*', '/', '\\', ___END
};

#endif //MY_COMPUTER_COMPILATOR_H

























