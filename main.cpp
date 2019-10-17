#include "Super_Stack_Library.h"
#include "MyLib.h"
#include "COMPILATOR.h"
#include <cstring>

// Компилятор
// В КАЖДОМ ENUM первый должен стоять ALL_RIGHT или начинаться с 1!
// ONLY IF sizeof(char) == 1    // Написать проверку на эту совместимость

struct Compiler_t {
    char *buf;              // Буффер, где лежит необр. текстовые команды
    char *MC;               // Итоговый машинный код
    unsigned size_buf;      // Размер буффера в байтах
    unsigned count_elem;    // Кол-во элементов в буфере (Пример: "push 5" -> count == 2)
    int state_stack;        // Код ошибки самого стека (например SYNTAX_ERROR)
    int state_func;         // Код ошибки последней функции, которая его обрабатывала (например CALLOC_ERROR)
};

struct CmdCode_t {
    comand_t code   : 5;
    comand_t mem    : 1;
    comand_t imm    : 1;
    comand_t reg    : 1;
};

void Cleaner (Compiler_t *data);

char *ConverterToMC (Compiler_t *data);

/*
 * DIM
 * Создаём структуру для того, чтобы эту программу легко можно было
 * внедтрить в другую программу и пользоваться её компонентами
*/

namespace compclnr {
    enum Cleaner_ERROR_CONST {
        ALL_RIGHT,
        ERROR_IN_BUFFER,
        ERROR_REALLOC,
        SYNTAX_ERROR
    };
}


int main () {
    const char name_file_input[] = "input.txt";
    Compiler_t data_ciler = {};     //DATA CompILER
    data_ciler.state_func = 0;
    data_ciler.size_buf = 0;

    data_ciler.buf = Read_File_To_Buffer (name_file_input, &data_ciler.size_buf, &data_ciler.state_func, true, true);
    if (data_ciler.state_func) {
        free (data_ciler.buf);
        return 0;
    }

    Cleaner (&data_ciler);
    if (data_ciler.state_func) {
        free (data_ciler.buf);
        return 0;
    }

    printf ("%s\n", data_ciler.buf);

    char *MC = ConverterToMC (&data_ciler);

    if (data_ciler.state_func != ALL_RIGHT || data_ciler.state_stack != ALL_RIGHT) {
        free (data_ciler.buf);
        return 0;
    }

    free (data_ciler.buf);
    return 0;
}

int CommandNumber (char *string) {
    int number = 0;
    for (; number < (int) size_commands; number++) {
        if (!strcmp (string, commands[number]))
            break;
    }

    if (number == size_commands)
        return -1;
    else
        return number;
}

int copy_str (char *to, char *from_b, char *from_e) {
    char *const_from_b = from_b, *const_from_e = from_e;

    if (const_from_e - const_from_b < MAXLENCOMM) {

        while (from_b != from_e)
            *to++ = *from_b++;
        *to = '\0';

    } else {
        PRINT_ERROR ("Command too large! ")
        return -1;
    }
    return 0;
}

namespace conv {
    enum Converter_ERROR {
        NOINSTRUCTION,      // Нет инструкции для выполнения этой команды
        NOINSTRUCTION_H,    // Команды нет в COMPILATOR.h
        CALLOC_ERROR,
        SYNTAX_ERROR,
        CMD_TOO_LARGE,
        BUF_ENDED,
        NUMBER,
        REGISTER,
        RAM_TYPE_OPEN,
        RAM_TYPE_CLOSE
    };
};

int buf_comp_POP (char **read_b, char *element) {
    char sep = symb_separator;

    char *read_e = (char *) strchr (*read_b, sep);

    if (read_e == nullptr)
        return conv::BUF_ENDED;
    // Выделяем слово, между пробелами

    if (copy_str (element, *read_b, read_e))
        return conv::CMD_TOO_LARGE;

    *read_b = read_e + 1;

    return 0;
}

