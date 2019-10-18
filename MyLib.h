//
// Created by tester on 09.10.2019.
//
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cassert>
#include <sys/sysinfo.h>
#include <cerrno>
#include <cstring>

#ifndef MY_COMPUTER_MYLIB_H
#define MY_COMPUTER_MYLIB_H

#define LOCATION __FILE__, __LINE__, __PRETTY_FUNCTION__
//#define PRINT_ERROR(message) perror(message); printf("\n%s(%d) ERROR in %s\n", LOCATION);   //strerror (errno)
#define PRINT_ERROR(message) printf("%s. ERRNO == %s", message, strerror(errno)); printf("\n%s(%d) ERROR in %s\n", LOCATION);
#define PRINT_WARNING(message) printf("\nwarning %s(%d) IN %s: %s\n", LOCATION, message);

FILE *open_file (const char *name, unsigned long *file_size, bool UNIX = false) {
    // Размер файла выдаётся в байтах
    // --------------------------------------------------------------------
    // В системе UNIX можно сделать проверку на существование и на
    // доступность чтения данного файла перед тем, как его открыть, и при
    // помощи fseek и ftell узнать размер файла (Переменной UNIX передать true).
    // Но для кроссплатформенности нужно использовать fstat (UNIX == false)
    // То есть: !(UNIX) => UNIX == false
    // --------------------------------------------------------------------

    enum CONST_ACCESS {
        EXIST, EXECUTE,
        WRITE, READ,
        READWRITE
    };

    assert(name != nullptr);
    assert(file_size != nullptr);

    FILE *file = nullptr;
    if (UNIX) { // Узнаём размер будучи в ОС на UNIX

        if ((file = fopen (name, "rb")) != nullptr) {
            fseek (file, 0, SEEK_END);
            *file_size = ftell (file);
            fseek (file, 0, SEEK_SET);
        } else {
            PRINT_ERROR ("\n" "ERROR open_file UNIX:")

            if (access (name, READ) && access (name, READWRITE)) {    // Проверим доступность файла
                printf ("\n" "ERROR open_file UNIX: "
                        "the file %s is ", name);
                if (access (name, EXIST))
                    printf ("NOT PRESENT\n");
                else
                    printf ("access is denied\n");
            }
        }

    } else {    // Узнаём размер будучи в ОС на Windows

        if ((file = fopen (name, "rb")) != nullptr) {
            struct stat file_specification = {};
            if (!fstat (fileno (file), &file_specification)) {
                *file_size = file_specification.st_size;
            } else {
                printf ("\n" "ERROR open_file: %s nullptr \n", name);
            }
        } else {
            printf ("\n" "ERROR open_file: "
                    "the file %s is either not present or access is denied\n", name);
            perror ("meow");
            perror ("\n" "ERROR open_file:\n");
        }
    }
    return file;
}
namespace rftb {
    enum Read_File_To_Buffer_CONST_ERRORS {
        ALL_RIGHT,
        EMPTY_FILE,
        DONT_OPEN_FILE,
        READ_ERROR,
        DONT_HAVE_FREE_MEMORY
    };

    char *Read_File_To_Buffer (const char *name, unsigned int *size, int *state_func, bool TEXT, bool UNIX) {
        // Сам очистит буффер при ошибке
        // В конце ставит \0 если TEXT == true, причём размер увеличивается на байт
        // state_func == 0 ошибки отсутствуют
        // state_func == 1 файл пустой
        // state_func == 2 ошибка чтения или записи в файл
        // state_func == 3 не хватает оперативной памяти для считывания текста

        const unsigned amount_of_free_RAM = 100; //MB

        assert (name != nullptr);
        assert (state_func != nullptr);

        unsigned long file_size = 0;
        bool error_read = false;
        FILE *file = open_file (name, &file_size, UNIX);

        if (file == nullptr) {
            *state_func = DONT_OPEN_FILE;
            return nullptr;
        } else if (file_size == 0) {
            *state_func = EMPTY_FILE;
            fclose (file);
            return nullptr;
        }

        // Проверка на наличие RAM для buf
        struct sysinfo info = {}; // The toopenkiy CLion cannot handle it :((
        sysinfo(&info);
        unsigned long asdfa = info.freeram;
        if (info.freeram - file_size < amount_of_free_RAM*(1024*1024)) {
            PRINT_ERROR("Read_File_To_Buffer: ERROR Not enough RAM for reading text");
            fclose (file);
            *state_func = DONT_HAVE_FREE_MEMORY;
            return nullptr;
        }

        // В buf будет храниться весь файл name + знак '\0', если TEXT == true
        char *buf = (char *) calloc (file_size + TEXT, sizeof (char));
        if (fread (buf, sizeof (char), file_size, file) != file_size) {
            if (feof (file)) {
                printf ("Read_File_To_Buffer: Error fread file %s\n"
                        "feof(%s) == 1\n", name, name);
                error_read = true;
            } else if (ferror ((file))) {
                printf ("Read_File_To_Buffer: Error fread file %s\n"
                        "ferror(%s) == 1\n", name, name);
                error_read = true;
            }
        }

        fclose (file);

        if (error_read) {
            *state_func = READ_ERROR;
            PRINT_ERROR ("fread: READ_ERROR")
            free (buf);
            return nullptr;
        }

        if (TEXT) {
            buf[file_size] = '\0';
            *size = ++file_size;
        } else {
            *size = file_size;
        }
        *state_func = ALL_RIGHT;
        return buf;
    }
}


#endif //MY_COMPUTER_MYLIB_H
