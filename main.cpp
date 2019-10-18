#include "Super_Stack_Library.h"
#include "MyLib.h"
#include "COMPILATOR.h"
#include <cstring>

// Компилятор
// В КАЖДОМ ENUM первый должен стоять ALL_RIGHT или начинаться с 1!
// ONLY IF sizeof(char) == 1    // Написать проверку на эту совместимость

void bprintf (unsigned char c) {
    unsigned int a[8];
    for (int i = 0; i < 8; i++, c /= 2)
        a[i] = c % 2;
    for (int i = 7; i >= 0; i--)
        printf ("%d", a[i]);
}

void bprintf_buf (unsigned char *MC_buf, size_t size) {
    size_t old_size = size;
    while (size)
        bprintf (MC_buf[old_size - size--]);
}

struct Compiler_t {
    char *buf;              // Буффер, где лежит необр. текстовые команды
    void *MC;               // Итоговый машинный код
    unsigned size_buf;      // Размер буффера в байтах
    unsigned count_elem;    // Кол-во элементов в буфере (Пример: "push 5" -> count == 2)
    int state_stack;        // Код ошибки самого стека (например SYNTAX_ERROR)
    int state_func;         // Код ошибки последней функции, которая его обрабатывала (например CALLOC_ERROR)
};

struct CmdCode_t {
    comand_t code   : 5;
    comand_t reg    : 1;
    comand_t imm    : 1;
    comand_t mem    : 1;
};

void Cleaner (Compiler_t *data);

unsigned char *ConverterToMC (Compiler_t *data);

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

namespace conv {
    enum Converter_ERROR {
        ALL_RIGTH,
        NOINSTRUCTION,      // Нет инструкции для выполнения этой команды
        NOINSTRUCTION_H,    // Команды нет в COMPILATOR.h
        CALLOC_ERROR,
        SYNTAX_ERROR,
        CMD_TOO_LARGE,
        BUF_ENDED,
        NUMBER,
        REGISTER,
        RAM_TYPE_OPEN,
        RAM_TYPE_CLOSE,
        PLUS
    };
};