int el_is_number (char c) {
    return c >= '0' && c <= '9';
}

int element_type (const char *element) {
    const char *necess_symbols = necess_symb;
    const char *RAM_symbols = RAM_symb;

    if (strchr (necess_symbols, *element)) {
        if (*element == RAM_symbols[0]) {
            return conv::RAM_TYPE_OPEN;
        }
    } else if (el_is_number (*element)) {
        while (el_is_number (*(++element)));
        if (*(element - 1) == '.')
            while (el_is_number (*(++element)));
        if (!el_is_number (*(element - 1)))
            return -1;
        return conv::NUMBER;
    } else {
        unsigned i = 0;
        for (; i < size_registers + 1; i++)
            if (i == size_registers)
                return -1;
            else if (!strcmp (element, registers[i]))
                return conv::REGISTER;
    }
    return 0;
}

char *ConverterToMC (Compiler_t *data) {
    // Для каждого типа конверитирование разное

    char *buf = data->buf;
    unsigned size_buf = data->size_buf;
    char sep = symb_separator;

    const size_t TYPE = typeid (number_t).hash_code ();
    if (TYPE == typeid (double).hash_code ()) {

        char *read_b = buf;     // Begin
        // Устанавливаем на начало слова
        if (*read_b == sep) {   // Буфер больше одного элемента
            read_b++;
            size_buf--;
        }

        CmdCode_t comand = {};
        char element[MAXLENCOMM];   // Тут хранится текущий объект buf (Например: push, add, 5)
        unsigned number_line = 0;

#define GET_NEXT !(data->state_func = buf_comp_POP (&read_b, element))

#define NUMBER (comand.imm = element_type(element) == conv::NUMBER)
#define REGISTER (comand.reg = element_type(element) == conv::REGISTER)
#define RAM_OPEN element_type(element) == conv::RAM_TYPE_OPEN
#define RAM_CLOSE (comand.mem = element_type(element) == conv::RAM_TYPE_CLOSE)

#define SYNTAXERR {printf ("\n--LINE-- (%d)\n \"%s\"", number_line, element);\
                    PRINT_ERROR (" - command NOT DEFINED in \"COMPILATOR.h\"") \
                    data->state_func = conv::NOINSTRUCTION_H; \
                    data->state_stack = conv::SYNTAX_ERROR; return nullptr;}

        data->MC = (char *) calloc (data->size_buf, 1);
        char *out_buf = data->MC;

        while (GET_NEXT) {  // Получает команду и записывает её в element
            comand = {0};

            unsigned i = 0;
            for (; i < size_commands; i++) {      // Проверяет существование команды в списке commands
                if (!strcmp (element, commands[i]))
                    break;
                else if (i == size_commands - 1) {              //Если список закончился, а команда не найдена
                    data->state_func = conv::NOINSTRUCTION;
                    data->state_stack = conv::SYNTAX_ERROR;

                    printf ("%s --LINE-- %d\n", element, number_line);
                    PRINT_ERROR ("Unknown command or syntax error in line")

                    free (out_buf);
                    return nullptr;
                }
            }
            number_line++;
            printf ("%s ", element);

//#include "instruction_comp.h"
            switch ((const unsigned) i) {
                case cmd_PUSH: {
                    if (GET_NEXT) {
                        if (NUMBER || REGISTER || RAM_OPEN)                 // push 5 || push ax


                        else if ((comand.mem = RAM_OPEN)) {             // push [
                            if (GET_NEXT) {
                                if ((comand.imm = NUMBER) ||            // push [ 5
                                    (comand.reg = REGISTER)) {          // push [ ax
                                    if (GET_NEXT) {
                                        if () {          // push [ ax



                                        } else SYNTAXERR
                                    } else SYNTAXERR


                                } else SYNTAXERR
                            } else SYNTAXERR
                        } else SYNTAXERR
                    } else SYNTAXERR
                    break;
                }
                default: SYNTAXERR

            }
#undef POP
        }

    } else {
        PRINT_ERROR ("No instructions for compiling this type.")
        data->state_func = conv::NOINSTRUCTION;
        return nullptr;
    }

    char *MC = (char *) calloc (size_buf, 1);
    return nullptr;
}

