#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <windows.h> 
#include "CStyleInterpreter.h"

int main(int argc, char* argv[]) {
    // Настраиваем кодировку консоли Windows на UTF-8 для корректного русского языка
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    std::string filename;

    std::cout << "=== MathLang C-Style Interpreter v2.0 ===" << std::endl;

    // Способ 1: Проверяем, передан ли файл через аргументы командной строки
    // (например, при перетаскивании файла на exe-шник или запуске из консоли)
    if (argc > 1) {
        filename = argv[1];
        std::cout << "[СИСТЕМА]: Запуск переданного файла: " << filename << std::endl;
    }
    // Способ 2: Если запуск обычный, запрашиваем имя файла у пользователя динамически
    else {
        std::cout << "Введите имя файла скрипта (например, script.mlang или utils.mlang): ";
        std::cin >> filename;
        // Очищаем буфер ввода, чтобы cin.get() в конце программы не срабатывал раньше времени
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    }

    std::vector<std::string> programLines;

    // Пытаемся открыть указанный пользователем файл
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "\n[КРИТИЧЕСКАЯ ОШИБКА]: Не удалось открыть файл '" << filename << "'!" << std::endl;
        std::cerr << "Проверьте, правильно ли указано имя и лежит ли файл в папке с проектом." << std::endl;
        std::cout << "\nНажмите Enter для выхода...";
        std::cin.get();
        return 1;
    }

    // Построчно считываем код из выбранного файла
    std::string line;
    while (std::getline(file, line)) {
        programLines.push_back(line);
    }
    file.close();

    std::cout << "[СИСТЕМА]: Файл успешно загружен. Начинается выполнение...\n" << std::endl;
    std::cout << "------------------ ВЫВОД СКРИПТА ------------------" << std::endl;

    // Запускаем наш интерпретатор
    CStyleInterpreter interpreter;
    int exitCode = 0;
    try {
        interpreter.run(programLines);
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ОШИБКА ВРЕМЕНИ ВЫПОЛНЕНИЯ]: " << e.what() << std::endl;
        exitCode = 1;
    }
    catch (...) {
        std::cerr << "\n[ОШИБКА ВРЕМЕНИ ВЫПОЛНЕНИЯ]: Unknown error occurred" << std::endl;
        exitCode = 1;
    }

    std::cout << "---------------------------------------------------" << std::endl;
    std::cout << "\nПрограмма завершена. Нажмите Enter для выхода...";
    std::cin.get();

    return exitCode;
}