int main () {
    const char name_file_input[] = "input.txt";
    const char name_file_output[] = "machinecode.mir";

    Compiler_t data_ciler = {};     //DATA CompILER
    data_ciler.state_func = 0;
    data_ciler.size_buf = 0;

    data_ciler.buf = rftb::Read_File_To_Buffer (name_file_input, &data_ciler.size_buf, &data_ciler.state_func, true,
                                                true);
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

    __uint8_t *MC = ConverterToMC (&data_ciler);

    if (data_ciler.state_func != conv::ALL_RIGTH || MC == nullptr ||
        data_ciler.state_stack != conv::ALL_RIGTH) {
        free (data_ciler.buf);
        free (data_ciler.MC);
        return 0;
    }

    FILE *file_out = fopen (name_file_output, "w");
    if (file_out == nullptr) {
        PRINT_ERROR ("")
        fclose (file_out);
        free (data_ciler.MC);
        free (data_ciler.buf);
        return 0;
    }

    fwrite (MC, 1, data_ciler.count_elem, file_out);

    free (data_ciler.MC);
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


int buf_comp_POP (char **read_b, char *element, bool next = true) {
    static char *read_e;

    if (!next) {
        *read_b = read_e + 1;
        return 0;
    }

    char sep = symb_separator;

    read_e = (char *) strchr (*read_b, sep);

    if (read_e == nullptr)
        return conv::BUF_ENDED;

    // Выделяем слово, между пробелами
    if (copy_str (element, *read_b, read_e))
        return conv::CMD_TOO_LARGE;

    return 0;
}

int el_is_number (char c) {
    return c >= '0' && c <= '9';
}

// Возвращает -1, если element не может быть аргументом функции
int element_type_d (const char *element) {
    const char *necess_symbols = necess_symb;
    const char *RAM_symbols = RAM_symb;
    const char *element_old = element;

    if (strchr (necess_symbols, *element)) {    // '[', ']', '+'
        if (*element == RAM_symbols[0]) {
            return conv::RAM_TYPE_OPEN;
        } else if (*element == RAM_symbols[1]) {
            return conv::RAM_TYPE_CLOSE;
        } else if (*element == necess_symbols[2]) {
            return conv::PLUS;
        }
    } else if (el_is_number (*element) || *element == '-') {    // number or -number
        while (el_is_number (*(++element)));
        if (*(element - 1) == '.')
            while (el_is_number (*(++element)));
        if (!el_is_number (*(element - 1)))
            return -1;
        return conv::NUMBER;
    } else {                                    // registers
        unsigned i = 0;
        for (; i < size_registers + 1; i++)
            if (i == size_registers)
                return -1;
            else if (!strcmp (element, registers[i]))
                return conv::REGISTER;
    }
    return -1;
}

unsigned char *ConverterToMC (Compiler_t *data) {
    // Для каждого типа конверитирование разное

    char *buf = data->buf;
    unsigned size_buf = data->size_buf;
    char sep = symb_separator;

    void *out_buf = calloc (data->size_buf, 1);
    data->MC = out_buf;
    size_t size_MC = 0;

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

#define GET_ELEM !(data->state_func = buf_comp_POP (&read_b, element, true))
#define NEXT buf_comp_POP (&read_b, element, false);
#define TYPE type = element_type_d(element);
#define SYNTAXERR {printf ("\n--LINE-- (%d)\n \"%s\"", number_line, element);\
                    PRINT_ERROR (" - command NOT DEFINED in \"COMPILATOR.h\"") \
                    data->state_func = conv::NOINSTRUCTION_H; \
                    data->state_stack = conv::SYNTAX_ERROR; return nullptr;}

        while (GET_ELEM) {  // Получает команду и записывает её в element
            NEXT

            unsigned i = 0;
            for (; i < size_commands; i++) {      // Проверяет существование команды в списке commands
                if (!strcmp (element, commands[i]))
                    break;
                else if (i == size_commands - 1) {              //Если список закончился, а команда не найдена
                    data->state_func = conv::NOINSTRUCTION;
                    data->state_stack = conv::SYNTAX_ERROR;

                    printf ("%s --LINE-- %d\n", element, number_line);
                    PRINT_ERROR ("Unknown command or syntax error in line")
                    printf ("LOL%sLOL", element);
                    free (out_buf);
                    return nullptr;
                }
            }
            number_line++;
            printf ("%s ", element);


            bool exit = false;
            comand = {0};
            number_t arg_num = 0;
            reg_t arg_reg = 0;
            comand.code = i;

#include "instruction_comp.h"

            // Здесь нужно положить команду в выходной буффер
            // Сначала записываются регистры, потом числа

            *(CmdCode_t *) out_buf = comand;
            size_MC += sizeof (CmdCode_t);
            out_buf = (CmdCode_t *) out_buf + 1;

            if (comand.reg) {
                *(reg_t *) out_buf = arg_reg;
                size_MC += sizeof (reg_t);
                out_buf = (reg_t *) out_buf + 1;
            }

            if (comand.imm) {
                *(number_t *) out_buf = arg_num;
                size_MC += sizeof (number_t);
                out_buf = (number_t *) out_buf + 1;
            }
            if (exit)
                break;
        }
#undef GET_ELEM
#undef NEXT
        if (data->state_func != conv::BUF_ENDED)
            return nullptr;
        else
            data->state_func = conv::ALL_RIGTH;

    } else {
        PRINT_ERROR ("No instructions for compiling this type.")
        data->state_func = conv::NOINSTRUCTION;
        return nullptr;
    }

    bprintf_buf ((unsigned char *) data->MC, size_MC);
    data->count_elem = size_MC;

    return (unsigned char *) data->MC;
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
                space = true;
            } else if (isalnum (r) || r == '-') {
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