void Cleaner (Compiler_t *data) {
    // На выходе получается текст, каждый элемент которого расположен через пробел
    // Остаются цифры, буквы и
    // Не рассматривает синтаксические ошибки, т.е. .0.3 не будет удалено
    // Так же пройдёт 6 . 45 как 6 45
    // Удаляет всю строку после символа комментария symbol_comment
    // Формат для компилятора требует примерно в 1-2 раза больше памяти, чем исходный
    // код (елси он написан чисто)

    double k_exp = 2;

    const char symbol_comment = symb_comment;
    const char *necess_symbol = necess_symb;
    const char sep = symb_separator;
    const char *forb = critically_forbidden_characters;

    char *buf;
    unsigned *size_buf = &data->size_buf;
    int *state_func = &data->state_func;

    // Создаю новый буфер (он потом заменит старый буфер)
    buf = (char *) calloc ((size_t) ((double) *size_buf * k_exp), 1);
    if (buf == nullptr) {
        PRINT_ERROR ("Failed to allocate memory for putting the buffer into conversion mode for the compiler")
        *state_func = compclnr::ERROR_REALLOC;
        return;
    }

    char *w = buf;
    char *old_buf = data->buf, *new_buf = buf;
    buf = data->buf;

    char r = 125;

    unsigned int new_size = 0;
    bool comment = false, space = false;

    while ((r = *buf++)) {
        if (comment)
            if (r != '\n')
                continue;
            else {
                if (buf - 1 != old_buf && isalnum (*(w - 1)))
                    *w++ = sep;
                comment = false;
            }
        else if (r == symbol_comment) {

            comment = true;
        } else {
            if (r == sep || isspace (r)) {
                if (!space) {
                    *w++ = sep;
                    new_size++;
                    space = true;
                }
            } else if (strchr (necess_symbol, r)) {
                if (buf - 1 != old_buf && *(w - 1) != sep) {    // push[ -> push [
                    *w++ = sep;
                    new_size++;
                }
                *w++ = r;
                *w++ = sep;
                new_size += 2;
                space = false;
            } else if (isalnum (r)) {
                *w++ = r;
                new_size++;
                space = false;
            } else if (r == '.' &&
                       (new_size > 0 && new_size < *size_buf) &&
                       (*(w - 1) == sep || (*(w - 1) >= '0' && *(w - 1) <= '9'))
                       && *buf >= '0' && *buf <= '9') {
                //Аккуратная проверка на десятичное число.
                //Десятичное число пишется через точку
                if (new_size > 1 && (*(w - 2) >= '0' && *(w - 2) <= '9'))
                    w--;
                *w++ = r;
                new_size++;
                space = false;
            }
        }
        /*
        // Проверка на критическую ошибку.(Её можно исправить очень просто (challenge))
        if (buf == nullptr || *buf != '\0') {
            PRINT_ERROR ("Buffer internal error. Most likely, the '\\n' symbol is not "
                         "set at the end of the buffer. Check the arguments to the "
                         "Read_File_To_Buffer function.If this function works correctly,"
                         " then we are hacked")
            *state_func = compclnr::ERROR_IN_BUFFER;
            return;
        }
        */
    }

    data->buf = new_buf;
    free (old_buf);
    if (*(w - 1) != sep)
        new_size++;

    if ((char *) realloc (old_buf, sizeof (char) * (++new_size)) == nullptr) {
        PRINT_ERROR ("Realloc was able to complete his task. Possibly out of memory.")
        *state_func = compclnr::ERROR_REALLOC;
        return;
    }
    if (*(w - 1) != sep)
        *w++ = sep;
    *w = '\0';

    *size_buf = new_size;
}